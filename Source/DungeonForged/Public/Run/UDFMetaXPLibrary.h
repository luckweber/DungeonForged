// Source/DungeonForged/Public/Run/UDFMetaXPLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "World/DFWorldTypes.h"
#include "UDFMetaXPLibrary.generated.h"

class UDataTable;
struct FDFMetaXPRewardRow;
struct FDFRunSummary;

/** Resolves Meta XP rewards from DevSettings DT or built-in defaults (no hardcoded switch in callers). */
UCLASS()
class DUNGEONFORGED_API UDFMetaXPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "DF|Meta|XP")
	static UDataTable* GetMetaXPRewardsTable();

	UFUNCTION(BlueprintPure, Category = "DF|Meta|XP")
	static bool FindRewardRowForOutcome(ETravelReason Outcome, FDFMetaXPRewardRow& OutRow);

	UFUNCTION(BlueprintPure, Category = "DF|Meta|XP")
	static int32 CalculateMetaXPGain(ETravelReason Outcome, const FDFRunSummary& Summary);
};
