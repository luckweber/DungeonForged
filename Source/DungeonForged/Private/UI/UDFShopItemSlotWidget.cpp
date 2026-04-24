// Source/DungeonForged/Private/UI/UDFShopItemSlotWidget.cpp
#include "UI/UDFShopItemSlotWidget.h"
#include "Data/DFDataTableStructs.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Engine/DataTable.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Merchant/ADFMerchantActor.h"
#include "Merchant/DFMerchantData.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/UDFItemTooltipWidget.h"
#include "Sound/SoundBase.h"

void UDFShopItemSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (BuyButton)
	{
		BuyButton->OnClicked.AddDynamic(this, &UDFShopItemSlotWidget::OnBuyClicked);
	}
}

void UDFShopItemSlotWidget::InitializeSlot(ADFMerchantActor* InOwner, int32 InSlotIndex, UDataTable* InItemTable)
{
	OwnerMerchant = InOwner;
	SlotIndex = InSlotIndex;
	ItemDataTable = InItemTable;
}

void UDFShopItemSlotWidget::RefreshFromEntry(const FDFMerchantStockEntry& Entry, int32 const PlayerGold, bool const bHasEntry)
{
	bEntryActive = bHasEntry;
	LastShownGold = PlayerGold;
	if (!bHasEntry)
	{
		if (ItemName)
		{
			ItemName->SetText(INVTEXT("—"));
		}
		if (RarityLabel)
		{
			RarityLabel->SetText(FText::GetEmpty());
		}
		if (PriceLabel)
		{
			PriceLabel->SetText(FText::GetEmpty());
		}
		if (QuantityLabel)
		{
			QuantityLabel->SetText(FText::GetEmpty());
		}
		if (ItemIcon)
		{
			ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (BuyButton)
		{
			BuyButton->SetIsEnabled(false);
		}
		return;
	}
	const FDFItemTableRow* const Row = ItemDataTable
		? ItemDataTable->FindRow<FDFItemTableRow>(Entry.ItemRowName, TEXT("ShopSlot"), false)
		: nullptr;
	if (ItemName)
	{
		ItemName->SetText(Row ? Row->ItemName : FText::FromName(Entry.ItemRowName));
	}
	if (RarityLabel)
	{
		if (Row)
		{
			RarityLabel->SetText(UDFMerchantUIStatics::RarityToDisplayName(Row->Rarity));
			const FSlateColor C(UDFMerchantUIStatics::RarityToColor(Row->Rarity));
			RarityLabel->SetColorAndOpacity(C);
		}
	}
	if (PriceLabel)
	{
		PriceLabel->SetText(FText::Format(
			INVTEXT("{0} Gold"), FText::AsNumber(Entry.UnitPrice)));
	}
	if (QuantityLabel)
	{
		QuantityLabel->SetText(Entry.bUnlimited
			? FText::FromString(TEXT("∞"))
			: FText::Format(INVTEXT("x{0}"), FText::AsNumber(Entry.Quantity)));
	}
	if (ItemIcon)
	{
		if (Row && Row->Icon)
		{
			ItemIcon->SetBrushFromTexture(Row->Icon, false);
			ItemIcon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (BuyButton)
	{
		const bool bCan = PlayerGold >= Entry.UnitPrice && (Entry.bUnlimited || Entry.Quantity > 0);
		BuyButton->SetIsEnabled(bCan);
	}
}

void UDFShopItemSlotWidget::OnBuyClicked()
{
	if (!bEntryActive)
	{
		return;
	}
	ADFMerchantActor* const M = OwnerMerchant.Get();
	ADFPlayerCharacter* const C = GetDFPlayerCharacter();
	if (M && C)
	{
		C->ServerMerchantPurchase(M, SlotIndex);
	}
}

void UDFShopItemSlotWidget::PlayPurchaseCoinSfx() const
{
	if (PurchaseCoinSfx)
	{
		UGameplayStatics::PlaySound2D(this, PurchaseCoinSfx);
	}
}
