// Source/DungeonForged/Public/World/DFWorldTypes.h
#pragma once

#include "CoreMinimal.h"
#include "GameModes/Run/DFRunTypes.h"
#include "DFWorldTypes.generated.h"

/** Why a level change was requested (Nexus, dungeon run, floor, or return). @see UDFWorldTransitionSubsystem */
UENUM(BlueprintType)
enum class ETravelReason : uint8
{
	NewRun = 0,
	NextFloor = 1,
	Victory = 2,
	Defeat = 3,
	AbandonRun = 4,
	FirstLaunch = 5,
	/** No pending world travel (internal). */
	None = 6
};

/** When the last @ref FDFRunState snapshot in @ref UDFSaveGame::LastCheckpoint was taken. */
UENUM(BlueprintType)
enum class ECheckpointType : uint8
{
	RunStart = 0,
	FloorComplete = 1,
	RunEnd = 2
};

namespace DFWorldTransition
{
	/** @c ERunNexusTravelReason values are a subset of travel reasons. */
	FORCEINLINE ETravelReason NexusReasonToTravel(const ERunNexusTravelReason R)
	{
		switch (R)
		{
		case ERunNexusTravelReason::Victory: return ETravelReason::Victory;
		case ERunNexusTravelReason::Defeat: return ETravelReason::Defeat;
		case ERunNexusTravelReason::Abandon: return ETravelReason::AbandonRun;
		case ERunNexusTravelReason::FirstLaunch: return ETravelReason::FirstLaunch;
		case ERunNexusTravelReason::MenuEntry: return ETravelReason::FirstLaunch;
		default: return ETravelReason::FirstLaunch;
		}
	}

	FORCEINLINE ERunNexusTravelReason TravelToNexusArrival(const ETravelReason T)
	{
		switch (T)
		{
		case ETravelReason::Victory: return ERunNexusTravelReason::Victory;
		case ETravelReason::Defeat: return ERunNexusTravelReason::Defeat;
		case ETravelReason::AbandonRun: return ERunNexusTravelReason::Abandon;
		case ETravelReason::FirstLaunch: return ERunNexusTravelReason::FirstLaunch;
		case ETravelReason::NewRun:
		case ETravelReason::NextFloor: return ERunNexusTravelReason::MenuEntry; // not used for Nexus
		default: return ERunNexusTravelReason::FirstLaunch;
		}
	}
} // namespace DFWorldTransition
