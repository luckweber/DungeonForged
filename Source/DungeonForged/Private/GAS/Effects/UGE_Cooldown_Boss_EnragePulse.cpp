// Source/DungeonForged/Private/GAS/Effects/UGE_Cooldown_Boss_EnragePulse.cpp
#include "GAS/Effects/UGE_Cooldown_Boss_EnragePulse.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Cooldown_Boss_EnragePulse::UGE_Cooldown_Boss_EnragePulse() = default;

void UGE_Cooldown_Boss_EnragePulse::ConfigureEffectCDO()
{
	Super::ConfigureEffectCDO();
	UTargetTagsGameplayEffectComponent& G = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer T;
	T.AddTag(FDFGameplayTags::Ability_Cooldown_Boss_EnragePulse);
	G.SetAndApplyTargetTagChanges(T);
}
