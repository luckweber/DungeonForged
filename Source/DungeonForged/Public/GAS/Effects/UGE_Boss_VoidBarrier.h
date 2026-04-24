// Source/DungeonForged/Public/GAS/Effects/UGE_Boss_VoidBarrier.h
#pragma once

#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Boss_VoidBarrier.generated.h"

/** 8s invuln + huge armor. Duration = SetByCaller Data.Duration. */
UCLASS()
class DUNGEONFORGED_API UGE_Boss_VoidBarrier : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Boss_VoidBarrier();

protected:
	virtual void ConfigureEffectCDO() override;
};
