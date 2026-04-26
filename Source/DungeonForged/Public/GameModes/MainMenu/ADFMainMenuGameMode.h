// Source/DungeonForged/Public/GameModes/MainMenu/ADFMainMenuGameMode.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameModes/MainMenu/ADFMainMenuHUD.h"
#include "ADFMainMenuGameMode.generated.h"

class ULevelSequence;
class APlayerController;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ADFMainMenuGameMode();

	/**
	 * Exposed in editor as the entry HUD for the main menu map (defaults to C++ @c HUDClass on @c ADFMainMenuHUD).
	 * Also copied to @c AGameModeBase::HUDClass.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|MainMenu|UI")
	TSubclassOf<ADFMainMenuHUD> MainMenuHUDClass = nullptr;

	/** Cinematic dolly; @c UDFCinematicSubsystem re-plays on finish. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|MainMenu|Cine")
	TObjectPtr<ULevelSequence> BackgroundLoopSequence = nullptr;

	/** True if any profile slot (0–2) or legacy @c DFMetaSave exists; set in @c InitGame after @c UDFSaveSlotManagerSubsystem is ready. */
	UPROPERTY(BlueprintReadOnly, Category = "DF|MainMenu|Save")
	bool bPlayerHasProfileSaveData = false;

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

protected:
	/** @deprecated Prefer InitGame; kept for internal / BP hooks. */
	void PlayBackgroundSequence();
	void StopBackgroundSequence();
};
