// Source/DungeonForged/Public/GAS/Effects/UDFMMC_BleedDoT.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "UDFMMC_BleedDoT.generated.h"

/** -(Data.Damage * 0.2) per period. */
UCLASS()
class DUNGEONFORGED_API UDFMMC_BleedDoT : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
protected:
	float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
