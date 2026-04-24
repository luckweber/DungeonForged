// Source/DungeonForged/Public/UI/UDFShopItemSlotWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Merchant/DFMerchantData.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFShopItemSlotWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UDataTable;
class USoundBase;
class UDFItemTooltipWidget;
class ADFMerchantActor;

UCLASS(Blueprintable)
class DUNGEONFORGED_API UDFShopItemSlotWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	void InitializeSlot(ADFMerchantActor* InOwner, int32 InSlotIndex, UDataTable* InItemTable);

	UFUNCTION(BlueprintCallable, Category = "DF|Merchant|UI")
	void RefreshFromEntry(const FDFMerchantStockEntry& Entry, int32 PlayerGold, bool bHasEntry);

	UFUNCTION(BlueprintImplementableEvent, Category = "DF|Merchant|UI")
	void OnPurchaseFeedback();

	UFUNCTION(BlueprintCallable, Category = "DF|Merchant|UI")
	void PlayPurchaseCoinSfx() const;

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnBuyClicked();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ItemIcon = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemName = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RarityLabel = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PriceLabel = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> QuantityLabel = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BuyButton = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Merchant|UI|Audio")
	TObjectPtr<USoundBase> PurchaseCoinSfx = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Merchant|UI")
	TSubclassOf<UDFItemTooltipWidget> ItemTooltipClass;

	TWeakObjectPtr<ADFMerchantActor> OwnerMerchant;
	int32 SlotIndex = INDEX_NONE;
	TObjectPtr<UDataTable> ItemDataTable = nullptr;
	bool bEntryActive = false;
	int32 LastShownGold = 0;
};
