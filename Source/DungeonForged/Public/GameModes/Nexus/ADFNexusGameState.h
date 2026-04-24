// Source/DungeonForged/Public/GameModes/Nexus/ADFNexusGameState.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ADFNexusGameState.generated.h"

class UDataTable;
class UDFSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDFNexusMetaLevelUp, int32, NewLevel, int32, NewMetaXP);

UCLASS()
class DUNGEONFORGED_API ADFNexusGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ADFNexusGameState();

	/** Filled from @c UDFSaveGame on hub load. */
	UPROPERTY(BlueprintReadOnly, Category = "Nexus|Meta")
	int32 TotalRunsCompleted = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Nexus|Meta")
	int32 TotalRunsWon = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Nexus|Meta")
	int32 MetaXP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Nexus|Meta")
	int32 MetaLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Nexus|Meta")
	TArray<FName> UnlockedClasses;

	UPROPERTY(BlueprintReadOnly, Category = "Nexus|Meta")
	TArray<FName> UnlockedNPCs;

	UPROPERTY(BlueprintReadOnly, Category = "Nexus|Meta")
	TArray<FName> CompletedUpgrades;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nexus|Data")
	TObjectPtr<UDataTable> NexusLevelsTable = nullptr;

	UPROPERTY(BlueprintAssignable, Category = "Nexus|Meta")
	FOnDFNexusMetaLevelUp OnMetaLevelUp;

	UFUNCTION(BlueprintCallable, Category = "Nexus|Meta")
	void ApplyFromSave(UDFSaveGame* Save);

	UFUNCTION(BlueprintCallable, Category = "Nexus|Meta")
	void AddMetaXP(int32 Amount, UDFSaveGame* SaveToUpdate);

	/** Uses @c NexusLevelsTable for next @c MetaXPRequired thresholds. Pushes new unlock rows into @a SaveToUpdate @c PendingUnlocks when a row defines rewards. */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Meta")
	void CheckMetaLevelUp(UDFSaveGame* SaveToUpdate);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nexus|Meta")
	int32 GetMetaXPToNextLevel() const;

	/** Progress within the current nexus level band (for HUD bar). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nexus|Meta")
	float GetNexusXPFillRatio() const;
};
