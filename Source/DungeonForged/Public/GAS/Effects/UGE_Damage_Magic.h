// Source/DungeonForged/Public/GAS/Effects/UGE_Damage_Magic.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Damage_Magic.generated.h"

/** Instant magic damage. Asset tag: Effect.Damage.Magic. Execution: UDFDamageCalculation. SetByCaller: Data.Damage. */
UCLASS()
class DUNGEONFORGED_API UGE_Damage_Magic : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Damage_Magic();
protected:
	void ConfigureEffectCDO() override;
};
