// Source/DungeonForged/Private/GAS/Effects/UGE_Leveling_SpendAttribute.cpp

#include "GAS/Effects/UGE_Leveling_SpendAttribute.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"

static void DF_InitSpendGE(UGameplayEffect& Self, const FGameplayAttribute& Attr)
{
	Self.DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo Mod;
	Mod.Attribute = Attr;
	Mod.ModifierOp = EGameplayModOp::Additive;
	FSetByCallerFloat Sbc;
	Sbc.DataTag = FDFGameplayTags::Data_Magnitude;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(Sbc);
	Self.Modifiers.Add(Mod);
}

UGE_Leveling_Spend_Strength::UGE_Leveling_Spend_Strength()
{
	DF_InitSpendGE(*this, UDFAttributeSet::GetStrengthAttribute());
}

UGE_Leveling_Spend_Intelligence::UGE_Leveling_Spend_Intelligence()
{
	DF_InitSpendGE(*this, UDFAttributeSet::GetIntelligenceAttribute());
}

UGE_Leveling_Spend_Agility::UGE_Leveling_Spend_Agility()
{
	DF_InitSpendGE(*this, UDFAttributeSet::GetAgilityAttribute());
}
