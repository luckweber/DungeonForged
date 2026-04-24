// Source/DungeonForged/Public/GAS/Effects/UGE_LevelUp_StatScaling.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_LevelUp_StatScaling.generated.h"

/**
 * Infinite-duration level band: replaces prior instance from UDFLevelingComponent.
 * Magnitudes are SetByCaller (Data.LevelUp.*) set in C++ before Apply.
 */
UCLASS()
class DUNGEONFORGED_API UGE_LevelUp_StatScaling : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_LevelUp_StatScaling();

protected:
	virtual void ConfigureEffectCDO() override;
};
