// Source/DungeonForged/Public/UI/UDFAbilitySelectionWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/UDFAbilitySelectionSubsystem.h"
#include "UDFAbilitySelectionWidget.generated.h"

class ADFPlayerState;
class UButton;
class UDFAbilityCardWidget;
class UProgressBar;
class UTextBlock;

UCLASS()
class DUNGEONFORGED_API UDFAbilitySelectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Called on client from PlayerState; registers with the world subsystem. */
	UFUNCTION(BlueprintCallable, Category = "DF|Rogue")
	void InitializeOffer(ADFPlayerState* InPlayerState, int32 FloorCleared, int32 InSkipGold, int32 InOfferId, const TArray<FDFAbilityRolledChoice>& InChoices, float InTimerSeconds = 30.f);

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SubtitleText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UDFAbilityCardWidget> Card0 = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UDFAbilityCardWidget> Card1 = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UDFAbilityCardWidget> Card2 = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SkipButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SkipButtonLabel = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> TimerBar = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TimerText = nullptr;

	UFUNCTION()
	void HandleSkipClicked();

	UFUNCTION()
	void HandleCard0Clicked();
	UFUNCTION()
	void HandleCard1Clicked();
	UFUNCTION()
	void HandleCard2Clicked();

	void RequestFinish(bool bSkipped, FName RowName);
	void OnTimerExpired();
	void ApplyTimerVisual(float Remaining) const;
	void UnbindAndClearTimer();
	void UnbindLocalEvents();

	int32 TimerSeconds = 30;
	FTimerHandle CountdownHandle;

	TObjectPtr<ADFPlayerState> OwnerPlayerState = nullptr;
	int32 OfferId = 0;
	bool bRequestInFlight = false;
};
