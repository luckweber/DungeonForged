// Source/DungeonForged/Public/Settings/UDFPerformanceDeveloperSettings.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UDFPerformanceDeveloperSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "DF Performance & Net"))
class DUNGEONFORGED_API UDFPerformanceDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UDFPerformanceDeveloperSettings();

	UPROPERTY(Config, EditAnywhere, Category = "Profiling", meta = (ClampMin = "0.5"))
	float ProfilingIntervalSeconds = 5.f;

	UPROPERTY(Config, EditAnywhere, Category = "Profiling")
	bool bCountNiagaraInProfiling = true;

	UPROPERTY(Config, EditAnywhere, Category = "Room Culling", meta = (ClampMin = "0.1"))
	float RoomCullIntervalSeconds = 0.5f;

	UPROPERTY(Config, EditAnywhere, Category = "Networking")
	int32 MaxCoopPlayers = 2;

	UPROPERTY(Config, EditAnywhere, Category = "Accessibility")
	float DefaultHitStopScale = 1.f;

	UPROPERTY(Config, EditAnywhere, Category = "Accessibility")
	float DefaultCameraShakeScale = 1.f;

	UPROPERTY(Config, EditAnywhere, Category = "Accessibility")
	float DefaultVfxScale = 1.f;
};
