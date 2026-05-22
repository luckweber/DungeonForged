// Copyright DungeonForged. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AlphaBlend.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DFAnimCombatLibrary.generated.h"

class UAnimInstance;
class UAnimMontage;

/**
 * Animation helpers for combat (montage play with runtime blend override).
 */
UCLASS()
class DUNGEONFORGED_API UDFAnimCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Plays a montage with a custom blend-in time (does not modify the asset). Returns montage length in seconds, or 0 if failed. */
	UFUNCTION(BlueprintCallable, Category = "DF|Combat|Animation", meta = (DisplayName = "Play Montage With Blend In"))
	static float PlayMontageWithBlendIn(
		UAnimInstance* AnimInstance,
		UAnimMontage* Montage,
		float PlayRate = 1.f,
		float BlendInTime = 0.12f,
		bool bStopAllMontages = true,
		EAlphaBlendOption BlendOption = EAlphaBlendOption::HermiteCubic,
		float InTimeToStartMontageAt = 0.f);
};
