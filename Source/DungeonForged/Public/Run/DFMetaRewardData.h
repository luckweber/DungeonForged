// Source/DungeonForged/Public/Run/DFMetaRewardData.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "World/DFWorldTypes.h"
#include "DFMetaRewardData.generated.h"

/**
 * DT_MetaXPRewards — one row per @c ETravelReason (Victory / Defeat / AbandonRun).
 * Tuning without recompile; fallback in @c UDFRunManager if table missing.
 */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFMetaXPRewardRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Meta|XP")
	ETravelReason Outcome = ETravelReason::Victory;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Meta|XP")
	int32 BaseXP = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Meta|XP")
	int32 XPPerFloor = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Meta|XP")
	int32 XPPerKill = 0;
};
