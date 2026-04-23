// Source/DungeonForged/Public/GAS/Effects/UDFExec_FortitudeWounded.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "UDFExec_FortitudeWounded.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFExec_FortitudeWounded : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

protected:
	void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
