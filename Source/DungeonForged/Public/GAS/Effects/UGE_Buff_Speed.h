// Source/DungeonForged/Public/GAS/Effects/UGE_Buff_Speed.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Buff_Speed.generated.h"

/** +0.3 MovementSpeedMultiplier. Duration: SetByCaller Data.Duration. Stacking: replace (AggregateByTarget 1). */
UCLASS()
class DUNGEONFORGED_API UGE_Buff_Speed : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Buff_Speed();
protected:
	void ConfigureEffectCDO() override;
};
