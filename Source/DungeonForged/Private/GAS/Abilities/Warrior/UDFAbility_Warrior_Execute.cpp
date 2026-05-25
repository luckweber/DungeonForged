// Source/DungeonForged/Private/GAS/Abilities/Warrior/UDFAbility_Warrior_Execute.cpp
#include "GAS/Abilities/Warrior/UDFAbility_Warrior_Execute.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Camera/UDFLockOnComponent.h"
#include "Characters/ADFEnemyBase.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Combat/UDFCombatEventsLibrary.h"
#include "Data/UDFCombatTuningData.h"
#include "DFAssetManager.h"
#include "FX/UDFCombatFeedbackLibrary.h"
#include "FX/UDFCombatFeedbackTypes.h"
#include "FX/UDFHitStopSubsystem.h"
#include "GameFramework/Character.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Damage_Physical.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffectTypes.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

static AActor* DFWarriorGetLockOn(ACharacter* const C)
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
		if (FDFGameplayTags::Event_Combat_Finisher_Available.IsValid())
		{
			FAbilityTriggerData Trigger;
			Trigger.TriggerTag = FDFGameplayTags::Event_Combat_Finisher_Available;
			Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
			AbilityTriggers.Add(Trigger);
		}
	}
}

AActor* UDFAbility_Warrior_Execute::ResolveFinisherTarget(
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayEventData* TriggerEventData) const
{
	if (TriggerEventData && IsValid(TriggerEventData->Target))
	{
		return const_cast<AActor*>(TriggerEventData->Target.Get());
	}
	ACharacter* const C = ActorInfo && ActorInfo->AvatarActor.IsValid()
		? Cast<ACharacter>(ActorInfo->AvatarActor.Get())
		: nullptr;
	return C ? DFWarriorGetLockOn(C) : nullptr;
}

bool UDFAbility_Warrior_Execute::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	if (!ActorInfo)
	{
		return false;
	}
	if (UAbilitySystemComponent* const ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		if (FDFGameplayTags::State_Combat_FinisherReady.IsValid()
			&& ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Combat_FinisherReady))
		{
			return ResolveFinisherTarget(ActorInfo, nullptr) != nullptr;
		}
	}
	AActor* const T = ResolveFinisherTarget(ActorInfo, nullptr);
	if (!T)
	{
		return false;
	}
	UAbilitySystemComponent* const TASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(T);
	if (!TASC)
	{
		return false;
	}
	float Threshold = HealthThresholdFraction;
	if (const UDFCombatTuningData* const Tuning = UDFAssetManager::Get().GetCombatTuningData())
	{
		Threshold = Tuning->FinisherHealthThreshold;
	}
	const float Hp = TASC->GetNumericAttribute(UDFAttributeSet::GetHealthAttribute());
	const float Mx = TASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute());
	return Mx > KINDA_SMALL_NUMBER && (Hp / Mx) < Threshold;
}

void UDFAbility_Warrior_Execute::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
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
	if (UAbilitySystemComponent* const Asc = GetAbilitySystemComponentFromActorInfo())
	{
		if (Asc->GetOwner() && Asc->GetOwner()->HasAuthority())
		{
			ApplyResourceCostsToOwner(Asc);
		}
	}

	FinisherTarget = ResolveFinisherTarget(ActorInfo, TriggerEventData);
	FinisherHitsLanded = 0;
	bFinisherInputPhaseActive = false;

	const TObjectPtr<UAnimMontage> M = ExecuteMontage ? ExecuteMontage : AbilityMontage;
	if (!M)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (UAbilityTask_PlayMontageAndWait* const T = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, M, 1.f, NAME_None, true, 1.f, 0.f, true))
	{
		T->OnCompleted.AddDynamic(this, &UDFAbility_Warrior_Execute::OnExecuteMontageEnd);
		T->OnInterrupted.AddDynamic(this, &UDFAbility_Warrior_Execute::OnExecuteMontageEnd);
		T->OnCancelled.AddDynamic(this, &UDFAbility_Warrior_Execute::OnExecuteMontageEnd);
		T->ReadyForActivation();
	}
	if (UAbilityTask_WaitGameplayEvent* const W = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, FDFGameplayTags::Event_Warrior_Execute_Trace, nullptr, true, true))
	{
		W->EventReceived.AddDynamic(this, &UDFAbility_Warrior_Execute::OnExecuteTraceEvent);
		W->ReadyForActivation();
	}
}

void UDFAbility_Warrior_Execute::OnExecuteTraceEvent(FGameplayEventData /*Payload*/)
{
	int32 HitCount = FinisherMultiHitCount;
	if (const UDFCombatTuningData* const Tuning = UDFAssetManager::Get().GetCombatTuningData())
	{
		HitCount = Tuning->FinisherMultiHitCount;
	}
	if (HitCount <= 1)
	{
		DoExecuteHit();
		return;
	}
	BeginFinisherInputPhase();
}

void UDFAbility_Warrior_Execute::BeginFinisherInputPhase()
{
	int32 HitCount = FinisherMultiHitCount;
	float WindowSec = FinisherInputWindowSec;
	if (const UDFCombatTuningData* const Tuning = UDFAssetManager::Get().GetCombatTuningData())
	{
		HitCount = Tuning->FinisherMultiHitCount;
		WindowSec = Tuning->FinisherInputWindowSec;
	}
	if (FinisherHitsLanded >= HitCount)
	{
		FinishFinisherChain(false);
		return;
	}
	bFinisherInputPhaseActive = true;
	if (UAbilityTask_WaitGameplayEvent* const InputTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, FDFGameplayTags::Event_Combat_Finisher_Input, nullptr, false, false))
	{
		InputTask->EventReceived.AddDynamic(this, &UDFAbility_Warrior_Execute::OnFinisherInputEvent);
		InputTask->ReadyForActivation();
	}
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			FinisherInputTimer,
			this,
			&UDFAbility_Warrior_Execute::OnFinisherInputWindowExpired,
			FMath::Max(0.1f, WindowSec),
			false);
	}
}

void UDFAbility_Warrior_Execute::OnFinisherInputEvent(FGameplayEventData /*Payload*/)
{
	if (!bFinisherInputPhaseActive)
	{
		return;
	}
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(FinisherInputTimer);
	}
	ApplyFinisherHit(FinisherHitsLanded);
	++FinisherHitsLanded;
	BeginFinisherInputPhase();
}

void UDFAbility_Warrior_Execute::OnFinisherInputWindowExpired()
{
	bFinisherInputPhaseActive = false;
	FinishFinisherChain(false);
}

void UDFAbility_Warrior_Execute::ApplyFinisherHit(const int32 HitIndex)
{
	ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* const Source = GetAbilitySystemComponentFromActorInfo();
	AActor* const Tgt = FinisherTarget.Get();
	if (!Char || !Source || !Tgt || !Char->HasAuthority())
	{
		return;
	}
	UAbilitySystemComponent* const TASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Tgt);
	if (!TASC)
	{
		return;
	}
	const float Mx = TASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute());
	const float HitDamage = Mx * FinisherHitDamageFractionOfMaxHP;
	FGameplayEffectContextHandle Ctx = Source->MakeEffectContext();
	Ctx.AddSourceObject(this);
	Ctx.AddInstigator(Char, Char);
	const FGameplayEffectSpecHandle S = Source->MakeOutgoingSpec(UGE_Damage_Physical::StaticClass(), 1.f, Ctx);
	if (S.IsValid() && S.Data.IsValid() && FDFGameplayTags::Data_Damage.IsValid())
	{
		S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, HitDamage);
		UDFCombatFeedbackLibrary::MarkSpecCombatFeedbackCentralized(*S.Data.Get());
		Source->ApplyGameplayEffectSpecToTarget(*S.Data.Get(), TASC);
	}
	if (FinisherHitMontages.IsValidIndex(HitIndex) && FinisherHitMontages[HitIndex])
	{
		if (UAnimInstance* const Anim = Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr)
		{
			Anim->Montage_Play(FinisherHitMontages[HitIndex], 1.f);
		}
	}
	FDFHitConfirmedContext HitCtx;
	HitCtx.Instigator = Char;
	HitCtx.Victim = Tgt;
	HitCtx.Magnitude = HitDamage;
	HitCtx.MaxHealth = Mx;
	HitCtx.DamagePercent = Mx > KINDA_SMALL_NUMBER ? (HitDamage / Mx) : 0.f;
	HitCtx.Location = Tgt->GetActorLocation();
	HitCtx.Band = EDFHitFeedbackBand::Heavy;
	UDFCombatFeedbackLibrary::DispatchOnHitConfirmed(Char, HitCtx);

	const float HpAfter = TASC->GetNumericAttribute(UDFAttributeSet::GetHealthAttribute());
	if (HpAfter <= KINDA_SMALL_NUMBER)
	{
		FinishFinisherChain(true);
	}
}

void UDFAbility_Warrior_Execute::DoExecuteHit()
{
	ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* const Source = GetAbilitySystemComponentFromActorInfo();
	AActor* const Tgt = FinisherTarget.Get();
	if (!Char || !Source || !Tgt || !Char->HasAuthority() || !Cast<ADFEnemyBase>(Tgt))
	{
		return;
	}
	UAbilitySystemComponent* const TASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Tgt);
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
		UDFCombatFeedbackLibrary::MarkSpecCombatFeedbackCentralized(*S.Data.Get());
		Source->ApplyGameplayEffectSpecToTarget(*S.Data.Get(), TASC);
	}
	const float HpAfter = TASC->GetNumericAttribute(UDFAttributeSet::GetHealthAttribute());
	FinishFinisherChain(HpAfter <= KINDA_SMALL_NUMBER);
}

void UDFAbility_Warrior_Execute::FinishFinisherChain(const bool bWasLethal)
{
	bFinisherInputPhaseActive = false;
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(FinisherInputTimer);
	}
	ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	AActor* const Tgt = FinisherTarget.Get();
	UAbilitySystemComponent* const Source = GetAbilitySystemComponentFromActorInfo();
	if (bWasLethal && Char && Source && Tgt)
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
		if (Char->IsLocallyControlled())
		{
			if (UWorld* const W = GetWorld())
			{
				if (UDFHitStopSubsystem* const HS = W->GetSubsystem<UDFHitStopSubsystem>())
				{
					HS->TriggerHitStop(0.14f, 0.02f, Char);
				}
			}
		}
		if (DeathBlowNiagara)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				Char, DeathBlowNiagara, Tgt->GetActorLocation(), FRotator::ZeroRotator, FVector(1.f), true, true,
				ENCPoolMethod::None, true);
		}
	}
	if (FDFGameplayTags::Event_Combat_Finisher_Completed.IsValid() && Char)
	{
		FGameplayEventData Payload;
		Payload.EventTag = FDFGameplayTags::Event_Combat_Finisher_Completed;
		Payload.Instigator = Char;
		Payload.Target = Tgt;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			Char, FDFGameplayTags::Event_Combat_Finisher_Completed, Payload);
	}
	ClearFinisherCombatState();
}

void UDFAbility_Warrior_Execute::ClearFinisherCombatState()
{
	if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (FDFGameplayTags::State_Combat_FinisherReady.IsValid())
		{
			ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Combat_FinisherReady, 1);
		}
	}
	FinisherTarget = nullptr;
	FinisherHitsLanded = 0;
}

void UDFAbility_Warrior_Execute::OnExecuteMontageEnd()
{
	if (!bFinisherInputPhaseActive)
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	}
}

void UDFAbility_Warrior_Execute::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(FinisherInputTimer);
	}
	ClearFinisherCombatState();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
