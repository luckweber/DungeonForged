// Source/DungeonForged/Public/GAS/DFDamageCalculation.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffectExecutionCalculation.h"
#include "DFDamageCalculation.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFDamageCalculation : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UDFDamageCalculation();

protected:
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

	FGameplayEffectAttributeCaptureDefinition IntelligenceCapture;
	FGameplayEffectAttributeCaptureDefinition StrengthCapture;
	FGameplayEffectAttributeCaptureDefinition MagicResistCapture;
	FGameplayEffectAttributeCaptureDefinition ArmorCapture;
	FGameplayEffectAttributeCaptureDefinition CritChanceCapture;
	FGameplayEffectAttributeCaptureDefinition CritMultCapture;
};
