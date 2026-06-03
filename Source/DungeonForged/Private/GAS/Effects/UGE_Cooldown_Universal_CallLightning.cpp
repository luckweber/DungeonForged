#include "GAS/Effects/UGE_Cooldown_Universal_CallLightning.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Cooldown_Universal_CallLightning::UGE_Cooldown_Universal_CallLightning()
{
	CooldownAssociatedAbilityTag = FDFGameplayTags::Ability_Universal_CallLightning;
}

void UGE_Cooldown_Universal_CallLightning::ConfigureEffectCDO()
{
	Super::ConfigureEffectCDO();
	UTargetTagsGameplayEffectComponent& Gr = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Tags;
	Tags.AddTag(FDFGameplayTags::Ability_Cooldown_CallLightning);
	Gr.SetAndApplyTargetTagChanges(Tags);
}
