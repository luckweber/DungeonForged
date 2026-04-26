// Source/DungeonForged/Public/GameModes/MainMenu/UDFCreditsUserWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Input/Events.h"
#include "UDFCreditsUserWidget.generated.h"

class UButton;
class UScrollBox;
class APlayerController;

UCLASS(Blueprintable)
class DUNGEONFORGED_API UDFCreditsUserWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	/** Pixels / second, optional speed-up on input. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|MainMenu|Credits")
	float AutoScrollSpeed = 60.f;

	/** Tempo (s) que o scroll permanece no fim antes de fechar; <=0 desativa fecho automático. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|MainMenu|Credits")
	float AutoCloseHoldSeconds = 2.f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(
		const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UFUNCTION()
	void OnBackClicked();
	UFUNCTION()
	void OnSkipClicked();

	void JumpScrollToEnd();

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UScrollBox> CreditsScroll = nullptr;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UButton> BackButton = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UButton> SkipButton = nullptr;

	uint8 bSkipSpeedMultiplier : 1 = false;
	uint8 bReachedEnd : 1 = false;
	float TimeAtEndSeconds = 0.f;
};
