// Source/DungeonForged/Public/Combat/UDFCombatSpectacleSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UDFCombatSpectacleSubsystem.generated.h"

class AActor;

/** Room-clear / last-kill juice (slow-mo, screen FX) for the local player. */
UCLASS()
class DUNGEONFORGED_API UDFCombatSpectacleSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DF|Combat|Spectacle")
	void PlayLastKillSpectacle(AActor* KilledEnemy, AActor* Killer = nullptr);

	UFUNCTION(BlueprintCallable, Category = "DF|Combat|Spectacle")
	void PlayRoomClearSpectacle();

	UPROPERTY(EditAnywhere, Category = "DF|Combat|Spectacle")
	float KillSlowMoDuration = 0.35f;

	UPROPERTY(EditAnywhere, Category = "DF|Combat|Spectacle")
	float KillSlowMoDilation = 0.25f;

	UPROPERTY(EditAnywhere, Category = "DF|Combat|Spectacle")
	float RoomClearSlowMoDuration = 0.5f;

	UPROPERTY(EditAnywhere, Category = "DF|Combat|Spectacle")
	float RoomClearSlowMoDilation = 0.15f;
};
