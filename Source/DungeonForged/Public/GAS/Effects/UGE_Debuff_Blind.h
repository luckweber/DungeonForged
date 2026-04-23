// Source/DungeonForged/Public/GAS/Effects/UGE_Debuff_Blind.h
#pragma once

#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Debuff_Blind.generated.h"

UCLASS()
class DUNGEONFORGED_API UGE_Debuff_Blind : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Debuff_Blind();

protected:
	virtual void ConfigureEffectCDO() override;
};
