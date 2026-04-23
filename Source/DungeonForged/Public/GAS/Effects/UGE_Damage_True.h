// Source/DungeonForged/Public/GAS/Effects/UGE_Damage_True.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Damage_True.generated.h"

/** True damage (ignores armor/MR). Asset tag: Effect.Damage.True. SetByCaller: Data.Damage. */
UCLASS()
class DUNGEONFORGED_API UGE_Damage_True : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Damage_True();
protected:
	void ConfigureEffectCDO() override;
};
