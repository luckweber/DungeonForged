// Source/DungeonForged/Public/Progression/DFLevelingData.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "DFLevelingData.generated.h"

/**
 * Cumulative total XP to **reach** this level (and stay there until next threshold).
 * Row.Level should match the level index (1 = first level above start, etc.) — use consistent authoring in DT_Levels.
 */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFLevelTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Leveling", meta = (ClampMin = "1"))
	int32 Level = 1;

	/** Cumulative experience required to reach this level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Leveling", meta = (ClampMin = "0"))
	int32 XPRequired = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Leveling", meta = (ClampMin = "0"))
	int32 AttributePointsGranted = 0;

	/** Row names in DT_Abilities (FDFAbilityTableRow) to grant on reaching this level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Leveling")
	TArray<FName> AbilitiesUnlocked;

	/** Per-level max-vitals multiplier; applied with additive bonus = BaseMax * (StatScalingMultiplier - 1) after previous scaling is removed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Leveling", meta = (ClampMin = "0.01"))
	float StatScalingMultiplier = 1.f;
};
