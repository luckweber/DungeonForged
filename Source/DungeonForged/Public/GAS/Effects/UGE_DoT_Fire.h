// Source/DungeonForged/Public/GAS/Effects/UGE_DoT_Fire.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_DoT_Fire.generated.h"

/** Fire DoT: HasDuration 3s, Period 1s. Stacking AggregateBySource max 3. */
UCLASS()
class DUNGEONFORGED_API UGE_DoT_Fire : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_DoT_Fire();
protected:
	void ConfigureEffectCDO() override;
};
