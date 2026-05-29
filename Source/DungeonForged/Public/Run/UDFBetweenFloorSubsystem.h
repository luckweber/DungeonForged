// Source/DungeonForged/Public/Run/UDFBetweenFloorSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Events/DFEventData.h"
#include "Run/DFBetweenFloorTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "UDFBetweenFloorSubsystem.generated.h"

class ADFRunGameMode;
class ADFRunPlayerController;
class ADFPlayerState;
class UDFRandomEventWidget;

/**
 * Server-driven between-floor pipeline: Event (optional) → Rest → Shop (periodic) → Draft → advance floor in-place.
 * Replaces the previous stub @c TriggerBetweenFloorSequence / dead @c TravelToNextFloor path for mid-run floors.
 */
UCLASS()
class DUNGEONFORGED_API UDFBetweenFloorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Floors divisible by this value (after clear) get a shop step when a merchant exists. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|BetweenFloor", meta = (ClampMin = "1"))
	int32 ShopEveryNFloors = 3;

	/** Rest step: heal this fraction of max HP (0–1). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|BetweenFloor", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RestHealFraction = 0.25f;

	/** Auto-advance rest step after this many seconds (server). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|BetweenFloor", meta = (ClampMin = "0.1"))
	float RestAutoAdvanceSeconds = 1.5f;

	/** WBP_RandomEvent; optional — event step auto-skips when unset. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|BetweenFloor|UI")
	TSubclassOf<UDFRandomEventWidget> RandomEventWidgetClass = nullptr;

	UFUNCTION(BlueprintPure, Category = "DF|BetweenFloor")
	bool IsFlowActive() const { return bFlowActive; }

	UFUNCTION(BlueprintPure, Category = "DF|BetweenFloor")
	EBetweenFloorStep GetCurrentStep() const { return CurrentStep; }

	/** Rolled event for the active flow (valid during @c EBetweenFloorStep::Event). */
	UFUNCTION(BlueprintPure, Category = "DF|BetweenFloor")
	FName GetPendingEventRowName() const { return PendingEventRowName; }

	UFUNCTION(BlueprintPure, Category = "DF|BetweenFloor")
	bool TryGetPendingEventRow(FDFRandomEventRow& OutRow) const;

	/** Entry from @ref ADFRunGameMode::TriggerBetweenFloorSequence (authority only). */
	void StartBetweenFloorFlow(ADFRunGameMode* GameMode);

	/** After random-event choice is applied on the server. */
	void NotifyEventResolved();

	/** Client finished shop/rest UI (or timeout). */
	void NotifyClientStepFinished(ADFRunPlayerController* FromController);

	/** Ability draft committed (server). */
	void NotifyDraftResolved(bool bSkipped, FName SelectedRowName, ADFPlayerState* FromPlayerState);

	/** Client: show the current step UI (event card, etc.). */
	void PresentStepForLocalPlayer(ADFRunPlayerController* PC);

protected:
	void BuildStepQueue(int32 ClearedFloor);
	void EnterNextStep();
	void EnterStep(EBetweenFloorStep Step);
	void FinishFlowAndAdvanceFloor();
	void ApplyRestHeal();
	void BeginDraftStep();
	void PauseForBetweenFloorUI();
	void ResumeTimeAfterBetweenFloorUI();
	bool IsAuthorityWorld() const;

	UFUNCTION()
	void HandleRestTimerElapsed();

	UPROPERTY(Transient)
	TWeakObjectPtr<ADFRunGameMode> OwningGameMode;

	TArray<EBetweenFloorStep> StepQueue;
	int32 StepQueueIndex = INDEX_NONE;
	EBetweenFloorStep CurrentStep = EBetweenFloorStep::None;
	int32 ClearedFloorNumber = 0;
	FName PendingEventRowName = NAME_None;
	FDFRandomEventRow PendingEventRow;
	bool bFlowActive = false;
	bool bAwaitingClientStep = false;
	FTimerHandle RestAdvanceTimer;
};
