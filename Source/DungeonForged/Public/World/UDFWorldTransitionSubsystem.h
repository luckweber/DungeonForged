// Source/DungeonForged/Public/World/UDFWorldTransitionSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "GameModes/Run/DFRunTypes.h"
#include "World/DFWorldTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UDFWorldTransitionSubsystem.generated.h"

/**
 * All Nexus <-> run travel, between-floor loads, and meta end-of-run persistence.
 *
 * Map world settings (for designers, assign in the map asset, not enforced here):
 * - Map "Nexus": @c GameMode = @c ADFNexusGameMode, @c DefaultPawn = @c ADFNexusPawn (non-combat),
 *   @c PlayerController = @c ADFNexusPlayerController. Sky: eternal golden hour. Ambient: tranquil MetaSound.
 * - Map "DungeonRun": @c GameMode = @c ADFRunGameMode, @c Pawn = @c ADFPlayerCharacter,
 *   @c PC = @c ADFRunPlayerController. NavMesh rebuild after PCG. Underground (no sky). Dungeon MetaSound.
 */
UCLASS()
class DUNGEONFORGED_API UDFWorldTransitionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** @c ETravelReason queued for the active transition (Nexus, floor, or run end). */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|World")
	ETravelReason PendingReason = ETravelReason::None;

	/** For @a NewRun, the chosen class row. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|World")
	FName PendingClass = NAME_None;

	UPROPERTY(BlueprintReadWrite, Category = "DF|World")
	bool bIsTransitioning = false;

	/** Short or long map path (e.g. /Game/Maps/Nexus or Nexus). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|World")
	FString NexusMapName = TEXT("Nexus");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|World")
	FString RunMapName = TEXT("DungeonRun");

	/**
	 * Return to the Nexus. Server / local authority. Runs @a FinalizeRunData for Victory / Defeat / Abandon.
	 * @see DFWorldTypes.h @c ETravelReason
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|World")
	void TravelToNexus(ETravelReason Reason);

	/** Blueprint-friendly wrapper; see TravelToNexus(ETravelReason). */
	UFUNCTION(BlueprintCallable, Category = "DF|World", meta = (DisplayName = "Travel To Nexus (Nexus Enum)"))
	void TravelToNexusFromNexusEnum(ERunNexusTravelReason Reason)
	{
		TravelToNexus(DFWorldTransition::NexusReasonToTravel(Reason));
	}

	/** Start a new dungeon run after class select in the Nexus. */
	UFUNCTION(BlueprintCallable, Category = "DF|World")
	void TravelToRun(FName SelectedClass);

	/**
	 * Same @a RunMapName, run GameMode restarts floor: capture state, save checkpoint, loading screen, @c OpenLevel.
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|World")
	void TravelToNextFloor(int32 NextFloor, int32 MaxFloors = 10);

	/** Called from UDFLoadingScreenSubsystem when the fade and min display time are done. */
	UFUNCTION(BlueprintCallable, Category = "DF|World")
	void NotifyLoadingFinished() { bIsTransitioning = false; }

	UFUNCTION(BlueprintCallable, Category = "DF|World")
	void FinalizeRunData(ETravelReason Reason);

	UFUNCTION(BlueprintCallable, Category = "DF|World")
	void SaveCheckpoint(ECheckpointType Type);

protected:
	void OpenMapByName(const FString& Map);
};
