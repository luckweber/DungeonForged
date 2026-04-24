// Source/DungeonForged/Public/Characters/ADFPlayerState.h

#pragma once

#include "CoreMinimal.h"
#include "DFDataTableStructs.h"
#include "UI/UDFAbilitySelectionSubsystem.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "ADFPlayerState.generated.h"

class UAbilitySystemComponent;
class UDataTable;
class UDFAttributeSet;
class UDFLevelingComponent;

UCLASS()
class DUNGEONFORGED_API ADFPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ADFPlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UDFAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Progression")
	TObjectPtr<UDFLevelingComponent> LevelingComponent;

	UFUNCTION(BlueprintPure, Category = "DF|Progression")
	UDFLevelingComponent* GetLevelingComponent() const { return LevelingComponent; }

	virtual void BeginPlay() override;

	/** Server / authority: grants every valid row in the table to this PlayerState's ASC. */
	UFUNCTION(BlueprintCallable, Category = "GAS")
	void GrantAbilitiesFromDataTable(UDataTable* AbilityTable);

	/**
	 * Server / authority: applies the startup Gameplay Effect from the named row (typically attribute base values).
	 * Call after InitAbilityActorInfo on the avatar so attributes are registered.
	 */
	UFUNCTION(BlueprintCallable, Category = "GAS")
	void InitializeAttributesFromDataTable(UDataTable* AttributeTable, FName RowName);

	/** Server rolled choices on floor clear; client opens the full-screen offer. */
	UFUNCTION(Client, Reliable, Category = "DF|Rogue")
	void Client_OpenAbilitySelectionScreen(int32 FloorCleared, const TArray<FDFAbilityRolledChoice>& OfferChoices, int32 SkipGold, int32 OfferId, float OptionalTimerSeconds);

	UFUNCTION(Client, Reliable, Category = "DF|Rogue")
	void Client_ResumeAfterAbilitySelection();

	/** Commit pick or skip. First valid resolution per `UDFDungeonManager::ActiveFloorOfferId` wins; others get resume only. */
	UFUNCTION(Server, Reliable, Category = "DF|Rogue")
	void Server_FinishAbilitySelection(int32 OfferId, bool bSkipped, FName SelectedRowName);
};
