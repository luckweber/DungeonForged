// Source/DungeonForged/Public/GAS/Effects/UGE_Buff_DamageUp.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Buff_DamageUp.generated.h"

/** Strength += SetByCaller Data.Magnitude. Duration: SetByCaller Data.Duration. */
UCLASS()
class DUNGEONFORGED_API UGE_Buff_DamageUp : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Buff_DamageUp();
protected:
	void ConfigureEffectCDO() override;
};
