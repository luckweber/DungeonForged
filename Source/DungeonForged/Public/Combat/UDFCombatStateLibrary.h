// Source/DungeonForged/Public/Combat/UDFCombatStateLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UDFCombatStateLibrary.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDFRoomCleared);

/** Applies @c State.InCombat and schedules exit after idle time. */
UCLASS()
class DUNGEONFORGED_API UDFCombatStateLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static FOnDFRoomCleared& GetOnRoomClearedDelegate();

	UFUNCTION(BlueprintCallable, Category = "DF|Combat|State", meta = (WorldContext = "WorldContextObject"))
	static void NotifyCombatActivity(UObject* WorldContextObject, AActor* CombatActor);

	UFUNCTION(BlueprintPure, Category = "DF|Combat|State", meta = (WorldContext = "WorldContextObject"))
	static bool IsActorInCombat(UObject* WorldContextObject, AActor* CombatActor);

	UFUNCTION(BlueprintCallable, Category = "DF|Combat|State", meta = (WorldContext = "WorldContextObject"))
	static void ForceExitCombat(UObject* WorldContextObject, AActor* CombatActor);

	/** Broadcast when the last enemy in a room dies or the floor is cleared (B8). */
	UFUNCTION(BlueprintCallable, Category = "DF|Combat|State", meta = (WorldContext = "WorldContextObject"))
	static void NotifyRoomCleared(UObject* WorldContextObject, bool bFloorCleared);

	/** True when @a DyingEnemy is the last tracked floor enemy (excludes bosses for celebration). */
	UFUNCTION(BlueprintPure, Category = "DF|Combat|State", meta = (WorldContext = "WorldContextObject"))
	static bool IsLastEnemyInRoom(UObject* WorldContextObject, AActor* DyingEnemy);

	static float GetCombatExitDelay(const UObject* WorldContextObject);
};
