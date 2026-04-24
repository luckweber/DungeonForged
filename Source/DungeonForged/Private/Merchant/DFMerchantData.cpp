// Source/DungeonForged/Private/Merchant/DFMerchantData.cpp
#include "Merchant/DFMerchantData.h"

FLinearColor UDFMerchantUIStatics::RarityToColor(const EItemRarity R)
{
	switch (R)
	{
		case EItemRarity::Common: return FLinearColor(0.7f, 0.7f, 0.7f, 1.f);
		case EItemRarity::Uncommon: return FLinearColor(0.2f, 0.85f, 0.25f, 1.f);
		case EItemRarity::Rare: return FLinearColor(0.2f, 0.5f, 1.f, 1.f);
		case EItemRarity::Epic: return FLinearColor(0.65f, 0.25f, 0.9f, 1.f);
		case EItemRarity::Legendary: return FLinearColor(1.f, 0.55f, 0.1f, 1.f);
		default: return FLinearColor::White;
	}
}

FText UDFMerchantUIStatics::RarityToDisplayName(const EItemRarity R)
{
	switch (R)
	{
		case EItemRarity::Common: return INVTEXT("Common");
		case EItemRarity::Uncommon: return INVTEXT("Uncommon");
		case EItemRarity::Rare: return INVTEXT("Rare");
		case EItemRarity::Epic: return INVTEXT("Epic");
		case EItemRarity::Legendary: return INVTEXT("Legendary");
		default: return INVTEXT("—");
	}
}
