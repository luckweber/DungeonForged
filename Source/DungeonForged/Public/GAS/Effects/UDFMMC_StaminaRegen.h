// Source/DungeonForged/Public/GAS/Effects/UDFMMC_StaminaRegen.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "UDFMMC_StaminaRegen.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFMMC_StaminaRegen : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UDFMMC_StaminaRegen();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
