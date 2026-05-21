// Source/DungeonForged/Private/Combat/UDFComboComponent.cpp
#include "Combat/UDFComboComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Combat/UDFMeleeAimComponent.h"
#include "DFAssetManager.h"
#include "Data/UDFCombatTuningData.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Combat/DFCombatDebug.h"
#include "Combat/UDFMeleeTraceComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Equipment/DFEquipmentTypes.h"
#include "Equipment/UDFEquipmentComponent.h"
#include "FX/UDFHitStopSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTagContainer.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "Data/UDFCombatTuningData.h"
#include "DFAssetManager.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "DungeonForgedModule.h"

UDFComboComponent::UDFComboComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

int32 UDFComboComponent::ResolveComboStepForActivation() const
{
	if (LockedComboActivationStep >= 0)
	{
		return LockedComboActivationStep;
	}
	if (PendingComboActivationStep >= 0)
	{
		return PendingComboActivationStep;
	}
	return CurrentComboStep;
}

void UDFComboComponent::ClearLockedComboStepAfterActivation(const int32 ActivatedStep)
{
	CurrentComboStep = FMath::Max(0, ActivatedStep);
	LockedComboActivationStep = -1;
	PendingComboActivationStep = -1;
	bComboChainAdvancePending = false;
}

void UDFComboComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UDFComboComponent, bComboChainAdvancePending, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UDFComboComponent, LockedComboActivationStep, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UDFComboComponent, bComboHeavyFinisherPending, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UDFComboComponent, CurrentComboStep, COND_OwnerOnly);
}

bool UDFComboComponent::IsInputBufferExpired(const float ExpireGameTime) const
{
	if (ExpireGameTime < 0.f)
	{
		return true;
	}
	UWorld* const W = GetWorld();
	if (!W)
	{
		return true;
	}
	if (UDFHitStopSubsystem* const HS = W->GetSubsystem<UDFHitStopSubsystem>())
	{
		if (HS->IsHitStopActive())
		{
			return false;
		}
	}
	return W->GetTimeSeconds() > ExpireGameTime;
}

void UDFComboComponent::ArmComboWindowTimer()
{
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	ComboWindowExpireTime = W->GetTimeSeconds() + ComboWindowDuration;
	W->GetTimerManager().SetTimer(
		ComboWindowTimer, this, &UDFComboComponent::OnComboWindowTimerExpired, ComboWindowDuration, false);
}

#if !UE_BUILD_SHIPPING
void UDFComboComponent::RecordComboWindowOpened(const FName Source, UAnimMontage* const MontageAtNotify)
{
	const FString SourceStr = Source.IsNone() ? TEXT("AdvanceCombo") : Source.ToString();
	UAnimInstance* const AnimInst = GetAnimInstance();
	const DFCombatDebug::FMontagePlaybackSample Sample =
		DFCombatDebug::SampleMontagePlayback(AnimInst, MontageAtNotify);
	LastComboWindowOpenSource = SourceStr;
	LastComboWindowOpenWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : -1.f;
	if (Sample.bValid)
	{
		LastComboWindowOpenMontageTime = Sample.PositionSec;
		LastComboWindowOpenMontageFrame = Sample.Frame;
		LastComboWindowOpenMontageName = Sample.MontageName;
	}
	else
	{
		LastComboWindowOpenMontageTime = -1.f;
		LastComboWindowOpenMontageFrame = -1;
		LastComboWindowOpenMontageName.Reset();
	}
	DFCombatDebug::LogComboMontageEvent(
		*FString::Printf(TEXT("ComboWindow OPEN (%s) timer=%.2fs"), *SourceStr, ComboWindowDuration),
		AnimInst,
		MontageAtNotify);
}

void UDFComboComponent::RecordComboWindowClosed(const FName Source)
{
	const FString SourceStr = Source.IsNone() ? TEXT("Unknown") : Source.ToString();
	DFCombatDebug::LogComboMontageEvent(
		*FString::Printf(TEXT("ComboWindow CLOSE (%s)"), *SourceStr),
		GetAnimInstance(),
		ResolveDirectionalComboMontage(CurrentComboStep));
}

void UDFComboComponent::RecordChainMontageBlendIn(const float RuntimeBlendIn, UAnimMontage* const Montage)
{
	LastChainRuntimeBlendIn = RuntimeBlendIn;
	DFCombatDebug::LogComboMontageEvent(TEXT("ChainMontagePlay"), GetAnimInstance(), Montage, RuntimeBlendIn);
}
#endif

void UDFComboComponent::NotifyOwnerHitConfirmed(const float ExtensionSeconds)
{
	bSwingHitConfirmedThisActivation = true;
	if (!bComboWindowActive)
	{
		return;
	}
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	const float Ext = ExtensionSeconds >= 0.f ? ExtensionSeconds : ComboRefreshOnHitExtension;
	const float NewExpire = W->GetTimeSeconds() + Ext;
	if (NewExpire > ComboWindowExpireTime)
	{
		ComboWindowExpireTime = NewExpire;
		const float Remaining = FMath::Max(0.01f, ComboWindowExpireTime - W->GetTimeSeconds());
		W->GetTimerManager().SetTimer(
			ComboWindowTimer, this, &UDFComboComponent::OnComboWindowTimerExpired, Remaining, false);
		UE_LOG(LogDFFeel, Verbose, TEXT("[Combo] Refresh on-hit +%.2fs"), Ext);
	}
}

void UDFComboComponent::SetAbilityCancelWindow(const FGameplayTagContainer& AllowedCancelTags)
{
	bAbilityCancelWindowActive = true;
	AllowedAbilityCancelTags = AllowedCancelTags;
}

void UDFComboComponent::ClearAbilityCancelWindow()
{
	bAbilityCancelWindowActive = false;
	AllowedAbilityCancelTags.Reset();
}

bool UDFComboComponent::IsAbilityCancellable(const FGameplayTagContainer& AbilityTags) const
{
	if (AbilityTags.IsEmpty())
	{
		return false;
	}

	const ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(GetOwner());
	const UAbilitySystemComponent* const ASC = PC ? PC->GetAbilitySystemComponent() : nullptr;

	const bool bCancelWindowTagOpen = ASC && FDFGameplayTags::State_Combat_CancelWindow_Open.IsValid()
		&& ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Combat_CancelWindow_Open);
	if (!bAbilityCancelWindowActive && !bCancelWindowTagOpen)
	{
		return false;
	}
	if (!AllowedAbilityCancelTags.IsEmpty() && AbilityTags.HasAny(AllowedAbilityCancelTags))
	{
		return true;
	}

	const UDFCombatTuningData* const Tuning = UDFAssetManager::Get().GetCombatTuningData();
	if (!Tuning || Tuning->CancelRules.Num() == 0)
	{
		return false;
	}
	if (!ASC)
	{
		return false;
	}

	FGameplayTagContainer ActiveAbilityTags;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.IsActive() && Spec.Ability)
		{
			ActiveAbilityTags.AppendTags(Spec.Ability->GetAssetTags());
			ActiveAbilityTags.AppendTags(Spec.Ability->AbilityTags);
		}
	}

	for (const FDFCancelRule& Rule : Tuning->CancelRules)
	{
		if (Rule.FromAbilityTags.IsEmpty() || !ActiveAbilityTags.HasAny(Rule.FromAbilityTags))
		{
			continue;
		}
		if (!Rule.AllowedTargetTags.IsEmpty() && !AbilityTags.HasAny(Rule.AllowedTargetTags))
		{
			continue;
		}
		if (Rule.bRequireHitConfirmed && !bSwingHitConfirmedThisActivation)
		{
			continue;
		}
		if (!Rule.bAllowOnWhiff && !bSwingHitConfirmedThisActivation)
		{
			continue;
		}
		return true;
	}
	return false;
}

float UDFComboComponent::ResolveChainBlendInForStep(const int32 Step) const
{
	FDFComboStep StepData;
	if (GetActiveComboStep(Step, StepData) && StepData.ChainBlendInTime >= 0.f)
	{
		return StepData.ChainBlendInTime;
	}
	return ComboChainMontageBlendInTime;
}

EAlphaBlendOption UDFComboComponent::ResolveChainBlendOptionForStep(const int32 Step) const
{
	FDFComboStep StepData;
	if (GetActiveComboStep(Step, StepData) && StepData.ChainBlendOptionOverride < 255)
	{
		return static_cast<EAlphaBlendOption>(StepData.ChainBlendOptionOverride);
	}
	return ComboChainBlendOption;
}

bool UDFComboComponent::IsOwnerAirborne() const
{
	const ACharacter* const Char = Cast<ACharacter>(GetOwner());
	if (!Char)
	{
		return false;
	}
	const UCharacterMovementComponent* const CMC = Char->GetCharacterMovement();
	return CMC && CMC->IsFalling();
}

bool UDFComboComponent::GetActiveComboStep(const int32 Step, FDFComboStep& OutStep) const
{
	const TArray<FDFComboStep>& Steps =
		(IsOwnerAirborne() && AerialComboSteps.Num() > 0) ? AerialComboSteps : ComboSteps;
	if (!Steps.IsValidIndex(Step))
	{
		return false;
	}
	OutStep = Steps[Step];
	return true;
}

UAnimMontage* UDFComboComponent::PickComboVariant(const TArray<FDFComboVariant>& Variants) const
{
	if (Variants.Num() == 0)
	{
		return nullptr;
	}

	const AActor* const Owner = GetOwner();
	const ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(Owner);
	const UAbilitySystemComponent* const ASC = PC ? PC->GetAbilitySystemComponent() : nullptr;

	FGameplayTagContainer AttackerTags;
	if (ASC)
	{
		ASC->GetOwnedGameplayTags(AttackerTags);
	}

	AActor* TargetActor = nullptr;
	if (PC && PC->MeleeAim)
	{
		TargetActor = PC->MeleeAim->ResolveCurrentTarget();
	}
	FGameplayTagContainer TargetTags;
	if (const IAbilitySystemInterface* const TargetASI = Cast<IAbilitySystemInterface>(TargetActor))
	{
		if (UAbilitySystemComponent* const TargetASC = TargetASI->GetAbilitySystemComponent())
		{
			TargetASC->GetOwnedGameplayTags(TargetTags);
		}
	}

	TArray<int32> EligibleIdx;
	float WeightSum = 0.f;
	for (int32 i = 0; i < Variants.Num(); ++i)
	{
		const FDFComboVariant& V = Variants[i];
		if (!V.Montage)
		{
			continue;
		}
		if (!V.RequiredAttackerTags.IsEmpty() && !AttackerTags.HasAll(V.RequiredAttackerTags))
		{
			continue;
		}
		if (!V.BlockedAttackerTags.IsEmpty() && AttackerTags.HasAny(V.BlockedAttackerTags))
		{
			continue;
		}
		if (!V.RequiredTargetTags.IsEmpty() && !TargetTags.HasAll(V.RequiredTargetTags))
		{
			continue;
		}
		EligibleIdx.Add(i);
		WeightSum += V.Weight;
	}
	if (EligibleIdx.Num() == 0 || WeightSum <= 0.f)
	{
		for (const FDFComboVariant& V : Variants)
		{
			if (V.Montage)
			{
				return V.Montage;
			}
		}
		return nullptr;
	}
	float Roll = FMath::FRand() * WeightSum;
	for (const int32 Idx : EligibleIdx)
	{
		Roll -= Variants[Idx].Weight;
		if (Roll <= 0.f)
		{
			return Variants[Idx].Montage;
		}
	}
	return Variants[EligibleIdx.Last()].Montage;
}

UAnimMontage* UDFComboComponent::ResolveStepMontageFromData(const FDFComboStep& StepData) const
{
	if (bComboHeavyFinisherPending)
	{
		if (StepData.HeavyBranchVariants.Num() > 0)
		{
			if (UAnimMontage* const Pick = PickComboVariant(StepData.HeavyBranchVariants))
			{
				return Pick;
			}
		}
		if (StepData.HeavyBranchMontage)
		{
			return StepData.HeavyBranchMontage;
		}
	}
	if (StepData.LightVariants.Num() > 0)
	{
		if (UAnimMontage* const Pick = PickComboVariant(StepData.LightVariants))
		{
			return Pick;
		}
	}
	return StepData.LightMontage;
}

void UDFComboComponent::EvaluateComboCurveWindow()
{
	if (!bUseCurveInsteadOfNotify || !bPlayingComboMontage)
	{
		return;
	}
	UAnimInstance* const Anim = GetAnimInstance();
	if (!Anim)
	{
		return;
	}
	const float Val = Anim->GetCurveValue(ComboWindowCurveName);
	const bool bShouldOpen = Val >= ComboWindowCurveThreshold;
	if (bShouldOpen && !bComboWindowActive)
	{
		AdvanceCombo(TEXT("CurveOpen"), Anim->GetCurrentActiveMontage());
	}
	else if (!bShouldOpen && bComboWindowActive && Val < ComboWindowCurveThreshold * 0.5f)
	{
		if (UWorld* const W = GetWorld())
		{
			W->GetTimerManager().ClearTimer(ComboWindowTimer);
		}
		bComboWindowActive = false;
		ComboWindowExpireTime = -1.f;
	}
}

void UDFComboComponent::OnRep_LockedComboActivationStep()
{
	if (LockedComboActivationStep < 0)
	{
		LastRepLockedComboStep = LockedComboActivationStep;
		return;
	}
	if (LockedComboActivationStep == LastRepLockedComboStep)
	{
		return;
	}
	const int32 PreviousValue = LastRepLockedComboStep;
	LastRepLockedComboStep = LockedComboActivationStep;

	const int32 LocalStep = CurrentComboStep;
	const int32 ServerStep = LockedComboActivationStep;
	if (FMath::Abs(LocalStep - ServerStep) >= 1)
	{
		UE_LOG(LogDungeonForged, Warning,
			TEXT("[Combo|Rollback] local=%d server=%d (prev=%d) → reconciling"),
			LocalStep, ServerStep, PreviousValue);
		if (UAnimInstance* Anim = GetAnimInstance())
		{
			for (const TObjectPtr<UAnimMontage>& M : ComboMontages)
			{
				if (M && Anim->Montage_IsPlaying(M))
				{
					Anim->Montage_Stop(ComboChainMontageStopBlendOutTime, M);
				}
			}
		}
		CurrentComboStep = ServerStep;
		PlayCurrentComboMontage();
	}
}

bool UDFComboComponent::TryHandleFinisherPrimaryInput()
{
	AActor* const Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}
	ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(Owner);
	UAbilitySystemComponent* const ASC = PC ? PC->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return false;
	}
	if (FDFGameplayTags::Ability_Warrior_Execute.IsValid())
	{
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (!Spec.IsActive() || !Spec.Ability)
			{
				continue;
			}
			if (!Spec.Ability->AbilityTags.HasTag(FDFGameplayTags::Ability_Warrior_Execute))
			{
				continue;
			}
			if (FDFGameplayTags::Event_Combat_Finisher_Input.IsValid())
			{
				FGameplayEventData Payload;
				Payload.EventTag = FDFGameplayTags::Event_Combat_Finisher_Input;
				Payload.Instigator = Owner;
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
					Owner, FDFGameplayTags::Event_Combat_Finisher_Input, Payload);
				return true;
			}
		}
	}
	if (FDFGameplayTags::State_Combat_FinisherReady.IsValid()
		&& ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Combat_FinisherReady)
		&& FDFGameplayTags::Ability_Warrior_Execute.IsValid())
	{
		FGameplayTagContainer Tags;
		Tags.AddTag(FDFGameplayTags::Ability_Warrior_Execute);
		return ASC->TryActivateAbilitiesByTag(Tags, true);
	}
	return false;
}

void UDFComboComponent::Server_ChainMeleeComboStep_Implementation(const int32 Step)
{
	const int32 ClampedStep = FMath::Max(0, Step);
	LockedComboActivationStep = ClampedStep;
	PendingComboActivationStep = ClampedStep;
	CurrentComboStep = ClampedStep;
	bComboChainAdvancePending = true;
	PrepareForComboChainActivation();
	(void)TryActivatePrimaryMeleeGameplayAbility();
}

void UDFComboComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!MeleeTrace)
	{
		if (AActor* O = GetOwner())
		{
			MeleeTrace = O->FindComponentByClass<UDFMeleeTraceComponent>();
		}
	}
	SetComponentTickEnabled(true);
}

void UDFComboComponent::TickComponent(const float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TryAdvanceComboBranchFromHold();
	EvaluateComboCurveWindow();
#if !UE_BUILD_SHIPPING
	const bool bWantCombo = DFCombatDebug::IsChannelEnabled(DFCombatDebug::EChannel::Combo);
	const bool bWantHeavy = DFCombatDebug::IsChannelEnabled(DFCombatDebug::EChannel::Heavy);
	if (bWantCombo || bWantHeavy)
	{
		DrawCombatDebug();
	}
#endif
}

int32 UDFComboComponent::GetEffectiveMaxComboSteps() const
{
	const int32 MontageCount = ComboSteps.Num() > 0 ? ComboSteps.Num() : ComboMontages.Num();
	if (MontageCount <= 0)
	{
		return MaxComboSteps;
	}
	return FMath::Min(MaxComboSteps, MontageCount);
}

void UDFComboComponent::ApplyComboStepData(const TArray<FDFComboStep>& Steps)
{
	ComboSteps = Steps;
	ComboMontages.Empty();
	ComboMontages.Reserve(ComboSteps.Num());
	for (const FDFComboStep& Step : ComboSteps)
	{
		UAnimMontage* M = Step.LightMontage;
		if (!M && Step.LightVariants.Num() > 0)
		{
			M = Step.LightVariants[0].Montage;
		}
		ComboMontages.Add(M);
	}
}

void UDFComboComponent::ClearComboStepData()
{
	ComboSteps.Empty();
}

void UDFComboComponent::BufferComboInputAndTryAdvance()
{
	bComboInputBuffered = true;
	if (bComboWindowActive)
	{
		AdvanceCombo();
	}
}

void UDFComboComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindMontageEndDelegate();
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(ComboWindowTimer);
	}
	Super::EndPlay(EndPlayReason);
}

UAnimInstance* UDFComboComponent::GetAnimInstance() const
{
	if (ACharacter* C = Cast<ACharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* M = C->GetMesh())
		{
			return M->GetAnimInstance();
		}
	}
	return nullptr;
}

void UDFComboComponent::UnbindMontageEndDelegate()
{
	if (UAnimInstance* A = GetAnimInstance())
	{
		if (LastBoundMontageForEnd)
		{
			FOnMontageEnded ClearDelegate;
			A->Montage_SetEndDelegate(ClearDelegate, LastBoundMontageForEnd.Get());
		}
	}
	LastBoundMontageForEnd = nullptr;
}

void UDFComboComponent::TryBindEndDelegateFor(UAnimMontage* Montage)
{
	UAnimInstance* A = GetAnimInstance();
	if (!A || !Montage)
	{
		return;
	}
	if (LastBoundMontageForEnd && LastBoundMontageForEnd != Montage)
	{
		FOnMontageEnded ClearDelegate;
		A->Montage_SetEndDelegate(ClearDelegate, LastBoundMontageForEnd.Get());
	}
	LastBoundMontageForEnd = Montage;
	FOnMontageEnded D;
	D.BindUObject(this, &UDFComboComponent::HandleMontageEndedInternal);
	A->Montage_SetEndDelegate(D, Montage);
}

void UDFComboComponent::ApplyCombatTuningFromDataAsset()
{
	const UDFCombatTuningData* const Tuning = UDFAssetManager::Get().GetCombatTuningData();
	if (!Tuning)
	{
		return;
	}
	ComboWindowDuration = Tuning->ComboWindowDuration;
	HeavyChargeThreshold = Tuning->HeavyChargeThreshold;
	HeavyDamageMultiplier = Tuning->HeavyDamageMultiplier;
	HeavyKnockbackMultiplier = Tuning->HeavyKnockbackMultiplier;
	HeavyStaminaCost = Tuning->HeavyStaminaCost;
	HeavyTraceRadiusBonus = Tuning->HeavyTraceRadiusBonus;
	AttackInputBufferDuration = Tuning->AttackInputBufferDuration;
}

void UDFComboComponent::StartChargeWindupMontage()
{
	if (!ChargeWindupMontage || bPlayingComboMontage)
	{
		return;
	}
	UAnimInstance* const AnimInst = GetAnimInstance();
	if (!AnimInst || AnimInst->Montage_IsPlaying(ChargeWindupMontage))
	{
		return;
	}
	if (AnimInst->Montage_Play(ChargeWindupMontage, 1.f) > 0.f)
	{
		bPlayingChargeWindup = true;
	}
}

void UDFComboComponent::StopChargeWindupMontage()
{
	if (!bPlayingChargeWindup)
	{
		return;
	}
	if (UAnimInstance* const AnimInst = GetAnimInstance())
	{
		if (ChargeWindupMontage)
		{
			AnimInst->Montage_Stop(0.1f, ChargeWindupMontage);
		}
	}
	bPlayingChargeWindup = false;
}

void UDFComboComponent::TryAdvanceComboBranchFromHold()
{
	if (!bComboWindowActive || !bComboInputBuffered || ComboBranchPressTime < 0.f)
	{
		return;
	}
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	const float Held = W->GetTimeSeconds() - ComboBranchPressTime;
	if (Held < HeavyChargeThreshold)
	{
		return;
	}
	bComboHeavyFinisherPending = true;
	ComboBranchPressTime = -1.f;
	AdvanceCombo();
}

void UDFComboComponent::OnPrimaryAttackPressed()
{
	if (TryHandleFinisherPrimaryInput())
	{
		return;
	}
	if (bComboWindowActive)
	{
		bComboInputBuffered = true;
		bComboHeavyFinisherPending = false;
		if (UWorld* const W = GetWorld())
		{
			ComboBranchPressTime = W->GetTimeSeconds();
		}
		return;
	}
	if (bPlayingComboMontage)
	{
		if (UWorld* const W = GetWorld())
		{
			bSwingInputBuffered = true;
			SwingInputBufferExpireTime = W->GetTimeSeconds() + AttackInputBufferDuration;
		}
		return;
	}
	if (UWorld* const W = GetWorld())
	{
		HeavyChargeStartTime = W->GetTimeSeconds();
	}
	StartChargeWindupMontage();
}

void UDFComboComponent::OnPrimaryAttackReleased()
{
	if (bComboWindowActive && bComboInputBuffered)
	{
		UWorld* const W = GetWorld();
		const float Held = (W && ComboBranchPressTime >= 0.f)
			? W->GetTimeSeconds() - ComboBranchPressTime
			: 0.f;
		ComboBranchPressTime = -1.f;
		bComboHeavyFinisherPending = Held >= HeavyChargeThreshold;
		AdvanceCombo();
		return;
	}
	if (bComboWindowActive)
	{
		return;
	}
	if (HeavyChargeStartTime < 0.f)
	{
		return;
	}
	UWorld* const W = GetWorld();
	if (!W)
	{
		HeavyChargeStartTime = -1.f;
		StopChargeWindupMontage();
		return;
	}
	const float Held = W->GetTimeSeconds() - HeavyChargeStartTime;
	HeavyChargeStartTime = -1.f;
	const bool bHadChargeWindup = bPlayingChargeWindup;
	StopChargeWindupMontage();
	bPendingChargeReleaseMontage = bHadChargeWindup && HeavyChargeReleaseMontage != nullptr;

	if (Held >= MaxHeavyChargeThreshold)
	{
		CommitMaxHeavyAttack();
	}
	else if (Held >= HeavyChargeThreshold)
	{
		CommitHeavyAttack();
	}
	else
	{
		bPendingChargeReleaseMontage = false;
		OnAttackInput();
	}
}

void UDFComboComponent::OnAttackInput()
{
	if (TryHandleFinisherPrimaryInput())
	{
		return;
	}
	if (bComboWindowActive)
	{
		BufferComboInputAndTryAdvance();
		return;
	}
	if (!bPlayingComboMontage)
	{
		if (LockedComboActivationStep < 0 && !bComboChainAdvancePending)
		{
			PrimeMeleeSwingAbilityChain();
		}
		if (ShouldRoutePrimaryMeleeThroughGAS())
		{
			// Do not fall back to StartCombo when GAS blocks activation (e.g. cooldown).
			(void)TryActivatePrimaryMeleeGameplayAbility();
			return;
		}
		StartCombo();
	}
}

bool UDFComboComponent::ShouldRoutePrimaryMeleeThroughGAS() const
{
	const ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(GetOwner());
	if (!PC || PC->bDisableWarriorMeleeSwingGameplayAbility)
	{
		return false;
	}
	if (UDFEquipmentComponent* const Eq = PC->Equipment)
	{
		if (!Eq->IsSlotEmpty(EEquipmentSlot::Weapon) && Eq->HasGrantedWeaponMeleeAbilitySpec())
		{
			return true;
		}
		if (Eq->IsSlotEmpty(EEquipmentSlot::Weapon))
		{
			return PC->GetDefaultUnarmedMeleeAbilityTag().IsValid();
		}
	}
	return FDFGameplayTags::Ability_Warrior_MeleeSwing.IsValid();
}

void UDFComboComponent::PrimeMeleeSwingAbilityChain()
{
	if (LockedComboActivationStep >= 0 || bComboChainAdvancePending || PendingComboActivationStep >= 0)
	{
		return;
	}
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(ComboWindowTimer);
	}
	bComboWindowActive = false;
	bComboInputBuffered = false;
	CurrentComboStep = 0;
}

bool UDFComboComponent::ShouldRouteHeavyAttackThroughGAS() const
{
	return ShouldRoutePrimaryMeleeThroughGAS();
}

bool UDFComboComponent::TryActivateHeavyAttackGameplayAbility()
{
	ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(GetOwner());
	if (!PC)
	{
		return false;
	}
	UAbilitySystemComponent* const ASC = PC->GetAbilitySystemComponent();
	if (!ASC || !FDFGameplayTags::Ability_Warrior_HeavyAttack.IsValid())
	{
		return false;
	}
	FGameplayTagContainer Tags;
	Tags.AddTag(FDFGameplayTags::Ability_Warrior_HeavyAttack);
	return ASC->TryActivateAbilitiesByTag(Tags, true);
}

bool UDFComboComponent::TryActivatePrimaryMeleeGameplayAbility()
{
	ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(GetOwner());
	if (!PC)
	{
		return false;
	}
	UAbilitySystemComponent* const ASC = PC->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}
	if (UDFEquipmentComponent* const Eq = PC->Equipment)
	{
		if (!Eq->IsSlotEmpty(EEquipmentSlot::Weapon))
		{
			if (Eq->HasGrantedWeaponMeleeAbilitySpec())
			{
				return Eq->TryActivateGrantedWeaponMeleeAbility();
			}
		}
		else
		{
			const FGameplayTag UnarmedTag = PC->GetDefaultUnarmedMeleeAbilityTag();
			if (UnarmedTag.IsValid())
			{
				FGameplayTagContainer UnarmedTags;
				UnarmedTags.AddTag(UnarmedTag);
				if (ASC->TryActivateAbilitiesByTag(UnarmedTags, true))
				{
					return true;
				}
			}
		}
	}
	if (!FDFGameplayTags::Ability_Warrior_MeleeSwing.IsValid())
	{
		return false;
	}
	FGameplayTagContainer Tags;
	Tags.AddTag(FDFGameplayTags::Ability_Warrior_MeleeSwing);
	return ASC->TryActivateAbilitiesByTag(Tags, true);
}

void UDFComboComponent::NotifyHeavyAbilitySwingMontageStarted(UAnimMontage* Montage)
{
	if (!Montage)
	{
		return;
	}
	bPendingChargeReleaseMontage = false;
	AActor* const Owner = GetOwner();
	if (Owner && Owner->HasAuthority() && MeleeTrace)
	{
		// Use the multipliers that match the *pending* tier — set by CommitHeavy / CommitMaxHeavy before this fires.
		if (bMaxHeavyPending)
		{
			MeleeTrace->ConfigureHeavySwing(MaxHeavyDamageMultiplier, MaxHeavyKnockbackMultiplier, MaxHeavyTraceRadiusBonus);
		}
		else
		{
			MeleeTrace->ConfigureHeavySwing(HeavyDamageMultiplier, HeavyKnockbackMultiplier, HeavyTraceRadiusBonus);
		}
	}
	bHeavySwingPending = true;
	NotifyAbilitySwingMontageStarted(Montage);
}

void UDFComboComponent::PrepareForComboChainActivation()
{
	UnbindMontageEndDelegate();
	if (UAnimInstance* const AnimInst = GetAnimInstance())
	{
		for (const TObjectPtr<UAnimMontage>& M : ComboMontages)
		{
			if (M && AnimInst->Montage_IsPlaying(M))
			{
				AnimInst->Montage_Stop(FMath::Max(0.f, ComboChainMontageStopBlendOutTime), M);
			}
		}
	}
	bPlayingComboMontage = false;
}

void UDFComboComponent::NotifyAbilitySwingMontageStarted(UAnimMontage* Montage)
{
	if (!Montage)
	{
		return;
	}
	bSwingHitConfirmedThisActivation = false;
	bComboHeavyFinisherPending = false;
	ComboBranchPressTime = -1.f;
	if (!ShouldRoutePrimaryMeleeThroughGAS())
	{
		TryBindEndDelegateFor(Montage);
	}
	else
	{
		UnbindMontageEndDelegate();
	}
	bPlayingComboMontage = true;
	if (MeleeTrace)
	{
		MeleeTrace->ScheduleAuthorityTraceWindowsFromMontage(Montage, 1.f);
	}
#if !UE_BUILD_SHIPPING
	if (DFCombatDebug::IsChannelEnabled(DFCombatDebug::EChannel::Combo))
	{
		UE_LOG(LogDungeonForged, Log, TEXT("[Combo|GAS] NotifySwingMontage step=%d montage=%s"),
			CurrentComboStep, *Montage->GetName());
	}
#endif
}

void UDFComboComponent::NotifyAbilitySwingMontagePlaybackEnded()
{
	bPlayingComboMontage = false;
}

void UDFComboComponent::StartCombo()
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(ComboWindowTimer);
	}
	bComboWindowActive = false;
	bComboInputBuffered = false;
	CurrentComboStep = 0;
	PlayCurrentComboMontage();
}

void UDFComboComponent::PlayCurrentComboMontage()
{
	UAnimMontage* const M = ResolveDirectionalComboMontage(CurrentComboStep);
	if (!M)
	{
		ResetCombo();
		return;
	}
	UAnimInstance* const AnimInst = GetAnimInstance();
	if (!AnimInst)
	{
		ResetCombo();
		return;
	}
	if (AnimInst->Montage_Play(M) <= 0.f)
	{
		ResetCombo();
		return;
	}
	TryBindEndDelegateFor(M);
	bPlayingComboMontage = true;
	if (MeleeTrace)
	{
		MeleeTrace->ScheduleAuthorityTraceWindowsFromMontage(M, 1.f);
	}
}

void UDFComboComponent::AdvanceCombo(const FName DebugSource, UAnimMontage* const MontageContext)
{
	UWorld* const W = GetWorld();
	if (W)
	{
		W->GetTimerManager().ClearTimer(ComboWindowTimer);
	}
	bComboWindowActive = false;

	const int32 EffectiveMax = GetEffectiveMaxComboSteps();
	if (bComboInputBuffered)
	{
		if (CurrentComboStep + 1 < EffectiveMax)
		{
			++CurrentComboStep;
			bComboInputBuffered = false;
			if (ShouldRoutePrimaryMeleeThroughGAS())
			{
				const int32 ChainStep = CurrentComboStep;
				LockedComboActivationStep = ChainStep;
				PendingComboActivationStep = ChainStep;
				bComboChainAdvancePending = true;
				AActor* const Owner = GetOwner();
				if (Owner && Owner->HasAuthority())
				{
					PrepareForComboChainActivation();
					(void)TryActivatePrimaryMeleeGameplayAbility();
				}
				else if (Owner)
				{
					Server_ChainMeleeComboStep(ChainStep);
					PrepareForComboChainActivation();
					(void)TryActivatePrimaryMeleeGameplayAbility();
				}
#if !UE_BUILD_SHIPPING
				if (DFCombatDebug::IsChannelEnabled(DFCombatDebug::EChannel::Combo))
				{
					UE_LOG(LogDungeonForged, Log,
						TEXT("[Combo|GAS] AdvanceCombo chain -> step=%d locked=%d montages=%d"),
						CurrentComboStep, LockedComboActivationStep, ComboMontages.Num());
				}
#endif
				return;
			}
			PlayCurrentComboMontage();
			return;
		}
		bComboInputBuffered = false;
	}

	if (CurrentComboStep + 1 < EffectiveMax)
	{
		bComboWindowActive = true;
		ArmComboWindowTimer();
#if !UE_BUILD_SHIPPING
		RecordComboWindowOpened(DebugSource,
			MontageContext ? MontageContext : ResolveDirectionalComboMontage(CurrentComboStep));
#endif
	}
}

void UDFComboComponent::OnComboWindowTimerExpired()
{
	UWorld* const W = GetWorld();
	if (W)
	{
		if (UDFHitStopSubsystem* const HS = W->GetSubsystem<UDFHitStopSubsystem>())
		{
			if (HS->IsHitStopActive())
			{
				W->GetTimerManager().SetTimer(
					ComboWindowTimer, this, &UDFComboComponent::OnComboWindowTimerExpired, 0.02f, false);
				return;
			}
		}
		if (W->GetTimeSeconds() < ComboWindowExpireTime)
		{
			const float Remaining = FMath::Max(0.01f, ComboWindowExpireTime - W->GetTimeSeconds());
			W->GetTimerManager().SetTimer(
				ComboWindowTimer, this, &UDFComboComponent::OnComboWindowTimerExpired, Remaining, false);
			return;
		}
	}
	bComboWindowActive = false;
	ComboWindowExpireTime = -1.f;
#if !UE_BUILD_SHIPPING
	if (DFCombatDebug::IsChannelEnabled(DFCombatDebug::EChannel::Combo))
	{
		RecordComboWindowClosed(FName(TEXT("TimerExpired")));
	}
#endif
}

void UDFComboComponent::ResetCombo()
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(ComboWindowTimer);
	}
	if (MeleeTrace)
	{
		MeleeTrace->ClearScheduledTraceWindows();
		MeleeTrace->EndTrace();
		MeleeTrace->ClearHeavySwing();
	}
	UnbindMontageEndDelegate();
	if (UAnimInstance* A = GetAnimInstance())
	{
		for (TObjectPtr<UAnimMontage> M : ComboMontages)
		{
			if (M && A->Montage_IsPlaying(M))
			{
				A->Montage_Stop(0.2f, M);
			}
		}
		if (UAnimMontage* const HeavyM = ResolveHeavyAttackMontage())
		{
			if (A->Montage_IsPlaying(HeavyM))
			{
				A->Montage_Stop(0.2f, HeavyM);
			}
		}
	}
	CurrentComboStep = 0;
	bComboInputBuffered = false;
	bComboWindowActive = false;
	bPlayingComboMontage = false;
	bHeavySwingPending = false;
	bMaxHeavyPending = false;
	bComboChainAdvancePending = false;
	LockedComboActivationStep = -1;
	PendingComboActivationStep = -1;
	HeavyChargeStartTime = -1.f;
	ComboBranchPressTime = -1.f;
	bComboHeavyFinisherPending = false;
	bPendingChargeReleaseMontage = false;
	ComboWindowExpireTime = -1.f;
	StopChargeWindupMontage();
}

UAnimMontage* UDFComboComponent::ResolveHeavyAttackMontage() const
{
	if (HeavyAttackMontage)
	{
		return HeavyAttackMontage.Get();
	}
	if (ComboMontages.Num() > 0 && ComboMontages[0])
	{
		return ComboMontages[0].Get();
	}
	return nullptr;
}

UAnimMontage* UDFComboComponent::ResolveMaxHeavyAttackMontage() const
{
	if (MaxHeavyAttackMontage)
	{
		return MaxHeavyAttackMontage.Get();
	}
	return ResolveHeavyAttackMontage();
}

UAnimMontage* UDFComboComponent::ResolveActiveHeavyMontage() const
{
	if (bMaxHeavyPending)
	{
		return ResolveMaxHeavyAttackMontage();
	}
	if (bPendingChargeReleaseMontage && HeavyChargeReleaseMontage)
	{
		return HeavyChargeReleaseMontage;
	}
	return ResolveHeavyAttackMontage();
}

UAnimMontage* UDFComboComponent::ResolveDirectionalComboMontage(const int32 Step) const
{
	const TArray<FDFComboStep>& ActiveSteps =
		(IsOwnerAirborne() && AerialComboSteps.Num() > 0) ? AerialComboSteps : ComboSteps;
	if (ActiveSteps.IsValidIndex(Step))
	{
		if (UAnimMontage* const Resolved = ResolveStepMontageFromData(ActiveSteps[Step]))
		{
			return Resolved;
		}
	}

	const AActor* const Owner = GetOwner();
	if (!Owner)
	{
		return ComboMontages.IsValidIndex(Step) ? ComboMontages[Step].Get() : nullptr;
	}

	FVector LocalInput = FVector::ZeroVector;
	if (const ACharacter* const Char = Cast<ACharacter>(Owner))
	{
		if (const UCharacterMovementComponent* const CMC = Char->GetCharacterMovement())
		{
			LocalInput = Char->GetActorTransform().InverseTransformVectorNoScale(CMC->GetLastInputVector());
		}
	}
	if (LocalInput.SizeSquared2D() < DirectionalInputThreshold * DirectionalInputThreshold)
	{
		const FVector WorldVel = Owner->GetVelocity();
		if (WorldVel.SizeSquared2D() >= DirectionalInputThreshold * DirectionalInputThreshold)
		{
			LocalInput = Owner->GetActorTransform().InverseTransformVectorNoScale(WorldVel);
		}
	}

	if (LocalInput.SizeSquared2D() < DirectionalInputThreshold * DirectionalInputThreshold)
	{
		return ComboMontages.IsValidIndex(Step) ? ComboMontages[Step].Get() : nullptr;
	}
	const float Threshold = DirectionalInputThreshold;
	if (LocalInput.X < -Threshold)
	{
		if (BackwardComboMontages.IsValidIndex(Step) && BackwardComboMontages[Step])
		{
			return BackwardComboMontages[Step].Get();
		}
	}
	else if (FMath::Abs(LocalInput.Y) > Threshold && FMath::Abs(LocalInput.Y) > FMath::Abs(LocalInput.X))
	{
		if (SideComboMontages.IsValidIndex(Step) && SideComboMontages[Step])
		{
			return SideComboMontages[Step].Get();
		}
	}
	return ComboMontages.IsValidIndex(Step) ? ComboMontages[Step].Get() : nullptr;
}

bool UDFComboComponent::ConsumeHeavyStamina()
{
	AActor* const Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || HeavyStaminaCost <= 0.f)
	{
		return true;
	}
	ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(Owner);
	if (!PC)
	{
		return true;
	}
	UAbilitySystemComponent* const ASC = PC->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}
	const UDFAttributeSet* const Attrs = ASC->GetSet<UDFAttributeSet>();
	if (!Attrs)
	{
		return false;
	}
	const float Current = Attrs->GetStamina();
	if (Current < HeavyStaminaCost)
	{
		return false;
	}
	const_cast<UDFAttributeSet*>(Attrs)->SetStamina(FMath::Max(0.f, Current - HeavyStaminaCost));
	return true;
}

bool UDFComboComponent::CanPerformHeavyAttack() const
{
	if (bPlayingComboMontage && !bHeavySwingPending)
	{
		return false;
	}
	if (!ResolveHeavyAttackMontage())
	{
		return false;
	}
	const ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(GetOwner());
	if (!PC)
	{
		return false;
	}
	if (UAbilitySystemComponent* const ASC = PC->GetAbilitySystemComponent())
	{
		if (FDFGameplayTags::State_Dead.IsValid() && ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Dead))
		{
			return false;
		}
		if (HeavyStaminaCost > 0.f)
		{
			const UDFAttributeSet* const Attrs = ASC->GetSet<UDFAttributeSet>();
			if (!Attrs || Attrs->GetStamina() < HeavyStaminaCost)
			{
				return false;
			}
		}
	}
	return true;
}

void UDFComboComponent::ServerCommitHeavyAttack()
{
	if (!CanPerformHeavyAttack())
	{
		return;
	}
	PrimeMeleeSwingAbilityChain();
	ExecuteHeavyAttackAuthority();
}

void UDFComboComponent::CommitHeavyAttack()
{
	if (!CanPerformHeavyAttack())
	{
		return;
	}
	bMaxHeavyPending = false;
	PrimeMeleeSwingAbilityChain();

	if (ShouldRouteHeavyAttackThroughGAS())
	{
		// Sync tier to server BEFORE the GA activation RPC so server-side GA reads the right tier.
		// Both RPCs use the same reliable channel → ordering is guaranteed.
		if (ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(GetOwner()))
		{
			if (!PC->HasAuthority())
			{
				PC->Server_NotifyHeavyAttackTier(false);
			}
		}
		(void)TryActivateHeavyAttackGameplayAbility();
		return;
	}

	ACharacter* const Character = Cast<ACharacter>(GetOwner());
	const bool bAuthority = GetOwner() && GetOwner()->HasAuthority();
	const bool bLocallyControlled = Character && Character->IsLocallyControlled();

	if (bAuthority)
	{
		ExecuteHeavyAttackAuthority();
	}
	else if (ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(GetOwner()))
	{
		PC->Server_CommitHeavyAttack();
	}

	if (bLocallyControlled)
	{
		ExecuteHeavyAttackPresentation();
	}
}

void UDFComboComponent::CommitMaxHeavyAttack()
{
	if (!CanPerformMaxHeavyAttack())
	{
		// Fall back gracefully to normal heavy if max isn't available (no montage / no stamina).
		CommitHeavyAttack();
		return;
	}
	bMaxHeavyPending = true;
	PrimeMeleeSwingAbilityChain();

	if (ShouldRouteHeavyAttackThroughGAS())
	{
		// Same ability handles both tiers; reads bMaxHeavyPending via ResolveActiveHeavyMontage.
		// Server RPC syncs the tier flag BEFORE the GA replicates (same reliable channel = ordered).
		if (ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(GetOwner()))
		{
			if (!PC->HasAuthority())
			{
				PC->Server_NotifyHeavyAttackTier(true);
			}
		}
		(void)TryActivateHeavyAttackGameplayAbility();
		return;
	}

	ACharacter* const Character = Cast<ACharacter>(GetOwner());
	const bool bAuthority = GetOwner() && GetOwner()->HasAuthority();
	const bool bLocallyControlled = Character && Character->IsLocallyControlled();

	if (bAuthority)
	{
		ExecuteMaxHeavyAttackAuthority();
	}
	else if (ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(GetOwner()))
	{
		PC->Server_CommitHeavyAttack(); // server resolves via bMaxHeavyPending (replicated through ability)
	}

	if (bLocallyControlled)
	{
		ExecuteMaxHeavyAttackPresentation();
	}
}

bool UDFComboComponent::CanPerformMaxHeavyAttack() const
{
	if (!ResolveMaxHeavyAttackMontage())
	{
		return false;
	}
	const ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(GetOwner());
	if (!PC)
	{
		return false;
	}
	if (UAbilitySystemComponent* const ASC = PC->GetAbilitySystemComponent())
	{
		if (FDFGameplayTags::State_Dead.IsValid() && ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Dead))
		{
			return false;
		}
		if (MaxHeavyStaminaCost > 0.f)
		{
			const UDFAttributeSet* const Attrs = ASC->GetSet<UDFAttributeSet>();
			if (!Attrs || Attrs->GetStamina() < MaxHeavyStaminaCost)
			{
				return false;
			}
		}
	}
	return true;
}

bool UDFComboComponent::ConsumeMaxHeavyStamina()
{
	AActor* const Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || MaxHeavyStaminaCost <= 0.f)
	{
		return true;
	}
	ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(Owner);
	if (!PC)
	{
		return true;
	}
	UAbilitySystemComponent* const ASC = PC->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}
	const UDFAttributeSet* const Attrs = ASC->GetSet<UDFAttributeSet>();
	if (!Attrs)
	{
		return false;
	}
	const float Current = Attrs->GetStamina();
	if (Current < MaxHeavyStaminaCost)
	{
		return false;
	}
	const_cast<UDFAttributeSet*>(Attrs)->SetStamina(FMath::Max(0.f, Current - MaxHeavyStaminaCost));
	return true;
}

void UDFComboComponent::ExecuteMaxHeavyAttackAuthority()
{
	if (!ConsumeMaxHeavyStamina())
	{
		return;
	}
	UAnimMontage* const Montage = ResolveMaxHeavyAttackMontage();
	if (!Montage || !MeleeTrace)
	{
		return;
	}
	MeleeTrace->ConfigureHeavySwing(MaxHeavyDamageMultiplier, MaxHeavyKnockbackMultiplier, MaxHeavyTraceRadiusBonus);
	MeleeTrace->ScheduleAuthorityTraceWindowsFromMontage(Montage, 1.f);
}

void UDFComboComponent::ExecuteMaxHeavyAttackPresentation()
{
	UAnimMontage* const Montage = ResolveMaxHeavyAttackMontage();
	if (!Montage)
	{
		return;
	}
	UAnimInstance* const AnimInst = GetAnimInstance();
	if (!AnimInst)
	{
		return;
	}
	if (AnimInst->Montage_Play(Montage) <= 0.f)
	{
		return;
	}
	TryBindEndDelegateFor(Montage);
	bPlayingComboMontage = true;
	bHeavySwingPending = true;
}

void UDFComboComponent::ExecuteHeavyAttackPresentation()
{
	UAnimMontage* const Montage = ResolveHeavyAttackMontage();
	if (!Montage)
	{
		return;
	}
	UAnimInstance* const AnimInst = GetAnimInstance();
	if (!AnimInst)
	{
		return;
	}
	if (AnimInst->Montage_Play(Montage) <= 0.f)
	{
		return;
	}
	TryBindEndDelegateFor(Montage);
	bPlayingComboMontage = true;
	bHeavySwingPending = true;
}

void UDFComboComponent::ExecuteHeavyAttackAuthority()
{
	if (!ConsumeHeavyStamina())
	{
		return;
	}
	UAnimMontage* const Montage = ResolveHeavyAttackMontage();
	if (!Montage || !MeleeTrace)
	{
		return;
	}
	MeleeTrace->ConfigureHeavySwing(HeavyDamageMultiplier, HeavyKnockbackMultiplier, HeavyTraceRadiusBonus);
	MeleeTrace->ScheduleAuthorityTraceWindowsFromMontage(Montage, 1.f);
}

void UDFComboComponent::HandleMontageEndedInternal(UAnimMontage* EndedMontage, bool bInterrupted)
{
	(void)EndedMontage;
	if (ShouldRoutePrimaryMeleeThroughGAS())
	{
		return;
	}
	if (bInterrupted)
	{
		if (CurrentComboStep > 0)
		{
			return;
		}
		bSwingInputBuffered = false;
		SwingInputBufferExpireTime = -1.f;
		return;
	}
	const bool bBufferedSwing = bSwingInputBuffered;
	float BufferExpire = SwingInputBufferExpireTime;
	bSwingInputBuffered = false;
	SwingInputBufferExpireTime = -1.f;
	ResetCombo();
	if (bBufferedSwing)
	{
		if (!IsInputBufferExpired(BufferExpire))
		{
			OnAttackInput();
		}
	}
}

void UDFComboComponent::OnMontageEnded(UAnimMontage* const EndedMontage, const bool bInterrupted)
{
	HandleMontageEndedInternal(EndedMontage, bInterrupted);
}

void UDFComboComponent::DrawCombatDebug() const
{
#if !UE_BUILD_SHIPPING
	const AActor* const Owner = GetOwner();
	UWorld* const World = GetWorld();
	if (!Owner || !World)
	{
		return;
	}
	const FVector BaseLoc = Owner->GetActorLocation() + FVector(0.f, 0.f, 110.f);
	int32 Line = 0;
	auto DrawLine = [&](const FColor Color, const FString& Text)
	{
		DrawDebugString(World, BaseLoc + FVector(0.f, 0.f, 16.f * Line++), Text, nullptr, Color, 0.f, true, 1.15f);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.f, Color, Text);
		}
	};

	if (DFCombatDebug::IsChannelEnabled(DFCombatDebug::EChannel::Combo))
	{
		const int32 DisplayStep = LockedComboActivationStep >= 0 ? LockedComboActivationStep : CurrentComboStep;
		const UAnimMontage* const StepM = ResolveDirectionalComboMontage(DisplayStep);
		float WindowRemain = -1.f;
		if (bComboWindowActive && ComboWindowExpireTime >= 0.f)
		{
			WindowRemain = FMath::Max(0.f, ComboWindowExpireTime - World->GetTimeSeconds());
		}
		DrawLine(FColor::Cyan,
			FString::Printf(TEXT("Combo step %d/%d | locked %d | win %s (%.2fs) | buf %s"),
				DisplayStep,
				GetEffectiveMaxComboSteps() - 1,
				LockedComboActivationStep,
				bComboWindowActive ? TEXT("OPEN") : TEXT("-"),
				WindowRemain,
				bComboInputBuffered ? TEXT("Y") : TEXT("n")));
		if (LastComboWindowOpenWorldTime >= 0.f)
		{
			DrawLine(FColor::Green,
				FString::Printf(TEXT("  last OPEN [%s] @ montage t=%.3fs fr=%d (%s)"),
					*LastComboWindowOpenSource,
					LastComboWindowOpenMontageTime,
					LastComboWindowOpenMontageFrame,
					*LastComboWindowOpenMontageName));
		}
		const DFCombatDebug::FMontagePlaybackSample Live =
			DFCombatDebug::SampleMontagePlayback(GetAnimInstance(), const_cast<UAnimMontage*>(StepM));
		if (Live.bValid)
		{
			DrawLine(FColor::White, FString::Printf(TEXT("  playing: %s"), *DFCombatDebug::FormatMontagePlayback(Live)));
		}
		else if (StepM)
		{
			DrawLine(FColor::White, FString::Printf(TEXT("  step montage (idle): %s"), *StepM->GetName()));
		}
		else if (ComboMontages.Num() == 0)
		{
			DrawLine(FColor::Red, TEXT("  !! ComboMontages EMPTY"));
		}
		if (LastChainRuntimeBlendIn >= 0.f)
		{
			DrawLine(FColor::Silver,
				FString::Printf(TEXT("  last chain runtimeBlendIn=%.3f (component default %.3f)"),
					LastChainRuntimeBlendIn, ComboChainMontageBlendInTime));
		}
	}

	if (DFCombatDebug::IsChannelEnabled(DFCombatDebug::EChannel::Heavy))
	{
		const float ChargeT = HeavyChargeStartTime >= 0.f && World
			? World->GetTimeSeconds() - HeavyChargeStartTime
			: -1.f;
		const UAnimMontage* const HeavyM = ResolveActiveHeavyMontage();
		DrawLine(FColor::Orange,
			FString::Printf(TEXT("Heavy charge %.2fs (thr %.2f/%.2f) | pending H=%s Max=%s"),
				ChargeT,
				HeavyChargeThreshold,
				MaxHeavyChargeThreshold,
				bHeavySwingPending ? TEXT("Y") : TEXT("n"),
				bMaxHeavyPending ? TEXT("Y") : TEXT("n")));
		if (HeavyM)
		{
			DrawLine(FColor::Yellow, FString::Printf(TEXT("  heavy montage: %s"), *HeavyM->GetName()));
		}
	}
#endif
}
