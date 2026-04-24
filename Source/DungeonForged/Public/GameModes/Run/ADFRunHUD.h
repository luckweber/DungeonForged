// Source/DungeonForged/Public/GameModes/Run/ADFRunHUD.h
#pragma once

#include "CoreMinimal.h"
#include "UI/ADFHUDBase.h"
#include "GameModes/Run/DFRunTypes.h"
#include "ADFRunHUD.generated.h"

class ADFRunGameState;
class UUserWidget;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFRunHUD : public ADFHUDBase
{
	GENERATED_BODY()

public:
	ADFRunHUD();

	/** 0-9: main HUD. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|UI")
	TSubclassOf<UUserWidget> WBP_HUDClass;

	/** ZOrder default 0 (corner) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|UI")
	TSubclassOf<UUserWidget> WBP_MinimapClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|UI")
	TSubclassOf<UUserWidget> WBP_StatusEffectBarClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|UI")
	TSubclassOf<UUserWidget> WBP_BossHealthBarClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|UI")
	TSubclassOf<UUserWidget> WBP_LockOnIndicatorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|UI")
	TSubclassOf<UUserWidget> WBP_FloorCounterClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|UI")
	TSubclassOf<UUserWidget> WBP_KillCounterClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Run|UI")
	TObjectPtr<UUserWidget> WBP_HUD;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Run|UI")
	TObjectPtr<UUserWidget> WBP_Minimap;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Run|UI")
	TObjectPtr<UUserWidget> WBP_StatusEffectBar;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Run|UI")
	TObjectPtr<UUserWidget> WBP_BossHealthBar;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Run|UI")
	TObjectPtr<UUserWidget> WBP_LockOnIndicator;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Run|UI")
	TObjectPtr<UUserWidget> WBP_FloorCounter;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Run|UI")
	TObjectPtr<UUserWidget> WBP_KillCounter;

	UFUNCTION()
	void OnRunPhaseChanged(ERunPhase NewPhase, ERunPhase OldPhase);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void CreateRunWidgets();
	void SetCombatWidgetsVisible(bool bVisible);
	void ShowBossHUD(bool bShow);
};
