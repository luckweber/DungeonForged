// Source/DungeonForged/Public/UI/Status/UDFStatusLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UI/Status/DFStatusEffectData.h"
#include "UDFStatusLibrary.generated.h"

/**
 * Maps GameplayTags to presentation data. Optional — GetStatusData falls back to native defaults
 * when a tag is missing from the asset.
 */
UCLASS(BlueprintType)
class DUNGEONFORGED_API UDFStatusLibrary : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|UI|Status")
	TMap<FGameplayTag, FDFStatusEffectDisplayData> StatusByTag;

	/** Editor / runtime: resolves the data asset first, then built-in fallbacks. (Not UFunction — USTRUCT cannot be exposed as pointer in BP.) */
	static const FDFStatusEffectDisplayData* GetStatusData(
		FGameplayTag Tag,
		const UDFStatusLibrary* OptionalLibrary = nullptr);

	/** C++ / Blueprint: by-value for designers; see also GetStatusData. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DF|UI|Status")
	FDFStatusEffectDisplayData GetStatusDataValue(
		FGameplayTag Tag) const;

	/** Union of this asset’s keys and project built-in status tags (for GAS effect queries / HUD refresh). */
	UFUNCTION(BlueprintCallable, Category = "DF|UI|Status")
	static void CollectAllStatusRootTags(const UDFStatusLibrary* OptionalLibrary, FGameplayTagContainer& OutTags);
};
