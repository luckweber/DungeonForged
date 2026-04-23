// Source/DungeonForged/Public/GAS/Effects/UGE_Debuff_FrostSlow.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Debuff_FrostSlow.generated.h"

/** 50% movement slow; duration from SetByCaller Data.Duration. */
UCLASS()
class DUNGEONFORGED_API UGE_Debuff_FrostSlow : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Debuff_FrostSlow();

protected:
	virtual void ConfigureEffectCDO() override;
};
