// Source/DungeonForged/Private/Equipment/UDFEquipmentSlotWidget.cpp
#include "Equipment/UDFEquipmentSlotWidget.h"
#include "Equipment/UDFEquipmentComponent.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Data/DFDataTableStructs.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Merchant/DFMerchantData.h"

void UDFEquipmentSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshFromEquipment();
}

void UDFEquipmentSlotWidget::RefreshFromEquipment()
{
	if (EquipSlot == EEquipmentSlot::None)
	{
		return;
	}
	const ADFPlayerCharacter* const PC = GetDFPlayerCharacter();
	if (!PC)
	{
		return;
	}
	const UDFEquipmentComponent* const Eq = PC->GetDFEquipment();
	if (!Eq)
	{
		return;
	}
	if (const FDFItemTableRow* const Row = Eq->GetEquippedItemDataRaw(EquipSlot))
	{
		if (SlotIcon && Row->Icon)
		{
			SlotIcon->SetBrushFromTexture(Row->Icon, true);
			SlotIcon->SetColorAndOpacity(FLinearColor::White);
		}
		if (ItemLevelText)
		{
			if (Row->ItemLevel > 0)
			{
				ItemLevelText->SetText(FText::AsNumber(Row->ItemLevel));
				ItemLevelText->SetVisibility(ESlateVisibility::Visible);
			}
			else
			{
				ItemLevelText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
		if (RarityBorder)
		{
			RarityBorder->SetColorAndOpacity(UDFMerchantUIStatics::RarityToColor(Row->Rarity));
			RarityBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
	else
	{
		if (SlotIcon)
		{
			SlotIcon->SetBrushFromTexture(nullptr, false);
			SlotIcon->SetColorAndOpacity(FLinearColor(0.35f, 0.35f, 0.35f, 0.9f));
		}
		if (ItemLevelText)
		{
			ItemLevelText->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (RarityBorder)
		{
			RarityBorder->SetColorAndOpacity(FLinearColor(0.2f, 0.2f, 0.2f, 0.7f));
		}
	}
}

bool UDFEquipmentSlotWidget::RequestEquipItemRow(const FName ItemRowName)
{
	ADFPlayerCharacter* const PC = GetDFPlayerCharacter();
	if (!PC || ItemRowName.IsNone() || EquipSlot == EEquipmentSlot::None)
	{
		return false;
	}
	if (UDFEquipmentComponent* const Eq = PC->GetDFEquipment())
	{
		return Eq->EquipItem(ItemRowName, EquipSlot);
	}
	return false;
}
