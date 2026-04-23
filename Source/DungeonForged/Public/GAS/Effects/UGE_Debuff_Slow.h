// Source/DungeonForged/Public/GAS/Effects/UGE_Debuff_Slow.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Debuff_Slow.generated.h"

/** Slow: UDFMMC_SlowMagnitude. Duration: SetByCaller Data.Duration. */
UCLASS()
class DUNGEONFORGED_API UGE_Debuff_Slow : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Debuff_Slow();
protected:
	void ConfigureEffectCDO() override;
};
