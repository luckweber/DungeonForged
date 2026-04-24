// Source/DungeonForged/Public/GameModes/Run/ADFRunGameMode.h
#pragma once

#include "CoreMinimal.h"
#include "Data/DFDataTableStructs.h"
#include "GameModes/Run/DFRunTypes.h"
#include "GameFramework/GameModeBase.h"
#include "GameplayEffect.h"
#include "ADFRunGameMode.generated.h"

class AActor;
class ADFRunGameState;
class ADFPlayerController;
class ADFPlayerCharacter;
class AActor;
class UDFAttributeSet;
class UDFRunManager;
class UDFDungeonManager;
class UUserWidget;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFRunGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADFRunGameMode();

	/** Set in the editor; @c InitGame copies this into @c DefaultPawnClass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Classes")
	TSubclassOf<class ADFPlayerCharacter> DefaultPlayerClass;

	/** Set in the editor; copied to @c GameStateClass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Classes")
	TSubclassOf<ADFRunGameState> GameStateType;

	/** Set in the editor; copied to @c HUDClass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Classes")
	TSubclassOf<class ADFRunHUD> HUDType;

	/** Set in the editor; copied to @c PlayerControllerClass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Classes")
	TSubclassOf<class ADFRunPlayerController> RunPlayerControllerType;

	/** @c DT_Dungeon — @see UDFDungeonManager::DungeonFloorTable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Data")
	TObjectPtr<UDataTable> DungeonFloorTable = nullptr;

	/** Pushed to @ref UDFDungeonManager::EnemyDataTable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Data")
	TObjectPtr<UDataTable> EnemyDataTable = nullptr;

	/** @see @ref UDFDungeonManager::PCGOwnerActor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Data")
	TObjectPtr<AActor> PCGOwnerActor = nullptr;

	/** Merged in @ref UDFDungeonManager for spawn when PCG is not used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Data")
	TArray<FTransform> SpawnPointsPreview;

	/** 0 = no limit. Server compares @ref ADFRunGameState::ElapsedRunTime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Rules", meta = (ClampMin = "0"))
	float RunTimeLimit = 0.f;

	/** 1-based max floor; used with dungeon table; align with your last row (e.g. 10). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Rules", meta = (ClampMin = "1"))
	int32 MaxRunFloor = 10;

	/** If @ref UDFRunManager::GetArrivalReason() is @c None, this row name starts the run. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Rules")
	FName DefaultClassRowName = NAME_None;

	/** Optional instant effect after data-table base attributes (e.g. @c GE_ClassBaseStats). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|GAS")
	TSubclassOf<UGameplayEffect> ClassBaseStatsEffect;

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** New run: @c StartNewRun + optional GE, mesh, etc. (server / authority). */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void InitializePlayerFromClass(APlayerController* PC, FName ClassName);

	/** Between-floor UMG chain: Blueprint may extend; base sets phase, capture, and pauses. */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void TriggerBetweenFloorSequence();

protected:
	UFUNCTION()
	void HandleDungeonRunCompleted();
	UFUNCTION()
	void HandleRunEnemyKilled(AActor* Enemy);
	UFUNCTION()
	void HandlePlayerOutOfHealth();

	UFUNCTION()
	void HandleRunTimeExpired();

	void TriggerVictory();
	void TriggerDefeat();
	void ScheduleFinishVictoryToNexus();
	void ScheduleFinishDefeatToNexus();
	void UnbindPawnOutOfHealth(APlayerController* PC);
	void TryBindPawnOutOfHealth(APlayerController* PC);
	void UnbindAllDungeonDelegates();
	void RegisterDungeonDelegates();

	FTimerHandle DefeatAfterDeathAnimTimer;
	FTimerHandle VictoryEndTimer;
	FTimerHandle DefeatEndTimer;
	FTimerHandle RunTimeCheckTimer;
	FTimerHandle BetweenFloorEndTimer;

	bool bDefeatInProgress = false;
	TWeakObjectPtr<UDFAttributeSet> BoundAttributeSet;
};
