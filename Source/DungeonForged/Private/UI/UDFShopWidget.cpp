// Source/DungeonForged/Private/UI/UDFShopWidget.cpp
#include "UI/UDFShopWidget.h"
#include "DungeonForgedModule.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Characters/ADFPlayerState.h"
#include "Merchant/ADFMerchantActor.h"
#include "Merchant/DFMerchantData.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Merchant/ADFMerchantActor.h"
#include "Run/DFRunManager.h"
#include "UI/UDFShopItemSlotWidget.h"
#include "Components/PanelWidget.h"

void UDFShopWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (RerollButton)
	{
		RerollButton->OnClicked.AddDynamic(this, &UDFShopWidget::OnRerollClicked);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UDFShopWidget::OnCloseClicked);
	}
}

void UDFShopWidget::NativeDestruct()
{
	if (ADFMerchantActor* const M = ActiveMerchant.Get())
	{
		M->OnStockChanged.RemoveDynamic(this, &UDFShopWidget::HandleStockChanged);
	}
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>())
		{
			RM->OnGoldChanged.RemoveDynamic(this, &UDFShopWidget::HandleGoldChanged);
		}
	}
	Super::NativeDestruct();
}

void UDFShopWidget::OpenForMerchant(ADFMerchantActor* Merchant)
{
	if (ActiveMerchant.IsValid())
	{
		if (ADFMerchantActor* const Old = ActiveMerchant.Get())
		{
			Old->OnStockChanged.RemoveDynamic(this, &UDFShopWidget::HandleStockChanged);
		}
	}
	ActiveMerchant = Merchant;
	if (Merchant)
	{
		Merchant->OnStockChanged.AddDynamic(this, &UDFShopWidget::HandleStockChanged);
		if (MerchantNameText)
		{
			MerchantNameText->SetText(
				Merchant->MerchantDisplayName.IsEmpty()
					? NSLOCTEXT("DF", "ShopDefault", "Shop")
					: Merchant->MerchantDisplayName);
		}
		if (MerchantPortrait && Merchant->MerchantPortrait)
		{
			MerchantPortrait->SetBrushFromTexture(Merchant->MerchantPortrait, false);
			MerchantPortrait->SetVisibility(ESlateVisibility::Visible);
		}
	}
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>())
		{
			RM->OnGoldChanged.AddDynamic(this, &UDFShopWidget::HandleGoldChanged);
		}
	}
	SlotWidgets.Reset();
	if (ItemGrid)
	{
		ItemGrid->ClearChildren();
		const TSubclassOf<UDFShopItemSlotWidget> SlotClass = Merchant->ShopItemSlotClass
			? Merchant->ShopItemSlotClass
			: TSubclassOf<UDFShopItemSlotWidget>(UDFShopItemSlotWidget::StaticClass());
		if (!SlotClass)
		{
			return;
		}
		UDataTable* IT = Merchant->ItemDataTableOverride.Get();
		if (!IT)
		{
			if (UGameInstance* const GI = GetGameInstance())
			{
				if (UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>())
				{
					IT = RM->ItemDataTable.Get();
				}
			}
		}
		for (int32 i = 0; i < Merchant->StockSlots; ++i)
		{
			if (UDFShopItemSlotWidget* const ItemSlot = CreateWidget<UDFShopItemSlotWidget>(this, SlotClass))
			{
				ItemSlot->InitializeSlot(Merchant, i, IT);
				ItemGrid->AddChildToUniformGrid(ItemSlot, i / 3, i % 3);
				SlotWidgets.Add(ItemSlot);
			}
		}
	}
	RefreshAll();
	if (APlayerController* const PC = GetOwningPlayer())
	{
		FInputModeUIOnly Mode;
		DFPrepareWidgetForUIModeFocus(this);
		Mode.SetWidgetToFocus(TakeWidget());
		PC->SetInputMode(Mode);
		PC->SetShowMouseCursor(true);
	}
}

void UDFShopWidget::PlaySlotPurchaseFeedback(int32 const SlotIndex)
{
	if (SlotWidgets.IsValidIndex(SlotIndex) && SlotWidgets[SlotIndex])
	{
		SlotWidgets[SlotIndex]->OnPurchaseFeedback();
		SlotWidgets[SlotIndex]->PlayPurchaseCoinSfx();
		if (PurchaseSlotFadeAnim)
		{
			PlayAnimation(PurchaseSlotFadeAnim);
		}
	}
}

void UDFShopWidget::CloseShop()
{
	if (ADFPlayerCharacter* const C = GetDFPlayerCharacter())
	{
		C->ClearActiveShopWidget();
	}
	if (APlayerController* const PC = GetOwningPlayer())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
	RemoveFromParent();
}

void UDFShopWidget::RefreshGold()
{
	int32 G = 0;
	if (ADFPlayerState* const PS = GetDFPlayerState())
	{
		G = PS->GetReplicatedRunGold();
	}
	if (GoldAmountText)
	{
		GoldAmountText->SetText(FText::AsNumber(G));
	}
	if (RerollCostText)
	{
		if (ADFMerchantActor* const M = ActiveMerchant.Get())
		{
			RerollCostText->SetText(FText::Format(
				NSLOCTEXT("DF", "RerollFmt", "Reroll stock — {0} Gold"),
				FText::AsNumber(M->GetNextRerollCost())));
		}
	}
}

void UDFShopWidget::RefreshAll()
{
	RefreshGold();
	ADFMerchantActor* const M = ActiveMerchant.Get();
	if (!M)
	{
		return;
	}
	int32 PlayerGold = 0;
	if (ADFPlayerState* const PS = GetDFPlayerState())
	{
		PlayerGold = PS->GetReplicatedRunGold();
	}
	const TArray<FDFMerchantStockEntry>& Stock = M->CurrentStock;
	for (int32 i = 0; i < SlotWidgets.Num(); ++i)
	{
		if (UDFShopItemSlotWidget* const S = SlotWidgets[i].Get())
		{
			if (Stock.IsValidIndex(i))
			{
				S->RefreshFromEntry(Stock[i], PlayerGold, true);
			}
			else
			{
				S->RefreshFromEntry(FDFMerchantStockEntry(), PlayerGold, false);
			}
		}
	}
}

void UDFShopWidget::OnRerollClicked()
{
	if (ADFPlayerCharacter* const C = GetDFPlayerCharacter())
	{
		if (ADFMerchantActor* const M = ActiveMerchant.Get())
		{
			C->ServerMerchantReroll(M);
		}
	}
}

void UDFShopWidget::OnCloseClicked()
{
	CloseShop();
}

void UDFShopWidget::HandleStockChanged()
{
	RefreshAll();
}

void UDFShopWidget::HandleGoldChanged(int32 /*NewGold*/)
{
	RefreshAll();
}
