// Source/DungeonForged/Private/GAS/Effects/UGE_Cooldown_Boss_VoidBarrier.cpp
#include "GAS/Effects/UGE_Cooldown_Boss_VoidBarrier.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Cooldown_Boss_VoidBarrier::UGE_Cooldown_Boss_VoidBarrier() = default;

void UGE_Cooldown_Boss_VoidBarrier::ConfigureEffectCDO()
{
	Super::ConfigureEffectCDO();
	UTargetTagsGameplayEffectComponent& G = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer T;
	T.AddTag(FDFGameplayTags::Ability_Cooldown_Boss_VoidBarrier);
	G.SetAndApplyTargetTagChanges(T);
}
