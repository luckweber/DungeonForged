// Source/DungeonForged/Public/Combat/UDFCombatStateLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UDFCombatStateLibrary.generated.h"

/** Applies @c State.InCombat and schedules exit after idle time. */
UCLASS()
class DUNGEONFORGED_API UDFCombatStateLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DF|Combat|State", meta = (WorldContext = "WorldContextObject"))
	static void NotifyCombatActivity(UObject* WorldContextObject, AActor* CombatActor);

	UFUNCTION(BlueprintPure, Category = "DF|Combat|State", meta = (WorldContext = "WorldContextObject"))
	static bool IsActorInCombat(UObject* WorldContextObject, AActor* CombatActor);

	UFUNCTION(BlueprintCallable, Category = "DF|Combat|State", meta = (WorldContext = "WorldContextObject"))
	static void ForceExitCombat(UObject* WorldContextObject, AActor* CombatActor);

	static float GetCombatExitDelay(const UObject* WorldContextObject);
};
