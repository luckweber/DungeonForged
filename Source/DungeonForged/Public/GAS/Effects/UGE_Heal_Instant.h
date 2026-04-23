// Source/DungeonForged/Public/GAS/Effects/UGE_Heal_Instant.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Heal_Instant.generated.h"

/** Health += SetByCaller Data.Healing. */
UCLASS()
class DUNGEONFORGED_API UGE_Heal_Instant : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Heal_Instant();
};
