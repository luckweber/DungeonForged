// Source/DungeonForged/Public/GameModes/Nexus/DFNexusUnlockData.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "DFNexusUnlockData.generated.h"

USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFNexusUnlockConditionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nexus|Condition")
	int32 MinRunsCompleted = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nexus|Condition")
	int32 MinWinsCompleted = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nexus|Condition")
	int32 MinMetaLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nexus|Condition", meta = (MultiLine = "true"))
	FText LockHint;
};
