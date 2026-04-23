// Source/DungeonForged/Public/GAS/Effects/UGE_StaminaRegen.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_StaminaRegen.generated.h"

/** +8% MaxStamina / 0.2s when not sprinting or dodging. */
UCLASS()
class DUNGEONFORGED_API UGE_StaminaRegen : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_StaminaRegen();
protected:
	void ConfigureEffectCDO() override;
};
