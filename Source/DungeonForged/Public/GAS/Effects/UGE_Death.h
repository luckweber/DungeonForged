// Source/DungeonForged/Public/GAS/Effects/UGE_Death.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Death.generated.h"

/** Instantly sets health to 0 and grants State.Dead. */
UCLASS()
class DUNGEONFORGED_API UGE_Death : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Death();
protected:
	void ConfigureEffectCDO() override;
};
