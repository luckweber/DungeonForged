// Source/DungeonForged/Public/Animation/DFDeathAnimation.h
#pragma once

#include "CoreMinimal.h"

class USkeletalMeshComponent;
class UAnimMontage;

/** Shared death montage playback (no auto blend-out, optional stop of other montages). */
namespace DFDeathAnimation
{
DUNGEONFORGED_API float PlayDeathMontage(
	USkeletalMeshComponent* Mesh,
	UAnimMontage* Montage,
	bool bStopOtherMontages = true);

DUNGEONFORGED_API void LockDeathPoseOnMesh(USkeletalMeshComponent* Mesh, UAnimMontage* Montage);

DUNGEONFORGED_API float GetDeathDestroyDelaySeconds(
	UAnimMontage* Montage,
	float MinSeconds = 2.f,
	float PaddingSeconds = 0.75f);
}
