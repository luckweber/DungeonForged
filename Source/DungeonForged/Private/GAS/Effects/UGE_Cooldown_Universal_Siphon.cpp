#include "GAS/Effects/UGE_Cooldown_Universal_Siphon.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Cooldown_Universal_Siphon::UGE_Cooldown_Universal_Siphon()
{
	CooldownAssociatedAbilityTag = FDFGameplayTags::Ability_Universal_Siphon;
}

void UGE_Cooldown_Universal_Siphon::ConfigureEffectCDO()
{
	Super::ConfigureEffectCDO();
	UTargetTagsGameplayEffectComponent& Gr = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Tags;
	Tags.AddTag(FDFGameplayTags::Ability_Cooldown_Siphon);
	Gr.SetAndApplyTargetTagChanges(Tags);
}
