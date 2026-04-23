// Source/DungeonForged/Public/GAS/Effects/UDFMMC_NegateDataMagnitude.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "UDFMMC_NegateDataMagnitude.generated.h"

/** -(Data.Magnitude) (e.g. armor reduction amount). */
UCLASS()
class DUNGEONFORGED_API UDFMMC_NegateDataMagnitude : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
protected:
	float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
