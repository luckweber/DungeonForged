// Source/DungeonForged/Public/GAS/Effects/UGE_Rogue_ShadowStep_Stealth.h
#pragma once

#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Rogue_ShadowStep_Stealth.generated.h"

UCLASS()
class DUNGEONFORGED_API UGE_Rogue_ShadowStep_Stealth : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Rogue_ShadowStep_Stealth();

protected:
	virtual void ConfigureEffectCDO() override;
};
