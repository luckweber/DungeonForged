// Source/DungeonForged/Private/GAS/Effects/UGE_ManaRegen.cpp
#include "GAS/Effects/UGE_ManaRegen.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"

UGE_ManaRegen::UGE_ManaRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(1.f);

	FAttributeBasedFloat Abf;
	Abf.Coefficient = FScalableFloat(0.03f);
	Abf.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
		UDFAttributeSet::GetMaxManaAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);
	Abf.AttributeCalculationType = EAttributeBasedFloatCalculationType::AttributeMagnitude;

	FGameplayModifierInfo Mod;
	Mod.Attribute = UDFAttributeSet::GetManaAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(Abf);
	Modifiers.Add(Mod);
}
