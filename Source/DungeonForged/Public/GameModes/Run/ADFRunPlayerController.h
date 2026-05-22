// Source/DungeonForged/Public/GameModes/Run/ADFRunPlayerController.h
#pragma once

#include "CoreMinimal.h"
#include "GameModes/Run/DFRunTypes.h"
#include "Characters/ADFPlayerController.h"
#include "Input/DFInputConfig.h"
#include "ADFRunPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UUserWidget;
class UDFVictoryScreenWidget;
class UDFDefeatScreenWidget;
class UEnhancedInputComponent;
struct FInputActionValue;

/**
 * Run gameplay input lives on the PlayerController (not on @ref ADFPlayerCharacter BPs).
 * Configure IMC + Input Actions on the Run PlayerController Blueprint once for all classes.
 */
UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFRunPlayerController : public ADFPlayerController
{
	GENERATED_BODY()

public:
	ADFRunPlayerController();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Input")
	TObjectPtr<UInputMappingContext> GameplayInputMapping;

	/** Legacy name; if set, used when @ref GameplayInputMapping is null. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Input", meta = (DisplayName = "Default Gameplay IMC (legacy)"))
	TObjectPtr<UInputMappingContext> DefaultGameplayIMC;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Input", meta = (DisplayPriority = "1"))
	int32 IMC_Priority = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Input")
	TObjectPtr<UDFInputConfig> InputConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Input")
	TObjectPtr<UInputAction> IA_Move;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Input")
	TObjectPtr<UInputAction> IA_Look;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Input")
	TObjectPtr<UInputAction> IA_Jump;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Input")
	TObjectPtr<UInputAction> IA_CameraZoom;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Input")
	TObjectPtr<UInputAction> IA_Attack;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Input")
	TObjectPtr<UInputAction> IA_Ability1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Input")
	TObjectPtr<UInputAction> IA_Ability2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Input")
	TObjectPtr<UInputAction> IA_Ability3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Input")
	TObjectPtr<UInputAction> IA_Ability4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Input|Combat")
	TObjectPtr<UInputAction> IA_SecondaryAttack;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Input|AbilityBar")
	TArray<TObjectPtr<UInputAction>> IA_AbilityBarSlots;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Input")
	TObjectPtr<UInputAction> IA_Interact;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Input")
	TObjectPtr<UInputAction> IA_Sprint;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Input")
	TObjectPtr<UInputAction> IA_Dodge;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Input|LockOn")
	TObjectPtr<UInputAction> IA_LockOn;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Input|LockOn")
	TObjectPtr<UInputAction> IA_CycleLockOnLeft;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Input|LockOn")
	TObjectPtr<UInputAction> IA_CycleLockOnRight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Input")
	TObjectPtr<UInputAction> IA_EquipmentWeaponToggle;

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

	/** Pause (optional) + mouse cursor + UI-only input focused on @a Widget (defeat/victory/pause/inventory). */
	UFUNCTION(BlueprintCallable, Category = "Run|Input")
	void SetupInputModeUIForWidget(UUserWidget* Widget, bool bPauseGame = true);

	/** Game-only mode + IMC after possess, respawn, or leaving UI. */
	UFUNCTION(BlueprintCallable, Category = "Run|Input")
	void EnsureGameplayInputReady();

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
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;

	UInputMappingContext* ResolveGameplayInputMapping() const;
	void AddGameplayMappingContext();
	void RemoveGameplayMappingContext();
	void RegisterAbilityInputFromConfig(UEnhancedInputComponent* EIC);
	void BindAbilityBarSlotInputs(UEnhancedInputComponent* EIC);

	class ADFPlayerCharacter* GetHeroPawn() const;

	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_JumpStart();
	void Input_JumpEnd();
	void Input_CameraZoom(const FInputActionValue& Value);
	void Input_AttackPressed();
	void Input_AttackReleased();
	void Input_SecondaryAttack();
	void Input_Interact();
	void Input_SprintStart();
	void Input_SprintEnd();
	void Input_Dodge();
	void Input_LockOn();
	void Input_CycleLockOnLeft();
	void Input_CycleLockOnRight();
	void Input_EquipmentWeaponToggle();

	static constexpr float MinLookPitch = -60.f;
	static constexpr float MaxLookPitch = 60.f;

	bool bGameplayMappingContextAdded = false;
};
