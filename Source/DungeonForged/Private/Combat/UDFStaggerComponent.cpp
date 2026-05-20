// Source/DungeonForged/Private/Combat/UDFStaggerComponent.cpp
#include "Combat/UDFStaggerComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Characters/ADFEnemyBase.h"
#include "Combat/UDFCombatDirectorSubsystem.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameFramework/Character.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogDFStagger, Log, All);

UDFStaggerComponent::UDFStaggerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UDFStaggerComponent::BeginPlay()
{
	Super::BeginPlay();
	BindToAbilitySystem();
}

void UDFStaggerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* const ASC = BoundASC.Get())
	{
		const FGameplayAttribute HealthAttr = UDFAttributeSet::GetHealthAttribute();
		if (HealthAttr.IsValid() && HealthChangeHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(HealthAttr).Remove(HealthChangeHandle);
		}
	}
	HealthChangeHandle.Reset();
	BoundASC.Reset();
	Super::EndPlay(EndPlayReason);
}

void UDFStaggerComponent::BindToAbilitySystem()
{
	AActor* const Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	UAbilitySystemComponent* const ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!ASC)
	{
		// PlayerState pawns may have ASC bound later (after PossessedBy / OnRep_PlayerState).
		// Retry on next tick via a one-shot timer.
		if (UWorld* const World = GetWorld())
		{
			FTimerHandle Handle;
			World->GetTimerManager().SetTimer(Handle,
				FTimerDelegate::CreateUObject(this, &UDFStaggerComponent::BindToAbilitySystem),
				0.25f, false);
		}
		return;
	}
	const FGameplayAttribute HealthAttr = UDFAttributeSet::GetHealthAttribute();
	if (!HealthAttr.IsValid())
	{
		return;
	}
	HealthChangeHandle = ASC->GetGameplayAttributeValueChangeDelegate(HealthAttr).AddUObject(
		this, &UDFStaggerComponent::HandleHealthChange);
	BoundASC = ASC;
}

bool UDFStaggerComponent::IsOnStaggerCooldown() const
{
	if (LastStaggerTime <= 0.0)
	{
		return false;
	}
	const UWorld* const World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	return (Now - LastStaggerTime) < StaggerCooldown;
}

void UDFStaggerComponent::ResetAccumulator()
{
	RecentHits.Reset();
}

void UDFStaggerComponent::SetNextPoiseDamageMultiplier(const float Multiplier)
{
	NextPoiseDamageMultiplier = FMath::Max(0.f, Multiplier);
}

float UDFStaggerComponent::ResolvePoiseMultiplierForTags(const FGameplayTagContainer& AttackTags) const
{
	float Best = 1.f;
	for (const TPair<FGameplayTag, float>& Pair : PoiseDamageMultipliers)
	{
		if (Pair.Key.IsValid() && AttackTags.HasTag(Pair.Key))
		{
			Best = FMath::Max(Best, Pair.Value);
		}
	}
	return Best;
}

void UDFStaggerComponent::TickComponent(const float DeltaTime, const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (PoiseRegenPerSecond <= KINDA_SMALL_NUMBER || RecentHits.IsEmpty())
	{
		return;
	}
	for (FStaggerHit& H : RecentHits)
	{
		H.Damage = FMath::Max(0.f, H.Damage - PoiseRegenPerSecond * DeltaTime);
	}
	RecentHits.RemoveAll([](const FStaggerHit& H) { return H.Damage <= KINDA_SMALL_NUMBER; });
}

void UDFStaggerComponent::PruneOldEntries()
{
	const UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	const double Threshold = Now - StaggerWindow;
	RecentHits.RemoveAll([Threshold](const FStaggerHit& H) { return H.TimeSeconds < Threshold; });
}

void UDFStaggerComponent::HandleHealthChange(const FOnAttributeChangeData& Data)
{
	const float Delta = Data.NewValue - Data.OldValue;
	if (Delta >= -KINDA_SMALL_NUMBER)
	{
		return; // heal or no-op
	}
	const float DamageRaw = -Delta;
	if (DamageRaw <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	const UWorld* const World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (Now < StaggerDRExpireTime && ActiveStaggerDR < 1.f)
	{
		// DR active
	}
	else
	{
		ActiveStaggerDR = 1.f;
	}
	float Damage = DamageRaw * NextPoiseDamageMultiplier * ActiveStaggerDR;
	NextPoiseDamageMultiplier = 1.f;
	if (Damage <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	AActor* const Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	if (bServerAuthoritative && !Owner->HasAuthority())
	{
		return;
	}
	if (IsOnStaggerCooldown())
	{
		return;
	}

	if (!World)
	{
		return;
	}
	RecentHits.Add({Damage, Now});
	PruneOldEntries();

	float WindowSum = 0.f;
	for (const FStaggerHit& H : RecentHits)
	{
		WindowSum += H.Damage;
	}

#if !UE_BUILD_SHIPPING
	if (bLogVerbose)
	{
		UE_LOG(LogDFStagger, Verbose, TEXT("[%s] +%.1f damage, window sum %.1f / poise %.1f"),
			*GetNameSafe(Owner), Damage, WindowSum, Poise);
	}
#endif

	if (WindowSum >= Poise)
	{
		const float Overshoot = WindowSum - Poise;
		TriggerStagger(Overshoot);
	}
}

void UDFStaggerComponent::TriggerStagger(const float Overshoot)
{
	AActor* const Owner = GetOwner();
	UAbilitySystemComponent* const ASC = BoundASC.Get();
	if (!Owner || !ASC)
	{
		return;
	}
	const UWorld* const World = GetWorld();
	LastStaggerTime = World ? World->GetTimeSeconds() : 0.0;
	ActiveStaggerDR = FMath::Clamp(StaggerDamageReduction, 0.f, 1.f);
	StaggerDRExpireTime = LastStaggerTime + StaggerDRWindowSeconds;
	RecentHits.Reset();

	if (StaggerGameplayEffect)
	{
		FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
		Ctx.AddInstigator(Owner, Owner);
		Ctx.AddSourceObject(this);
		const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(StaggerGameplayEffect, 1.f, Ctx);
		if (SpecHandle.IsValid() && SpecHandle.Data)
		{
			if (StaggerSetByCallerTag.IsValid() && StaggerSetByCallerMagnitude > KINDA_SMALL_NUMBER)
			{
				SpecHandle.Data->SetSetByCallerMagnitude(StaggerSetByCallerTag, StaggerSetByCallerMagnitude);
			}
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	if (FDFGameplayTags::Event_Combat_Stagger_Triggered.IsValid())
	{
		FGameplayEventData Payload;
		Payload.EventTag = FDFGameplayTags::Event_Combat_Stagger_Triggered;
		Payload.Instigator = Owner;
		Payload.Target = Owner;
		Payload.EventMagnitude = Overshoot;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			Owner, FDFGameplayTags::Event_Combat_Stagger_Triggered, Payload);
	}

	if (StaggerMontage)
	{
		if (const ACharacter* const Char = Cast<ACharacter>(Owner))
		{
			if (USkeletalMeshComponent* const Mesh = Char->GetMesh())
			{
				if (UAnimInstance* const Anim = Mesh->GetAnimInstance())
				{
					Anim->Montage_Play(StaggerMontage, 1.f);
				}
			}
		}
	}

	if (UWorld* const W = GetWorld())
	{
		if (UDFCombatDirectorSubsystem* const Dir = W->GetSubsystem<UDFCombatDirectorSubsystem>())
		{
			if (ADFEnemyBase* const Enemy = Cast<ADFEnemyBase>(Owner))
			{
				Dir->ReleaseAttackToken(Enemy);
			}
		}
	}
	if (ASC)
	{
		FGameplayTagContainer WithoutDeath;
		if (FDFGameplayTags::Ability_Death.IsValid())
		{
			WithoutDeath.AddTag(FDFGameplayTags::Ability_Death);
		}
		if (FDFGameplayTags::Ability_Death_Enemy.IsValid())
		{
			WithoutDeath.AddTag(FDFGameplayTags::Ability_Death_Enemy);
		}
		ASC->CancelAbilities(nullptr, WithoutDeath.Num() > 0 ? &WithoutDeath : nullptr);
	}

#if !UE_BUILD_SHIPPING
	UE_LOG(LogDFStagger, Log, TEXT("[%s] STAGGERED (overshoot %.1f). Cooldown %.1fs."),
		*GetNameSafe(Owner), Overshoot, StaggerCooldown);
#endif
}
