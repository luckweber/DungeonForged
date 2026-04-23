// Source/DungeonForged/Private/GAS/Effects/UGE_Passive_Rogue_Predator.cpp
#include "GAS/Effects/UGE_Passive_Rogue_Predator.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"

UGE_Passive_Rogue_Predator::UGE_Passive_Rogue_Predator()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	{
		FGameplayModifierInfo Mm;
		Mm.Attribute = UDFAttributeSet::GetMovementSpeedMultiplierAttribute();
		Mm.ModifierOp = EGameplayModOp::Additive;
		Mm.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.08f));
		Modifiers.Add(Mm);
	}
	{
		FGameplayModifierInfo Mc;
		Mc.Attribute = UDFAttributeSet::GetCritChanceAttribute();
		Mc.ModifierOp = EGameplayModOp::Additive;
		Mc.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.1f));
		Modifiers.Add(Mc);
	}
}
