// Source/DungeonForged/Private/GAS/Effects/UGE_StaminaRegen.cpp
#include "GAS/Effects/UGE_StaminaRegen.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"

UGE_StaminaRegen::UGE_StaminaRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(0.2f);

	FAttributeBasedFloat Abf;
	Abf.Coefficient = FScalableFloat(0.08f);
	Abf.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
		UDFAttributeSet::GetMaxStaminaAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);
	Abf.AttributeCalculationType = EAttributeBasedFloatCalculationType::AttributeMagnitude;

	FGameplayModifierInfo Mod;
	Mod.Attribute = UDFAttributeSet::GetStaminaAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(Abf);
	Modifiers.Add(Mod);
}

void UGE_StaminaRegen::ConfigureEffectCDO()
{
	UTargetTagRequirementsGameplayEffectComponent& Req = FindOrAddComponent<UTargetTagRequirementsGameplayEffectComponent>();
	FGameplayTagRequirements R;
	R.IgnoreTags.AddTag(FDFGameplayTags::State_Sprinting);
	R.IgnoreTags.AddTag(FDFGameplayTags::State_Dodging);
	Req.OngoingTagRequirements = R;
}
