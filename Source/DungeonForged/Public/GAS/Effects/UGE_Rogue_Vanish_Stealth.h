// Source/DungeonForged/Public/GAS/Effects/UGE_Rogue_Vanish_Stealth.h
#pragma once

#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Rogue_Vanish_Stealth.generated.h"

/** Long vanish: stealth + move speed; remove early from ability (damage, attack, or timer). */
UCLASS()
class DUNGEONFORGED_API UGE_Rogue_Vanish_Stealth : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Rogue_Vanish_Stealth();

protected:
	virtual void ConfigureEffectCDO() override;
};
