// Source/DungeonForged/Public/GameModes/MainMenu/ADFMainMenuHUD.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GameModes/MainMenu/DFMainMenuTypes.h"
#include "ADFMainMenuHUD.generated.h"

class APlayerController;
class UUserWidget;
class UDFSplashScreenUserWidget;
class UDFMainMenuUserWidget;
class UDFConfirmDialogUserWidget;
class UDFCreditsUserWidget;
class UDFSaveSlotSelectionUserWidget;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFMainMenuHUD : public AHUD
{
	GENERATED_BODY()
public:
	ADFMainMenuHUD();

	/** @c AGameModeBase::PostLogin: show splash, then the rest of the flow. */
	void OnLocalPlayerMenuReady(APlayerController* ForPC);

	/** Remove splash, show main; first-time slot flow may skip the main and show @c WBP_SaveSlotSelection first. */
	UFUNCTION(BlueprintCallable, Category = "DF|MainMenu|UI")
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable, Category = "DF|MainMenu|UI")
	void ShowSaveSlotLayer(EDFSlotScreenMode Mode);

	UFUNCTION(BlueprintCallable, Category = "DF|MainMenu|UI")
	void HideSaveSlotLayer();

	UFUNCTION(BlueprintCallable, Category = "DF|MainMenu|UI")
	void ShowCredits();

	UFUNCTION(BlueprintCallable, Category = "DF|MainMenu|UI")
	void ShowConfirmDialog(UDFConfirmDialogUserWidget* Inst);

	/** Devolve o foco UI ao menu principal (pós-fecho de Credits/Options/etc.). */
	UFUNCTION(BlueprintCallable, Category = "DF|MainMenu|UI")
	void RestoreMainMenuFocus();

	/**
	 * Retira Main + slots do viewport (@c RemoveFromParent) para o 3D aparecer sob WBP_ClassSelection.
	 * Com false, volta a @c AddToViewport com Z @c DFMainMenuUI (só o que estava visível antes).
	 * @return true se removeu ou repôs widgets; false se não havia nada a fazer.
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|MainMenu|UI")
	bool SuppressUnderlyingMenuForClassSelectionWorldPreview(bool bSuppress);

	/** Após fechar escolha de classe (modo mundo): foco em slots se visíveis, senão menu principal. */
	UFUNCTION(BlueprintCallable, Category = "DF|MainMenu|UI")
	void RestoreFocusAfterClassSelectionWorldPreview();

	/** Blueprint child sets classes in Defaults; parent C++ is @c WBP_*. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|MainMenu|UI")
	TSubclassOf<UDFSplashScreenUserWidget> SplashWidgetClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|MainMenu|UI")
	TSubclassOf<UDFMainMenuUserWidget> MainMenuWidgetClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|MainMenu|UI")
	TSubclassOf<UDFCreditsUserWidget> CreditsWidgetClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|MainMenu|UI")
	TSubclassOf<UDFConfirmDialogUserWidget> ConfirmWidgetClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|MainMenu|UI")
	TSubclassOf<UDFSaveSlotSelectionUserWidget> SaveSlotWidgetClass = nullptr;

	/** Option overlay (Prompt 45). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|MainMenu|UI")
	TSubclassOf<UUserWidget> OptionsWidgetClass = nullptr;

	/** Achievement list (Prompt 61). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|MainMenu|UI")
	TSubclassOf<UUserWidget> AchievementListWidgetClass = nullptr;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|MainMenu|UI")
	TObjectPtr<UDFSplashScreenUserWidget> Splash = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|MainMenu|UI")
	TObjectPtr<UDFMainMenuUserWidget> Main = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|MainMenu|UI")
	TObjectPtr<UDFCreditsUserWidget> Credits = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|MainMenu|UI")
	TObjectPtr<UDFSaveSlotSelectionUserWidget> SaveSlotLayer = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|MainMenu|UI")
	TObjectPtr<UUserWidget> Options = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|MainMenu|UI")
	TObjectPtr<UUserWidget> Achievements = nullptr;

	bool bDetachedMenusForWorldClassSelection = false;
	bool bHadMainInViewportBeforeWorldClassSelection = false;
	bool bHadSaveSlotInViewportBeforeWorldClassSelection = false;
};
