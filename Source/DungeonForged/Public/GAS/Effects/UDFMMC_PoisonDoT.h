// Source/DungeonForged/Public/GAS/Effects/UDFMMC_PoisonDoT.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "UDFMMC_PoisonDoT.generated.h"

/** -max(5, Data.Damage) if Data.Damage set, else -5. */
UCLASS()
class DUNGEONFORGED_API UDFMMC_PoisonDoT : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
protected:
	float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
