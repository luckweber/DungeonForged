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
		const ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(GetOwner());
		if (!PC || !PC->bDisableWarriorMeleeSwingGameplayAbility)
		{
			if (TryActivatePrimaryMeleeGameplayAbility())
			{
				return;
			}
		}
		StartCombo();
	}
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

void UDFComboComponent::NotifyAbilitySwingMontageStarted(UAnimMontage* Montage)
{
	if (!Montage)
	{
		return;
	}
	TryBindEndDelegateFor(Montage);
	bPlayingComboMontage = true;
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
			const ADFPlayerCharacter* const GAOwner = Cast<ADFPlayerCharacter>(GetOwner());
			if (!GAOwner || !GAOwner->bDisableWarriorMeleeSwingGameplayAbility)
			{
				if (TryActivatePrimaryMeleeGameplayAbility())
				{
					return;
				}
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
	}
	CurrentComboStep = 0;
	bComboInputBuffered = false;
	bComboWindowActive = false;
	bPlayingComboMontage = false;
}

void UDFComboComponent::HandleMontageEndedInternal(UAnimMontage* EndedMontage, bool bInterrupted)
{
	(void)EndedMontage;
	if (bInterrupted)
	{
		return;
	}
	ResetCombo();
}

void UDFComboComponent::OnMontageEnded(UAnimMontage* const EndedMontage, const bool bInterrupted)
{
	HandleMontageEndedInternal(EndedMontage, bInterrupted);
}
