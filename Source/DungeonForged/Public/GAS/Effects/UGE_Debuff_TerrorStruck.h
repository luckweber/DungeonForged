// Source/DungeonForged/Public/GAS/Effects/UGE_Debuff_TerrorStruck.h
#pragma once

#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Debuff_TerrorStruck.generated.h"

/**
 * Slow + terrified tag. Duration = SetByCaller Data.Duration.
 * Movement speed multiplier: -0.5 additive.
 */
UCLASS()
class DUNGEONFORGED_API UGE_Debuff_TerrorStruck : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Debuff_TerrorStruck();

protected:
	virtual void ConfigureEffectCDO() override;
};
