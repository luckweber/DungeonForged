// Source/DungeonForged/Public/GameModes/Run/UDFVictoryScreenWidget.h
#pragma once

#include "CoreMinimal.h"
#include "GameModes/Run/DFRunTypes.h"
#include "Blueprint/UserWidget.h"
#include "UDFVictoryScreenWidget.generated.h"

class UTextBlock;
class UButton;
class UScrollBox;
class UProgressBar;
class UNiagaraSystem;

/**
 * C++ base for WBP_VictoryScreen. Hook visuals in UMG; optional Niagara in @c GoldCelebrationNiagara.
 */
UCLASS(Blueprintable, Abstract)
class DUNGEONFORGED_API UDFVictoryScreenWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	UFUNCTION(BlueprintCallable, Category = "Run|UI")
	void SetSummary(FDFRunSummary const& Summary);

	/** Golden particle shower (e.g. NS_UI_Victory_Gold). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|VFX")
	TSoftObjectPtr<UNiagaraSystem> GoldCelebrationNiagara;

	/** @meta BindWidget: TitleText in WBP */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TimeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> KillsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GoldText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> AbilitiesCollected;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> UnlocksEarned;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> MetaXPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ReturnNexus;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> PlayAgain;

	UFUNCTION()
	void HandleReturnNexus();
	UFUNCTION()
	void HandlePlayAgain();
};
