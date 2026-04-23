// Source/DungeonForged/Public/ADFDungeonManager.h
// Session-wide dungeon run / floor state. "GameMode subsystem" pattern = UGameInstanceSubsystem (per session singleton).
// Class name: UDFDungeonManager (UObject) — the ADF file prefix is the requested asset-style name, not AActor.
#pragma once

#include "CoreMinimal.h"
#include "Data/DFDataTableStructs.h"
#include "Subsystems/GameInstanceSubsystem.h"

class AActor;
class UDataTable;
class UPCGComponent;

#include "ADFDungeonManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFloorCleared);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossSpawned, FName, BossRowName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRunCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerDied, AActor*, Player);

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

	UFUNCTION(BlueprintCallable, Category = "DF|Dungeon")
	void StartFloor(int32 FloorNumber);

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
	FOnFloorCleared OnFloorCleared;

	UPROPERTY(BlueprintAssignable, Category = "DF|Dungeon|Events")
	FOnBossSpawned OnBossSpawned;

	UPROPERTY(BlueprintAssignable, Category = "DF|Dungeon|Events")
	FOnRunCompleted OnRunCompleted;

	/** User naming: "OnRunFailed" in spec — C++ event type FOnPlayerDied, delegate instance OnRunFailed. */
	UPROPERTY(BlueprintAssignable, Category = "DF|Dungeon|Events", meta = (DisplayName = "On Run Failed"))
	FOnPlayerDied OnRunFailed;

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
