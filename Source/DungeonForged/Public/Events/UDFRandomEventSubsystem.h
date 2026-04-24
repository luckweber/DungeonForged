// Source/DungeonForged/Public/Events/UDFRandomEventSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Data/DFDataTableStructs.h"
#include "Events/DFEventData.h"
#include "Subsystems/WorldSubsystem.h"
#include "UDFRandomEventSubsystem.generated.h"

class ADFPlayerCharacter;
class UAbilitySystemComponent;
class UDataTable;
class UDFRunManager;
class UDFAbilitySelectionSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRandomEventOutcome, FText, OutcomeMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnRandomEventSpecialEncounter,
	FName,
	EventRowName,
	float,
	DesignPayload,
	ADFPlayerCharacter*,
	Player);

/**
 * Roguelike random events between floors: data from `DT_RandomEvents` (`FDFRandomEventRow`).
 */
UCLASS()
class DUNGEONFORGED_API UDFRandomEventSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Per run, clear when `UDFRunManager::StartNewRun` runs (called from there). */
	UFUNCTION(BlueprintCallable, Category = "DF|Events")
	void ResetUsedEvents();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DF|Events")
	bool ShouldTriggerEvent() const;

	/**
	 * Weighted random among rows matching `CurrentFloor` and repeat rules.
	 * Returns nullptr if no table or no eligible row. Does **not** mark used until you call `MarkEventUsed`.
	 */
	const FDFRandomEventRow* RollEvent(int32 CurrentFloor, FName& OutRowName);

	/** Call after the player commits to an event instance (e.g. after `RollEvent`) if `!Row.bCanRepeat`. */
	UFUNCTION(BlueprintCallable, Category = "DF|Events")
	void MarkEventUsed(FName RowName, bool bCanRepeat);

	/**
	 * Server / authority: apply GAS + run state; broadcasts `OnOutcome` with `Choice.OutcomeText`.
	 * `AddAbility` with `AbilityRowName==None` calls `UDFRunManager::TryGrantRandomAbilityByMinimumRarity(EItemRarity::Epic, …)`.
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|Events")
	void ExecuteChoice(const FDFEventChoice& Choice, ADFPlayerCharacter* Player, FName SourceEventRow);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DF|Events")
	bool DoesPlayerMeetChoiceRequirements(const FDFEventChoice& Choice, ADFPlayerCharacter* Player) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DF|Events")
	TObjectPtr<UDataTable> EventTable = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "DF|Events")
	TArray<FName> UsedEvents;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DF|Events", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EventChancePerFloor = 0.4f;

	/** Fires after `ExecuteChoice` so UMG can play `WBP_EventOutcome` + animations. */
	UPROPERTY(BlueprintAssignable, Category = "DF|Events")
	FOnRandomEventOutcome OnOutcome;

	/** `SpawnSpecialEncounter` outcomes: spawn miniboss, encounter BP, etc. */
	UPROPERTY(BlueprintAssignable, Category = "DF|Events")
	FOnRandomEventSpecialEncounter OnRequestSpecialEncounter;

protected:
	UDFRunManager* ResolveRunManager() const;
	UDFAbilitySelectionSubsystem* ResolveAbilitySelection() const;
	UAbilitySystemComponent* ResolvePlayerAsc(ADFPlayerCharacter* Player) const;
	void ApplyHeal(ADFPlayerCharacter* Player, const FDFEventChoice& Choice) const;
	void ApplyDamage(ADFPlayerCharacter* Player, const FDFEventChoice& Choice) const;
	void ApplyDamageWithAmount(ADFPlayerCharacter* Player, float RawAmount) const;
	void ApplyRandomGood(ADFPlayerCharacter* Player) const;
	void ApplyRandomBad(ADFPlayerCharacter* Player) const;
	void AddItemToPlayer(ADFPlayerCharacter* Player, FName ItemRow, int32 Quantity) const;
};
