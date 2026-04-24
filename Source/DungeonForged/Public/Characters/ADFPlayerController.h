// Source/DungeonForged/Public/Characters/ADFPlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "Equipment/DFEquipmentTypes.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "ADFPlayerController.generated.h"

class UDFGASDebugOverlayWidget;

UCLASS()
class DUNGEONFORGED_API ADFPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ADFPlayerController();

	/** Toggles the GAS debug overlay. No-op in shipping. */
	UFUNCTION(BlueprintCallable, Category = "DF|Debug")
	void ToggleGASDebugOverlay();

	/** GAS / perf overlay; only populated in non-shipping. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|Debug")
	TObjectPtr<UDFGASDebugOverlayWidget> GASDebugOverlay;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|Debug")
	bool bGASDebugOverlayVisible = false;

	//~ --- DF net patterns (UDFNetworkLibrary forwards here; BFL cannot be RPC) ---

	UFUNCTION(Server, Reliable, WithValidation, Category = "DF|Net")
	void Server_RequestEquipItem(FName ItemRow, EEquipmentSlot Slot);

	UFUNCTION(Server, Reliable, WithValidation, Category = "DF|Net")
	void Server_RequestPurchase(int32 ShopSlotIndex);

	UFUNCTION(Client, Reliable, Category = "DF|Net")
	void Client_ShowEventCard(FName EventRow);

	UFUNCTION(Client, Reliable, Category = "DF|Net")
	void Client_ShowLevelUpScreen();

	UFUNCTION(Client, Reliable, Category = "DF|Net")
	void Client_PlayVictorySequence();

	UFUNCTION(NetMulticast, Unreliable, Category = "DF|Net|FX")
	void Multicast_SpawnHitVFX(FVector Location, FRotator Normal, FGameplayTag DamageType);

	UFUNCTION(NetMulticast, Unreliable, Category = "DF|Net|FX")
	void Multicast_PlayBossRoar(FVector BossLocation);

	UFUNCTION(NetMulticast, Unreliable, Category = "DF|Net|FX")
	void Multicast_TriggerPhaseTransitionFX(int32 Phase);
};
