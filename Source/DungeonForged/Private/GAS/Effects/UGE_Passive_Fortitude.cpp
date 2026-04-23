// Source/DungeonForged/Private/GAS/Effects/UGE_Passive_Fortitude.cpp
#include "GAS/Effects/UGE_Passive_Fortitude.h"
#include "GAS/Effects/UDFExec_FortitudeWounded.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"

UGE_Passive_Fortitude::UGE_Passive_Fortitude()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(5.f);
	FGameplayEffectExecutionDefinition Exec;
	Exec.CalculationClass = UDFExec_FortitudeWounded::StaticClass();
	Executions.Add(Exec);

	{
		FAttributeBasedFloat Abf;
		Abf.Coefficient = FScalableFloat(5.f);
		Abf.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
			UDFAttributeSet::GetStrengthAttribute(), EGameplayEffectAttributeCaptureSource::Target, true);
		Abf.AttributeCalculationType = EAttributeBasedFloatCalculationType::AttributeMagnitude;
		FGameplayModifierInfo Mh;
		Mh.Attribute = UDFAttributeSet::GetMaxHealthAttribute();
		Mh.ModifierOp = EGameplayModOp::Additive;
		Mh.ModifierMagnitude = FGameplayEffectModifierMagnitude(Abf);
		Modifiers.Add(Mh);
	}
	{
		FGameplayModifierInfo Ma;
		Ma.Attribute = UDFAttributeSet::GetArmorAttribute();
		Ma.ModifierOp = EGameplayModOp::Additive;
		Ma.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(10.f));
		Modifiers.Add(Ma);
	}
}
