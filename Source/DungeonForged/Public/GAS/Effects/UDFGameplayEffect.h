// Source/DungeonForged/Public/GAS/Effects/UDFGameplayEffect.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "UDFGameplayEffect.generated.h"

/**
 * Project base for native GameplayEffect CDOs.
 * FindOrAddComponent uses NewObject and must not run from a UGameplayEffect constructor (CDO init will assert).
 * Override ConfigureEffectCDO and perform component/tag configuration there; OnGameplayEffectChanged is called after.
 */
UCLASS(Abstract, Blueprintable)
class DUNGEONFORGED_API UDFGameplayEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UDFGameplayEffect() = default;
	virtual void PostInitProperties() override;
protected:
	virtual void ConfigureEffectCDO();
};
