// Source/DungeonForged/Private/GAS/Effects/UGE_Passive_BleedMastery_Extra.cpp
#include "GAS/Effects/UGE_Passive_BleedMastery_Extra.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"

UGE_Passive_BleedMastery_Extra::UGE_Passive_BleedMastery_Extra()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(8.f));
	Period = FScalableFloat(0.5f);
	StackingType = EGameplayEffectStackingType::AggregateBySource;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;

	// -Source.Agility * 0.2 to Health per tick
	FAttributeBasedFloat Abf;
	Abf.Coefficient = FScalableFloat(-0.2f);
	Abf.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
		UDFAttributeSet::GetAgilityAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
	Abf.AttributeCalculationType = EAttributeBasedFloatCalculationType::AttributeMagnitude;
	FGameplayModifierInfo Mh;
	Mh.Attribute = UDFAttributeSet::GetHealthAttribute();
	Mh.ModifierOp = EGameplayModOp::Additive;
	Mh.ModifierMagnitude = FGameplayEffectModifierMagnitude(Abf);
	Modifiers.Add(Mh);
}
