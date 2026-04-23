// Source/DungeonForged/Public/Run/DFRunManager.h
#pragma once

#include "CoreMinimal.h"
#include "Data/DFDataTableStructs.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DFRunManager.generated.h"

class ADFPlayerCharacter;
class ADFPlayerState;
class UDataTable;
class UDFInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDFRunFailed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDFShowDeathScreen);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDFRunCompleted, int32, FinalScore);

/**
 * In-memory roguelike run; not serialized to disk (meta is in UDFSaveGame).
 */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFRunState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 CurrentFloor = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	FName SelectedClass = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	TArray<FName> EquippedItems;

	/** Starting + floor rewards; row names in DT_Abilities. */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	TArray<FName> GrantedAbilities;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 Gold = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 Score = 0;

	/** FPlatformTime::Seconds() when the run started (stable across level travel). */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	float RunStartTime = 0.f;
};

/** GameInstance subsystem: run state, DT_Classes / DT_Abilities, meta save @see UDFSaveGame. */
UCLASS()
class DUNGEONFORGED_API UDFRunManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	/** DataTable asset (e.g. DT_Classes) using FDFClassTableRow. */
	UPROPERTY(EditDefaultsOnly, Category = "Run|Data")
	TObjectPtr<UDataTable> ClassDataTable = nullptr;

	/** DataTable (e.g. DT_Abilities) using FDFAbilityTableRow; used to grant ability rows. */
	UPROPERTY(EditDefaultsOnly, Category = "Run|Data")
	TObjectPtr<UDataTable> AbilityDataTable = nullptr;

	/** Optional: if set, inventory restore uses this table (see UDFInventoryComponent::ItemDataTable). */
	UPROPERTY(EditDefaultsOnly, Category = "Run|Data")
	TObjectPtr<UDataTable> ItemDataTable = nullptr;

	/** Seconds before OnShowDeathScreen fires after a failed run. */
	UPROPERTY(EditDefaultsOnly, Category = "Run|Presentation", meta = (ClampMin = "0.0"))
	float DeathScreenDelaySeconds = 2.f;

	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnDFRunFailed OnRunFailed;

	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnDFShowDeathScreen OnShowDeathScreen;

	/** Fired with final score when the run ends in success (avoids clashing with UFUNCTION name OnRunCompleted). */
	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnDFRunCompleted OnRunEndedSuccessfully;

	/** Reset run state, resolve the class row from ClassDataTable, init starting abilities from the row, and apply to the current possessable player if any. */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void StartNewRun(FName ClassName);

	/** Meta: high score, TotalRuns, broadcast fail + delayed death screen. */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void OnPlayerDied();

	/** Finalize score, meta unlocks, save; broadcasts OnRunCompleted. */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void OnRunCompleted();

	/** C++: reference to the live in-memory run; valid until the run ends or StartNewRun. */
	const FDFRunState& GetCurrentRunState() const { return RunState; }

	/** Blueprint/UMG: same data as a copy (USTRUCT not exposed by ref in BP). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Run")
	FDFRunState GetRunStateCopy() const { return RunState; }

	/** After the player picks one of the random offers, add that ability row for the run. */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void AddAbilityReward(FName AbilityRow);

	/** Build up to InCount row names from AbilityDataTable, excluding already-granted; shuffled (e.g. pick 1 of 3 between floors). */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void GetRandomAbilityOfferCandidates(int32 InCount, TArray<FName>& OutRowNames) const;

	/** Increase CurrentFloor (e.g. after floor clear or portal). */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void AdvanceFloor(int32 FloorDelta = 1);

	UFUNCTION(BlueprintCallable, Category = "Run")
	void AddRunGold(int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "Run")
	void AddRunScore(int32 Delta);

	/** Call after the pawn/ASC is ready (e.g. after level travel) — server / authority only. */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void ApplyRunStateToPlayer(ADFPlayerCharacter* Player);

	/** Rebuild ASC abilities from `RunState.GrantedAbilities` (e.g. after a between-floor pick). */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void GrantAbilitiesForCurrentRun(ADFPlayerState* PlayerState);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Run")
	bool IsRunInProgress() const { return bRunInProgress; }

protected:
	/** Base score from run stats; override in a subclass to tune. */
	virtual int32 CalculateFinalScore() const;

private:
	void ShowDeathScreenCallback();
	const FDFClassTableRow* FindClassRow(FName ClassName) const;
	void ApplyClassToAttributes(class UAbilitySystemComponent* ASC, const FDFClassTableRow& ClassRow) const;
	void RestoreInventoryFromRunState(UDFInventoryComponent* Inv) const;
	void AddUniqueName(TArray<FName>& ToArray, FName Name) const;

	UPROPERTY(Transient)
	FDFRunState RunState;

	uint8 bRunInProgress : 1 = false;

	FTimerHandle DeathScreenTimerHandle;
};
