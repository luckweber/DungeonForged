// Source/DungeonForged/Private/Combat/UDFCombatSpectacleSubsystem.cpp
#include "Combat/UDFCombatSpectacleSubsystem.h"

#include "Characters/ADFPlayerCharacter.h"
#include "FX/UDFCameraShakeFunctionLibrary.h"
#include "FX/UDFHitStopSubsystem.h"
#include "FX/UDFScreenEffectsComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
ADFPlayerCharacter* GetLocalPlayerCharacter(UWorld* const World)
{
	if (!World)
	{
		return nullptr;
	}
	if (APlayerController* const PC = UGameplayStatics::GetPlayerController(World, 0))
	{
		return Cast<ADFPlayerCharacter>(PC->GetPawn());
	}
	return nullptr;
}
} // namespace

void UDFCombatSpectacleSubsystem::PlayLastKillSpectacle(AActor* const KilledEnemy, AActor* const Killer)
{
	(void)KilledEnemy;
	if (IsRunningDedicatedServer())
	{
		return;
	}
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}
	if (UDFHitStopSubsystem* const HS = World->GetSubsystem<UDFHitStopSubsystem>())
	{
		HS->BossSlam(Killer);
	}
	if (ADFPlayerCharacter* const Player = GetLocalPlayerCharacter(World))
	{
		if (Player->ScreenEffects)
		{
			Player->ScreenEffects->ApplyKillSpectacle();
		}
		if (APlayerController* const PC = Player->GetController<APlayerController>())
		{
			UDFCameraShakeFunctionLibrary::PlayHeavyHitOnOwner(Player, PC);
		}
	}
}

void UDFCombatSpectacleSubsystem::PlayRoomClearSpectacle()
{
	if (IsRunningDedicatedServer())
	{
		return;
	}
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}
	if (UDFHitStopSubsystem* const HS = World->GetSubsystem<UDFHitStopSubsystem>())
	{
		HS->TriggerHitStop(RoomClearSlowMoDuration, RoomClearSlowMoDilation, nullptr);
	}
	if (ADFPlayerCharacter* const Player = GetLocalPlayerCharacter(World))
	{
		if (Player->ScreenEffects)
		{
			Player->ScreenEffects->ApplyRoomClearSpectacle();
		}
	}
}
