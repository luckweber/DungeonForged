// Source/DungeonForged/Private/GAS/Abilities/Passive/UDFAbility_Passive_Rogue_Predator.cpp
#include "GAS/Abilities/Passive/UDFAbility_Passive_Rogue_Predator.h"
#include "AbilitySystemComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Passive_Rogue_Predator.h"

UDFAbility_Passive_Rogue_Predator::UDFAbility_Passive_Rogue_Predator() = default;

void UDFAbility_Passive_Rogue_Predator::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Passive_Rogue_Predator);
	}
}

void UDFAbility_Passive_Rogue_Predator::OnPassiveAbilityActivated(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo& ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	(void)Handle;
	(void)ActivationInfo;
	(void)TriggerEventData;
	if (!ActorInfo)
	{
		return;
	}
	AActor* const Av = GetAvatarActorFromActorInfo();
	if (!Av || !Av->HasAuthority() || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return;
	}
	UAbilitySystemComponent* const ASC = ActorInfo->AbilitySystemComponent.Get();
	const FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
	const FGameplayEffectSpecHandle S = ASC->MakeOutgoingSpec(UGE_Passive_Rogue_Predator::StaticClass(), 1.f, Ctx);
	if (!S.IsValid() || !S.Data)
	{
		return;
	}
	AuraGEHandle = ASC->ApplyGameplayEffectSpecToSelf(*S.Data);
}

void UDFAbility_Passive_Rogue_Predator::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && AuraGEHandle.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(AuraGEHandle);
	}
	UDFAbilityPassive::OnRemoveAbility(ActorInfo, Spec);
}
