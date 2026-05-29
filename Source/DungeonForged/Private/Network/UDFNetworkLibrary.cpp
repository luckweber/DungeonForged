// Source/DungeonForged/Private/Network/UDFNetworkLibrary.cpp

#include "Network/UDFNetworkLibrary.h"
#include "Characters/ADFPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

APlayerController* UDFNetworkLibrary::GetLocalPlayerController(UObject* const WorldContextObject)
{
	if (!GEngine)
	{
		return nullptr;
	}
	if (UWorld* const W = GEngine->GetWorldFromContextObject(
		WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		return GEngine->GetFirstLocalPlayerController(W);
	}
	return nullptr;
}

ADFPlayerController* UDFNetworkLibrary::ResolveLocalPlayerController(UObject* const WorldContextObject)
{
	return Cast<ADFPlayerController>(GetLocalPlayerController(WorldContextObject));
}

void UDFNetworkLibrary::MulticastToAllDFPlayerControllers(
	UObject* const WorldContextObject, TFunctionRef<void(ADFPlayerController*)> Fn)
{
	if (!GEngine)
	{
		return;
	}
	UWorld* const W = GEngine->GetWorldFromContextObject(
		WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!W || W->GetNetMode() == NM_Client)
	{
		return;
	}
	for (FConstPlayerControllerIterator It = W->GetPlayerControllerIterator(); It; ++It)
	{
		if (ADFPlayerController* const DPC = Cast<ADFPlayerController>(It->Get()))
		{
			Fn(DPC);
		}
	}
}

void UDFNetworkLibrary::ServerRequestEquipItem(
	UObject* const WorldContextObject, const FName ItemRow, const EEquipmentSlot Slot)
{
	if (ADFPlayerController* const DPC = ResolveLocalPlayerController(WorldContextObject))
	{
		DPC->Server_RequestEquipItem(ItemRow, Slot);
	}
}

void UDFNetworkLibrary::ServerRequestPurchase(UObject* const WorldContextObject, const int32 ShopSlotIndex)
{
	if (ADFPlayerController* const DPC = ResolveLocalPlayerController(WorldContextObject))
	{
		DPC->Server_RequestPurchase(ShopSlotIndex);
	}
}

void UDFNetworkLibrary::ClientShowEventCard(UObject* const WorldContextObject, const FName EventRow)
{
	if (ADFPlayerController* const DPC = ResolveLocalPlayerController(WorldContextObject))
	{
		DPC->Client_ShowEventCard(EventRow);
	}
}

void UDFNetworkLibrary::ClientShowLevelUpScreen(UObject* const WorldContextObject)
{
	if (ADFPlayerController* const DPC = ResolveLocalPlayerController(WorldContextObject))
	{
		DPC->Client_ShowLevelUpScreen();
	}
}

void UDFNetworkLibrary::ClientPlayVictorySequence(UObject* const WorldContextObject)
{
	if (ADFPlayerController* const DPC = ResolveLocalPlayerController(WorldContextObject))
	{
		DPC->Client_PlayVictorySequence();
	}
}

void UDFNetworkLibrary::MulticastSpawnHitVFX(
	UObject* const WorldContextObject, const FVector Location, const FRotator Normal, const FGameplayTag DamageType)
{
	MulticastToAllDFPlayerControllers(
		WorldContextObject,
		[Location, Normal, DamageType](ADFPlayerController* const DPC)
		{
			DPC->Multicast_SpawnHitVFX(Location, Normal, DamageType);
		});
}

void UDFNetworkLibrary::MulticastPlayBossRoar(UObject* const WorldContextObject, const FVector BossLocation)
{
	MulticastToAllDFPlayerControllers(
		WorldContextObject,
		[BossLocation](ADFPlayerController* const DPC) { DPC->Multicast_PlayBossRoar(BossLocation); });
}

void UDFNetworkLibrary::MulticastTriggerPhaseTransitionFX(
	UObject* const WorldContextObject, const int32 Phase)
{
	MulticastToAllDFPlayerControllers(
		WorldContextObject,
		[Phase](ADFPlayerController* const DPC) { DPC->Multicast_TriggerPhaseTransitionFX(Phase); });
}
