// Source/DungeonForged/Public/Equipment/UDFEquipmentComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
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

	/** Loose weapon tags applied to the owner ASC while a weapon row is equipped (server only). */
	FGameplayTagContainer AppliedWeaponLooseTags;

	/** Granted from DT row `WeaponMeleeGameplayAbility` while a weapon occupies the slot (server revoke on unequip). */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|Equipment|GAS")
	FGameplayAbilitySpecHandle GrantedWeaponMeleeAbilitySpecHandle;

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
	bool EquipItem(FName ItemRowName, EEquipmentSlot Slot, int32 SourceBagSlotIndex = -1);

	UFUNCTION(BlueprintCallable, Category = "DF|Equipment")
	void RequestEquipItem(FName ItemRowName, EEquipmentSlot Slot, int32 SourceBagSlotIndex = -1);

	UFUNCTION(BlueprintCallable, Category = "DF|Equipment")
	void UnequipSlot(EEquipmentSlot Slot);

	UFUNCTION(BlueprintCallable, Category = "DF|Equipment")
	void RequestUnequipSlot(EEquipmentSlot Slot);

	/** Unequips into a specific inventory grid cell (falls back to AddItem if placement fails). */
	UFUNCTION(BlueprintCallable, Category = "DF|Equipment")
	void RequestUnequipToBagSlot(EEquipmentSlot EquipmentSlot, int32 TargetBagSlotIndex);

	/** C++: use GetItemData(EquippedItems) or this. UHT: no USTRUCT* return. */
	const FDFItemTableRow* GetEquippedItemDataRaw(EEquipmentSlot Slot) const;

	UFUNCTION(BlueprintCallable, Category = "DF|Equipment")
	bool TryGetEquippedItemData(EEquipmentSlot Slot, FDFItemTableRow& OutRow) const;

	/** True when the equipped Weapon row configures @ref FDFItemTableRow::WeaponMeleeGameplayAbility (server grants on equip). */
	UFUNCTION(BlueprintPure, Category = "DF|Equipment|GAS")
	bool HasGrantedWeaponMeleeAbilitySpec() const;

	/** Activates WeaponMeleeGameplayAbility via ASC lookup (handles replicated specs; no client-side spec handle cache required). */
	UFUNCTION(BlueprintCallable, Category = "DF|Equipment|GAS")
	bool TryActivateGrantedWeaponMeleeAbility();

	/** Server: (re-)grants/removes WeaponMeleeGameplayAbility from the DT row currently in the Weapon slot. Call after run init so starter gear gets its GA after ASC is finalized. */
	UFUNCTION(BlueprintCallable, Category = "DF|Equipment|GAS")
	void SyncWeaponMeleeGameplayAbilityGrant();

	UFUNCTION(BlueprintPure, Category = "DF|Equipment")
	bool IsSlotEmpty(EEquipmentSlot Slot) const;

	/** Client/UI: same rules as server equip (bag has item, slot matches, GAS ready). Does not mutate. */
	UFUNCTION(BlueprintCallable, Category = "DF|Equipment")
	bool PredictCanEquipItem(
		FName ItemRowName,
		EEquipmentSlot Slot,
		FString& OutReason,
		int32 PreferredSourceBagSlot = -1) const;

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
	void RefreshWeaponAnimSetOnOwner();
	bool ValidateEquipPrerequisites(
		FName ItemRowName,
		EEquipmentSlot Slot,
		FString& OutError,
		int32 PreferredSourceBagSlot = INDEX_NONE) const;
	bool EquipItemInternal(
		FName ItemRowName,
		EEquipmentSlot Slot,
		FString& OutError,
		int32 SourceBagSlotIndex = INDEX_NONE);
	void UnequipSlotInternal(EEquipmentSlot Slot, int32 TargetBagSlotIndex = INDEX_NONE);

	void RevokeGrantedWeaponMeleeAbility(UAbilitySystemComponent* ASC);
	void TryGrantWeaponMeleeAbilityFromEquippedRow(UAbilitySystemComponent* ASC, const FDFItemTableRow* Row);

	void SyncEquippedWeaponLooseTags(UAbilitySystemComponent* ASC, const FDFItemTableRow* WeaponRow);
	void ClearEquippedWeaponLooseTags(UAbilitySystemComponent* ASC);

	UAbilitySystemComponent* ResolveOwnerASC() const;
	UDFInventoryComponent* ResolveInventory() const;
	const FDFItemTableRow* GetItemData(FName RowName) const;

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerEquipItem(FName ItemRowName, EEquipmentSlot Slot, int32 SourceBagSlotIndex);
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerUnequipSlot(EEquipmentSlot Slot);
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerUnequipSlotToBagIndex(EEquipmentSlot Slot, int32 TargetBagSlotIndex);
};
