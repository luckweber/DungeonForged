// Source/DungeonForged/Private/Characters/ADFPlayerController.cpp

#include "Characters/ADFPlayerController.h"
#include "Debug/UDFCheatManager.h"
#include "Debug/UDFGASDebugOverlayWidget.h"
#include "Blueprint/UserWidget.h"
#include "DungeonForgedModule.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ADFPlayerController::ADFPlayerController()
{
	CheatClass = UDFCheatManager::StaticClass();
}

void ADFPlayerController::ToggleGASDebugOverlay()
{
#if !UE_BUILD_SHIPPING
	if (!GASDebugOverlay)
	{
		GASDebugOverlay = CreateWidget<UDFGASDebugOverlayWidget>(this, UDFGASDebugOverlayWidget::StaticClass());
		if (GASDebugOverlay)
		{
			GASDebugOverlay->AddToViewport(2000);
		}
	}
	if (GASDebugOverlay)
	{
		bGASDebugOverlayVisible = !bGASDebugOverlayVisible;
		GASDebugOverlay->SetVisibility(
			bGASDebugOverlayVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
#endif
}

bool ADFPlayerController::Server_RequestEquipItem_Validate(const FName ItemRow, const EEquipmentSlot /*Slot*/)
{
	return !ItemRow.IsNone();
}

void ADFPlayerController::Server_RequestEquipItem_Implementation(const FName ItemRow, const EEquipmentSlot Slot)
{
	DF_LOG(Verbose, "Server_RequestEquipItem %s slot=%d", *ItemRow.ToString(), static_cast<int32>(Slot));
}

bool ADFPlayerController::Server_RequestPurchase_Validate(const int32 ShopSlotIndex)
{
	return ShopSlotIndex >= 0;
}

void ADFPlayerController::Server_RequestPurchase_Implementation(const int32 ShopSlotIndex)
{
	DF_LOG(Verbose, "Server_RequestPurchase slot=%d", ShopSlotIndex);
}

void ADFPlayerController::Client_ShowEventCard_Implementation(const FName EventRow)
{
	DF_LOG(Log, "Client_ShowEventCard %s", *EventRow.ToString());
}

void ADFPlayerController::Client_ShowLevelUpScreen_Implementation()
{
	DF_LOG(Log, "Client_ShowLevelUpScreen");
}

void ADFPlayerController::Client_PlayVictorySequence_Implementation()
{
	DF_LOG(Log, "Client_PlayVictorySequence");
}

void ADFPlayerController::Multicast_SpawnHitVFX_Implementation(
	const FVector Location, const FRotator Normal, const FGameplayTag DamageType)
{
	DF_LOG(Verbose, "Multicast_SpawnHitVFX at %s", *Location.ToString());
	(void)Normal;
	(void)DamageType;
}

void ADFPlayerController::Multicast_PlayBossRoar_Implementation(const FVector BossLocation)
{
	DF_LOG(Verbose, "Multicast_PlayBossRoar at %s", *BossLocation.ToString());
}

void ADFPlayerController::Multicast_TriggerPhaseTransitionFX_Implementation(const int32 Phase)
{
	DF_LOG(Verbose, "Multicast_TriggerPhaseTransitionFX phase=%d", Phase);
}
