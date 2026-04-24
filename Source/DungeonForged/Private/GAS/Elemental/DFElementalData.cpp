// Source/DungeonForged/Private/GAS/Elemental/DFElementalData.cpp
#include "GAS/Elemental/DFElementalData.h"

float FDFElementalAffinityRow::GetResistance(const EDFElementType VsElement) const
{
	if (VsElement == EDFElementType::None)
	{
		return 1.f;
	}
	for (const FDFElementalResistanceEntry& E : Resistances)
	{
		if (E.Element == VsElement)
		{
			return E.Multiplier;
		}
	}
	return 1.f;
}

FGameplayTag FDFElementalAffinityRow::GetReactionForIncoming(const EDFElementType Incoming) const
{
	if (Incoming == EDFElementType::None)
	{
		return FGameplayTag();
	}
	for (const FDFElementalReactionTagEntry& E : ReactionByIncomingElement)
	{
		if (E.WhenHitBy == Incoming)
		{
			return E.ReactionTag;
		}
	}
	return FGameplayTag();
}
