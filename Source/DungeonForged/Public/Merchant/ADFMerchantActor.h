// Source/DungeonForged/Public/Merchant/ADFMerchantActor.h
#pragma once

#include "CoreMinimal.h"
#include "Interaction/ADFInteractableBase.h"
#include "Merchant/DFMerchantData.h"
#include "ADFMerchantActor.generated.h"

class UDataTable;
class UAnimMontage;
class UDFShopWidget;
class UDFShopItemSlotWidget;
class USkeletalMeshComponent;
class UTexture2D;
class ADFPlayerCharacter;
class UDFRunManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDFMerchantPurchase, ADFPlayerCharacter*, Buyer, FName, ItemRowName, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDFMerchantStockChanged);

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFMerchantActor : public ADFInteractableBase
{
	GENERATED_BODY()

public:
	ADFMerchantActor();

	//~ AActor
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//~ IDFInteractable
	virtual FText GetInteractionText_Implementation() const override;
	virtual void Interact_Implementation(ACharacter* Interactor) override;

	/** Regenerate the shop table for the current run floor. Authority only. */
	UFUNCTION(BlueprintCallable, Category = "DF|Merchant")
	void GenerateStock();

	/** Deducts gold from run, rolls new stock, doubles next reroll cost. Authority; Buyer must be valid. */
	UFUNCTION(BlueprintCallable, Category = "DF|Merchant")
	bool RerollStock(ADFPlayerCharacter* Buyer);

	/** Spends run gold, delivers item, updates quantity. Authority only. */
	UFUNCTION(BlueprintCallable, Category = "DF|Merchant")
	bool PurchaseItem(int32 SlotIndex, ADFPlayerCharacter* Buyer);

	/** After a new floor, restocks consumable lines only. Authority. */
	UFUNCTION(BlueprintCallable, Category = "DF|Merchant")
	void RestockConsumablesFromTable();

	UFUNCTION(BlueprintPure, Category = "DF|Merchant")
	int32 GetNextRerollCost() const;

	/** Base gold for first reroll; each successful reroll doubles the *next* cost (2^RerollCount * Base). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Merchant", Replicated, meta = (ClampMin = "0"))
	int32 RerollBaseCost = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Merchant", Replicated, meta = (ClampMin = "0"))
	int32 RerollCount = 0;

	/** How many item slots the merchant shows. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Merchant", meta = (ClampMin = "1", ClampMax = "24"))
	int32 StockSlots = 6;

	/** If null, UDFRunManager::ItemDataTable is used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Merchant")
	TObjectPtr<UDataTable> ItemDataTableOverride = nullptr;

	/** DataTable of FDFMerchantStockRow. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Merchant")
	TObjectPtr<UDataTable> MerchantStockTable = nullptr;

	/** C++: WBP_Shop (child of UDFShopWidget). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Merchant|UI")
	TSubclassOf<UDFShopWidget> ShopWidgetClass;

	/** C++: WBP_ShopItemSlot (concrete class). Required for populating the slot grid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Merchant|UI")
	TSubclassOf<UDFShopItemSlotWidget> ShopItemSlotClass;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentStock, Category = "DF|Merchant")
	TArray<FDFMerchantStockEntry> CurrentStock;

	/** UMG header + portrait. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Merchant|UI")
	FText MerchantDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Merchant|UI")
	TObjectPtr<UTexture2D> MerchantPortrait = nullptr;

	/** Skeletal display (optional; hides the default static interactable mesh). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Merchant|Visuals")
	TObjectPtr<USkeletalMeshComponent> MerchantMesh = nullptr;

	/** Loop/idle; played on BeginPlay on MerchantMesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Merchant|Visuals")
	TObjectPtr<UAnimMontage> IdleAnimation = nullptr;

	UPROPERTY(BlueprintAssignable, Category = "DF|Merchant|Events")
	FOnDFMerchantPurchase OnPurchaseComplete;

	UPROPERTY(BlueprintAssignable, Category = "DF|Merchant|Events")
	FOnDFMerchantStockChanged OnStockChanged;

protected:
	UFUNCTION()
	void OnRep_CurrentStock();

	/** Called on server when run floor advances. */
	UFUNCTION()
	void HandleRunFloorChanged(int32 NewFloor);

	UDFRunManager* ResolveRunManager() const;
	UDataTable* ResolveItemTable() const;
	bool AuthoritySpendGold(ADFPlayerCharacter* Buyer, int32 Amount);
	bool IsBuyerInInteractionRange(ADFPlayerCharacter* Buyer) const;
	static int32 RarityToPickWeightIndex(EItemRarity R);

	/** Filled from the stock row; used to restore consumables on floor up. */
	UPROPERTY(Transient)
	TMap<FName, int32> RestockTemplateQuantities;
};
