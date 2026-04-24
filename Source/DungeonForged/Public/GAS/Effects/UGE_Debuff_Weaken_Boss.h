// Source/DungeonForged/Public/GAS/Effects/UGE_Debuff_Weaken_Boss.h
#pragma once

#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Debuff_Weaken_Boss.generated.h"

/**
 * Boss debuff: -30 Armor, -20 MagicResist, 10s. Grants Effect.Debuff.Weaken.
 */
UCLASS()
class DUNGEONFORGED_API UGE_Debuff_Weaken_Boss : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Debuff_Weaken_Boss();

protected:
	virtual void ConfigureEffectCDO() override;
};
