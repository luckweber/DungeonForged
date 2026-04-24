// Source/DungeonForged/Public/GameModes/Run/ADFRunGameState.h
#pragma once

#include "CoreMinimal.h"
#include "GameModes/Run/DFRunTypes.h"
#include "GameFramework/GameStateBase.h"
#include "ADFRunGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRunPhaseChanged, ERunPhase, NewPhase, ERunPhase, OldPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRunKillsChanged, int32, NewKills);

/**
 * Replicated run statistics and phase. Clients read this when @c AGameMode is unavailable.
 */
UCLASS()
class DUNGEONFORGED_API ADFRunGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ADFRunGameState();

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 1-based; mirrored from dungeon / @ref UDFRunManager. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentFloor, Category = "Run")
	int32 CurrentFloor = 1;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ElapsedRunTime, Category = "Run")
	float ElapsedRunTime = 0.f;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_TotalKills, Category = "Run")
	int32 TotalKills = 0;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_TotalGoldCollected, Category = "Run")
	int32 TotalGoldCollected = 0;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentPhase, Category = "Run")
	ERunPhase CurrentPhase = ERunPhase::PreRun;

	UFUNCTION()
	void OnRep_CurrentFloor();
	UFUNCTION()
	void OnRep_ElapsedRunTime();
	UFUNCTION()
	void OnRep_TotalKills();
	UFUNCTION()
	void OnRep_TotalGoldCollected();
	UFUNCTION()
	void OnRep_CurrentPhase();

	/** Authority */
	void AuthorityIncrementKills(int32 const Delta = 1);

	/** Authority */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void AddGold(int32 const Amount);

	/** Authority: updates @c CurrentPhase and notifies clients. */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void SetPhase(ERunPhase const Phase);

	UFUNCTION(BlueprintCallable, Category = "Run")
	FDFRunSummary GetRunSummary() const;

	/** Binds: HUD, pause menu, etc. */
	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnRunPhaseChanged OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnRunKillsChanged OnKillsChanged;

protected:
	ERunPhase LastPhaseNotified = ERunPhase::PreRun;
};
