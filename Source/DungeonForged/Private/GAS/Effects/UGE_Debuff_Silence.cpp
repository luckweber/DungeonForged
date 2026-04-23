// Source/DungeonForged/Private/GAS/Effects/UGE_Debuff_Silence.cpp
#include "GAS/Effects/UGE_Debuff_Silence.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/BlockAbilityTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Debuff_Silence::UGE_Debuff_Silence()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	{
		FSetByCallerFloat Sbc;
		Sbc.DataTag = FDFGameplayTags::Data_Duration;
		DurationMagnitude = FGameplayEffectModifierMagnitude(Sbc);
	}
}

void UGE_Debuff_Silence::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& Grant = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Gr;
	Gr.AddTag(FDFGameplayTags::State_Silenced);
	Grant.SetAndApplyTargetTagChanges(Gr);
	UBlockAbilityTagsGameplayEffectComponent& Block = FindOrAddComponent<UBlockAbilityTagsGameplayEffectComponent>();
	FInheritedTagContainer Bt;
	Bt.AddTag(FDFGameplayTags::Ability_Fire);
	Bt.AddTag(FDFGameplayTags::Ability_Ice);
	Block.SetAndApplyBlockedAbilityTagChanges(Bt);
}
