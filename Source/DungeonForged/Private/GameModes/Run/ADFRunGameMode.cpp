// Source/DungeonForged/Private/GameModes/Run/ADFRunGameMode.cpp
#include "GameModes/Run/ADFRunGameMode.h"
#include "ADFDungeonManager.h"
#include "GameModes/Run/ADFRunGameState.h"
#include "GameModes/Run/ADFRunHUD.h"
#include "GameModes/Run/ADFRunPlayerController.h"
#include "World/UDFWorldTransitionSubsystem.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Characters/ADFPlayerState.h"
#include "GAS/UDFAttributeSet.h"
#include "Run/DFRunManager.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace
{
	constexpr int32 kVictoryZOrder = 20;
}

ADFRunGameMode::ADFRunGameMode()
{
	DefaultPawnClass = ADFPlayerCharacter::StaticClass();
	GameStateClass = ADFRunGameState::StaticClass();
	PlayerControllerClass = ADFRunPlayerController::StaticClass();
	HUDClass = ADFRunHUD::StaticClass();
}

void ADFRunGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	if (DefaultPlayerClass)
	{
		DefaultPawnClass = DefaultPlayerClass;
	}
	if (GameStateType)
	{
		GameStateClass = GameStateType;
	}
	if (HUDType)
	{
		HUDClass = HUDType;
	}
	if (RunPlayerControllerType)
	{
		PlayerControllerClass = RunPlayerControllerType;
	}
	Super::InitGame(MapName, Options, ErrorMessage);
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFDungeonManager* const DM = GI->GetSubsystem<UDFDungeonManager>())
		{
			if (DungeonFloorTable)
			{
				DM->DungeonFloorTable = DungeonFloorTable;
			}
		}
	}
	RegisterDungeonDelegates();
	if (RunTimeLimit > 0.f)
	{
		if (UWorld* const W = GetWorld())
		{
			W->GetTimerManager().SetTimer(
				RunTimeCheckTimer, this, &ADFRunGameMode::HandleRunTimeExpired, 1.f, true);
		}
	}
}

void ADFRunGameMode::RegisterDungeonDelegates()
{
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFDungeonManager* const DM = GI->GetSubsystem<UDFDungeonManager>())
		{
			DM->OnRunCompleted.AddDynamic(this, &ADFRunGameMode::HandleDungeonRunCompleted);
			DM->OnRunEnemyKilled.AddDynamic(this, &ADFRunGameMode::HandleRunEnemyKilled);
		}
	}
}

void ADFRunGameMode::UnbindAllDungeonDelegates()
{
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFDungeonManager* const DM = GI->GetSubsystem<UDFDungeonManager>())
		{
			DM->OnRunCompleted.RemoveAll(this);
			DM->OnRunEnemyKilled.RemoveAll(this);
		}
	}
}

void ADFRunGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindAllDungeonDelegates();
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(VictoryEndTimer);
		W->GetTimerManager().ClearTimer(DefeatEndTimer);
		W->GetTimerManager().ClearTimer(DefeatAfterDeathAnimTimer);
		W->GetTimerManager().ClearTimer(RunTimeCheckTimer);
	}
	if (UObject* O = BoundAttributeSet.Get())
	{
		if (UDFAttributeSet* const AS = Cast<UDFAttributeSet>(O))
		{
			AS->OnOutOfHealth.RemoveAll(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ADFRunGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	if (GetNetMode() == NM_Client)
	{
		return;
	}
	UGameInstance* const GI = GetGameInstance();
	UDFRunManager* const RM = GI ? GI->GetSubsystem<UDFRunManager>() : nullptr;
	UDFDungeonManager* const DM = GI ? GI->GetSubsystem<UDFDungeonManager>() : nullptr;
	if (!RM || !DM)
	{
		return;
	}
	if (EnemyDataTable)
	{
		DM->EnemyDataTable = EnemyDataTable;
	}
	if (DungeonFloorTable)
	{
		DM->DungeonFloorTable = DungeonFloorTable;
	}
	DM->PCGOwnerActor = PCGOwnerActor;
	DM->SpawnPointsPreview = SpawnPointsPreview;
	ADFPlayerCharacter* const P = NewPlayer->GetPawn<ADFPlayerCharacter>();
	const EDFRunTravelReason Arrival = RM->GetArrivalReason();
	const FName PendingClass = RM->GetPendingClassName();
	if (Arrival == EDFRunTravelReason::NewRun && !PendingClass.IsNone())
	{
		InitializePlayerFromClass(NewPlayer, PendingClass);
	}
	else if (Arrival == EDFRunTravelReason::NextFloor && P)
	{
		RM->RestoreRunState(P);
	}
	else if (!RM->IsRunInProgress() && !DefaultClassRowName.IsNone())
	{
		InitializePlayerFromClass(NewPlayer, DefaultClassRowName);
	}
	else if (P)
	{
		RM->ApplyRunStateToPlayer(P);
	}
	if (ADFRunGameState* const RGS = GetGameState<ADFRunGameState>())
	{
		RGS->CurrentFloor = FMath::Max(1, RM->GetCurrentRunState().CurrentFloor);
		RGS->SetPhase(ERunPhase::InCombat);
	}
	const int32 FloorToStart = FMath::Max(1, RM->GetCurrentRunState().CurrentFloor);
	DM->StartFloor(FloorToStart);
	RM->ClearRunArrivalContext();
	TryBindPawnOutOfHealth(NewPlayer);
}

void ADFRunGameMode::InitializePlayerFromClass(APlayerController* const PC, FName const ClassName)
{
	UGameInstance* const GI = GetGameInstance();
	UDFRunManager* const RM = GI ? GI->GetSubsystem<UDFRunManager>() : nullptr;
	if (!PC || !RM)
	{
		return;
	}
	RM->StartNewRun(ClassName);
	if (ADFPlayerCharacter* const P = PC->GetPawn<ADFPlayerCharacter>())
	{
		RM->ApplyRunStateToPlayer(P);
	}
	if (ClassBaseStatsEffect)
	{
		if (ADFPlayerState* const PS = PC->GetPlayerState<ADFPlayerState>())
		{
			if (UAbilitySystemComponent* const ASC = PS->GetAbilitySystemComponent())
			{
				ASC->InitAbilityActorInfo(PS, PC->GetPawn());
				const FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
				const FGameplayEffectSpecHandle SpecH = ASC->MakeOutgoingSpec(ClassBaseStatsEffect, 1, Ctx);
				if (SpecH.IsValid())
				{
					ASC->ApplyGameplayEffectSpecToSelf(*SpecH.Data);
				}
			}
		}
	}
}

void ADFRunGameMode::TryBindPawnOutOfHealth(APlayerController* const PC)
{
	if (!PC)
	{
		return;
	}
	UnbindPawnOutOfHealth(PC);
	if (ADFPlayerState* const PS = PC->GetPlayerState<ADFPlayerState>())
	{
		if (UDFAttributeSet* const AS = PS->AttributeSet)
		{
			AS->OnOutOfHealth.AddUObject(this, &ADFRunGameMode::HandlePlayerOutOfHealth);
			BoundAttributeSet = AS;
		}
	}
}

void ADFRunGameMode::UnbindPawnOutOfHealth(APlayerController* const PC)
{
	if (UObject* O = BoundAttributeSet.Get())
	{
		if (UDFAttributeSet* const AS = Cast<UDFAttributeSet>(O))
		{
			AS->OnOutOfHealth.RemoveAll(this);
		}
	}
	BoundAttributeSet = nullptr;
}

void ADFRunGameMode::Logout(AController* const Exiting)
{
	if (APlayerController* const PC = Cast<APlayerController>(Exiting))
	{
		UnbindPawnOutOfHealth(PC);
	}
	Super::Logout(Exiting);
}

void ADFRunGameMode::HandlePlayerOutOfHealth()
{
	if (bDefeatInProgress)
	{
		return;
	}
	if (UWorld* const W = GetWorld())
	{
		if (UDFRunManager* const RM = GetGameInstance() ? GetGameInstance()->GetSubsystem<UDFRunManager>() : nullptr)
		{
			RM->OnPlayerDied(false);
		}
		W->GetTimerManager().SetTimer(
			DefeatAfterDeathAnimTimer,
			[this]()
			{
				TriggerDefeat();
			},
			3.f,
			false);
	}
}

void ADFRunGameMode::HandleRunEnemyKilled(AActor* const /*Enemy*/)
{
	if (ADFRunGameState* const RGS = GetGameState<ADFRunGameState>())
	{
		RGS->AuthorityIncrementKills(1);
	}
}

void ADFRunGameMode::HandleRunTimeExpired()
{
	if (RunTimeLimit <= 0.f)
	{
		return;
	}
	const ADFRunGameState* const RGS = GetGameState<ADFRunGameState>();
	if (!RGS)
	{
		return;
	}
	if (RGS->ElapsedRunTime >= RunTimeLimit)
	{
		TriggerDefeat();
	}
}

void ADFRunGameMode::HandleDungeonRunCompleted()
{
	TriggerVictory();
}

void ADFRunGameMode::TriggerBetweenFloorSequence()
{
	if (ADFRunGameState* const RGS = GetGameState<ADFRunGameState>())
	{
		RGS->SetPhase(ERunPhase::BetweenFloors);
	}
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>())
		{
			RM->CaptureRunState();
		}
	}
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.0001f);
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ADFRunPlayerController* const RPC = Cast<ADFRunPlayerController>(It->Get()))
		{
			RPC->Client_PresentBetweenFloorUI();
		}
	}
}

void ADFRunGameMode::TriggerVictory()
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.3f);
	if (ADFRunGameState* const RGS = GetGameState<ADFRunGameState>())
	{
		RGS->SetPhase(ERunPhase::Victory);
	}
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>())
		{
			RM->OnRunCompleted();
		}
	}
	if (UWorld* const W = GetWorld())
	{
		if (const ADFRunGameState* const RGS = W->GetGameState<ADFRunGameState>())
		{
			const FDFRunSummary S = RGS->GetRunSummary();
			for (FConstPlayerControllerIterator It = W->GetPlayerControllerIterator(); It; ++It)
			{
				if (ADFRunPlayerController* const RPC = Cast<ADFRunPlayerController>(It->Get()))
				{
					RPC->Client_OpenVictoryScreen(S);
				}
			}
		}
		W->GetTimerManager().SetTimer(
			VictoryEndTimer, this, &ADFRunGameMode::ScheduleFinishVictoryToNexus, 5.f, false);
	}
}

void ADFRunGameMode::ScheduleFinishVictoryToNexus()
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f);
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFWorldTransitionSubsystem* const T = GI->GetSubsystem<UDFWorldTransitionSubsystem>())
		{
			T->TravelToNexus(ERunNexusTravelReason::Victory);
		}
	}
}

void ADFRunGameMode::TriggerDefeat()
{
	if (bDefeatInProgress)
	{
		return;
	}
	bDefeatInProgress = true;
	if (ADFRunGameState* const RGS = GetGameState<ADFRunGameState>())
	{
		RGS->SetPhase(ERunPhase::Defeat);
	}
	if (UWorld* const W = GetWorld())
	{
		if (const ADFRunGameState* const RGS = W->GetGameState<ADFRunGameState>())
		{
			const FDFRunSummary S = RGS->GetRunSummary();
			for (FConstPlayerControllerIterator It = W->GetPlayerControllerIterator(); It; ++It)
			{
				if (ADFRunPlayerController* const RPC = Cast<ADFRunPlayerController>(It->Get()))
				{
					RPC->Client_OpenDefeatScreen(S, FString());
				}
			}
		}
		W->GetTimerManager().SetTimer(
			DefeatEndTimer, this, &ADFRunGameMode::ScheduleFinishDefeatToNexus, 5.f, false);
	}
}

void ADFRunGameMode::ScheduleFinishDefeatToNexus()
{
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFWorldTransitionSubsystem* const T = GI->GetSubsystem<UDFWorldTransitionSubsystem>())
		{
			T->TravelToNexus(ERunNexusTravelReason::Defeat);
		}
	}
}
