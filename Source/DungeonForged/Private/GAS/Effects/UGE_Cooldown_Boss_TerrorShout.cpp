// Source/DungeonForged/Private/GAS/Effects/UGE_Cooldown_Boss_TerrorShout.cpp
#include "GAS/Effects/UGE_Cooldown_Boss_TerrorShout.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Cooldown_Boss_TerrorShout::UGE_Cooldown_Boss_TerrorShout() = default;

void UGE_Cooldown_Boss_TerrorShout::ConfigureEffectCDO()
{
	Super::ConfigureEffectCDO();
	UTargetTagsGameplayEffectComponent& G = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer T;
	T.AddTag(FDFGameplayTags::Ability_Cooldown_Boss_TerrorShout);
	G.SetAndApplyTargetTagChanges(T);
}
