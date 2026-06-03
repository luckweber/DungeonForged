#include "GAS/Effects/UGE_Cooldown_Universal_BattleHymn.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Cooldown_Universal_BattleHymn::UGE_Cooldown_Universal_BattleHymn()
{
	CooldownAssociatedAbilityTag = FDFGameplayTags::Ability_Universal_BattleHymn;
}

void UGE_Cooldown_Universal_BattleHymn::ConfigureEffectCDO()
{
	Super::ConfigureEffectCDO();
	UTargetTagsGameplayEffectComponent& Gr = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Tags;
	Tags.AddTag(FDFGameplayTags::Ability_Cooldown_BattleHymn);
	Gr.SetAndApplyTargetTagChanges(Tags);
}
