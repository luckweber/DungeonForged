// Source/DungeonForged/Public/GAS/Effects/UGE_DoT_Frost.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_DoT_Frost.generated.h"

/** Stackable frost DoT marker (and light periodic damage); used by Blizzard. */
UCLASS()
class DUNGEONFORGED_API UGE_DoT_Frost : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_DoT_Frost();

protected:
	virtual void ConfigureEffectCDO() override;
};
