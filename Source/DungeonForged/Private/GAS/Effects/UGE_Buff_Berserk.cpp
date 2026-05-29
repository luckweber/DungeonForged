// Source/DungeonForged/Private/GAS/Effects/UGE_Buff_Berserk.cpp
#include "GAS/Effects/UGE_Buff_Berserk.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Buff_Berserk::UGE_Buff_Berserk()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(15.f));
	Period = FScalableFloat(1.f);

	{
		FGameplayModifierInfo Mod;
		Mod.Attribute = UDFAttributeSet::GetStrengthAttribute();
		Mod.ModifierOp = EGameplayModOp::Additive;
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(25.f));
		Modifiers.Add(Mod);
	}
	{
		FGameplayModifierInfo Mod;
		Mod.Attribute = UDFAttributeSet::GetMovementSpeedMultiplierAttribute();
		Mod.ModifierOp = EGameplayModOp::Additive;
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.35f));
		Modifiers.Add(Mod);
	}
	{
		FGameplayModifierInfo Mod;
		Mod.Attribute = UDFAttributeSet::GetCritMultiplierAttribute();
		Mod.ModifierOp = EGameplayModOp::Additive;
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.5f));
		Modifiers.Add(Mod);
	}
	{
		FGameplayModifierInfo Mod;
		Mod.Attribute = UDFAttributeSet::GetHealthAttribute();
		Mod.ModifierOp = EGameplayModOp::Additive;
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-8.f));
		Modifiers.Add(Mod);
	}
}

void UGE_Buff_Berserk::ConfigureEffectCDO()
{
	UTargetTagsGameplayEffectComponent& Grant = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Gr;
	Gr.AddTag(FDFGameplayTags::State_Berserk);
	Grant.SetAndApplyTargetTagChanges(Gr);
}
