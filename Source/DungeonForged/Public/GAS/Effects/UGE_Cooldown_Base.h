// Source/DungeonForged/Public/GAS/Effects/UGE_Cooldown_Base.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_Cooldown_Base.generated.h"

/**
 * Cooldown window only: duration = SetByCaller Data.Cooldown.
 * Grant a child GE or UAssetTag / UTarget tag per-ability in editor (e.g. Ability.Cooldown.Fireball).
 */
UCLASS()
class DUNGEONFORGED_API UGE_Cooldown_Base : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Cooldown_Base();

protected:
	virtual void ConfigureEffectCDO() override;
};
