// Source/DungeonForged/Private/World/UDFWorldTransitionSubsystem.cpp
#include "World/UDFWorldTransitionSubsystem.h"
#include "DungeonForgedModule.h"
#include "Run/DFRunManager.h"
#include "Run/DFSaveGame.h"
#include "Settings/UDFWorldDeveloperSettings.h"
#include "World/UDFLoadingScreenSubsystem.h"
#include "GameModes/Run/ADFRunGameState.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

/*
  PRIMEIRO LAUNCH
        |
  [Nexus - ETravelReason::FirstLaunch]
  ADFNexusGameMode: Ferreiro + Cronista desbloqueados
        |  (Portal, WBP_ClassSelection, confirmar)
  TravelToRun(Class) -> tela "Gerando Dungeon..."
        |
  [DungeonRun - floor 1]
  ADFRunGameMode::HandleStartingNewPlayer -> InitializePlayerFromClass
        |
  Combate, loot, floor cleared, BetweenFloorSequence
        |
  TravelToNextFloor(2) -> tela "Andar 2..."
        |
  Andares 2-9, depois floor 10 boss
        |
  Boss derrotado | Jogador morreu | AbandonRun
  TriggerVictory   TriggerDefeat     pause menu
        |                 |                 |
  FinalizeRunData  FinalizeRunData  FinalizeRunData
  MetaXP (vitória)  MetaXP (derrota) MetaXP (abandono)
        \----------------|---------------/
        |
  Loading, OpenLevel(Nexus)
        |
  ADFNexusGameMode: ProcessPendingUnlocks, spawn, WBP_Unlock
        |
  Explora o Nexus, nova run
*/

void UDFWorldTransitionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (const UDFWorldDeveloperSettings* const Dev = GetDefault<UDFWorldDeveloperSettings>())
	{
		NexusMapName = UDFWorldDeveloperSettings::ResolveMapPath(Dev->NexusMap, NexusMapName);
		RunMapName = UDFWorldDeveloperSettings::ResolveMapPath(Dev->RunMap, RunMapName);
	}
	DF_LOG(Log, "[DF|WorldTransition] Initialize: NexusMapName='%s' RunMapName='%s'",
		*NexusMapName, *RunMapName);
}

void UDFWorldTransitionSubsystem::OpenMapByName(const FString& Map)
{
	UGameInstance* const GI = GetGameInstance();
	if (!GI)
	{
		return;
	}
	if (UWorld* const W = GI->GetWorld())
	{
		if (W->GetNetMode() == NM_Client)
		{
			return;
		}
		if (Map.IsEmpty())
		{
			DF_LOG(Error,
				"[DF|WorldTransition] OpenMapByName: nome de mapa vazio. Configure NexusMapName / RunMapName "
				"no UDFWorldTransitionSubsystem (defaults em /Game/DungeonForged/Maps/...).");
			bIsTransitioning = false;
			return;
		}
		DF_LOG(Log, "[DF|WorldTransition] OpenMapByName: '%s'", *Map);
		UGameplayStatics::OpenLevel(GI, FName(*Map), true);
	}
}

void UDFWorldTransitionSubsystem::TravelToNexus(const ETravelReason Reason)
{
	UGameInstance* const GI = GetGameInstance();
	if (!GI)
	{
		return;
	}
	UWorld* const W = GI->GetWorld();
	if (!W || W->GetNetMode() == NM_Client)
	{
		return;
	}
	if (bIsTransitioning)
	{
		return;
	}
	bIsTransitioning = true;
	PendingReason = Reason;
	PendingClass = NAME_None;
	if (UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>())
	{
		RM->SetNexusArrivalReason(DFWorldTransition::TravelToNexusArrival(Reason));
	}
	if (Reason == ETravelReason::Victory || Reason == ETravelReason::Defeat || Reason == ETravelReason::AbandonRun)
	{
		FinalizeRunData(Reason);
		SaveCheckpoint(ECheckpointType::RunEnd);
	}
	if (UDFLoadingScreenSubsystem* const L = GI->GetSubsystem<UDFLoadingScreenSubsystem>())
	{
		L->ShowLoadingScreen(Reason, 1, 10);
	}
	OpenMapByName(NexusMapName);
}

void UDFWorldTransitionSubsystem::TravelToRun(const FName SelectedClass)
{
	UGameInstance* const GI = GetGameInstance();
	if (!GI)
	{
		return;
	}
	UWorld* const W = GI->GetWorld();
	if (!W || W->GetNetMode() == NM_Client)
	{
		return;
	}
	if (bIsTransitioning)
	{
		return;
	}
	if (SelectedClass.IsNone())
	{
		return;
	}
	bIsTransitioning = true;
	PendingReason = ETravelReason::NewRun;
	PendingClass = SelectedClass;
	if (UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>())
	{
		RM->SetPendingWorldTravel(ETravelReason::NewRun, SelectedClass);
		RM->CaptureRunState();
	}
	SaveCheckpoint(ECheckpointType::RunStart);
	if (UDFLoadingScreenSubsystem* const L = GI->GetSubsystem<UDFLoadingScreenSubsystem>())
	{
		L->ShowLoadingScreen(ETravelReason::NewRun, 1, 10);
	}
	OpenMapByName(RunMapName);
}

void UDFWorldTransitionSubsystem::TravelToNextFloor(const int32 NextFloor, const int32 MaxFloors)
{
	UGameInstance* const GI = GetGameInstance();
	if (!GI)
	{
		return;
	}
	UWorld* const W = GI->GetWorld();
	if (!W || W->GetNetMode() == NM_Client)
	{
		return;
	}
	if (bIsTransitioning)
	{
		return;
	}
	bIsTransitioning = true;
	PendingReason = ETravelReason::NextFloor;
	PendingClass = NAME_None;
	if (UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>())
	{
		RM->SetPendingWorldTravel(ETravelReason::NextFloor, NAME_None);
		RM->AdvanceFloor(NextFloor - RM->GetCurrentRunState().CurrentFloor);
		RM->CaptureRunState();
	}
	SaveCheckpoint(ECheckpointType::FloorComplete);
	if (UDFLoadingScreenSubsystem* const L = GI->GetSubsystem<UDFLoadingScreenSubsystem>())
	{
		L->ShowLoadingScreen(ETravelReason::NextFloor, NextFloor, MaxFloors);
	}
	OpenMapByName(RunMapName);
}

void UDFWorldTransitionSubsystem::FinalizeRunData(const ETravelReason Reason)
{
	if (Reason != ETravelReason::Victory && Reason != ETravelReason::Defeat
		&& Reason != ETravelReason::AbandonRun)
	{
		return;
	}
	UGameInstance* const GI = GetGameInstance();
	if (!GI)
	{
		return;
	}
	UWorld* const W = GI->GetWorld();
	if (!W || W->GetNetMode() == NM_Client)
	{
		return;
	}
	FDFRunSummary Summary;
	if (const ADFRunGameState* const RGS = W->GetGameState<ADFRunGameState>())
	{
		Summary = RGS->GetRunSummary();
	}
	if (UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>())
	{
		RM->ApplyEndOfRunPersistence(Reason, Summary);
	}
}

void UDFWorldTransitionSubsystem::SaveCheckpoint(const ECheckpointType Type)
{
	UGameInstance* const GI = GetGameInstance();
	if (!GI)
	{
		return;
	}
	UDFSaveGame* const Save = UDFSaveGame::Load();
	if (!Save)
	{
		return;
	}
	if (UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>())
	{
		Save->LastCheckpoint = RM->GetRunStateCopy();
		Save->LastCheckpointType = Type;
	}
	UDFSaveGame::Save(Save);
}
