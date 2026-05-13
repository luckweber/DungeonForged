// Source/DungeonForged/Public/DFInventoryComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Data/DFDataTableStructs.h"
#include "Components/ActorComponent.h"
#include "GameplayEffect.h"
#include "DFInventoryComponent.generated.h"

class UAbilitySystemComponent;
class UDataTable;
class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDFInventoryChanged);

UCLASS(ClassGroup = (DF), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFInventoryComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Items, Category = "DF|Inventory")
	int32 MaxSlots = 25;

	/**
	 * Maximum carried weight across all stacks in Items. 0 = unlimited (legacy).
	 * Inspired by replicated grid inventories (item weight totals); tweak per character if needed.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		ReplicatedUsing = OnRep_Items,
		Category = "DF|Inventory|Encumbrance",
		meta = (ClampMin = "0"))
	float MaxCarryWeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Inventory")
	TObjectPtr<UDataTable> ItemDataTable = nullptr;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Items, Category = "DF|Inventory")
	TArray<FDFInventorySlot> Items;

	UFUNCTION(BlueprintCallable, Category = "DF|Inventory")
	bool AddItem(FName RowName, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "DF|Inventory", meta = (AdvancedDisplay = "PreferredSlotIndex"))
	void RemoveItem(FName RowName, int32 Quantity = 1, int32 PreferredSlotIndex = -1);

	UFUNCTION(BlueprintCallable, Category = "DF|Inventory")
	void EquipItem(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "DF|Inventory")
	void UnequipItem(int32 SlotIndex);

	/** Non-null if RowName exists in ItemDataTable. */
	const FDFItemTableRow* GetItemData(FName RowName) const;

	UFUNCTION(BlueprintPure, Category = "DF|Inventory")
	bool IsSlotValidIndex(int32 SlotIndex) const { return Items.IsValidIndex(SlotIndex); }

	UFUNCTION(BlueprintPure, Category = "DF|Inventory|Encumbrance")
	float GetCurrentCarriedWeight() const;

	UFUNCTION(BlueprintPure, Category = "DF|Inventory|Encumbrance")
	float GetEffectiveMaxCarryWeight() const { return MaxCarryWeight; }

	UFUNCTION(BlueprintPure, Category = "DF|Inventory|Encumbrance")
	float GetRemainingCarryCapacity() const;

	UFUNCTION(BlueprintPure, Category = "DF|Inventory|Encumbrance")
	bool IsCarryWeightLimited() const { return MaxCarryWeight > KINDA_SMALL_NUMBER; }

	/** True when MaxCarryWeight is set and BagRowQty would push total over capacity (prediction from replicated Items). */
	UFUNCTION(BlueprintPure, Category = "DF|Inventory|Encumbrance")
	bool WouldRejectAddDueToWeight(FName RowName, int32 Quantity) const;

	/**
	 * Server-only helper: place qty from unequipped gear into BagIndex (empty / merge / displace-and-re-home).
	 * Returns false if nothing was committed (caller can use AddItem as fallback).
	 */
	bool ReceiveUnequippedItemAtBagIndex(int32 BagIndex, FName RowName, int32 Quantity);

	/**
	 * Client/UI prediction: same rules as @ref ReceiveUnequippedItemAtBagIndex without mutating.
	 */
	UFUNCTION(BlueprintPure, Category = "DF|Inventory")
	bool PredictCanReceiveUnequippedStackAtBagIndex(int32 BagIndex, FName RowName, int32 Quantity = 1) const;

	/** Reorder bag: swap stacks at two indices (runs on authority; forwards via Server RPC from clients). */
	UFUNCTION(BlueprintCallable, Category = "DF|Inventory")
	void RequestMoveBagSlot(int32 SourceSlotIndex, int32 TargetSlotIndex);

	UPROPERTY(BlueprintAssignable, Category = "DF|Inventory|Events")
	FOnDFInventoryChanged OnInventoryChanged;

	/** Called from loot actors when a non-authority client overlaps (RPC to server). */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerPickUpFromLoot(class ADFLootDrop* Source);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRequestMoveBagSlot(int32 SourceSlotIndex, int32 TargetSlotIndex);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Items();

	UAbilitySystemComponent* ResolveOwnerASC() const;
	bool IsAuthority() const;

	/** Server-only: other equipped slots of the same item type are unequipped first. */
	void UnequipOthersOfType(EItemType Type, int32 ExceptSlot);

	/** Server-only: fixed bag grid sized to MaxSlots (empty slots = None + qty 0). */
	void EnsureAuthorityBagGridSized();

	float ComputeStackContributionWeight(const FDFInventorySlot& Slot) const;

	/** Unlimited when MaxCarryWeight <= 0. */
	bool CanAffordCarryWeightDelta(float DeltaWeight) const;

	/** Server-only: slot index -> active OnEquip effect (indices shift on RemoveItem). */
	TMap<int32, FActiveGameplayEffectHandle> EquipHandles;

	/** Server-only: swaps Items stacks and EquipHandles keys so equipped effects stay aligned. */
	void MoveBagSlotInternal(int32 SourceSlotIndex, int32 TargetSlotIndex);
};
