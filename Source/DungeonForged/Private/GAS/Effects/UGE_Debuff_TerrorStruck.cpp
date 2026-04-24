// Source/DungeonForged/Private/GAS/Effects/UGE_Debuff_TerrorStruck.cpp
#include "GAS/Effects/UGE_Debuff_TerrorStruck.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Debuff_TerrorStruck::UGE_Debuff_TerrorStruck()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	{
		FSetByCallerFloat Sbc;
		Sbc.DataTag = FDFGameplayTags::Data_Duration;
		DurationMagnitude = FGameplayEffectModifierMagnitude(Sbc);
	}
	FGameplayModifierInfo Mod;
	Mod.Attribute = UDFAttributeSet::GetMovementSpeedMultiplierAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-0.5f));
	Modifiers.Add(Mod);
}

void UGE_Debuff_TerrorStruck::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& G = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer T;
	T.AddTag(FDFGameplayTags::Effect_Debuff_Terrified);
	G.SetAndApplyTargetTagChanges(T);
}
