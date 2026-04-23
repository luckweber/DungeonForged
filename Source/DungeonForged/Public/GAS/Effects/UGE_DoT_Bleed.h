// Source/DungeonForged/Public/GAS/Effects/UGE_DoT_Bleed.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_DoT_Bleed.generated.h"

/** Bleed DoT: -(Data.Damage * 0.2) per tick, period 0.5s, duration 4s. */
UCLASS()
class DUNGEONFORGED_API UGE_DoT_Bleed : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_DoT_Bleed();
protected:
	void ConfigureEffectCDO() override;
};
