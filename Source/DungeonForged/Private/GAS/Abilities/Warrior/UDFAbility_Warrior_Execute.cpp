// Source/DungeonForged/Private/GAS/Abilities/Warrior/UDFAbility_Warrior_Execute.cpp
#include "GAS/Abilities/Warrior/UDFAbility_Warrior_Execute.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimMontage.h"
#include "Camera/UDFLockOnComponent.h"
#include "Characters/ADFEnemyBase.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Damage_Physical.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffectTypes.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "GameplayEffect.h"

static AActor* DFWarriorGetLockOn(ACharacter* C)
{
	const ADFPlayerCharacter* const P = Cast<ADFPlayerCharacter>(C);
	return P && P->LockOnComponent ? P->LockOnComponent->GetCurrentTarget() : nullptr;
}

UDFAbility_Warrior_Execute::UDFAbility_Warrior_Execute()
{
	AbilityCost_Mana = 50.f;
	AbilityCost_Stamina = 0.f;
	BaseCooldown = 20.f;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UDFAbility_Warrior_Execute::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Warrior_Execute);
		ActivationOwnedTags.AddTag(FDFGameplayTags::State_Attacking);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Stunned);
	}
}

bool UDFAbility_Warrior_Execute::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	if (!ActorInfo)
	{
		return false;
	}
	ACharacter* const C = ActorInfo->AvatarActor.IsValid() ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	AActor* const T = C ? DFWarriorGetLockOn(C) : nullptr;
	if (!C || !T)
	{
		return false;
	}
	UAbilitySystemComponent* TASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(T);
	if (!TASC)
	{
		return false;
	}
	const float Hp = TASC->GetNumericAttribute(UDFAttributeSet::GetHealthAttribute());
	const float Mx = TASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute());
	if (Mx <= KINDA_SMALL_NUMBER)
	{
		return false;
	}
	return (Hp / Mx) < HealthThresholdFraction;
}

void UDFAbility_Warrior_Execute::ActivateAbility(
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
	if (UAbilitySystemComponent* asc = GetAbilitySystemComponentFromActorInfo())
	{
		if (asc->GetOwner() && asc->GetOwner()->HasAuthority())
		{
			ApplyResourceCostsToOwner(asc);
		}
	}
	const TObjectPtr<UAnimMontage> M = ExecuteMontage ? ExecuteMontage : AbilityMontage;
	if (!M)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (UAbilityTask_PlayMontageAndWait* T = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, M, 1.f, NAME_None, true, 1.f, 0.f, true))
	{
		T->OnCompleted.AddDynamic(this, &UDFAbility_Warrior_Execute::OnExecuteMontageEnd);
		T->ReadyForActivation();
	}
	if (UAbilityTask_WaitGameplayEvent* W =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FDFGameplayTags::Event_Warrior_Execute_Trace, nullptr, true, true))
	{
		W->EventReceived.AddDynamic(this, &UDFAbility_Warrior_Execute::OnExecuteTraceEvent);
		W->ReadyForActivation();
	}
}

void UDFAbility_Warrior_Execute::OnExecuteTraceEvent(FGameplayEventData /*Payload*/)
{
	DoExecuteHit();
}

void UDFAbility_Warrior_Execute::DoExecuteHit()
{
	ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* const Source = GetAbilitySystemComponentFromActorInfo();
	AActor* const Tgt = Char ? DFWarriorGetLockOn(Char) : nullptr;
	if (!Char || !Source || !Tgt || !Char->HasAuthority() || !Cast<ADFEnemyBase>(Tgt))
	{
		return;
	}
	UAbilitySystemComponent* TASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Tgt);
	if (!TASC)
	{
		return;
	}
	const float Str = Source->GetNumericAttribute(UDFAttributeSet::GetStrengthAttribute());
	const float Mx = TASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute());
	const float Hp = TASC->GetNumericAttribute(UDFAttributeSet::GetHealthAttribute());
	const float Missing = FMath::Max(0.f, Mx - Hp);
	const float DataDamage = 2.5f * Str + 0.5f * Missing;
	FGameplayEffectContextHandle Ctx = Source->MakeEffectContext();
	Ctx.AddSourceObject(this);
	Ctx.AddInstigator(Char, Char);
	const FGameplayEffectSpecHandle S = Source->MakeOutgoingSpec(UGE_Damage_Physical::StaticClass(), 1.f, Ctx);
	if (S.IsValid() && S.Data.IsValid())
	{
		S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, DataDamage);
		Source->ApplyGameplayEffectSpecToTarget(*S.Data.Get(), TASC);
	}
	const float HpAfter = TASC->GetNumericAttribute(UDFAttributeSet::GetHealthAttribute());
	if (HpAfter <= KINDA_SMALL_NUMBER)
	{
		if (ExecuteKillBonusEffect)
		{
			const FGameplayEffectContextHandle BC = Source->MakeEffectContext();
			Source->ApplyGameplayEffectToSelf(ExecuteKillBonusEffect.GetDefaultObject(), 1.f, BC);
		}
		else
		{
			Source->ApplyModToAttribute(UDFAttributeSet::GetManaAttribute(), EGameplayModOp::Additive, 50.f);
		}
		if (UWorld* W = GetWorld())
		{
			if (Char->IsLocallyControlled())
			{
				UGameplayStatics::SetGlobalTimeDilation(W, FMath::Max(0.01f, HitStopTimeDilation));
				W->GetTimerManager().SetTimer(HitStopTimer, this, &UDFAbility_Warrior_Execute::RestoreTimeDilationAfterHitStop, FMath::Max(0.01f, HitStopDurationSec), false);
			}
		}
		if (DeathBlowNiagara)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				Char, DeathBlowNiagara, Tgt->GetActorLocation(), FRotator::ZeroRotator, FVector(1.f), true, true, ENCPoolMethod::None, true);
		}
	}
}

void UDFAbility_Warrior_Execute::OnExecuteMontageEnd()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UDFAbility_Warrior_Execute::RestoreTimeDilationAfterHitStop()
{
	if (UWorld* W = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(W, 1.f);
	}
}

void UDFAbility_Warrior_Execute::EndAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(HitStopTimer);
	}
	if (GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
