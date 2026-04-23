// Source/DungeonForged/Private/GAS/Effects/UGE_Buff_Rogue_Ambush.cpp
#include "GAS/Effects/UGE_Buff_Rogue_Ambush.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Buff_Rogue_Ambush::UGE_Buff_Rogue_Ambush()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	// Tags only; C++ abilities remove the effect after first damage.
}

void UGE_Buff_Rogue_Ambush::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& Grant = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer T;
	T.AddTag(FDFGameplayTags::Buff_Rogue_Ambush);
	Grant.SetAndApplyTargetTagChanges(T);
}
