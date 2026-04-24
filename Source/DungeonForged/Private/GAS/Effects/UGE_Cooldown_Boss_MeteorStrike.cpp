// Source/DungeonForged/Private/GAS/Effects/UGE_Cooldown_Boss_MeteorStrike.cpp
#include "GAS/Effects/UGE_Cooldown_Boss_MeteorStrike.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Cooldown_Boss_MeteorStrike::UGE_Cooldown_Boss_MeteorStrike() = default;

void UGE_Cooldown_Boss_MeteorStrike::ConfigureEffectCDO()
{
	Super::ConfigureEffectCDO();
	UTargetTagsGameplayEffectComponent& G = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer T;
	T.AddTag(FDFGameplayTags::Ability_Cooldown_Boss_MeteorStrike);
	G.SetAndApplyTargetTagChanges(T);
}
