// Source/DungeonForged/Public/GAS/Effects/UGE_Cooldown_Boss_VoidBarrier.h
#pragma once
#include "GAS/Effects/UGE_Cooldown_Base.h"
#include "UGE_Cooldown_Boss_VoidBarrier.generated.h"

UCLASS()
class DUNGEONFORGED_API UGE_Cooldown_Boss_VoidBarrier : public UGE_Cooldown_Base
{
	GENERATED_BODY()
public:
	UGE_Cooldown_Boss_VoidBarrier();
protected:
	virtual void ConfigureEffectCDO() override;
};
