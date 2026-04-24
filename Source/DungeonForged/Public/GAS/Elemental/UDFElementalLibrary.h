// Source/DungeonForged/Public/GAS/Elemental/UDFElementalLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Elemental/DFElementalData.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UDFElementalLibrary.generated.h"

class UTexture2D;

/** Rock–paper–scissors style multipliers + helpers (icons, tags). */
UCLASS()
class DUNGEONFORGED_API UDFElementalLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Matrix only: attacker vs defender's *primary* element. */
	UFUNCTION(BlueprintPure, Category = "DF|Elemental")
	static float GetAdvantageMultiplier(EDFElementType AttackElement, EDFElementType DefenderPrimaryElement);

	/** Optional small icon before damage text (Unicode; replace in WBP with images if desired). */
	UFUNCTION(BlueprintPure, Category = "DF|Elemental")
	static FText GetElementGlyph(EDFElementType Element);

	static FLinearColor GetElementColor(EDFElementType Element);

	/** `Effect.Element.*` for adding to a damage `GameplayEffect` spec. */
	UFUNCTION(BlueprintPure, Category = "DF|Elemental|Tags")
	static FGameplayTag GetElementEffectTag(EDFElementType Element);
};
