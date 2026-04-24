// Source/DungeonForged/Public/GameModes/Nexus/ADFNexusGameMode.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameModes/Run/DFRunTypes.h"
#include "ADFNexusGameMode.generated.h"

class ADFNexusHUD;
class ADFNexusPlayerController;
class ADFNexusGameState;
class UDFSaveGame;
class APlayerStart;

UCLASS()
class DUNGEONFORGED_API ADFNexusGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADFNexusGameMode();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Classes")
	TSubclassOf<APawn> NexusPawnClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nexus|Spawn")
	FName DefaultEntranceStartTag = FName(TEXT("Nexus.Entrance"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nexus|Spawn")
	FName CenterPlazaStartTag = FName(TEXT("Nexus.Plaza"));

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;

	/** Applies @a PendingUnlocks, updates NPC actors, saves, queues HUD. */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Meta")
	void ProcessPendingUnlocks(UDFSaveGame* Save, APlayerController* ForNotifications);

	/** Called after @c UDFSaveGame is updated in-hub (e.g. meta level-up) to flush unlock queue. */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Meta")
	void ProcessPendingUnlocksFromSave(UDFSaveGame* Save, APlayerController* ForNotifications);

	UFUNCTION(BlueprintNativeEvent, Category = "Nexus|Presentation")
	void PlayNexusArrivalPresentation(ERunNexusTravelReason Reason, APlayerController* LocalPC);

protected:
	/** Chosen in @c PostLogin before @c Super::PostLogin, used by @c FindPlayerStart. */
	UPROPERTY(Transient)
	FName ActivePlayerStartTag;

	FName SelectSpawnTagForArrival(ERunNexusTravelReason Reason) const;
};
