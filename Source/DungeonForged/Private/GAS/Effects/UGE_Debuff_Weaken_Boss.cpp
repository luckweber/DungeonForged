// Source/DungeonForged/Private/GAS/Effects/UGE_Debuff_Weaken_Boss.cpp
#include "GAS/Effects/UGE_Debuff_Weaken_Boss.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Debuff_Weaken_Boss::UGE_Debuff_Weaken_Boss()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(10.f));
	{
		FGameplayModifierInfo Ma;
		Ma.Attribute = UDFAttributeSet::GetArmorAttribute();
		Ma.ModifierOp = EGameplayModOp::Additive;
		Ma.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-30.f));
		Modifiers.Add(Ma);
	}
	{
		FGameplayModifierInfo Mm;
		Mm.Attribute = UDFAttributeSet::GetMagicResistAttribute();
		Mm.ModifierOp = EGameplayModOp::Additive;
		Mm.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-20.f));
		Modifiers.Add(Mm);
	}
}

void UGE_Debuff_Weaken_Boss::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& G = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer T;
	T.AddTag(FDFGameplayTags::Effect_Debuff_Weaken);
	G.SetAndApplyTargetTagChanges(T);
}
