// Source/DungeonForged/Public/Equipment/UDFEquipmentComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Equipment/DFEquipmentTypes.h"
#include "GameplayEffectTypes.h"
#include "AttributeSet.h"
#include "Data/DFDataTableStructs.h"
#include "UDFEquipmentComponent.generated.h"

class UAbilitySystemComponent;
class UDataTable;
class USkeletalMesh;
class USkeletalMeshComponent;
class UDFInventoryComponent;
struct FGameplayAttribute;

USTRUCT(BlueprintType)
struct FDFEquippedItemRep
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "DF|Equipment")
	EEquipmentSlot Slot = EEquipmentSlot::None;

	UPROPERTY(BlueprintReadOnly, Category = "DF|Equipment")
	FName ItemRow = NAME_None;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnDFEquipmentChanged, EEquipmentSlot, Slot, FName, ItemRow);

UCLASS(ClassGroup = (DF), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFEquipmentComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Equipment")
	TObjectPtr<UDataTable> ItemDataTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Equipment|Visuals")
	TMap<EEquipmentSlot, TObjectPtr<USkeletalMesh>> DefaultNakedMeshes;

	UPROPERTY(BlueprintReadOnly, Category = "DF|Equipment")
	TMap<EEquipmentSlot, FName> EquippedItems;

	TMap<EEquipmentSlot, FActiveGameplayEffectHandle> EquipEffectHandles;
	TMap<EEquipmentSlot, USkeletalMeshComponent*> SlotMeshComponents;

	/** Modularity: skin mesh from the player; used for SetLeaderPoseComponent. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|Equipment|Visuals")
	TObjectPtr<USkeletalMeshComponent> BaseBodyMesh = nullptr;

	UPROPERTY(BlueprintAssignable, Category = "DF|Equipment|Events")
	FOnDFEquipmentChanged OnEquipmentChanged;

	UPROPERTY(ReplicatedUsing = OnRep_Loadout, BlueprintReadOnly, Category = "DF|Equipment")
	TArray<FDFEquippedItemRep> ReplicatedLoadout;

	UFUNCTION(BlueprintCallable, Category = "DF|Equipment|Visuals")
	void RegisterBaseBodyMesh(USkeletalMeshComponent* BaseMesh) { BaseBodyMesh = BaseMesh; }

	UFUNCTION(BlueprintCallable, Category = "DF|Equipment")
	bool EquipItem(FName ItemRowName, EEquipmentSlot Slot);

	UFUNCTION(BlueprintCallable, Category = "DF|Equipment")
	void RequestEquipItem(FName ItemRowName, EEquipmentSlot Slot);

	UFUNCTION(BlueprintCallable, Category = "DF|Equipment")
	void UnequipSlot(EEquipmentSlot Slot);

	UFUNCTION(BlueprintCallable, Category = "DF|Equipment")
	void RequestUnequipSlot(EEquipmentSlot Slot);

	/** C++: use GetItemData(EquippedItems) or this. UHT: no USTRUCT* return. */
	const FDFItemTableRow* GetEquippedItemDataRaw(EEquipmentSlot Slot) const;

	UFUNCTION(BlueprintCallable, Category = "DF|Equipment")
	bool TryGetEquippedItemData(EEquipmentSlot Slot, FDFItemTableRow& OutRow) const;

	UFUNCTION(BlueprintPure, Category = "DF|Equipment")
	bool IsSlotEmpty(EEquipmentSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "DF|Equipment")
	float GetTotalStatBonus(FGameplayAttribute Attribute) const;

	UFUNCTION(BlueprintCallable, Category = "DF|Equipment")
	void RegisterSlotMesh(EEquipmentSlot Slot, USkeletalMeshComponent* Mesh);

	USkeletalMeshComponent* GetSlotMesh(EEquipmentSlot Slot) const;

	UFUNCTION(BlueprintCallable, Category = "DF|Equipment|Visuals")
	void SwapSlotMesh(EEquipmentSlot Slot, USkeletalMesh* NewMesh, USkeletalMeshComponent* BaseMesh);

	static bool DoesItemMatchEquipmentSlot(
		const FDFItemTableRow& Row, EEquipmentSlot RequestedSlot, FString* OutError = nullptr);
	static EEquipmentSlot ResolveItemEquipmentSlot(const FDFItemTableRow& Row);

	/** After registering all slot meshes (e.g. in BeginPlay). */
	UFUNCTION(BlueprintCallable, Category = "DF|Equipment|Visuals")
	void RefreshEquipmentVisuals() { RecalculateAllVisuals(); }

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Loadout();

	void SyncReplicatedArrayFromMap();
	void RebuildMapFromReplicated();
	void RecalculateAllVisuals();
	void RecalculateVisualsForSlot(EEquipmentSlot Slot);
	bool EquipItemInternal(FName ItemRowName, EEquipmentSlot Slot, FString& OutError);
	void UnequipSlotInternal(EEquipmentSlot Slot);

	UAbilitySystemComponent* ResolveOwnerASC() const;
	UDFInventoryComponent* ResolveInventory() const;
	const FDFItemTableRow* GetItemData(FName RowName) const;

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerEquipItem(FName ItemRowName, EEquipmentSlot Slot);
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerUnequipSlot(EEquipmentSlot Slot);
};
