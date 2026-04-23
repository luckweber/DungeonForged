// Source/DungeonForged/Public/GAS/Effects/UGE_HealthRegen.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_HealthRegen.generated.h"

/** +2% MaxHealth / s while not State.InCombat. */
UCLASS()
class DUNGEONFORGED_API UGE_HealthRegen : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_HealthRegen();
protected:
	void ConfigureEffectCDO() override;
};
