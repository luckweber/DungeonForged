// Source/DungeonForged/Private/GAS/Effects/UGE_Heal_Instant.cpp
#include "GAS/Effects/UGE_Heal_Instant.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"

UGE_Heal_Instant::UGE_Heal_Instant()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo Mod;
	Mod.Attribute = UDFAttributeSet::GetHealthAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	{
		FSetByCallerFloat Sbc;
		Sbc.DataTag = FDFGameplayTags::Data_Healing;
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(Sbc);
	}
	Modifiers.Add(Mod);
}
