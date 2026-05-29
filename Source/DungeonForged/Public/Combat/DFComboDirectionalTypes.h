// Source/DungeonForged/Public/Combat/DFComboDirectionalTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Combat/DFDodgeTypes.h"
#include "DFComboDirectionalTypes.generated.h"

class UAnimMontage;

/** Per-step 8-way combo montage overrides (soft refs for data tables). */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFComboDirectionalMontageSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Directional")
	TSoftObjectPtr<UAnimMontage> Forward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Directional")
	TSoftObjectPtr<UAnimMontage> ForwardRight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Directional")
	TSoftObjectPtr<UAnimMontage> Right;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Directional")
	TSoftObjectPtr<UAnimMontage> BackwardRight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Directional")
	TSoftObjectPtr<UAnimMontage> Backward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Directional")
	TSoftObjectPtr<UAnimMontage> BackwardLeft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Directional")
	TSoftObjectPtr<UAnimMontage> Left;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Directional")
	TSoftObjectPtr<UAnimMontage> ForwardLeft;

	bool IsConfigured() const;
	UAnimMontage* ResolveSoft(const EDFDodgeDirection Dir) const;
};

/** Runtime cache populated at equip / async load. */
USTRUCT()
struct FDFComboDirectionalMontageCache
{
	GENERATED_BODY()

	TObjectPtr<UAnimMontage> Forward = nullptr;
	TObjectPtr<UAnimMontage> ForwardRight = nullptr;
	TObjectPtr<UAnimMontage> Right = nullptr;
	TObjectPtr<UAnimMontage> BackwardRight = nullptr;
	TObjectPtr<UAnimMontage> Backward = nullptr;
	TObjectPtr<UAnimMontage> BackwardLeft = nullptr;
	TObjectPtr<UAnimMontage> Left = nullptr;
	TObjectPtr<UAnimMontage> ForwardLeft = nullptr;

	bool IsConfigured() const;
	UAnimMontage* ResolveWithFallback(const EDFDodgeDirection Dir) const;
};

DUNGEONFORGED_API UAnimMontage* DFResolveComboDirectionalMontage(
	const FDFComboDirectionalMontageSet& SoftSet, const EDFDodgeDirection Dir);

DUNGEONFORGED_API void DFBuildComboDirectionalCache(
	const FDFComboDirectionalMontageSet& SoftSet, FDFComboDirectionalMontageCache& OutCache);
