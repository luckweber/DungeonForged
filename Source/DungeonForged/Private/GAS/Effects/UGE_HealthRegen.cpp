// Source/DungeonForged/Private/GAS/Effects/UGE_HealthRegen.cpp
#include "GAS/Effects/UGE_HealthRegen.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"

UGE_HealthRegen::UGE_HealthRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(1.f);

	FAttributeBasedFloat Abf;
	Abf.Coefficient = FScalableFloat(0.02f);
	Abf.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
		UDFAttributeSet::GetMaxHealthAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);
	Abf.AttributeCalculationType = EAttributeBasedFloatCalculationType::AttributeMagnitude;

	FGameplayModifierInfo Mod;
	Mod.Attribute = UDFAttributeSet::GetHealthAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(Abf);
	Modifiers.Add(Mod);
}

void UGE_HealthRegen::ConfigureEffectCDO()
{
	UTargetTagRequirementsGameplayEffectComponent& Req = FindOrAddComponent<UTargetTagRequirementsGameplayEffectComponent>();
	FGameplayTagRequirements R;
	R.IgnoreTags.AddTag(FDFGameplayTags::State_InCombat);
	Req.OngoingTagRequirements = R;
}
