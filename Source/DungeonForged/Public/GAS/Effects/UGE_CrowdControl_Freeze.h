// Source/DungeonForged/Public/GAS/Effects/UGE_CrowdControl_Freeze.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_CrowdControl_Freeze.generated.h"

/** Root; duration SetByCaller Data.Duration (default 1.5s frost shatter). */
UCLASS()
class DUNGEONFORGED_API UGE_CrowdControl_Freeze : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_CrowdControl_Freeze();

protected:
	virtual void ConfigureEffectCDO() override;
};
