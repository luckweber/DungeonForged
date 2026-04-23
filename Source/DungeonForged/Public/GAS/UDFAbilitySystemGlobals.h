// Source/DungeonForged/Public/GAS/UDFAbilitySystemGlobals.h
// Aura course pattern: custom UAbilitySystemGlobals + DefaultGame AbilitySystemGlobalsClassName
// https://github.com/DruidMech/GameplayAbilitySystem_Aura
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemGlobals.h"
#include "UDFAbilitySystemGlobals.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()

public:
	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;
};
