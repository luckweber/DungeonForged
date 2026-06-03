#include "GAS/Abilities/Universal/UDFAbility_Universal_SecondWind.h"
#include "AbilitySystemComponent.h"
#include "GAS/DFGameplayTags.h"

UDFAbility_Universal_SecondWind::UDFAbility_Universal_SecondWind() = default;

void UDFAbility_Universal_SecondWind::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Universal_SecondWind);
	}
}

void UDFAbility_Universal_SecondWind::OnPassiveAbilityActivated(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo& ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	(void)Handle;
	(void)ActivationInfo;
	(void)TriggerEventData;
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return;
	}
	AActor* const Av = GetAvatarActorFromActorInfo();
	if (!Av || !Av->HasAuthority())
	{
		return;
	}
	if (FDFGameplayTags::State_Universal_SecondWindAvailable.IsValid())
	{
		ActorInfo->AbilitySystemComponent->AddLooseGameplayTag(FDFGameplayTags::State_Universal_SecondWindAvailable, 1);
	}
}

void UDFAbility_Universal_SecondWind::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid()
		&& FDFGameplayTags::State_Universal_SecondWindAvailable.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RemoveLooseGameplayTag(FDFGameplayTags::State_Universal_SecondWindAvailable, 1);
	}
	Super::OnRemoveAbility(ActorInfo, Spec);
}
