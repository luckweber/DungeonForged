// Source/DungeonForged/Public/AI/UDFAILibrary.h
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UDFAILibrary.generated.h"

/**
 * Shared hostile-player resolution for AI services, BT tasks, and boss abilities.
 * Replaces hard-coded PlayerController index 0 scans.
 */
UCLASS()
class DUNGEONFORGED_API UDFAILibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "DF|AI")
	static bool IsValidHostilePlayerTarget(AActor* Target);

	/** Nearest alive player-controlled pawn within @c MaxRadius. */
	UFUNCTION(BlueprintPure, Category = "DF|AI", meta = (WorldContext = "WorldContextObject"))
	static AActor* FindNearestHostilePlayerTarget(
		const UObject* WorldContextObject,
		const FVector Origin,
		float MaxRadius,
		AActor* PreferredTarget = nullptr,
		bool bRequireLineOfSight = false,
		AActor* LineOfSightFrom = nullptr);

	UFUNCTION(BlueprintPure, Category = "DF|AI", meta = (WorldContext = "WorldContextObject"))
	static ACharacter* FindNearestHostilePlayerCharacter(
		const UObject* WorldContextObject,
		const FVector Origin,
		float MaxRadius,
		AActor* IgnoreActor = nullptr,
		AActor* PreferredTarget = nullptr);

	/** Threat table when @c UDFAIThreatComponent present; else nearest hostile. */
	UFUNCTION(BlueprintPure, Category = "DF|AI", meta = (WorldContext = "WorldContextObject"))
	static AActor* FindBestHostilePlayerTarget(
		const UObject* WorldContextObject,
		AActor* SelfActor,
		const FVector Origin,
		float MaxRadius,
		AActor* PreferredTarget = nullptr,
		bool bRequireLineOfSight = false);
};
