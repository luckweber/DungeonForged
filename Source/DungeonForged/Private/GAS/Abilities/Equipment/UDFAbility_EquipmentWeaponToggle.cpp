// Source/DungeonForged/Private/GAS/Abilities/Equipment/UDFAbility_EquipmentWeaponToggle.cpp
#include "GAS/Abilities/Equipment/UDFAbility_EquipmentWeaponToggle.h"

#include "Abilities/GameplayAbility.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Equipment/UDFEquipmentComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GameFramework/Actor.h"

UDFAbility_EquipmentWeaponToggle::UDFAbility_EquipmentWeaponToggle()
{
	bRetriggerInstancedAbility = true;
	AbilityCost_Mana = 0.f;
	AbilityCost_Stamina = 0.f;
	BaseCooldown = 0.f;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UDFAbility_EquipmentWeaponToggle::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Equipment_WeaponToggle);
		AbilityTags.AddTag(FDFGameplayTags::Ability_Equipment);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Stunned);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Silenced);
	}
}

void UDFAbility_EquipmentWeaponToggle::ApplyEquipOrUnequipRequest()
{
	ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!PC || !PC->Equipment || WeaponSlot == EEquipmentSlot::None)
	{
		return;
	}
	UDFEquipmentComponent* const Eq = PC->Equipment;
	if (Eq->IsSlotEmpty(WeaponSlot))
	{
		if (WeaponItemRowWhenUnarmed.IsNone())
		{
			return;
		}
		Eq->RequestEquipItem(WeaponItemRowWhenUnarmed, WeaponSlot);
		return;
	}
	Eq->RequestUnequipSlot(WeaponSlot);
}

UAnimMontage* UDFAbility_EquipmentWeaponToggle::ResolveMontageForCurrentState(
	ADFPlayerCharacter* const PC,
	bool& OutShouldDraw) const
{
	if (!PC || !PC->Equipment || WeaponSlot == EEquipmentSlot::None)
	{
		OutShouldDraw = true;
		return DrawWeaponMontage.Get();
	}
	const UDFEquipmentComponent* Eq = PC->Equipment;
	OutShouldDraw = Eq->IsSlotEmpty(WeaponSlot);
	return (OutShouldDraw ? DrawWeaponMontage : StowWeaponMontage).Get();
}

void UDFAbility_EquipmentWeaponToggle::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	(void)TriggerEventData;
	if (!ActorInfo || !CommitAbility(Handle, ActorInfo, ActivationInfo, nullptr))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(ActorInfo->AvatarActor.Get());
	if (!PC || !PC->Equipment)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bool bShouldDrawUnused = false;
	UAnimMontage* MontToPlay = ResolveMontageForCurrentState(PC, bShouldDrawUnused);
	if (!MontToPlay)
	{
		if (UDFEquipmentComponent* const Eq = PC->Equipment)
		{
			if (Eq->IsSlotEmpty(WeaponSlot) && WeaponItemRowWhenUnarmed.IsNone())
			{
				EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
				return;
			}
		}
		ApplyEquipOrUnequipRequest();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (UAbilityTask_PlayMontageAndWait* const PlayTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, MontToPlay,
				1.f, NAME_None, true, 1.f, 0.f, true))
	{
		PlayTask->OnCompleted.AddDynamic(this, &UDFAbility_EquipmentWeaponToggle::OnMontageCompletedOrCancelled);
		PlayTask->OnBlendOut.AddDynamic(this, &UDFAbility_EquipmentWeaponToggle::OnMontageCompletedOrCancelled);
		PlayTask->OnInterrupted.AddDynamic(this, &UDFAbility_EquipmentWeaponToggle::OnMontageCompletedOrCancelled);
		PlayTask->OnCancelled.AddDynamic(this, &UDFAbility_EquipmentWeaponToggle::OnMontageCompletedOrCancelled);
		PlayTask->ReadyForActivation();
	}
	else
	{
		ApplyEquipOrUnequipRequest();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UDFAbility_EquipmentWeaponToggle::OnMontageCompletedOrCancelled()
{
	ApplyEquipOrUnequipRequest();
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
