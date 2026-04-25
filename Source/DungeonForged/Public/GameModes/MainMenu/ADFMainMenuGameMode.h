// Source/DungeonForged/Public/GameModes/MainMenu/ADFMainMenuGameMode.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ADFMainMenuGameMode.generated.h"

class ULevelSequence;
class ULevelSequencePlayer;
class ALevelSequenceActor;
class APlayerController;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ADFMainMenuGameMode();

	/** Cinematic dolly; assign in the MainMenu map. Loop in asset or re-play from @a OnMainMenuSequenceFinished. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|MainMenu|Cine")
	TObjectPtr<ULevelSequence> BackgroundLoopSequence = nullptr;

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void StartPlay() override;

protected:
	/** Looping or one-shot: bind @c OnFinished to loop if the asset is not set to loop. */
	void PlayBackgroundSequence();
	UFUNCTION()
	void OnMainMenuSequenceFinished();
	void StopBackgroundSequence();

	TObjectPtr<ULevelSequencePlayer> BgSequencePlayer;
	TObjectPtr<ALevelSequenceActor> BgSequenceActor;
};
