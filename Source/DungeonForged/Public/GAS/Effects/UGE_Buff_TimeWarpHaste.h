// Source/DungeonForged/Public/GAS/Effects/UGE_Buff_TimeWarpHaste.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Buff_TimeWarpHaste.generated.h"

/** +50% CDR (additive to attribute) and +30% spell amp for 8s. */
UCLASS()
class DUNGEONFORGED_API UGE_Buff_TimeWarpHaste : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Buff_TimeWarpHaste();

protected:
	virtual void ConfigureEffectCDO() override;
};
