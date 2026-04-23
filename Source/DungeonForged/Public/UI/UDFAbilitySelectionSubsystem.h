// Source/DungeonForged/Public/UI/UDFAbilitySelectionSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Data/DFDataTableStructs.h"
#include "Subsystems/WorldSubsystem.h"
#include "UDFAbilitySelectionSubsystem.generated.h"

class ADFPlayerCharacter;
class UDataTable;
class UDFAbilitySelectionWidget;

USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFAbilityRolledChoice
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "DF|Rogue")
	FName RowName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "DF|Rogue")
	FDFAbilityTableRow Data;
};

/**
 * Per-world ability-offer state for the roguelike between-floor 1-of-3 screen.
 * Uses UDFRunManager (GameInstance) for DT_Abilities and run-granted list.
 */
UCLASS()
class DUNGEONFORGED_API UDFAbilitySelectionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** If unset, UDFRunManager::AbilityDataTable is used. */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Rogue")
	TObjectPtr<UDataTable> AbilityTable = nullptr;

	/** Shown in skip button text / SkipSelection. */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Rogue", meta = (ClampMin = "0"))
	int32 SkipGoldReward = 50;

	/**
	 * Cache of FNames already granted on this run; kept in sync with UDFRunManager
	 * when Grant / Skip is applied from this path.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DF|Rogue")
	TArray<FName> PlayerAbilityHistory;

	/** WBP; also assignable on UDFAbilitySelectionSubsystem CDO. */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Rogue")
	TSubclassOf<UDFAbilitySelectionWidget> SelectionWidgetClass;

	/**
	 * While the between-floor UMG is on screen (client), we keep a weak ref so
	 * `Client_Resume` can close all copies when the first player locks in.
	 */
	void RegisterActiveSelectionWidget(UDFAbilitySelectionWidget* Widget);
	void UnregisterActiveSelectionWidget(UDFAbilitySelectionWidget* Widget);
	UFUNCTION(BlueprintCallable, Category = "DF|Rogue")
	void CloseActiveSelectionWidget();

	UFUNCTION(BlueprintCallable, Category = "DF|Rogue")
	TArray<FDFAbilityRolledChoice> RollAbilityChoices(int32 Count = 3);

	/** Authority: add to run, re-grant all abilities in slot order, notify run manager. */
	UFUNCTION(BlueprintCallable, Category = "DF|Rogue")
	void GrantSelectedAbility(FName AbilityRowName, ADFPlayerCharacter* Player);

	/** Authority: add skip gold, no new ability. */
	UFUNCTION(BlueprintCallable, Category = "DF|Rogue")
	void SkipSelection(ADFPlayerCharacter* Player);

protected:
	void SyncHistoryFromRun();
	UDataTable* ResolveAbilityTable() const;

	/** All full-screen offer UIs on this world (co-op: one per local player in PIE, etc.). */
	TArray<TWeakObjectPtr<UDFAbilitySelectionWidget>> ActiveSelectionWidgets;
};
