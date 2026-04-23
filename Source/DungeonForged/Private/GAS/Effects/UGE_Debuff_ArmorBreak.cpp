// Source/DungeonForged/Private/GAS/Effects/UGE_Debuff_ArmorBreak.cpp
#include "GAS/Effects/UGE_Debuff_ArmorBreak.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UDFMMC_NegateDataMagnitude.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Debuff_ArmorBreak::UGE_Debuff_ArmorBreak()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(5.f));
	StackingType = EGameplayEffectStackingType::AggregateBySource;
	StackLimitCount = 3;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;

	FGameplayModifierInfo Mod;
	Mod.Attribute = UDFAttributeSet::GetArmorAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	FCustomCalculationBasedFloat Cc;
	Cc.CalculationClassMagnitude = UDFMMC_NegateDataMagnitude::StaticClass();
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(Cc);
	Modifiers.Add(Mod);
}

void UGE_Debuff_ArmorBreak::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& Grant = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Gr;
	Gr.AddTag(FDFGameplayTags::Effect_Debuff_ArmorBreak);
	Grant.SetAndApplyTargetTagChanges(Gr);
}
