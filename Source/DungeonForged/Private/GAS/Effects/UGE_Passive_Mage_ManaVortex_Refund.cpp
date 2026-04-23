// Source/DungeonForged/Private/GAS/Effects/UGE_Passive_Mage_ManaVortex_Refund.cpp
#include "GAS/Effects/UGE_Passive_Mage_ManaVortex_Refund.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"

UGE_Passive_Mage_ManaVortex_Refund::UGE_Passive_Mage_ManaVortex_Refund()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo M;
	M.Attribute = UDFAttributeSet::GetManaAttribute();
	M.ModifierOp = EGameplayModOp::Additive;
	M.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(20.f));
	Modifiers.Add(M);
}
