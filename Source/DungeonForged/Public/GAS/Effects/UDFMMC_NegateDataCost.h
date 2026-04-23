// Source/DungeonForged/Public/GAS/Effects/UDFMMC_NegateDataCost.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "UDFMMC_NegateDataCost.generated.h"

/** -(Data.Cost) for Mana or Stamina cost effects. */
UCLASS()
class DUNGEONFORGED_API UDFMMC_NegateDataCost : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
protected:
	float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
