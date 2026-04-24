// Source/DungeonForged/Private/GameModes/Run/ADFRunGameState.cpp
#include "GameModes/Run/ADFRunGameState.h"
#include "Run/DFRunManager.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

ADFRunGameState::ADFRunGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void ADFRunGameState::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	if (CurrentPhase == ERunPhase::InCombat || CurrentPhase == ERunPhase::BossEncounter
		|| CurrentPhase == ERunPhase::BetweenFloors)
	{
		ElapsedRunTime += DeltaSeconds;
	}
}

void ADFRunGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADFRunGameState, CurrentFloor);
	DOREPLIFETIME(ADFRunGameState, ElapsedRunTime);
	DOREPLIFETIME(ADFRunGameState, TotalKills);
	DOREPLIFETIME(ADFRunGameState, TotalGoldCollected);
	DOREPLIFETIME(ADFRunGameState, CurrentPhase);
}

void ADFRunGameState::OnRep_CurrentFloor() {}

void ADFRunGameState::OnRep_ElapsedRunTime() {}

void ADFRunGameState::OnRep_TotalKills()
{
	OnKillsChanged.Broadcast(TotalKills);
}

void ADFRunGameState::OnRep_TotalGoldCollected() {}

void ADFRunGameState::OnRep_CurrentPhase()
{
	ERunPhase const Old = LastPhaseNotified;
	LastPhaseNotified = CurrentPhase;
	OnPhaseChanged.Broadcast(CurrentPhase, Old);
}

void ADFRunGameState::AuthorityIncrementKills(int32 const Delta)
{
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	TotalKills = FMath::Max(0, TotalKills + FMath::Max(0, Delta));
	OnKillsChanged.Broadcast(TotalKills);
}

void ADFRunGameState::AddGold(int32 const Amount)
{
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	TotalGoldCollected = FMath::Max(0, TotalGoldCollected + Amount);
}

void ADFRunGameState::SetPhase(ERunPhase const Phase)
{
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	ERunPhase const Old = CurrentPhase;
	CurrentPhase = Phase;
	LastPhaseNotified = CurrentPhase;
	// Server / standalone: also broadcast (no OnRep in single process).
	OnPhaseChanged.Broadcast(CurrentPhase, Old);
}

FDFRunSummary ADFRunGameState::GetRunSummary() const
{
	FDFRunSummary S;
	S.FloorReached = CurrentFloor;
	S.Kills = TotalKills;
	S.Gold = TotalGoldCollected;
	S.TimeSeconds = ElapsedRunTime;
	if (UWorld* const W = GetWorld())
	{
		if (const UGameInstance* const GI = W->GetGameInstance())
		{
			if (const UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>())
			{
				const FDFRunState& R = RM->GetCurrentRunState();
				S.ClassName = R.SelectedClass;
				S.AbilitiesCollected = R.GrantedAbilities;
			}
		}
	}
	return S;
}
