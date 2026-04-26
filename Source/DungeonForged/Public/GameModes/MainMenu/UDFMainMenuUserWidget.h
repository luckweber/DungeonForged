// Source/DungeonForged/Public/GameModes/MainMenu/UDFMainMenuUserWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UDFMainMenuUserWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;

UCLASS(Blueprintable)
class DUNGEONFORGED_API UDFMainMenuUserWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "DF|MainMenu|UI")
	void RefreshForCurrentSaveState();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** e.g. scale/glow "Nova Aventura" on first run or when no save exists. */
	UFUNCTION(BlueprintNativeEvent, Category = "DF|MainMenu|UI")
	void OnNewAdventureEmphasisChanged(bool bEmphasize);
	virtual void OnNewAdventureEmphasisChanged_Implementation(bool bEmphasize);

	UFUNCTION()
	void OnContinueAdventure();
	UFUNCTION()
	void OnNewAdventure();
	UFUNCTION()
	void OnManageProfiles();
	UFUNCTION()
	void OnOptions();
	UFUNCTION()
	void OnAchievements();
	UFUNCTION()
	void OnCredits();
	UFUNCTION()
	void OnQuit();

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UButton> ContinueAdventureButton = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UButton> NewAdventureButton = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UButton> ManageProfilesButton = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UButton> OptionsButton = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UButton> AchievementsButton = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UButton> CreditsButton = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UButton> QuitButton = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UImage> LogoImage = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SubtitleText = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VersionText = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CopyrightText = nullptr;

	/** e.g. "Andar 3 — Guerreiro" when a run is in progress. */
	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ContinueSubText = nullptr;
};
