// Source/DungeonForged/Public/Run/DFRunManager.h
#pragma once

#include "CoreMinimal.h"
#include "Data/DFDataTableStructs.h"
#include "GameModes/Run/DFRunTypes.h"
#include "World/DFWorldTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DFRunManager.generated.h"

class ADFPlayerCharacter;
class ADFPlayerState;
class UDataTable;
class UDFInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDFRunFailed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDFShowDeathScreen);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDFRunCompleted, int32, FinalScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDFRunGoldChanged, int32, NewTotalGold);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDFRunFloorChanged, int32, NewFloor);

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

	/**
	 * Global multiplier for enemy outgoing damage (random events, curses).
	 * Spawners/AI that compute damage can multiply by this; default 1.0.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	float EnemyOutgoingDamageScale = 1.f;

	/** -1 = leave vitals to class defaults; else 0–1 of max after @ref ApplyClassToAttributes. */
	UPROPERTY(BlueprintReadOnly, Category = "Run|Checkpoint")
	float HealthPercent = -1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Run|Checkpoint")
	float ManaPercent = -1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Run|Checkpoint")
	int32 RunCharacterLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Run|Checkpoint")
	int32 RunXP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Run|Checkpoint")
	int32 ComboPoints = 0;

	/** Best-effort history (e.g. recent grants) for debug / UI. */
	UPROPERTY(BlueprintReadOnly, Category = "Run|Checkpoint")
	TArray<FName> AbilityHistory;
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

	/** Fires on server (and for listen host) when in-run gold changes. Clients should prefer ADFPlayerState::OnReplicatedRunGoldChanged. */
	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnDFRunGoldChanged OnGoldChanged;

	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnDFRunFloorChanged OnRunFloorChanged;

	/** Reset run state, resolve the class row from ClassDataTable, init starting abilities from the row, and apply to the current possessable player if any. */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void StartNewRun(FName ClassName);

	/**
	 * Meta: high score, TotalRuns, broadcast fail + optional delayed death screen delegate.
	 * @param bQueueDefaultDeathScreen If false, @c OnShowDeathScreen is not scheduled (e.g. @c ADFRunGameMode handles defeat UI).
	 */
	UFUNCTION(BlueprintCallable, Category = "Run", meta = (AdvancedDisplay = "bQueueDefaultDeathScreen"))
	void OnPlayerDied(bool bQueueDefaultDeathScreen = true);

	/** Finalize score, meta unlocks, save; broadcasts OnRunCompleted. */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void OnRunCompleted();

	/**
	 * Persists end-of-run meta and MetaXP (Victory / Defeat / Abandon) when returning to the Nexus
	 * via @ref UDFWorldTransitionSubsystem. Safe to call once per run end; drives @a OnRunEndedSuccessfully on victory.
	 */
	UFUNCTION(BlueprintCallable, Category = "Run|Meta")
	void ApplyEndOfRunPersistence(ETravelReason Why, FDFRunSummary const& Summary);

	//~ --- Arrival (Nexus / between-floor travel) @see EDFRunTravelReason, ADFRunGameMode ---

	/**
	 * Set before @c UDFWorldTransitionSubsystem::TravelToNexus; consumed by @c ADFNexusGameMode
	 * for spawn and presentation. Cleared after the Nexus GameMode uses it.
	 */
	UFUNCTION(BlueprintCallable, Category = "Run|Nexus")
	void SetNexusArrivalReason(ERunNexusTravelReason InReason);

	/** Default @c ERunNexusTravelReason::FirstLaunch if @c SetNexusArrivalReason was not used for this visit. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Run|Nexus")
	ERunNexusTravelReason GetNexusArrivalReason() const;

	UFUNCTION(BlueprintCallable, Category = "Run|Nexus")
	void ClearNexusArrivalContext();

	/** Call from Nexus (before @c OpenLevel / @c ServerTravel) or when queueing a floor transition. */
	UFUNCTION(BlueprintCallable, Category = "Run|Travel")
	void SetPendingRunArrival(EDFRunTravelReason InReason, FName InClassForNewRun = NAME_None);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Run|Travel")
	EDFRunTravelReason GetArrivalReason() const { return PendingArrivalReason; }

	/**
	 * World transition reason (Nexus, floor, or run end) last queued before travel.
	 * @c ETravelReason::None when not set.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Run|Travel")
	ETravelReason GetTravelArrivalReason() const { return LastTravelReason; }

	/** Only meaningful when @a GetArrivalReason() is @c NewRun. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Run|Travel")
	FName GetPendingClassName() const { return PendingClassForArrival; }

	/** Queues the selected class row for the next @c NewRun (Nexus / class select). */
	UFUNCTION(BlueprintCallable, Category = "Run|Travel")
	void SetPendingClass(FName ClassName) { PendingClassForArrival = ClassName; }

	/**
	 * Queues both legacy @a EDFRunTravelReason and @a ETravelReason for @ref UDFWorldTransitionSubsystem.
	 */
	UFUNCTION(BlueprintCallable, Category = "Run|Travel")
	void SetPendingWorldTravel(ETravelReason WorldReason, FName ClassForNewRun = NAME_None);

	/** Clears arrival fields after the run GameMode has applied them. */
	UFUNCTION(BlueprintCallable, Category = "Run|Travel")
	void ClearRunArrivalContext();

	/**
	 * Syncs in-memory @c RunState from authoritatives (dungeon floor, optional GameState mirrors).
	 * Call before between-floor travel or nexus return if you have modified floor outside the manager.
	 */
	UFUNCTION(BlueprintCallable, Category = "Run|State")
	void CaptureRunState();

	/** Server: applies @c RunState to the pawn (GAS, mesh, inventory) — e.g. after "Next floor" load. */
	UFUNCTION(BlueprintCallable, Category = "Run|State")
	void RestoreRunState(ADFPlayerCharacter* Player);

	/** @c FindClassRow for DT_Classes; C++ (Blueprint cannot return table row pointer). */
	const FDFClassTableRow* FindClassTableRow(FName ClassRowName) const;

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

	/** Adds gold to the in-run total (server). Broadcasts OnGoldChanged and syncs UDFInGameHUD via ADFPlayerState. */
	UFUNCTION(BlueprintCallable, Category = "Run|Gold")
	void AddRunGold(int32 Delta);

	/** Blueprint alias for AddRunGold. */
	UFUNCTION(BlueprintCallable, Category = "Run|Gold", meta = (DisplayName = "Add Gold (Run)"))
	void AddGold(int32 Amount) { AddRunGold(Amount); }

	/** Spends run gold for shops/costs. Fails if insufficient. */
	UFUNCTION(BlueprintCallable, Category = "Run|Gold")
	bool SpendGold(int32 Amount);

	/** Multiplies in-run `EnemyOutgoingDamageScale` (e.g. 1.2f = +20% enemy damage). Server / authority. */
	UFUNCTION(BlueprintCallable, Category = "Run|Events")
	void MulEnemyOutgoingDamageScale(float Mult);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Run|Events")
	float GetEnemyOutgoingDamageScale() const { return RunState.EnemyOutgoingDamageScale; }

	UFUNCTION(BlueprintCallable, Category = "Run")
	void AddRunScore(int32 Delta);

	/** Server-only mirror of in-run gold (authoritative for logic). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Run|Gold")
	int32 GetCurrentGold() const { return RunState.Gold; }

	/** Call after the pawn/ASC is ready (e.g. after level travel) — server / authority only. */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void ApplyRunStateToPlayer(ADFPlayerCharacter* Player);

	/** Rebuild ASC abilities from `RunState.GrantedAbilities` (e.g. after a between-floor pick). */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void GrantAbilitiesForCurrentRun(ADFPlayerState* PlayerState);

	/** Roguelike events: remove one random row from the granted list, then re-grant. No-op if the list is empty. */
	UFUNCTION(BlueprintCallable, Category = "Run|Events")
	void RemoveOneRandomGrantedAbility(ADFPlayerState* PlayerState);

	/**
	 * Pick a row from AbilityDataTable with `Rarity >= MinRarity`, not in `RunState.GrantedAbilities`, grant it, re-grant.
	 * @return false if no candidate was found.
	 */
	UFUNCTION(BlueprintCallable, Category = "Run|Events")
	bool TryGrantRandomAbilityByMinimumRarity(EItemRarity MinRarity, ADFPlayerState* PlayerState);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Run")
	bool IsRunInProgress() const { return bRunInProgress; }

	/** Set by @ref UDFSaveGame::LastCheckpoint for crash recovery / resume. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Run|Checkpoint")
	FDFRunState RestoredRunState;

protected:
	/** Base score from run stats; override in a subclass to tune. */
	virtual int32 CalculateFinalScore() const;

private:
	void ShowDeathScreenCallback();

	/**
	 * @deprecated Prefer @a FindClassTableRow (public). Kept for internal use.
	 */
	const FDFClassTableRow* FindClassRow(FName ClassName) const;
	void ApplyClassToAttributes(class UAbilitySystemComponent* ASC, const FDFClassTableRow& ClassRow) const;
	void RestoreInventoryFromRunState(UDFInventoryComponent* Inv) const;
	void AddUniqueName(TArray<FName>& ToArray, FName Name) const;
	void SyncReplicatedRunGoldToPlayerStates() const;

	UPROPERTY(Transient)
	FDFRunState RunState;

	EDFRunTravelReason PendingArrivalReason = EDFRunTravelReason::None;
	FName PendingClassForArrival = NAME_None;

	ETravelReason LastTravelReason = ETravelReason::None;

	uint8 bNexusArrivalSet : 1 = false;
	ERunNexusTravelReason LastNexusArrivalReason = ERunNexusTravelReason::FirstLaunch;

	uint8 bRunInProgress : 1 = false;
	/** Prevents double MetaXP / TotalRuns if @a ApplyEndOfRunPersistence is invoked twice. */
	uint8 bEndRunPersistenceApplied : 1 = false;

	FTimerHandle DeathScreenTimerHandle;
};
