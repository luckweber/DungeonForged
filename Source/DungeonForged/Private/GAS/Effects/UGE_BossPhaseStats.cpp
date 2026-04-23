// Source/DungeonForged/Private/GAS/Effects/UGE_BossPhaseStats.cpp
#include "GAS/Effects/UGE_BossPhaseStats.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"

UGE_BossPhaseStats::UGE_BossPhaseStats()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	{
		FGameplayModifierInfo M1;
		M1.Attribute = UDFAttributeSet::GetStrengthAttribute();
		M1.ModifierOp = EGameplayModOp::Additive;
		M1.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(20.f));
		Modifiers.Add(M1);
	}
	{
		FGameplayModifierInfo M2;
		M2.Attribute = UDFAttributeSet::GetMovementSpeedMultiplierAttribute();
		M2.ModifierOp = EGameplayModOp::Additive;
		M2.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.1f));
		Modifiers.Add(M2);
	}
}
