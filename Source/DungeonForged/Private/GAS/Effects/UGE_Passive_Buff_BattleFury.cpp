// Source/DungeonForged/Private/GAS/Effects/UGE_Passive_Buff_BattleFury.cpp
#include "GAS/Effects/UGE_Passive_Buff_BattleFury.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"

UGE_Passive_Buff_BattleFury::UGE_Passive_Buff_BattleFury()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(10.f));
	StackingType = EGameplayEffectStackingType::AggregateBySource;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;

	FGameplayModifierInfo M;
	M.Attribute = UDFAttributeSet::GetStrengthAttribute();
	M.ModifierOp = EGameplayModOp::Additive;
	M.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(15.f));
	Modifiers.Add(M);
}
