// Source/DungeonForged/Private/GAS/Effects/UGE_StaminaRegen.cpp
#include "GAS/Effects/UGE_StaminaRegen.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UDFMMC_StaminaRegen.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"

UGE_StaminaRegen::UGE_StaminaRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(0.2f);

	FCustomCalculationBasedFloat Cc;
	Cc.CalculationClassMagnitude = UDFMMC_StaminaRegen::StaticClass();

	FGameplayModifierInfo Mod;
	Mod.Attribute = UDFAttributeSet::GetStaminaAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(Cc);
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
