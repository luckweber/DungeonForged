// Source/DungeonForged/Public/GAS/Effects/UGE_EnemyDeath.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UGE_Death.h"
#include "UGE_EnemyDeath.generated.h"

/** Death GE for enemies: State.Dead + replicated GameplayCue.Enemy.Death (montage). */
UCLASS()
class DUNGEONFORGED_API UGE_EnemyDeath : public UGE_Death
{
	GENERATED_BODY()

public:
	UGE_EnemyDeath();

protected:
	virtual void ConfigureEffectCDO() override;
};
