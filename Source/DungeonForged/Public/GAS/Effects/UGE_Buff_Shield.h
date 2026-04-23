// Source/DungeonForged/Public/GAS/Effects/UGE_Buff_Shield.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Buff_Shield.generated.h"

/** Invulnerability window. Duration: SetByCaller Data.Duration. */
UCLASS()
class DUNGEONFORGED_API UGE_Buff_Shield : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Buff_Shield();
protected:
	void ConfigureEffectCDO() override;
};
