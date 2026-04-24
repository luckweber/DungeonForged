// Source/DungeonForged/Private/GAS/Elemental/UDFElementalLibrary.cpp
#include "GAS/Elemental/UDFElementalLibrary.h"
#include "GAS/DFGameplayTags.h"

FGameplayTag UDFElementalLibrary::GetElementEffectTag(EDFElementType Element)
{
	switch (Element)
	{
		case EDFElementType::Fire: return FDFGameplayTags::Effect_Element_Fire;
		case EDFElementType::Ice: return FDFGameplayTags::Effect_Element_Ice;
		case EDFElementType::Water: return FDFGameplayTags::Effect_Element_Water;
		case EDFElementType::Lightning: return FDFGameplayTags::Effect_Element_Lightning;
		case EDFElementType::Earth: return FDFGameplayTags::Effect_Element_Earth;
		case EDFElementType::Arcane: return FDFGameplayTags::Effect_Element_Arcane;
		case EDFElementType::Physical: return FDFGameplayTags::Effect_Element_Physical;
		case EDFElementType::ElementTrue: return FDFGameplayTags::Effect_Element_True;
		default: return FGameplayTag();
	}
}

namespace
{
	constexpr float Strong = 1.5f;
	constexpr float Weak = 0.7f;
	constexpr float Neut = 1.f;

	bool Index(const EDFElementType A, const EDFElementType D, int32& OutA, int32& OutD)
	{
		const int32 IA = static_cast<int32>(A);
		const int32 ID = static_cast<int32>(D);
		if (IA <= 0 || ID <= 0 || IA >= static_cast<int32>(EDFElementType::MAX) || ID >= static_cast<int32>(EDFElementType::MAX))
		{
			return false;
		}
		OutA = IA;
		OutD = ID;
		return true;
	}
}

float UDFElementalLibrary::GetAdvantageMultiplier(const EDFElementType AttackElement, const EDFElementType DefenderPrimaryElement)
{
	int32 A = 0, D = 0;
	if (!Index(AttackElement, DefenderPrimaryElement, A, D))
	{
		return Neut;
	}
	// [Attack][Defender] = multiplier. Order of enum: None(0) unused in matrix rows 1..8
	static float M[9][9];
	static bool bInit = false;
	if (!bInit)
	{
		bInit = true;
		for (int32 I = 0; I < 9; ++I)
		{
			for (int32 J = 0; J < 9; ++J)
			{
				M[I][J] = Neut;
			}
		}
		const int32 F = static_cast<int32>(EDFElementType::Fire);
		const int32 Ic = static_cast<int32>(EDFElementType::Ice);
		const int32 W = static_cast<int32>(EDFElementType::Water);
		const int32 L = static_cast<int32>(EDFElementType::Lightning);
		const int32 E = static_cast<int32>(EDFElementType::Earth);

		// Fire
		M[F][Ic] = Strong;
		M[F][W] = Weak;
		M[F][E] = Weak;
		// Ice
		M[Ic][E] = Strong;
		M[Ic][F] = Weak;
		// Lightning
		M[L][W] = Strong;
		M[L][E] = Weak;
		// Earth
		M[E][L] = Strong;
		M[E][Ic] = Weak;
	}
	return M[A][D];
}

FText UDFElementalLibrary::GetElementGlyph(const EDFElementType Element)
{
	// Kept as text so UMG can render; swap for Texture2D in WBP.
	switch (Element)
	{
		case EDFElementType::Fire: return FText::FromString(TEXT("\U0001F525"));
		case EDFElementType::Ice: return FText::FromString(TEXT("\U00002744\U0000FE0F"));
		case EDFElementType::Water: return FText::FromString(TEXT("\U0001F4A7"));
		case EDFElementType::Lightning: return FText::FromString(TEXT("\U000026A1"));
		case EDFElementType::Earth: return FText::FromString(TEXT("\U0001FAA8"));
		case EDFElementType::Arcane: return FText::FromString(TEXT("\u2728"));
		case EDFElementType::Physical: return FText::FromString(TEXT("\U0001F4A5"));
		case EDFElementType::ElementTrue: return FText::FromString(TEXT("\u2605"));
		default: return FText::GetEmpty();
	}
}

FLinearColor UDFElementalLibrary::GetElementColor(const EDFElementType Element)
{
	switch (Element)
	{
		case EDFElementType::Fire: return FLinearColor(1.f, 0.35f, 0.1f, 1.f);
		case EDFElementType::Ice: return FLinearColor(0.4f, 0.8f, 1.f, 1.f);
		case EDFElementType::Water: return FLinearColor(0.2f, 0.5f, 1.f, 1.f);
		case EDFElementType::Lightning: return FLinearColor(1.f, 1.f, 0.2f, 1.f);
		case EDFElementType::Earth: return FLinearColor(0.6f, 0.4f, 0.2f, 1.f);
		case EDFElementType::Arcane: return FLinearColor(0.7f, 0.2f, 1.f, 1.f);
		case EDFElementType::Physical: return FLinearColor(0.9f, 0.9f, 0.9f, 1.f);
		case EDFElementType::ElementTrue: return FLinearColor(1.f, 0.2f, 0.2f, 1.f);
		default: return FLinearColor::White;
	}
}
