// Source/DungeonForged/Private/GAS/Effects/UGE_Cooldown_Universal_SecondWind.cpp
#include "GAS/Effects/UGE_Cooldown_Universal_SecondWind.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Cooldown_Universal_SecondWind::UGE_Cooldown_Universal_SecondWind() = default;

void UGE_Cooldown_Universal_SecondWind::ConfigureEffectCDO()
{
	Super::ConfigureEffectCDO();
	UTargetTagsGameplayEffectComponent& Gr = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Tags;
	Tags.AddTag(FDFGameplayTags::Ability_Cooldown_SecondWind);
	Gr.SetAndApplyTargetTagChanges(Tags);
}
