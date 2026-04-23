// Source/DungeonForged/Public/GAS/Effects/UDFMMC_SlowMagnitude.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "UDFMMC_SlowMagnitude.generated.h"

/** -0.4 to movement multiplier, or -0.2 if target has Effect.Buff.Speed. */
UCLASS()
class DUNGEONFORGED_API UDFMMC_SlowMagnitude : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
protected:
	float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
