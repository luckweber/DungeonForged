// Source/DungeonForged/Private/GAS/Abilities/Passive/UDFAbilityPassive.cpp
#include "GAS/Abilities/Passive/UDFAbilityPassive.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"

UDFAbilityPassive::UDFAbilityPassive()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationPolicy = EAbilityActivationPolicy::Passive;
	AbilityCost_Mana = 0.f;
	AbilityCost_Stamina = 0.f;
	BaseCooldown = 0.f;
}

void UDFAbilityPassive::OnGiveAbility(const FGameplayAbilityActorInfo* const ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
	}
}

void UDFAbilityPassive::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// Do not use UDFGameplayAbility:: path (Commit + montage + costs).
	UGameplayAbility::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!ActorInfo)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	OnPassiveAbilityActivated(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UDFAbilityPassive::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	if (IsActive() && ActorInfo)
	{
		EndAbility(Spec.Handle, ActorInfo, GetCurrentActivationInfo(), true, true);
	}
	Super::OnRemoveAbility(ActorInfo, Spec);
}
