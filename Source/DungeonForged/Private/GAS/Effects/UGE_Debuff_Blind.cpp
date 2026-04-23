// Source/DungeonForged/Private/GAS/Effects/UGE_Debuff_Blind.cpp
#include "GAS/Effects/UGE_Debuff_Blind.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Debuff_Blind::UGE_Debuff_Blind()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	{
		FSetByCallerFloat Sbc;
		Sbc.DataTag = FDFGameplayTags::Data_Duration;
		DurationMagnitude = FGameplayEffectModifierMagnitude(Sbc);
	}
}

void UGE_Debuff_Blind::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& Gr = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer T;
	T.AddTag(FDFGameplayTags::Effect_Debuff_Blinded);
	Gr.SetAndApplyTargetTagChanges(T);
}
