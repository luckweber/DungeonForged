// Source/DungeonForged/Public/ADFDungeonManager.h
// Session-wide dungeon run / floor state. "GameMode subsystem" pattern = UGameInstanceSubsystem (per session singleton).
// Class name: UDFDungeonManager (UObject) — the ADF file prefix is the requested asset-style name, not AActor.
#pragma once

#include "CoreMinimal.h"
#include "Data/DFDataTableStructs.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UI/Minimap/ADFMinimapRoom.h"

class AActor;
class UDataTable;
class UPCGComponent;

#include "ADFDungeonManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFloorCleared);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossSpawned, FName, BossRowName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRunCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerDied, AActor*, Player);
/** Authority: a tracked floor enemy was killed (before last-enemy / floor-cleared). For run stats / @ref ADFRunGameState::IncrementKills. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRunEnemyKilled, AActor*, Enemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDFMinimapRoom, ADFMinimapRoom*, InRoom);

UCLASS(BlueprintType)
class DUNGEONFORGED_API UDFDungeonManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Dungeon")
	int32 CurrentFloor = 0;

	/** Row struct FDFDungeonFloorRow (e.g. DT_Dungeon). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Dungeon")
	TObjectPtr<UDataTable> DungeonFloorTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Dungeon")
	TObjectPtr<UDataTable> EnemyDataTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Dungeon|PCG")
	TObjectPtr<AActor> PCGOwnerActor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Dungeon")
	TArray<TObjectPtr<AActor>> SpawnedEnemies;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Dungeon")
	int32 EnemiesRemaining = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Dungeon")
	bool bFloorCleared = false;

	/** Bumped on each between-floor offer; only one `Server_FinishAbilitySelection` per id advances the run. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Dungeon|Rogue")
	int32 ActiveFloorOfferId = 0;

	/** Set true on server once an offer is resolved (or auto-advanced with no offers). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Dungeon|Rogue")
	bool bFloorOfferResolved = false;

	UFUNCTION(BlueprintCallable, Category = "DF|Dungeon")
	void StartFloor(int32 FloorNumber);

	/** Minimap: register room actors; WBP_Minimap binds to `OnRoomRevealed` / `OnRoomVisited` / `OnPlayerMinimapRoomChanged`. */
	UFUNCTION(BlueprintCallable, Category = "DF|Minimap")
	void RegisterMinimapRoom(ADFMinimapRoom* Room);

	UFUNCTION(BlueprintCallable, Category = "DF|Minimap")
	void UnregisterMinimapRoom(ADFMinimapRoom* Room);

	/** Called from `ADFMinimapRoom::RevealRoom` (server/client local). */
	UFUNCTION(BlueprintCallable, Category = "DF|Minimap")
	void NotifyMinimapRoomRevealed(ADFMinimapRoom* Room);

	/** Called from `ADFMinimapRoom::VisitRoom`. */
	UFUNCTION(BlueprintCallable, Category = "DF|Minimap")
	void NotifyMinimapRoomVisited(ADFMinimapRoom* Room);

	/** Called from `UDFMinimapFogComponent` so the current-room icon can pulse. */
	UFUNCTION(BlueprintCallable, Category = "DF|Minimap")
	void SetPlayerCurrentMinimapRoom(ADFMinimapRoom* Room);

	UFUNCTION(BlueprintPure, Category = "DF|Minimap")
	ADFMinimapRoom* GetPlayerCurrentMinimapRoom() const { return CurrentPlayerMinimapRoom; }

	UFUNCTION(BlueprintCallable, Category = "DF|Dungeon")
	void GenerateDungeon();

	UFUNCTION(BlueprintCallable, Category = "DF|Dungeon")
	void SpawnEnemies(const FDFDungeonFloorRow& FloorData);

	UFUNCTION(BlueprintCallable, Category = "DF|Dungeon")
	void OnEnemyKilled(AActor* Enemy);

	/**
	 * Last enemy down: open exit, loot, broadcast the OnFloorCleared delegate.
	 * (Cannot be named OnFloorCleared — the delegate OnFloorCleared already uses that identifier.)
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|Dungeon")
	void PerformFloorCleared();

	UFUNCTION(BlueprintCallable, Category = "DF|Dungeon")
	void AdvanceToNextFloor();

	UFUNCTION(BlueprintPure, Category = "DF|Dungeon")
	bool GetCurrentFloorData(FDFDungeonFloorRow& OutRow) const;

	UFUNCTION(BlueprintCallable, Category = "DF|Dungeon")
	void NotifyRunFailed(AActor* Player);

	/** Blueprint hook: place modular room BPs / level instances before PCG runs. */
	UFUNCTION(BlueprintNativeEvent, Category = "DF|Dungeon")
	void PlaceRoomTemplates();
	virtual void PlaceRoomTemplates_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "DF|Dungeon")
	void OnFloorCleared_OpenExitAndLoot();
	virtual void OnFloorCleared_OpenExitAndLoot_Implementation();

	/** Override in BP to supply spawn points; default merges SpawnPointsPreview + PCG point outputs. */
	UFUNCTION(BlueprintNativeEvent, Category = "DF|Dungeon")
	void CollectSpawnPoints(TArray<FTransform>& OutSpawnPoints) const;
	virtual void CollectSpawnPoints_Implementation(TArray<FTransform>& OutSpawnPoints) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Dungeon|Spawning")
	TArray<FTransform> SpawnPointsPreview;

	UPROPERTY(BlueprintAssignable, Category = "DF|Dungeon|Events")
	FOnRunEnemyKilled OnRunEnemyKilled;

	UPROPERTY(BlueprintAssignable, Category = "DF|Dungeon|Events")
	FOnFloorCleared OnFloorCleared;

	UPROPERTY(BlueprintAssignable, Category = "DF|Dungeon|Events")
	FOnBossSpawned OnBossSpawned;

	UPROPERTY(BlueprintAssignable, Category = "DF|Dungeon|Events")
	FOnRunCompleted OnRunCompleted;

	/** User naming: "OnRunFailed" in spec — C++ event type FOnPlayerDied, delegate instance OnRunFailed. */
	UPROPERTY(BlueprintAssignable, Category = "DF|Dungeon|Events", meta = (DisplayName = "On Run Failed"))
	FOnPlayerDied OnRunFailed;

	/** Fires when a room is revealed (enter overlap or script). WBP_Minimap: add/update icon. */
	UPROPERTY(BlueprintAssignable, Category = "DF|Minimap|Events")
	FOnDFMinimapRoom OnRoomRevealed;

	/** Fires when a room is marked visited (exit overlap or script). */
	UPROPERTY(BlueprintAssignable, Category = "DF|Minimap|Events")
	FOnDFMinimapRoom OnRoomVisited;

	/** Fires when the local player's current `ADFMinimapRoom` changes (fog overlap). */
	UPROPERTY(BlueprintAssignable, Category = "DF|Minimap|Events")
	FOnDFMinimapRoom OnPlayerMinimapRoomChanged;

	/** All `ADFMinimapRoom` that registered (BeginPlay). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Minimap")
	TArray<TObjectPtr<ADFMinimapRoom>> RegisteredMinimapRooms;

	/** Pawn's current room; driven by `UDFMinimapFogComponent`. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Minimap")
	TObjectPtr<ADFMinimapRoom> CurrentPlayerMinimapRoom = nullptr;

#if !UE_BUILD_SHIPPING
	/** Development: clear tracked enemies and run `PerformFloorCleared` (authority). */
	void Dev_ForceFloorCleared();

	/** Development: `RevealRoom` on every registered minimap room. */
	void Dev_RevealAllMinimapRooms();

	/** Development: spawn `Count` enemies in a ring around `Anchor` using `EnemyDataTable` row. */
	void Dev_SpawnEnemiesAt(FName RowName, int32 Count, AActor* Anchor);

	/** Development: single spawn from row at `Anchor` (e.g. boss). */
	void Dev_SpawnAt(FName RowName, AActor* Anchor);
#endif

protected:
	bool bWaitingForPCG = false;
	FDFDungeonFloorRow PendingFloorRow;
	bool bHasPendingFloorRow = false;
	FDFDungeonFloorRow CachedCurrentFloorRow;
	bool bHasCurrentFloorRow = false;

	TWeakObjectPtr<UPCGComponent> PCGBoundComponent;

	FTimerHandle PCGFallbackTimer;

	UFUNCTION()
	void OnPCGGenerationFinished(UPCGComponent* PCG);

	UFUNCTION()
	void HandleEnemyDied(AActor* Enemy, AActor* Killer, float ExperienceReward);

	void FinishPCGAndSpawn();
	void UnbindPCG();
	void ClearFloorActors();
	void RegisterSpawnedEnemy(AActor* Enemy);
	void UnregisterEnemy(AActor* Enemy);
	bool FindFloorRowByNumber(int32 InFloor, FDFDungeonFloorRow& OutRow) const;
	bool IsAuthorityWorld() const;
	float ComputeSpawnWeight(const FDFEnemyTableRow& EnemyRow, const FDFDungeonFloorRow& FloorRow) const;
	FName PickWeightedRandomEnemyRow(const TArray<FName>& RowNames, const FDFDungeonFloorRow& FloorRow) const;
	UPCGComponent* ResolvePCGComponent() const;
};
