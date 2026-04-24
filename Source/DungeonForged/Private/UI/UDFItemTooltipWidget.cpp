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

void UDFItemTooltipWidget::SetItemDataEx(
	const FDFItemTableRow& Hovered,
	const FDFItemTableRow& EquippedInSlot,
	const bool bCompare,
	const bool bEquippedSlotIsEmpty,
	UDataTable* const /*ItemTableForLookup*/)
{
	SetItemData(Hovered, EquippedInSlot, false, nullptr);
	if (!bCompare)
	{
		if (CompareText)
		{
			CompareText->SetText(FText::GetEmpty());
		}
		return;
	}
	if (bEquippedSlotIsEmpty)
	{
		if (StatBlock)
		{
			StatBlock->SetText(FText::Format(
				INVTEXT("{0}\n\n(NEW)"),
				StatBlock->GetText()));
		}
		if (CompareText)
		{
			CompareText->SetText(FText::FromString(TEXT("NEW (empty equipment slot)")));
			CompareText->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.9f, 0.4f, 1.f)));
		}
		return;
	}
	FString DeltaLines;
	for (const TPair<FGameplayAttribute, float>& P : Hovered.AttributeModifiers)
	{
		if (!P.Key.IsValid())
		{
			continue;
		}
		float B = 0.f;
		if (const float* F = EquippedInSlot.AttributeModifiers.Find(P.Key))
		{
			B = *F;
		}
		const float D = P.Value - B;
		if (D > 0.01f)
		{
			DeltaLines += FString::Printf(
				TEXT("▲ +%.1f %s (vs equipped)\n"), D, *P.Key.GetName());
		}
		else if (D < -0.01f)
		{
			DeltaLines += FString::Printf(
				TEXT("▼ %.1f %s (vs equipped)\n"), D, *P.Key.GetName());
		}
	}
	if (CompareText)
	{
		CompareText->SetText(
			DeltaLines.IsEmpty() ? FText::GetEmpty() : FText::FromString(DeltaLines));
		CompareText->SetColorAndOpacity(
			FSlateColor(
				DeltaLines.Contains(TEXT("▼")) ? FLinearColor(0.9f, 0.2f, 0.2f, 1.f)
					: (DeltaLines.Contains(TEXT("▲")) ? FLinearColor(0.2f, 0.85f, 0.25f, 1.f)
						: FLinearColor(0.9f, 0.9f, 0.9f, 1.f))));
	}
}
