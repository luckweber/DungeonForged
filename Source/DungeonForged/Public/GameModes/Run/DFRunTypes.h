// Source/DungeonForged/Public/GameModes/Run/DFRunTypes.h
#pragma once

#include "CoreMinimal.h"
#include "DFRunTypes.generated.h"

/** Phases of an active dungeon run, replicated on @ref ADFRunGameState. */
UENUM(BlueprintType)
enum class ERunPhase : uint8
{
	PreRun,
	InCombat,
	BetweenFloors,
	BossEncounter,
	Victory,
	Defeat
};

/**
 * How the run map was entered; set on @ref UDFRunManager before travel, consumed when the player
 * is initialized (see @ref ADFRunGameMode::HandleStartingNewPlayer).
 */
UENUM(BlueprintType)
enum class EDFRunTravelReason : uint8
{
	None = 0,
	/** New run from Nexus: apply class row and run init. */
	NewRun = 1,
	/** Next floor: restore stats/inventory/abilities from @ref UDFRunManager. */
	NextFloor = 2
};

/** Return destination / reason for @ref UDFWorldTransitionSubsystem::TravelToNexus. */
UENUM(BlueprintType)
enum class ERunNexusTravelReason : uint8
{
	Victory = 0,
	Defeat = 1,
	Abandon = 2,
	/** First-time or direct map load (hub entrance spawn). */
	FirstLaunch = 3,
	/** Travel from main menu / editor test (entrance). */
	MenuEntry = 4
};

/** Snapshot for end-of-run UI and meta reporting. */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFRunSummary
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 FloorReached = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 Kills = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 Gold = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	float TimeSeconds = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	FName ClassName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	TArray<FName> AbilitiesCollected;
};
