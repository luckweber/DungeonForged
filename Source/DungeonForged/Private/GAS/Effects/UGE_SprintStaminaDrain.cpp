// Source/DungeonForged/Private/GAS/Effects/UGE_SprintStaminaDrain.cpp
#include "GAS/Effects/UGE_SprintStaminaDrain.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_SprintStaminaDrain::UGE_SprintStaminaDrain()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(0.1f);

	FAttributeBasedFloat Abf;
	Abf.Coefficient = FScalableFloat(-0.1f);
	Abf.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
		UDFAttributeSet::GetSprintStaminaDrainAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);
	Abf.AttributeCalculationType = EAttributeBasedFloatCalculationType::AttributeMagnitude;

	FGameplayModifierInfo Mod;
	Mod.Attribute = UDFAttributeSet::GetStaminaAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(Abf);
	Modifiers.Add(Mod);
}

void UGE_SprintStaminaDrain::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& Grant = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Gr;
	Gr.AddTag(FDFGameplayTags::State_Sprinting);
	Grant.SetAndApplyTargetTagChanges(Gr);
}
