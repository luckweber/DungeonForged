// Source/DungeonForged/Public/GAS/Effects/UDFMMC_FireDoT.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "UDFMMC_FireDoT.generated.h"

/** DoT: -(Strength * 0.1) on health (additive). */
UCLASS()
class DUNGEONFORGED_API UDFMMC_FireDoT : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
public:
	UDFMMC_FireDoT();

protected:
	float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

	FGameplayEffectAttributeCaptureDefinition StrengthCapture;
};
