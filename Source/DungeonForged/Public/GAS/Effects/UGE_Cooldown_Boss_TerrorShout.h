// Source/DungeonForged/Public/GAS/Effects/UGE_Cooldown_Boss_TerrorShout.h
#pragma once
#include "GAS/Effects/UGE_Cooldown_Base.h"
#include "UGE_Cooldown_Boss_TerrorShout.generated.h"

UCLASS()
class DUNGEONFORGED_API UGE_Cooldown_Boss_TerrorShout : public UGE_Cooldown_Base
{
	GENERATED_BODY()
public:
	UGE_Cooldown_Boss_TerrorShout();
protected:
	virtual void ConfigureEffectCDO() override;
};
