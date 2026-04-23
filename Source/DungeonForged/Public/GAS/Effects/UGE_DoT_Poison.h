// Source/DungeonForged/Public/GAS/Effects/UGE_DoT_Poison.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_DoT_Poison.generated.h"

/** Poison DoT: 5 flat / tick, 5s, period 1s. AggregateByTarget max 1. SetByCaller Data.Damage available for tuning in variants. */
UCLASS()
class DUNGEONFORGED_API UGE_DoT_Poison : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_DoT_Poison();
protected:
	void ConfigureEffectCDO() override;
};
