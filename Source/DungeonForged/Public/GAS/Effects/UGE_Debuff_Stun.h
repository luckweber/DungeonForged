// Source/DungeonForged/Public/GAS/Effects/UGE_Debuff_Stun.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Debuff_Stun.generated.h"

/** Stun + cancels active abilities (Ability root tag). Duration: SetByCaller Data.Duration. */
UCLASS()
class DUNGEONFORGED_API UGE_Debuff_Stun : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Debuff_Stun();
protected:
	void ConfigureEffectCDO() override;
};
