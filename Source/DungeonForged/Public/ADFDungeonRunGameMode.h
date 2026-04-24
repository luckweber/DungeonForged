// Source/DungeonForged/Public/ADFDungeonRunGameMode.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ADFDungeonRunGameMode.generated.h"

class UDataTable;

/**
 * Server-side orchestration for a dungeon "run": copies config into UDFDungeonManager and starts a floor.
 * Use a Blueprint child to assign DT assets / spawn transforms; or call StartDungeonRun() manually.
 */
UCLASS(Blueprintable, meta = (DisplayName = "DF Dungeon Run Game Mode"))
class DUNGEONFORGED_API ADFDungeonRunGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADFDungeonRunGameMode();

	/** Pushes table refs + optional PCG / spawn preview into UDFDungeonManager, then calls StartFloor. */
	UFUNCTION(BlueprintCallable, Category = "DF|Dungeon|Run")
	void StartDungeonRun();

	/** If true, StartDungeonRun is called from BeginPlay on authority (PIE: Standalone/Listen). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Dungeon|Run")
	bool bAutoStartOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Dungeon|Run", meta = (ClampMin = "1"))
	int32 StartFloorNumber = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Dungeon|Run")
	TObjectPtr<UDataTable> EnemyDataTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Dungeon|Run")
	TObjectPtr<UDataTable> DungeonFloorTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Dungeon|Run|PCG")
	TObjectPtr<AActor> PCGOwnerActor = nullptr;

	/** Merged with PCG output in UDFDungeonManager::CollectSpawnPoints; use for quick tests without PCG. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Dungeon|Run|Spawning")
	TArray<FTransform> SpawnPointsPreview;

protected:
	virtual void BeginPlay() override;
};
