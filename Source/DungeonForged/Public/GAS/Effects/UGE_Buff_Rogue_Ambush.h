// Source/DungeonForged/Public/GAS/Effects/UGE_Buff_Rogue_Ambush.h
#pragma once

#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Buff_Rogue_Ambush.generated.h"

/** Next attack out of vanish: 2x damage + combo; removed after first hit in abilities. */
UCLASS()
class DUNGEONFORGED_API UGE_Buff_Rogue_Ambush : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Buff_Rogue_Ambush();

protected:
	virtual void ConfigureEffectCDO() override;
};
