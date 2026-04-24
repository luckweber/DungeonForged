// Source/DungeonForged/Public/GameModes/Nexus/DFNexusLevelData.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "DFNexusLevelData.generated.h"

USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFNexusLevelRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nexus|Level")
	int32 NexusLevel = 1;

	/** Cumulative @c MetaXP required to *reach* this nexus level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nexus|Level")
	int32 MetaXPRequired = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nexus|Level", meta = (MultiLine = "true"))
	FText UnlockDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nexus|Unlocks")
	FName UnlockNPCRow = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nexus|Unlocks")
	FName UnlockClassRow = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nexus|Unlocks")
	TArray<FName> UnlockUpgradeRows;
};
