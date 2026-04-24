// Source/DungeonForged/Public/GAS/Effects/UGE_Cooldown_Boss_EnragePulse.h
#pragma once
#include "GAS/Effects/UGE_Cooldown_Base.h"
#include "UGE_Cooldown_Boss_EnragePulse.generated.h"

UCLASS()
class DUNGEONFORGED_API UGE_Cooldown_Boss_EnragePulse : public UGE_Cooldown_Base
{
	GENERATED_BODY()
public:
	UGE_Cooldown_Boss_EnragePulse();
protected:
	virtual void ConfigureEffectCDO() override;
};
