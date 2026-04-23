// Source/DungeonForged/Public/GAS/Effects/UGE_Debuff_Silence.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Debuff_Silence.generated.h"

/** State.Silenced + block Ability.Fire and Ability.Ice. Duration: SetByCaller Data.Duration. */
UCLASS()
class DUNGEONFORGED_API UGE_Debuff_Silence : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Debuff_Silence();
protected:
	void ConfigureEffectCDO() override;
};
