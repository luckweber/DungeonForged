// Source/DungeonForged/Public/Run/UDFSaveSlotManagerSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UDFSaveSlotManagerSubsystem.generated.h"

class UDFSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDFSaveSlotChanged, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDFSaveSlotLoadFailed, int32, SlotIndex, FString, Reason);

/**
 * Up to 3 profile saves. Primary slot files use @c UDFSaveGame::GetProfileSlotFName.
 * @see UDFSaveGame, Main Menu, Nexus meta flow.
 */
UCLASS()
class DUNGEONFORGED_API UDFSaveSlotManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	static constexpr int32 MaxSlots = 3;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** @c LoadAllSlotHeaders + one-time copy of @c DFMetaSave to profile 0. */
	UFUNCTION(BlueprintCallable, Category = "DF|Save|Slots")
	void InitializeOrMigrateSlots();

	UFUNCTION(BlueprintCallable, Category = "DF|Save|Slots")
	void LoadAllSlotHeaders();

	UFUNCTION(BlueprintPure, Category = "DF|Save|Slots")
	UDFSaveGame* GetSlotData(int32 SlotIndex) const;

	/** Active profile used by runtime (Nexus, world transition). -1 = none. */
	UFUNCTION(BlueprintPure, Category = "DF|Save|Slots")
	int32 GetActiveSlotIndex() const { return ActiveSlotIndex; }

	UFUNCTION(BlueprintPure, Category = "DF|Save|Slots")
	UDFSaveGame* GetActiveSave() const;

	UFUNCTION(BlueprintCallable, Category = "DF|Save|Slots")
	void SelectSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "DF|Save|Slots")
	bool SaveActiveSlot();

	UFUNCTION(BlueprintCallable, Category = "DF|Save|Slots")
	void DeleteSlot(int32 SlotIndex);

	void BroadcastSlotChanged(int32 const SlotIndex);

	UFUNCTION(BlueprintPure, Category = "DF|Save|Slots")
	bool IsSlotEmpty(int32 SlotIndex) const;

	/** True if any profile slot is occupied or the legacy @c UDFSaveGame::GetSlotName exists. */
	UFUNCTION(BlueprintPure, Category = "DF|Save|Slots")
	bool HasAnyProfileOrLegacySave() const;

	/** Active save if selected; else falls back to legacy @c UDFSaveGame::Load. */
	UFUNCTION(BlueprintCallable, Category = "DF|Save|Slots")
	UDFSaveGame* GetActiveOrLegacyMetaSave();

	/** Any slot data changed (load, select, save, delete). */
	UPROPERTY(BlueprintAssignable, Category = "DF|Save|Slots")
	FOnDFSaveSlotChanged OnSlotChanged;

	/** @c OnSlotDeleted kept for prior listeners. */
	UPROPERTY(BlueprintAssignable, Category = "DF|Save|Slots")
	FOnDFSaveSlotChanged OnSlotDeleted;

	/** Deserialization failed — slot treated as empty. */
	UPROPERTY(BlueprintAssignable, Category = "DF|Save|Slots")
	FOnDFSaveSlotLoadFailed OnSlotLoadFailed;

protected:
	/** 0, 1, 2: cached headers (full object). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UDFSaveGame>> LoadedSlots;

	/** -1 = no profile selected. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|Save|Slots")
	int32 ActiveSlotIndex = -1;
};
