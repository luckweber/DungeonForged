// Source/DungeonForged/Private/Combat/UDFComboComponent.cpp
#include "Combat/UDFComboComponent.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Combat/UDFMeleeTraceComponent.h"
#include "Components/SkeletalMeshComponent.h"
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

UDFComboComponent::UDFComboComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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
		bComboInputBuffered = true;
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

	if (Held >= HeavyChargeThreshold)
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
		bComboInputBuffered = true;
		return;
	}
	if (!bPlayingComboMontage)
	{
		PrimeMeleeSwingAbilityChain();
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
		MeleeTrace->ConfigureHeavySwing(HeavyDamageMultiplier, HeavyKnockbackMultiplier, HeavyTraceRadiusBonus);
	}
	bHeavySwingPending = true;
	NotifyAbilitySwingMontageStarted(Montage);
}

void UDFComboComponent::NotifyAbilitySwingMontageStarted(UAnimMontage* Montage)
{
	if (!Montage)
	{
		return;
	}
	TryBindEndDelegateFor(Montage);
	bPlayingComboMontage = true;
	if (MeleeTrace)
	{
		MeleeTrace->ScheduleAuthorityTraceWindowsFromMontage(Montage, 1.f);
	}
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
	if (!ComboMontages.IsValidIndex(CurrentComboStep) || !ComboMontages[CurrentComboStep])
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
	UAnimMontage* const M = ComboMontages[CurrentComboStep].Get();
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

	if (bComboInputBuffered)
	{
		if (CurrentComboStep + 1 < MaxComboSteps
			&& CurrentComboStep + 1 < ComboMontages.Num())
		{
			++CurrentComboStep;
			bComboInputBuffered = false;
			if (ShouldRoutePrimaryMeleeThroughGAS())
			{
				(void)TryActivatePrimaryMeleeGameplayAbility();
				return;
			}
			PlayCurrentComboMontage();
			return;
		}
		bComboInputBuffered = false;
	}

	if (CurrentComboStep + 1 < MaxComboSteps
		&& CurrentComboStep + 1 < ComboMontages.Num())
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
	PrimeMeleeSwingAbilityChain();

	if (ShouldRouteHeavyAttackThroughGAS())
	{
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
	if (bInterrupted)
	{
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
