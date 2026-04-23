// Source/DungeonForged/Public/GAS/Effects/UGE_ManaRegen.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_ManaRegen.generated.h"

/** +3% MaxMana / s. */
UCLASS()
class DUNGEONFORGED_API UGE_ManaRegen : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_ManaRegen();
};
