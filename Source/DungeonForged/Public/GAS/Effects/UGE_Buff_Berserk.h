// Source/DungeonForged/Public/GAS/Effects/UGE_Buff_Berserk.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Buff_Berserk.generated.h"

UCLASS()
class DUNGEONFORGED_API UGE_Buff_Berserk : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Buff_Berserk();
protected:
	virtual void ConfigureEffectCDO() override;
};
