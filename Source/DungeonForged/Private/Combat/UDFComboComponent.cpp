// Source/DungeonForged/Private/Combat/UDFComboComponent.cpp
#include "Combat/UDFComboComponent.h"
#include "AbilitySystemComponent.h"
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
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "Data/UDFCombatTuningData.h"
#include "DFAssetManager.h"
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
#if !UE_BUILD_SHIPPING
	SetComponentTickEnabled(true);
#endif
}

void UDFComboComponent::TickComponent(const float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
#if !UE_BUILD_SHIPPING
	const bool bWantCombo = DFCombatDebug::IsChannelEnabled(DFCombatDebug::EChannel::Combo);
	const bool bWantHeavy = DFCombatDebug::IsChannelEnabled(DFCombatDebug::EChannel::Heavy);
	SetComponentTickEnabled(bWantCombo || bWantHeavy);
	if (bWantCombo || bWantHeavy)
	{
		DrawCombatDebug();
	}
#endif
}

int32 UDFComboComponent::GetEffectiveMaxComboSteps() const
{
	if (ComboMontages.Num() <= 0)
	{
		return MaxComboSteps;
	}
	return FMath::Min(MaxComboSteps, ComboMontages.Num());
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

void UDFComboComponent::OnPrimaryAttackPressed()
{
	if (bComboWindowActive)
	{
		BufferComboInputAndTryAdvance();
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
}

void UDFComboComponent::OnPrimaryAttackReleased()
{
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
		return;
	}
	const float Held = W->GetTimeSeconds() - HeavyChargeStartTime;
	HeavyChargeStartTime = -1.f;

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
		OnAttackInput();
	}
}

void UDFComboComponent::OnAttackInput()
{
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

void UDFComboComponent::AdvanceCombo()
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
		if (W)
		{
			W->GetTimerManager().SetTimer(
				ComboWindowTimer, this, &UDFComboComponent::OnComboWindowTimerExpired, ComboWindowDuration, false);
		}
	}
}

void UDFComboComponent::OnComboWindowTimerExpired()
{
	bComboWindowActive = false;
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
	return ResolveHeavyAttackMontage();
}

UAnimMontage* UDFComboComponent::ResolveDirectionalComboMontage(const int32 Step) const
{
	const AActor* const Owner = GetOwner();
	if (!Owner)
	{
		return ComboMontages.IsValidIndex(Step) ? ComboMontages[Step].Get() : nullptr;
	}
	const FVector WorldVel = Owner->GetVelocity();
	if (WorldVel.SizeSquared2D() < DirectionalInputThreshold * DirectionalInputThreshold)
	{
		// Neutral / forward-ish: use default array.
		return ComboMontages.IsValidIndex(Step) ? ComboMontages[Step].Get() : nullptr;
	}
	const FVector LocalVel = Owner->GetActorTransform().InverseTransformVectorNoScale(WorldVel);
	const float Threshold = DirectionalInputThreshold;
	if (LocalVel.X < -Threshold)
	{
		if (BackwardComboMontages.IsValidIndex(Step) && BackwardComboMontages[Step])
		{
			return BackwardComboMontages[Step].Get();
		}
	}
	else if (FMath::Abs(LocalVel.Y) > Threshold && FMath::Abs(LocalVel.Y) > LocalVel.X)
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
		if (UWorld* const W = GetWorld())
		{
			if (W->GetTimeSeconds() <= BufferExpire)
			{
				OnAttackInput();
			}
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
		DrawLine(FColor::Cyan,
			FString::Printf(TEXT("Combo step %d/%d | locked %d | montages %d | win %s | buf %s"),
				DisplayStep,
				GetEffectiveMaxComboSteps() - 1,
				LockedComboActivationStep,
				ComboMontages.Num(),
				bComboWindowActive ? TEXT("OPEN") : TEXT("-"),
				bComboInputBuffered ? TEXT("Y") : TEXT("n")));
		if (StepM)
		{
			DrawLine(FColor::White, FString::Printf(TEXT("  montage: %s"), *StepM->GetName()));
		}
		else if (ComboMontages.Num() == 0)
		{
			DrawLine(FColor::Red, TEXT("  !! ComboMontages EMPTY"));
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
