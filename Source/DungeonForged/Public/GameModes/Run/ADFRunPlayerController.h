// Source/DungeonForged/Public/GameModes/Run/ADFRunPlayerController.h
#pragma once

#include "CoreMinimal.h"
#include "GameModes/Run/DFRunTypes.h"
#include "Characters/ADFPlayerController.h"
#include "ADFRunPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UDFVictoryScreenWidget;
class UDFDefeatScreenWidget;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFRunPlayerController : public ADFPlayerController
{
	GENERATED_BODY()

public:
	ADFRunPlayerController();

	/** IMC for gameplay; if null, run tries the possessed @ref ADFPlayerCharacter::IMC_Default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Input")
	TObjectPtr<UInputMappingContext> DefaultGameplayIMC;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Input", meta = (DisplayPriority = "1"))
	int32 IMC_Priority = 0;

	/** WBP_CharacterScreen — inventory / build. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|UI")
	TSubclassOf<UUserWidget> CharacterScreenClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Run|UI")
	TObjectPtr<UUserWidget> CharacterScreenInstance;

	/** WBP_PauseMenu / options entry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|UI")
	TSubclassOf<UUserWidget> PauseMenuClass;

	/** WBP_OptionsScreen — opened from pause. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|UI")
	TSubclassOf<UUserWidget> OptionsScreenClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Run|UI")
	TObjectPtr<UUserWidget> PauseMenuInstance;

	UFUNCTION(BlueprintCallable, Category = "Run|Input")
	void SetupInputModeGameplay();

	/** Pauses the game; shows cursor. */
	UFUNCTION(BlueprintCallable, Category = "Run|Input")
	void SetupInputModeUI();

	UFUNCTION(BlueprintCallable, Category = "Run|UI")
	void ToggleInventory();

	UFUNCTION(BlueprintCallable, Category = "Run|UI")
	void OnPause();

	UFUNCTION(BlueprintCallable, Category = "Run|UI")
	void CloseCharacterScreen();

	UFUNCTION(BlueprintCallable, Category = "Run|UI")
	void ClosePauseMenu();

	//~ End screens: called from @ref ADFRunGameMode (server) via these Client RPCs.

	UFUNCTION(Client, Reliable, Category = "Run|UI")
	void Client_OpenVictoryScreen(FDFRunSummary Summary);

	UFUNCTION(Client, Reliable, Category = "Run|UI")
	void Client_OpenDefeatScreen(FDFRunSummary Summary, const FString& DefeatCause);

	/**
	 * Between floors: UMG/Blueprint can drive level-up, ability pick (often already in @c UDFDungeonManager),
	 * events, and transition. Default does nothing; override in BPC or WBP.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Run|UI")
	void PresentBetweenFloorFlow();
	virtual void PresentBetweenFloorFlow_Implementation();

	UFUNCTION(Client, Reliable, Category = "Run|UI")
	void Client_PresentBetweenFloorUI();

	/** Server: continue after a Blueprint-driven between-floor step (optional; default travel uses UDFWorldTransitionSubsystem). */
	UFUNCTION(Server, Reliable, Category = "Run|UI", WithValidation)
	void Server_FinishBetweenFloorUI();

	/** C++: assign WBP_VictoryScreen parent; used by @a Client_OpenVictoryScreen. */
	UPROPERTY(EditDefaultsOnly, Category = "Run|UI")
	TSubclassOf<UDFVictoryScreenWidget> VictoryScreenWidgetClass;

	/** C++: assign WBP_DefeatScreen parent. */
	UPROPERTY(EditDefaultsOnly, Category = "Run|UI")
	TSubclassOf<UDFDefeatScreenWidget> DefeatScreenWidgetClass;

	/** Client or listen host: nexus from end screens (routes through server for @c ServerTravel). */
	UFUNCTION(BlueprintCallable, Category = "Run|UI")
	void RequestReturnToNexus(ERunNexusTravelReason Reason);

	UFUNCTION(Server, Reliable, WithValidation, Category = "Run|UI")
	void Server_RequestReturnToNexus(ERunNexusTravelReason Reason);

	/** Client: re-enter run (implementation: travel URL / save slot). */
	UFUNCTION(BlueprintCallable, Category = "Run|UI")
	void RequestPlayAgain();

protected:
	virtual void OnPossess(APawn* InPawn) override;
};
