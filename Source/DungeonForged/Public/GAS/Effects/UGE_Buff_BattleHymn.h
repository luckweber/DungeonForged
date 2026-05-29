// Source/DungeonForged/Public/GAS/Effects/UGE_Buff_BattleHymn.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Buff_BattleHymn.generated.h"

UCLASS()
class DUNGEONFORGED_API UGE_Buff_BattleHymn : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Buff_BattleHymn();
protected:
	virtual void ConfigureEffectCDO() override;
};
