// Source/DungeonForged/Private/Network/UDFNetworkLibrary.cpp

#include "Network/UDFNetworkLibrary.h"
#include "Characters/ADFPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ADFPlayerController* UDFNetworkLibrary::ResolveLocalPlayerController(UObject* const WorldContextObject)
{
	if (!GEngine)
	{
		return nullptr;
	}
	if (UWorld* const W = GEngine->GetWorldFromContextObject(
		WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (APlayerController* const PC = UGameplayStatics::GetPlayerController(W, 0))
		{
			return Cast<ADFPlayerController>(PC);
		}
	}
	return nullptr;
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
	// Client RPC is invoked from server; local PC still resolves for targeted Client RPCs on owning PC
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
	if (UWorld* const W = GEngine ? GEngine->GetWorldFromContextObject(
		WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr)
	{
		if (W->GetNetMode() == NM_Client)
		{
			return;
		}
		if (APlayerController* const PC = UGameplayStatics::GetPlayerController(W, 0))
		{
			if (ADFPlayerController* const DPC = Cast<ADFPlayerController>(PC))
			{
				DPC->Multicast_SpawnHitVFX(Location, Normal, DamageType);
			}
		}
	}
}

void UDFNetworkLibrary::MulticastPlayBossRoar(UObject* const WorldContextObject, const FVector BossLocation)
{
	if (UWorld* const W = GEngine ? GEngine->GetWorldFromContextObject(
		WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr)
	{
		if (W->GetNetMode() == NM_Client)
		{
			return;
		}
		if (APlayerController* const PC = UGameplayStatics::GetPlayerController(W, 0))
		{
			if (ADFPlayerController* const DPC = Cast<ADFPlayerController>(PC))
			{
				DPC->Multicast_PlayBossRoar(BossLocation);
			}
		}
	}
}

void UDFNetworkLibrary::MulticastTriggerPhaseTransitionFX(
	UObject* const WorldContextObject, const int32 Phase)
{
	if (UWorld* const W = GEngine ? GEngine->GetWorldFromContextObject(
		WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr)
	{
		if (W->GetNetMode() == NM_Client)
		{
			return;
		}
		if (APlayerController* const PC = UGameplayStatics::GetPlayerController(W, 0))
		{
			if (ADFPlayerController* const DPC = Cast<ADFPlayerController>(PC))
			{
				DPC->Multicast_TriggerPhaseTransitionFX(Phase);
			}
		}
	}
}
