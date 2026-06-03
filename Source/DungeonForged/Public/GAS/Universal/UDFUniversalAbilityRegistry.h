// Source/DungeonForged/Public/GAS/Universal/UDFUniversalAbilityRegistry.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UDFUniversalAbilityRegistry.generated.h"

class UDFGameplayAbility;

/** Maps native `Ability.Universal.*` tags to C++ ability classes when DT rows omit AbilityClass. */
UCLASS()
class DUNGEONFORGED_API UDFUniversalAbilityRegistry : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "DF|Universal")
	static TSubclassOf<UDFGameplayAbility> ResolveAbilityClassFromTag(FGameplayTag AbilityTag);
};
