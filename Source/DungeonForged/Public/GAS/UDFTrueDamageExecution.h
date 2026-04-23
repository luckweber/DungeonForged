// Source/DungeonForged/Public/GAS/UDFTrueDamageExecution.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffectExecutionCalculation.h"
#include "UDFTrueDamageExecution.generated.h"

/** Instant: Health -= SetByCaller(Data.Damage); no armor/MR, no crit. */
UCLASS()
class DUNGEONFORGED_API UDFTrueDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
public:
	UDFTrueDamageExecution();

protected:
	void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
