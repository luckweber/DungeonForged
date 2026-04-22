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
	int32 MaxSlots = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Inventory")
	TObjectPtr<UDataTable> ItemDataTable = nullptr;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Items, Category = "DF|Inventory")
	TArray<FDFInventorySlot> Items;

	UFUNCTION(BlueprintCallable, Category = "DF|Inventory")
	bool AddItem(FName RowName, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "DF|Inventory")
	void RemoveItem(FName RowName, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "DF|Inventory")
	void EquipItem(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "DF|Inventory")
	void UnequipItem(int32 SlotIndex);

	/** Non-null if RowName exists in ItemDataTable. */
	const FDFItemTableRow* GetItemData(FName RowName) const;

	UFUNCTION(BlueprintPure, Category = "DF|Inventory")
	bool IsSlotValidIndex(int32 SlotIndex) const { return Items.IsValidIndex(SlotIndex); }

	UPROPERTY(BlueprintAssignable, Category = "DF|Inventory|Events")
	FOnDFInventoryChanged OnInventoryChanged;

	/** Called from loot actors when a non-authority client overlaps (RPC to server). */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerPickUpFromLoot(class ADFLootDrop* Source);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Items();

	UAbilitySystemComponent* ResolveOwnerASC() const;
	bool IsAuthority() const;

	/** Server-only: other equipped slots of the same item type are unequipped first. */
	void UnequipOthersOfType(EItemType Type, int32 ExceptSlot);

	/** Server-only: slot index -> active OnEquip effect (indices shift on RemoveItem). */
	TMap<int32, FActiveGameplayEffectHandle> EquipHandles;
	void ReindexEquipHandlesAfterRemoveAt(int32 RemovedIndex);
};
