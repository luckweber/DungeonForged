// Source/DungeonForged/Private/GAS/Abilities/Warrior/UDFAbility_Warrior_Whirlwind.cpp
#include "GAS/Abilities/Warrior/UDFAbility_Warrior_Whirlwind.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "Animation/AnimInstance.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimMontage.h"
#include "Characters/ADFEnemyBase.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Damage_Physical.h"
#include "GAS/UDFAttributeSet.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

UDFAbility_Warrior_Whirlwind::UDFAbility_Warrior_Whirlwind()
{
	AbilityCost_Mana = 40.f;
	AbilityCost_Stamina = 20.f;
	BaseCooldown = 12.f;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UDFAbility_Warrior_Whirlwind::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Warrior_Whirlwind);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Stunned);
	}
}

void UDFAbility_Warrior_Whirlwind::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	(void)TriggerEventData;
	if (!ActorInfo)
	{
		EndAbility(Handle, nullptr, ActivationInfo, true, true);
		return;
	}
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo, nullptr))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	UAbilitySystemComponent* const ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC && ASC->GetOwner() && ASC->GetOwner()->HasAuthority())
	{
		ApplyResourceCostsToOwner(ASC);
	}
	ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (Char && Char->HasAuthority() && Char->GetCharacterMovement())
	{
		CachedMaxWalkSpeed = Char->GetCharacterMovement()->MaxWalkSpeed;
		bHasWalkCache = true;
		Char->GetCharacterMovement()->MaxWalkSpeed *= 0.5f;
	}
	if (ASC && Char && Char->HasAuthority())
	{
		if (WhirlwindActiveEffect)
		{
			FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
			WhirlwindActiveHandle = ASC->ApplyGameplayEffectToSelf(WhirlwindActiveEffect.GetDefaultObject(), 1.f, Ctx);
		}
		else
		{
			ASC->AddLooseGameplayTag(FDFGameplayTags::State_Spinning);
		}
	}
	const TObjectPtr<UAnimMontage> M = WhirlwindMontage ? WhirlwindMontage : AbilityMontage;
	if (M && Char)
	{
		if (UAbilityTask_PlayMontageAndWait* T = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, NAME_None, M, 1.f, NAME_None, false, 1.f, 0.f, true))
		{
			T->ReadyForActivation();
		}
	}
	if (UAbilityTask_WaitGameplayEvent* W =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FDFGameplayTags::Event_Ability_Whirlwind_Tick, nullptr, false, true))
	{
		W->EventReceived.AddDynamic(this, &UDFAbility_Warrior_Whirlwind::OnWhirlwindTickEvent);
		W->ReadyForActivation();
	}
	if (UAbilityTask_WaitInputRelease* I = UAbilityTask_WaitInputRelease::WaitInputRelease(this, false))
	{
		I->OnRelease.AddDynamic(this, &UDFAbility_Warrior_Whirlwind::OnWhirlwindInputReleased);
		I->ReadyForActivation();
	}
	if (UWorld* W = GetWorld())
	{
		if (UAbilityTask_WaitDelay* D = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(0.01f, WhirlwindMaxDuration)))
		{
			D->OnFinish.AddDynamic(this, &UDFAbility_Warrior_Whirlwind::OnWhirlwindMaxTime);
			D->ReadyForActivation();
		}
	}
}

void UDFAbility_Warrior_Whirlwind::OnWhirlwindTickEvent(FGameplayEventData /*Payload*/)
{
	ApplyWhirlwindDamageTick();
}

void UDFAbility_Warrior_Whirlwind::ApplyWhirlwindDamageTick()
{
	ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* const Source = GetAbilitySystemComponentFromActorInfo();
	if (!Char || !Source || !GetWorld() || !Char->HasAuthority())
	{
		return;
	}
	const float Str = Source->GetNumericAttribute(UDFAttributeSet::GetStrengthAttribute());
	const float Scaled = FMath::Max(0.f, 25.f - 0.2f * Str);
	const UWorld* W = GetWorld();
	FCollisionQueryParams Q(SCENE_QUERY_STAT(Whirlwind), false, Char);
	FCollisionObjectQueryParams Obj(ECC_Pawn);
	const FVector C = Char->GetActorLocation();
	TArray<FOverlapResult> Ov;
	W->OverlapMultiByObjectType(Ov, C, FQuat::Identity, Obj, FCollisionShape::MakeSphere(WhirlwindOverlapRadius), Q);
	const double Now = W->GetTimeSeconds();
	for (FOverlapResult& R : Ov)
	{
		AActor* A = R.GetActor();
		if (!A || A == Char || !Cast<ADFEnemyBase>(A))
		{
			continue;
		}
		const float* Last = LastHitTime.Find(A);
		if (Last && (float)(Now - *Last) < WhirlwindTickInterval)
		{
			continue;
		}
		LastHitTime.Add(A, (float)Now);
		UAbilitySystemComponent* TASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(A);
		if (!TASC)
		{
			continue;
		}
		FGameplayEffectContextHandle Ctx = Source->MakeEffectContext();
		Ctx.AddSourceObject(this);
		Ctx.AddInstigator(Char, Char);
		const FGameplayEffectSpecHandle S = Source->MakeOutgoingSpec(UGE_Damage_Physical::StaticClass(), 1.f, Ctx);
		if (S.IsValid() && S.Data.IsValid())
		{
			S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, Scaled);
			Source->ApplyGameplayEffectSpecToTarget(*S.Data.Get(), TASC);
		}
		if (WhirlwindHitSparks)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				Char, WhirlwindHitSparks, A->GetActorLocation(), FRotator::ZeroRotator, FVector(0.4f), true, true, ENCPoolMethod::None, true);
		}
	}
}

void UDFAbility_Warrior_Whirlwind::OnWhirlwindInputReleased(float)
{
	FinishWhirlwind();
}

void UDFAbility_Warrior_Whirlwind::OnWhirlwindMaxTime()
{
	FinishWhirlwind();
}

void UDFAbility_Warrior_Whirlwind::FinishWhirlwind()
{
	if (bWhirlwindEnding)
	{
		return;
	}
	bWhirlwindEnding = true;
	const bool bHadActiveGE = WhirlwindActiveHandle.IsValid();
	RemoveWhirlwindEffect();
	ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (!bHadActiveGE)
		{
			ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Spinning);
		}
	}
	if (Char && WhirlwindStopMontage)
	{
		if (UAnimInstance* const AI = Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr)
		{
			AI->Montage_Play(WhirlwindStopMontage, 1.f, EMontagePlayReturnType::Duration, 0.f, true);
		}
	}
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UDFAbility_Warrior_Whirlwind::RemoveWhirlwindEffect()
{
	if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (WhirlwindActiveHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(WhirlwindActiveHandle, 1);
			WhirlwindActiveHandle = FActiveGameplayEffectHandle();
		}
	}
}

void UDFAbility_Warrior_Whirlwind::EndAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ACharacter* const Char = ActorInfo && ActorInfo->AvatarActor.IsValid() ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (bHasWalkCache && Char && Char->GetCharacterMovement())
	{
		Char->GetCharacterMovement()->MaxWalkSpeed = CachedMaxWalkSpeed;
		bHasWalkCache = false;
	}
	RemoveWhirlwindEffect();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
