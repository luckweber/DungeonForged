// Source/DungeonForged/Private/GAS/Effects/UGE_DoT_Frost.cpp
#include "GAS/Effects/UGE_DoT_Frost.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_DoT_Frost::UGE_DoT_Frost()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.f));
	Period = FScalableFloat(1.f);
	StackingType = EGameplayEffectStackingType::AggregateBySource;
	StackLimitCount = 5;

	FGameplayModifierInfo Mod;
	Mod.Attribute = UDFAttributeSet::GetHealthAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-2.f));
	Modifiers.Add(Mod);
}

void UGE_DoT_Frost::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& Grant = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Gr;
	Gr.AddTag(FDFGameplayTags::Effect_DoT_Frost);
	Grant.SetAndApplyTargetTagChanges(Gr);
}
