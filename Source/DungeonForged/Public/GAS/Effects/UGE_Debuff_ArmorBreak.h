// Source/DungeonForged/Public/GAS/Effects/UGE_Debuff_ArmorBreak.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Debuff_ArmorBreak.generated.h"

/** Armor -= Data.Magnitude. Duration 5s. AggregateBySource max 3. */
UCLASS()
class DUNGEONFORGED_API UGE_Debuff_ArmorBreak : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Debuff_ArmorBreak();
protected:
	void ConfigureEffectCDO() override;
};
