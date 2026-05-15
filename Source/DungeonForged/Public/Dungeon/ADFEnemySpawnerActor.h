// Source/DungeonForged/Public/Dungeon/ADFEnemySpawnerActor.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "ADFEnemySpawnerActor.generated.h"

class UBoxComponent;
class USceneComponent;
class UDFDungeonManager;

/** How random spawn offsets are picked relative to this actor's transform. */
UENUM(BlueprintType)
enum class EDFEnemySpawnerDistribution : uint8
{
	OrientedBox UMETA(DisplayName = "Random inside oriented box (local half-extents)"),
	SphereVolume UMETA(DisplayName = "Random inside sphere (radius)"),
	RingXY UMETA(DisplayName = "Random ring / annulus on XY (uses actor Z)"),
	ActorOrigin UMETA(DisplayName = "Single point at actor location"),
};

/** Weighted row from DT_Enemies (`FDFEnemyTableRow`). */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFWeightedEnemySpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	FName EnemyRowName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (ClampMin = "0.0001"))
	float Weight = 1.f;
};

/**
 * Placeable spawner for dungeon runs: weighted DT_Enemies rows, random count, volume distribution.
 * Registers spawned pawns with @ref UDFDungeonManager so kills / floor clear behave like PCG dungeon spawns.
 * Uses `EnemyDataTable` from `UDFDungeonManager` (filled by `ADFRunGameMode`), optional override table on this actor.
 */
UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFEnemySpawnerActor : public AActor
{
	GENERATED_BODY()

public:
	ADFEnemySpawnerActor();

	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Authority: spawn wave immediately (respects optional run-floor gate). Safe to call from Blueprint triggers. */
	UFUNCTION(BlueprintCallable, Category = "DF|EnemySpawner")
	void TriggerSpawnWave();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Visual aid only — collisions disabled, hidden in game. Extents mirror SpawnHalfExtents. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> EditorBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|EnemySpawner")
	bool bSpawnOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|EnemySpawner", meta = (ClampMin = "0"))
	float SpawnDelaySeconds = 0.f;

	/** When true, spawned enemies call `UDFDungeonManager::RegisterSpawnedEnemy` (recommended for run flow). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|EnemySpawner")
	bool bRegisterWithDungeonManager = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|EnemySpawner|Count", meta = (ClampMin = "0"))
	int32 MinSpawnCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|EnemySpawner|Count", meta = (ClampMin = "0"))
	int32 MaxSpawnCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|EnemySpawner")
	TArray<FDFWeightedEnemySpawnEntry> WeightedEnemyEntries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|EnemySpawner")
	TObjectPtr<UDataTable> EnemyDataTableOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|EnemySpawner|Volume")
	EDFEnemySpawnerDistribution Distribution = EDFEnemySpawnerDistribution::OrientedBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|EnemySpawner|Volume", meta = (ClampMin = "1"))
	FVector SpawnHalfExtents = FVector(500.f, 500.f, 100.f);

	/**
	 * Random XY inside the bound; Z starts at the **top** of the box/rings (`+SpawnHalfExtents.Z`) so `SnapEnemySpawnToWorld`
	 * (nav or line trace) searches downward and finds the real floor. Use when the bound sits on/near the floor — starting
	 * at the bottom face can leave Z at/below the floor and the downward trace misses geometry.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|EnemySpawner|Volume")
	bool bProjectToFloor = false;

	/**
	 * When true (default), random XY only on the bottom face of the volume (local Z = -SpawnHalfExtents.Z),
	 * matching `EditorBounds` — avoids sampling near the top of the box / ceiling. Turn off to use full 3D volume.
	 * Ignored for Z when @ref bProjectToFloor is true (top-of-volume spawn used for downward snap).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|EnemySpawner|Volume", meta = (EditCondition = "!bProjectToFloor"))
	bool bSpawnOnVolumeBottomFace = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|EnemySpawner|Volume", meta = (ClampMin = "1"))
	float SphereRadiusCm = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|EnemySpawner|Volume", meta = (ClampMin = "0"))
	float RingInnerRadiusCm = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|EnemySpawner|Volume", meta = (ClampMin = "1"))
	float RingOuterRadiusCm = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|EnemySpawner")
	bool bRandomYaw = true;

	/**
	 * Usa `ProjectPointToNavigation` (Recast) — o mesmo chão verde do Nav Mesh Bounds Volume.
	 * A caixa pode ficar no ar; o query em Z leva o candidato até o solo para animação tipo birth no chão.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|EnemySpawner|NavMesh")
	bool bSnapSpawnToNavMesh = true;

	/** Extensão vertical (cm) da caixa de procura nav: do ponto candidato até encontrar o polígono navegável. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|EnemySpawner|NavMesh", meta = (ClampMin = "100", EditCondition = "bSnapSpawnToNavMesh"))
	float NavMeshVerticalQueryCm = 6000.f;

	/** When true, wave runs only if active floor matches Required Run Floor (uses DungeonManager CurrentFloor when available). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|EnemySpawner|Run")
	bool bRequireMatchingRunFloor = false;

	/** Must equal the dungeon floor index when starting this map (same as DT_Dungeon floor / boss room number). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|EnemySpawner|Run", meta = (ClampMin = "1", EditCondition = "bRequireMatchingRunFloor"))
	int32 RequiredRunFloor = 1;

protected:
	UFUNCTION()
	void HandleSpawnDelayElapsed();

private:
	int32 ComputeSpawnCount() const;
	FName PickWeightedEnemyRow(UDataTable const* EnemyTable) const;
	FVector ComputeRandomSpawnLocation() const;
	/** XY inside `EditorBounds` OBB (Z ignored — allows snap below volume base). */
	bool IsWorldPointInsideBoundsFootprint(FVector const& WorldPosition) const;

	UPROPERTY()
	FTimerHandle SpawnDelayTimerHandle;
};
