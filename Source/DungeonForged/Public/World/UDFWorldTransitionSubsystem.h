// Source/DungeonForged/Public/World/UDFWorldTransitionSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "GameModes/Run/DFRunTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UDFWorldTransitionSubsystem.generated.h"

/**
 * End-of-run and between-floor level transitions. Uses @c ServerTravel / @c OpenLevel with optional
 * query parameters; configure map paths in the instance CDO or project settings.
 */
UCLASS()
class DUNGEONFORGED_API UDFWorldTransitionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** e.g. @c /Game/Maps/Nexus — defaults to UDFGameInstance::MainMenuMapName if empty at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|World")
	FString NexusMapPath;

	/**
	 * Packaged / same-map floor reload: if non-empty, @c TravelToNextFloor will @c ServerTravel here with
	 * @c ?df_run_floor=N ; leave empty to call @c UDFDungeonManager::AdvanceToNextFloor in-place.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|World")
	FString DungeonRunMapPath;

	/** Server / authority: return to the Nexus (meta hub). */
	UFUNCTION(BlueprintCallable, Category = "DF|World")
	void TravelToNexus(ERunNexusTravelReason Reason);

	/**
	 * Server / authority: start a new dungeon run (class already chosen in Nexus).
	 * Sets @c EDFRunTravelReason::NewRun on @c UDFRunManager and @c ServerTravels to @c DungeonRunMapPath.
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|World")
	void TravelToRun(FName SelectedClassRow);

	/**
	 * Server / authority: move to the next floor — either in-place (dungeon manager) or @c ServerTravel
	 * to @c DungeonRunMapPath.
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|World")
	void TravelToNextFloor(int32 NextFloor);
};
