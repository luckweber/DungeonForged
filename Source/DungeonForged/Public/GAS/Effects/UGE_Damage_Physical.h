// Source/DungeonForged/Public/GAS/Effects/UGE_Damage_Physical.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Damage_Physical.generated.h"

/**
 * Instant physical damage. Asset tag: Effect.Damage.Physical.
 * Execution: UDFDamageCalculation. SetByCaller: Data.Damage, Data.Knockback (read from spec in gameplay code).
 */
UCLASS()
class DUNGEONFORGED_API UGE_Damage_Physical : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Damage_Physical();
protected:
	void ConfigureEffectCDO() override;
};
