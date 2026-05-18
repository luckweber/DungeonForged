// Source/DungeonForged/Public/Animation/DFDeathAnimation.h
#pragma once

#include "CoreMinimal.h"

class UObject;
class USkeletalMeshComponent;
class UAnimMontage;

/** Shared death montage playback (no auto blend-out, optional stop of other montages). */
namespace DFDeathAnimation
{
/** Console: df.EnemyDeathDebug (0=off, 1=summary, 2=verbose). */
DUNGEONFORGED_API int32 GetEnemyDeathDebugLevel();
DUNGEONFORGED_API void LogEnemyDeath(int32 MinLevel, const UObject* Context, const FString& Message);

/**
 * @param bBypassAnimBlueprint When true, uses AnimationSingleNode so locomotion ABP cannot override the montage (enemies).
 */
DUNGEONFORGED_API float PlayDeathMontage(
	USkeletalMeshComponent* Mesh,
	UAnimMontage* Montage,
	bool bStopOtherMontages = true,
	const UObject* LogContext = nullptr,
	bool bBypassAnimBlueprint = false);

DUNGEONFORGED_API void LockDeathPoseOnMesh(USkeletalMeshComponent* Mesh, UAnimMontage* Montage);

DUNGEONFORGED_API float GetDeathDestroyDelaySeconds(
	UAnimMontage* Montage,
	float MinSeconds = 3.f,
	float PaddingSeconds = 1.5f);
}
