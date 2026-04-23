// Source/DungeonForged/Private/GAS/Effects/UGE_Buff_SmokeCover.cpp
#include "GAS/Effects/UGE_Buff_SmokeCover.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Buff_SmokeCover::UGE_Buff_SmokeCover()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	{
		FSetByCallerFloat Sbc;
		Sbc.DataTag = FDFGameplayTags::Data_Duration;
		DurationMagnitude = FGameplayEffectModifierMagnitude(Sbc);
	}
}

void UGE_Buff_SmokeCover::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& Gr = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer T;
	T.AddTag(FDFGameplayTags::State_Concealed);
	Gr.SetAndApplyTargetTagChanges(T);
}
