// Source/DungeonForged/Public/Run/DFBetweenFloorTypes.h
#pragma once

#include "CoreMinimal.h"
#include "DFBetweenFloorTypes.generated.h"

/** Steps in the between-floor roguelike sequence (server-authoritative). */
UENUM(BlueprintType)
enum class EBetweenFloorStep : uint8
{
	None = 0,
	/** Optional DT_RandomEvents roll + WBP_RandomEvent. */
	Event = 1,
	/** Short rest: partial heal before the next combat floor. */
	Rest = 2,
	/** In-run merchant (every N floors); skipped if no merchant actor. */
	Shop = 3,
	/** 1-of-3 ability draft (UDFAbilitySelectionSubsystem). */
	Draft = 4,
	/** Regenerate the next floor in-place and resume combat. */
	AdvanceFloor = 5
};
