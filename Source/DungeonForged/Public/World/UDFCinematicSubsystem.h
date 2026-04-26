// Source/DungeonForged/Public/World/UDFCinematicSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UDFCinematicSubsystem.generated.h"

class ULevelSequence;
class ULevelSequencePlayer;
class ALevelSequenceActor;

/**
 * World-local cinematic playback (e.g. main menu looping camera dolly).
 * @see Prompt 54 — pairs with @c ULevelSequence assets and Lumen-lit maps.
 */
UCLASS()
class DUNGEONFORGED_API UDFCinematicSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Deinitialize() override;

	/** Re-plays the sequence when a non-looping sequence finishes. */
	UFUNCTION(BlueprintCallable, Category = "DF|Cinematic|MainMenu")
	void PlayLooping(ULevelSequence* Sequence);

	UFUNCTION(BlueprintCallable, Category = "DF|Cinematic|MainMenu")
	void Stop();

protected:
	UFUNCTION()
	void OnSequenceFinished();

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequence> ActiveSequence = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> SequencePlayer = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> SequenceActor = nullptr;
};
