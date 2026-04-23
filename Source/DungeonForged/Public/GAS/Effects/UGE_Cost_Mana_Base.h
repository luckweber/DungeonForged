// Source/DungeonForged/Public/GAS/Effects/UGE_Cost_Mana_Base.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Cost_Mana_Base.generated.h"

/** Mana -= SetByCaller Data.Cost. */
UCLASS()
class DUNGEONFORGED_API UGE_Cost_Mana_Base : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Cost_Mana_Base();
};
