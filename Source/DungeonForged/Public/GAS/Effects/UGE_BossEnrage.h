// Source/DungeonForged/Public/GAS/Effects/UGE_BossEnrage.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/UDFGameplayEffect.h"
#include "UGE_BossEnrage.generated.h"

/**
 * Boss enrage: +Strength / +MovementSpeed, grants State.CCIgnore and State.BossEnraged for animation/UI.
 * Infinite until removed.
 */
UCLASS()
class DUNGEONFORGED_API UGE_BossEnrage : public UDFGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_BossEnrage();
protected:
	void ConfigureEffectCDO() override;
};
