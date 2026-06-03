#include "GAS/Effects/UGE_Cooldown_Universal_HealthPotion.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Cooldown_Universal_HealthPotion::UGE_Cooldown_Universal_HealthPotion()
{
	CooldownAssociatedAbilityTag = FDFGameplayTags::Ability_Universal_HealthPotion;
}

void UGE_Cooldown_Universal_HealthPotion::ConfigureEffectCDO()
{
	Super::ConfigureEffectCDO();
	UTargetTagsGameplayEffectComponent& Gr = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Tags;
	Tags.AddTag(FDFGameplayTags::Ability_Cooldown_HealthPotion);
	Gr.SetAndApplyTargetTagChanges(Tags);
}
