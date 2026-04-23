// Source/DungeonForged/Public/GAS/Effects/UGE_Cooldown_Universal_SecondWind.h
#pragma once

#include "GAS/Effects/UGE_Cooldown_Base.h"
#include "UGE_Cooldown_Universal_SecondWind.generated.h"

/** 120s window via SetByCaller Data.Cooldown; grants Ability.Cooldown.SecondWind. */
UCLASS()
class DUNGEONFORGED_API UGE_Cooldown_Universal_SecondWind : public UGE_Cooldown_Base
{
	GENERATED_BODY()

public:
	UGE_Cooldown_Universal_SecondWind();

protected:
	virtual void ConfigureEffectCDO() override;
};
