// Source/DungeonForged/Public/UI/UDFShopWidget.h
#pragma once

#include "CoreMinimal.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFShopWidget.generated.h"

class UUniformGridPanel;
class UButton;
class UTextBlock;
class UImage;
class USoundBase;
class UWidgetAnimation;
class ADFMerchantActor;
class UDFShopItemSlotWidget;

UCLASS(Blueprintable, Abstract)
class DUNGEONFORGED_API UDFShopWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	void OpenForMerchant(ADFMerchantActor* Merchant);

	/** FX/SFX for a slot after a successful buy (driven from ClientNotifyMerchantPurchase). */
	UFUNCTION(BlueprintCallable, Category = "DF|Merchant|UI")
	void PlaySlotPurchaseFeedback(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "DF|Merchant|UI")
	void CloseShop();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void RefreshAll();
	void RefreshGold();

	UFUNCTION()
	void OnRerollClicked();

	UFUNCTION()
	void OnCloseClicked();

	UFUNCTION()
	void HandleStockChanged();

	UFUNCTION()
	void HandleGoldChanged(int32 NewGold);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MerchantNameText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> MerchantPortrait = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GoldAmountText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CoinIcon = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> ItemGrid = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RerollButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RerollCostText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton = nullptr;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> PurchaseSlotFadeAnim = nullptr;

	TWeakObjectPtr<ADFMerchantActor> ActiveMerchant;
	TArray<TObjectPtr<UDFShopItemSlotWidget>> SlotWidgets;
};
