// Source/DungeonForged/Private/GAS/Effects/UGE_Passive_ArcaneMastery.cpp
#include "GAS/Effects/UGE_Passive_ArcaneMastery.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"

UGE_Passive_ArcaneMastery::UGE_Passive_ArcaneMastery()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	// Intelligence += CharacterLevel * 3
	{
		FAttributeBasedFloat Abf;
		Abf.Coefficient = FScalableFloat(3.f);
		Abf.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
			UDFAttributeSet::GetCharacterLevelAttribute(), EGameplayEffectAttributeCaptureSource::Target, true);
		Abf.AttributeCalculationType = EAttributeBasedFloatCalculationType::AttributeMagnitude;
		FGameplayModifierInfo Mi;
		Mi.Attribute = UDFAttributeSet::GetIntelligenceAttribute();
		Mi.ModifierOp = EGameplayModOp::Additive;
		Mi.ModifierMagnitude = FGameplayEffectModifierMagnitude(Abf);
		Modifiers.Add(Mi);
	}
	// CooldownReduction += 0.05
	{
		FGameplayModifierInfo Mc;
		Mc.Attribute = UDFAttributeSet::GetCooldownReductionAttribute();
		Mc.ModifierOp = EGameplayModOp::Additive;
		Mc.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.05f));
		Modifiers.Add(Mc);
	}
	// MaxMana += Intelligence * 2
	{
		FAttributeBasedFloat Abf;
		Abf.Coefficient = FScalableFloat(2.f);
		Abf.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
			UDFAttributeSet::GetIntelligenceAttribute(), EGameplayEffectAttributeCaptureSource::Target, true);
		Abf.AttributeCalculationType = EAttributeBasedFloatCalculationType::AttributeMagnitude;
		FGameplayModifierInfo Mm;
		Mm.Attribute = UDFAttributeSet::GetMaxManaAttribute();
		Mm.ModifierOp = EGameplayModOp::Additive;
		Mm.ModifierMagnitude = FGameplayEffectModifierMagnitude(Abf);
		Modifiers.Add(Mm);
	}
}
