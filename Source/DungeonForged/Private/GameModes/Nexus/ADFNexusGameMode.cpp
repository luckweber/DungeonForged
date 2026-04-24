// Source/DungeonForged/Private/GameModes/Nexus/ADFNexusGameMode.cpp
#include "GameModes/Nexus/ADFNexusGameMode.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "GameModes/Nexus/ADFNexusGameState.h"
#include "GameModes/Nexus/ADFNexusHUD.h"
#include "GameModes/Nexus/ADFNexusNPCBase.h"
#include "GameModes/Nexus/ADFNexusCharacter.h"
#include "GameModes/Nexus/ADFNexusPlayerController.h"
#include "Run/DFSaveGame.h"
#include "Run/DFRunManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameModes/Nexus/DFNexusTypes.h"

ADFNexusGameMode::ADFNexusGameMode()
{
	GameStateClass = ADFNexusGameState::StaticClass();
	PlayerControllerClass = ADFNexusPlayerController::StaticClass();
	HUDClass = ADFNexusHUD::StaticClass();
	NexusPawnClass = ADFNexusCharacter::StaticClass();
	DefaultPawnClass = NexusPawnClass;
}

void ADFNexusGameMode::PostLogin(APlayerController* const NewPlayer)
{
	if (GetNetMode() != NM_Client && NewPlayer)
	{
		UGameInstance* const GI = GetGameInstance();
		UDFRunManager* const RM = GI ? GI->GetSubsystem<UDFRunManager>() : nullptr;
		const ERunNexusTravelReason Arrival = RM ? RM->GetNexusArrivalReason() : ERunNexusTravelReason::FirstLaunch;
		ActivePlayerStartTag = SelectSpawnTagForArrival(Arrival);

		if (UDFSaveGame* const S = UDFSaveGame::Load())
		{
			if (ADFNexusGameState* const GS = GetGameState<ADFNexusGameState>())
			{
				GS->ApplyFromSave(S);
			}
			ProcessPendingUnlocks(S, NewPlayer);
			UDFSaveGame::Save(S);
		}
	}

	Super::PostLogin(NewPlayer);

	if (GetNetMode() == NM_Client || !NewPlayer)
	{
		return;
	}
	UGameInstance* const GI = GetGameInstance();
	UDFRunManager* const RM = GI ? GI->GetSubsystem<UDFRunManager>() : nullptr;
	const ERunNexusTravelReason R = RM ? RM->GetNexusArrivalReason() : ERunNexusTravelReason::FirstLaunch;
	PlayNexusArrivalPresentation(R, NewPlayer);
	if (RM)
	{
		RM->ClearNexusArrivalContext();
	}
}

FName ADFNexusGameMode::SelectSpawnTagForArrival(const ERunNexusTravelReason Reason) const
{
	if (Reason == ERunNexusTravelReason::Victory)
	{
		return CenterPlazaStartTag;
	}
	return DefaultEntranceStartTag;
}

AActor* ADFNexusGameMode::FindPlayerStart_Implementation(
	AController* const Player, const FString& IncomingName)
{
	if (!GetWorld() || ActivePlayerStartTag.IsNone())
	{
		return Super::FindPlayerStart_Implementation(Player, IncomingName);
	}
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		if (It->PlayerStartTag == ActivePlayerStartTag)
		{
			return *It;
		}
	}
	return Super::FindPlayerStart_Implementation(Player, IncomingName);
}

void ADFNexusGameMode::ProcessPendingUnlocks(UDFSaveGame* const Save, APlayerController* const ForNotifications)
{
	ProcessPendingUnlocksFromSave(Save, ForNotifications);
}

void ADFNexusGameMode::ProcessPendingUnlocksFromSave(UDFSaveGame* const Save, APlayerController* const ForNotifications)
{
	if (!Save)
	{
		return;
	}
	const TArray<FDFPendingUnlockEntry> ToProcess = Save->PendingUnlocks;
	for (const FDFPendingUnlockEntry& E : ToProcess)
	{
		switch (E.Type)
		{
		case ENexusPendingUnlockType::UnlockClass:
			if (!E.ClassRow.IsNone() && !Save->UnlockedClasses.Contains(E.ClassRow))
			{
				Save->UnlockedClasses.Add(E.ClassRow);
			}
			break;
		case ENexusPendingUnlockType::UnlockNPC:
			if (!E.NPCId.IsNone() && !Save->UnlockedNPCs.Contains(E.NPCId))
			{
				Save->UnlockedNPCs.Add(E.NPCId);
			}
			if (UWorld* const W = GetWorld())
			{
				for (TActorIterator<ADFNexusNPCBase> NIt(W); NIt; ++NIt)
				{
					if (NIt->GetNPCId() == E.NPCId)
					{
						NIt->SetNexusUnlockedFromSave(true);
						break;
					}
				}
			}
			break;
		case ENexusPendingUnlockType::UnlockUpgrade:
			if (!E.UpgradeRow.IsNone() && !Save->CompletedUpgrades.Contains(E.UpgradeRow))
			{
				Save->CompletedUpgrades.Add(E.UpgradeRow);
			}
			break;
		default: break;
		}
	}
	Save->PendingUnlocks.Reset();
	UDFSaveGame::Save(Save);
	if (ADFNexusGameState* const GS = GetGameState<ADFNexusGameState>())
	{
		GS->ApplyFromSave(Save);
	}
	if (ADFNexusHUD* const H = ForNotifications ? ForNotifications->GetHUD<ADFNexusHUD>() : nullptr)
	{
		for (const FDFPendingUnlockEntry& E : ToProcess)
		{
			H->QueueUnlockNotificationForEntry(E);
		}
	}
}

void ADFNexusGameMode::PlayNexusArrivalPresentation_Implementation(
	ERunNexusTravelReason const /*Reason*/, APlayerController* const /*LocalPC*/)
{
}
