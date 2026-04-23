// Source/DungeonForged/Public/GAS/Effects/UGE_Buff_SmokeCover.h
#pragma once

#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Buff_SmokeCover.generated.h"

/** Applied while the owner is inside a smoke volume; re-stacked from ADFSmokeBombActor. */
UCLASS()
class DUNGEONFORGED_API UGE_Buff_SmokeCover : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Buff_SmokeCover();

protected:
	virtual void ConfigureEffectCDO() override;
};
