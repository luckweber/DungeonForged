// Source/DungeonForged/Private/GAS/Abilities/Equipment/UDFAbility_StowWeapon.cpp
#include "GAS/Abilities/Equipment/UDFAbility_StowWeapon.h"

#include "Abilities/GameplayAbility.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Equipment/UDFEquipmentComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GameFramework/Actor.h"

UDFAbility_StowWeapon::UDFAbility_StowWeapon()
{
	bRetriggerInstancedAbility = false;
	AbilityCost_Mana = 0.f;
	AbilityCost_Stamina = 0.f;
	BaseCooldown = 0.f;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UDFAbility_StowWeapon::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Equipment_StowWeapon);
		AbilityTags.AddTag(FDFGameplayTags::Ability_Equipment);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Stunned);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Silenced);
	}
}

void UDFAbility_StowWeapon::RequestUnequipFromAnimation()
{
	if (ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UDFEquipmentComponent* Eq = PC->Equipment; Eq && StowSlot != EEquipmentSlot::None)
		{
			Eq->RequestUnequipSlot(StowSlot);
		}
	}
}

void UDFAbility_StowWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	(void)TriggerEventData;
	if (!ActorInfo || !CommitAbility(Handle, ActorInfo, ActivationInfo, nullptr))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* const Avatar = ActorInfo->AvatarActor.Get();
	const ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(Avatar);
	if (!Avatar || !PC || !PC->Equipment || StowSlot == EEquipmentSlot::None)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAnimMontage* MontToPlay = AbilityMontage.Get();
	if (!MontToPlay)
	{
		RequestUnequipFromAnimation();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (UAbilityTask_PlayMontageAndWait* const PlayTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, MontToPlay,
				1.f, NAME_None, true, 1.f, 0.f, true))
	{
		PlayTask->OnCompleted.AddDynamic(this, &UDFAbility_StowWeapon::OnMontageCompletedOrCancelled);
		PlayTask->OnBlendOut.AddDynamic(this, &UDFAbility_StowWeapon::OnMontageCompletedOrCancelled);
		PlayTask->OnInterrupted.AddDynamic(this, &UDFAbility_StowWeapon::OnMontageCompletedOrCancelled);
		PlayTask->OnCancelled.AddDynamic(this, &UDFAbility_StowWeapon::OnMontageCompletedOrCancelled);
		PlayTask->ReadyForActivation();
	}
	else
	{
		RequestUnequipFromAnimation();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UDFAbility_StowWeapon::OnMontageCompletedOrCancelled()
{
	RequestUnequipFromAnimation();
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
