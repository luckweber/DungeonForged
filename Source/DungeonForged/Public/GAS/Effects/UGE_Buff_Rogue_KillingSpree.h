// Source/DungeonForged/Public/GAS/Effects/UGE_Buff_Rogue_KillingSpree.h
#pragma once

#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Buff_Rogue_KillingSpree.generated.h"

/** 5-CP evisc proc: +40% agi and +20% move as attack-speed proxy. */
UCLASS()
class DUNGEONFORGED_API UGE_Buff_Rogue_KillingSpree : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Buff_Rogue_KillingSpree();

protected:
	virtual void ConfigureEffectCDO() override;
};
