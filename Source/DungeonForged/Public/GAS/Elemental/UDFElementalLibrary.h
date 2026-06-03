// Source/DungeonForged/Public/GAS/Elemental/UDFElementalLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Elemental/DFElementalData.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UDFElementalLibrary.generated.h"

class UAbilitySystemComponent;
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

	/** Maps `Effect.Element.*` tag back to enum (first match wins). */
	UFUNCTION(BlueprintPure, Category = "DF|Elemental|Tags")
	static EDFElementType GetElementFromEffectTag(FGameplayTag Tag);

	/**
	 * Reads `Effect.Element.*` from spec dynamic asset tags and GE asset tags.
	 * Falls back to ability tags and `Effect.Damage.*` asset tags when no element is stamped.
	 * Used by `UDFDamageCalculation` and pre-apply elemental massaging.
	 */
	UFUNCTION(BlueprintPure, Category = "DF|Elemental|Tags")
	static EDFElementType ResolveElementFromEffectSpec(const FGameplayEffectSpec& Spec);

	/** Maps `Ability.*` tags (and explicit element tags) to an element type. */
	UFUNCTION(BlueprintPure, Category = "DF|Elemental|Tags")
	static EDFElementType InferElementFromGameplayTags(const FGameplayTagContainer& Tags);

	/** Adds `Effect.Element.*` to the spec when missing, using inference or @p ExplicitElement. */
	UFUNCTION(BlueprintCallable, Category = "DF|Elemental|Tags")
	static void StampDefaultElementOnDamageSpec(
		UPARAM(ref) FGameplayEffectSpec& Spec,
		EDFElementType ExplicitElement);

	/** Stamps a default element (if needed) then applies the spec to the target ASC. */
	static FActiveGameplayEffectHandle ApplyOutgoingDamageSpecToTarget(
		UAbilitySystemComponent* SourceASC,
		UAbilitySystemComponent* TargetASC,
		FGameplayEffectSpec& Spec);
};
