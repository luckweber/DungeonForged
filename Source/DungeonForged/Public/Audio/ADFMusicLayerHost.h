// Source/DungeonForged/Public/Audio/ADFMusicLayerHost.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ADFMusicLayerHost.generated.h"

class UAudioComponent;
class USceneComponent;

/**
 * In-world anchor for 3 non-spatialized music layers (no dedicated audio actor in engine).
 * Spawned by UDFMusicManagerSubsystem.
 */
UCLASS(NotPlaceable, Transient, Hidden)
class DUNGEONFORGED_API ADFMusicLayerHost : public AActor
{
	GENERATED_BODY()

public:
	ADFMusicLayerHost();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Audio|Music")
	TObjectPtr<USceneComponent> AudioRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Audio|Music")
	TObjectPtr<UAudioComponent> MusicLayerBase = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Audio|Music")
	TObjectPtr<UAudioComponent> MusicLayerCombat = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Audio|Music")
	TObjectPtr<UAudioComponent> MusicLayerBoss = nullptr;
};
