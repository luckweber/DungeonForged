// Source/DungeonForged/Public/Network/UDFNetworkLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "Equipment/DFEquipmentTypes.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UDFNetworkLibrary.generated.h"

class ADFPlayerController;
class UObject;

/**
 * Blueprint helpers that forward to `ADFPlayerController` RPCs. `UBlueprintFunctionLibrary` cannot
 * declare `UFUNCTION(Server|Client|NetMulticast)`; replication lives on the controller.
 */
UCLASS()
class DUNGEONFORGED_API UDFNetworkLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DF|Net", meta = (WorldContext = "WorldContextObject"))
	static void ServerRequestEquipItem(UObject* WorldContextObject, FName ItemRow, EEquipmentSlot Slot);

	UFUNCTION(BlueprintCallable, Category = "DF|Net", meta = (WorldContext = "WorldContextObject"))
	static void ServerRequestPurchase(UObject* WorldContextObject, int32 ShopSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "DF|Net", meta = (WorldContext = "WorldContextObject"))
	static void ClientShowEventCard(UObject* WorldContextObject, FName EventRow);

	UFUNCTION(BlueprintCallable, Category = "DF|Net", meta = (WorldContext = "WorldContextObject"))
	static void ClientShowLevelUpScreen(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "DF|Net", meta = (WorldContext = "WorldContextObject"))
	static void ClientPlayVictorySequence(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "DF|Net", meta = (WorldContext = "WorldContextObject"))
	static void MulticastSpawnHitVFX(
		UObject* WorldContextObject, FVector Location, FRotator Normal, FGameplayTag DamageType);

	UFUNCTION(BlueprintCallable, Category = "DF|Net", meta = (WorldContext = "WorldContextObject"))
	static void MulticastPlayBossRoar(UObject* WorldContextObject, FVector BossLocation);

	UFUNCTION(BlueprintCallable, Category = "DF|Net", meta = (WorldContext = "WorldContextObject"))
	static void MulticastTriggerPhaseTransitionFX(UObject* WorldContextObject, int32 Phase);

	/** PIE / single-player: player index 0. For co-op, obtain the `APlayerController` on the server and call RPCs directly. */
	static ADFPlayerController* ResolveLocalPlayerController(UObject* WorldContextObject);
};
