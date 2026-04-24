// Source/DungeonForged/Private/UI/UDFItemTooltipWidget.cpp
#include "UI/UDFItemTooltipWidget.h"
#include "AttributeSet.h"
#include "Data/DFDataTableStructs.h"
#include "Engine/DataTable.h"
#include "Merchant/DFMerchantData.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Internationalization/Internationalization.h"

void UDFItemTooltipWidget::SetItemData(
	const FDFItemTableRow& Row,
	const FDFItemTableRow& OptionalEquipped,
	bool const bUseEquippedCompare,
	UDataTable* /*ItemTableForLookup*/)
{
	if (ItemName)
	{
		ItemName->SetText(Row.ItemName);
		const FSlateColor C(UDFMerchantUIStatics::RarityToColor(Row.Rarity));
		ItemName->SetColorAndOpacity(C);
	}
	if (ItemType)
	{
		static const UEnum* TypeEnum = StaticEnum<EItemType>();
		ItemType->SetText(TypeEnum
			? FText::FromString(TypeEnum->GetNameStringByValue(static_cast<int64>(Row.ItemType)))
			: FText::GetEmpty());
	}
	if (Description)
	{
		Description->SetText(Row.Description);
	}
	if (StatBlock)
	{
		FString S;
		for (const TPair<FGameplayAttribute, float>& P : Row.AttributeModifiers)
		{
			if (P.Key.IsValid())
			{
				S += FString::Printf(
					TEXT("+%.0f %s\n"), P.Value, *P.Key.GetName());
			}
		}
		S.TrimEndInline();
		StatBlock->SetText(FText::FromString(S));
	}
	if (CompareText)
	{
		if (bUseEquippedCompare && OptionalEquipped.ItemType == Row.ItemType)
		{
			float SumA = 0.f;
			for (const TPair<FGameplayAttribute, float>& P : Row.AttributeModifiers)
			{
				SumA += P.Value;
			}
			float SumB = 0.f;
			for (const TPair<FGameplayAttribute, float>& P : OptionalEquipped.AttributeModifiers)
			{
				SumB += P.Value;
			}
			const float D = SumA - SumB;
			if (D > 0.5f)
			{
				CompareText->SetText(FText::Format(
					INVTEXT("▲ {0} vs equipped"), FText::AsNumber(int32(D))));
				CompareText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.9f, 0.2f, 1.f)));
			}
			else if (D < -0.5f)
			{
				CompareText->SetText(FText::Format(
					INVTEXT("▼ {0} vs equipped"), FText::AsNumber(int32(D))));
				CompareText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.2f, 0.2f, 1.f)));
			}
			else
			{
				CompareText->SetText(FText::GetEmpty());
			}
		}
		else
		{
			CompareText->SetText(FText::GetEmpty());
		}
	}
}
