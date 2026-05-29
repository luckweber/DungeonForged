// Source/DungeonForged/Private/World/UDFWorldTransitionSubsystem.cpp
#include "World/UDFWorldTransitionSubsystem.h"
#include "DungeonForgedModule.h"
#include "Run/DFRunManager.h"
#include "Run/DFSaveGame.h"
#include "Run/UDFSaveLibrary.h"
#include "Run/UDFSaveSlotManagerSubsystem.h"
#include "Settings/UDFWorldDeveloperSettings.h"
#include "World/UDFLoadingScreenSubsystem.h"
#include "GameModes/Run/ADFRunGameState.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

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

void UDFWorldTransitionSubsystem::Deinitialize()
{
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UWorld* const W = GI->GetWorld())
		{
			W->GetTimerManager().ClearTimer(DeferredOpenMapTimer);
			W->GetTimerManager().ClearTimer(TransitionSafetyTimer);
		}
	}
	DeferredMapToOpen.Reset();
	Super::Deinitialize();
}

void UDFWorldTransitionSubsystem::HandleTransitionSafetyTimeout()
{
	bIsTransitioning = false;
}

void UDFWorldTransitionSubsystem::ArmTransitionSafetyTimer()
{
	UGameInstance* const GI = GetGameInstance();
	UWorld* const W = GI ? GI->GetWorld() : nullptr;
	if (!W)
	{
		return;
	}
	W->GetTimerManager().SetTimer(
		TransitionSafetyTimer,
		this,
		&UDFWorldTransitionSubsystem::HandleTransitionSafetyTimeout,
		45.f,
		false);
}

void UDFWorldTransitionSubsystem::ScheduleOpenMapAfterPaint(const FString& Map)
{
	UGameInstance* const GI = GetGameInstance();
	UWorld* const W = GI ? GI->GetWorld() : nullptr;
	if (!W)
	{
		OpenMapByName(Map);
		return;
	}
	if (W->GetNetMode() == NM_Client)
	{
		return;
	}
	if (DeferredOpenMapTimer.IsValid())
	{
		W->GetTimerManager().ClearTimer(DeferredOpenMapTimer);
	}
	DeferredMapToOpen = Map;
	DF_LOG(Log,
		"[DF|WorldTransition] Agenda OpenLevel (50ms): permite desenhar a UI de loading antes do hitch sincrono.");
	W->GetTimerManager().SetTimer(
		DeferredOpenMapTimer,
		FTimerDelegate::CreateUObject(this, &UDFWorldTransitionSubsystem::ExecuteDeferredOpenMap),
		0.05f,
		false);
}

void UDFWorldTransitionSubsystem::ExecuteDeferredOpenMap()
{
	FString Copy = MoveTemp(DeferredMapToOpen);
	DeferredMapToOpen.Reset();
	if (!Copy.IsEmpty())
	{
		OpenMapByName(Copy);
	}
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
	ArmTransitionSafetyTimer();
	PendingReason = Reason;
	PendingClass = NAME_None;
	if (UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>())
	{
		RM->SetNexusArrivalReason(DFWorldTransition::TravelToNexusArrival(Reason));
		if (Reason == ETravelReason::FirstLaunch)
		{
			if (UDFSaveGame const* const Save = UDFSaveLibrary::GetMetaSave(this))
			{
				if (!Save->LastRunClass.IsNone())
				{
					RM->SetSessionSelectedClass(Save->LastRunClass);
				}
			}
		}
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
	ScheduleOpenMapAfterPaint(NexusMapName);
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
	ArmTransitionSafetyTimer();
	PendingReason = ETravelReason::NewRun;
	PendingClass = SelectedClass;
	if (UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>())
	{
		RM->SetPendingWorldTravel(ETravelReason::NewRun, SelectedClass);
		RM->CaptureRunState();
	}
	if (UDFSaveGame* const Save = UDFSaveLibrary::ResolveMutableMetaSave(this))
	{
		Save->bHasActiveRun = true;
		Save->LastRunClass = SelectedClass;
		Save->LastRunFloor = 1;
		(void)UDFSaveLibrary::SaveMetaSave(this, Save);
	}
	SaveCheckpoint(ECheckpointType::RunStart);
	if (UDFLoadingScreenSubsystem* const L = GI->GetSubsystem<UDFLoadingScreenSubsystem>())
	{
		L->ShowLoadingScreen(ETravelReason::NewRun, 1, 10);
	}
	ScheduleOpenMapAfterPaint(RunMapName);
}

void UDFWorldTransitionSubsystem::TravelToRunFromCheckpoint()
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
	UDFSaveGame* const Save = UDFSaveLibrary::ResolveMutableMetaSave(this);
	if (!UDFRunManager::CanResumeFromSave(Save))
	{
		return;
	}
	UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>();
	if (!RM || !RM->LoadRunFromCheckpoint(Save->LastCheckpoint))
	{
		return;
	}
	bIsTransitioning = true;
	ArmTransitionSafetyTimer();
	PendingReason = ETravelReason::NextFloor;
	PendingClass = Save->LastCheckpoint.SelectedClass;
	RM->SetPendingRunArrival(EDFRunTravelReason::ResumeCheckpoint, Save->LastCheckpoint.SelectedClass);
	const int32 Floor = FMath::Max(1, Save->LastCheckpoint.CurrentFloor);
	if (UDFLoadingScreenSubsystem* const L = GI->GetSubsystem<UDFLoadingScreenSubsystem>())
	{
		L->ShowLoadingScreen(ETravelReason::NextFloor, Floor, 10);
	}
	ScheduleOpenMapAfterPaint(RunMapName);
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
	ArmTransitionSafetyTimer();
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
	ScheduleOpenMapAfterPaint(RunMapName);
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
	UDFSaveGame* Save = UDFSaveLibrary::ResolveMutableMetaSave(this);
	if (!Save)
	{
		return;
	}
	if (UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>())
	{
		RM->CaptureRunState();
		Save->LastCheckpoint = RM->GetRunStateCopy();
		Save->LastCheckpointType = Type;
		if (Type != ECheckpointType::RunEnd)
		{
			Save->bHasActiveRun = true;
			Save->LastRunClass = Save->LastCheckpoint.SelectedClass;
			Save->LastRunFloor = Save->LastCheckpoint.CurrentFloor;
		}
	}
	(void)UDFSaveLibrary::SaveMetaSave(this, Save);
}
