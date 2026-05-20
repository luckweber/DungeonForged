// Source/DungeonForged/Public/GAS/UDFGEComponent_StatusResistDuration.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "UDFGEComponent_StatusResistDuration.generated.h"

/** Scales CC duration on apply using the target's StatusResist attribute. Attach to stun/freeze/slow GEs. */
UCLASS(DisplayName = "DF Status Resist Duration")
class DUNGEONFORGED_API UDFGEComponent_StatusResistDuration : public UGameplayEffectComponent
{
	GENERATED_BODY()

public:
	virtual void OnGameplayEffectApplied(
		FActiveGameplayEffectsContainer& ActiveGEContainer,
		FGameplayEffectSpec& GESpec,
		FPredictionKey& PredictionKey) const override;
};
