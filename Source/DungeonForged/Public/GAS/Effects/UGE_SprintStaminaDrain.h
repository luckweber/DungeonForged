// Source/DungeonForged/Public/GAS/Effects/UGE_SprintStaminaDrain.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_SprintStaminaDrain.generated.h"

/** Infinite, period 0.1s. Stamina -= SprintStaminaDrain * 0.1. State.Sprinting. */
UCLASS()
class DUNGEONFORGED_API UGE_SprintStaminaDrain : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_SprintStaminaDrain();
protected:
	void ConfigureEffectCDO() override;
};
