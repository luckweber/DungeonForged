// Source/DungeonForged/Private/GAS/Effects/UGE_DoT_Fire.cpp
#include "GAS/Effects/UGE_DoT_Fire.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UDFMMC_FireDoT.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_DoT_Fire::UGE_DoT_Fire()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.f));
	Period = FScalableFloat(1.f);

	StackingType = EGameplayEffectStackingType::AggregateBySource;
	StackLimitCount = 3;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::NeverReset;

	FGameplayModifierInfo Mod;
	Mod.Attribute = UDFAttributeSet::GetHealthAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	FCustomCalculationBasedFloat Cc;
	Cc.CalculationClassMagnitude = UDFMMC_FireDoT::StaticClass();
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(Cc);
	Modifiers.Add(Mod);
}

void UGE_DoT_Fire::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& Grant = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Gr;
	Gr.AddTag(FDFGameplayTags::Effect_DoT_Fire);
	Grant.SetAndApplyTargetTagChanges(Gr);
}
