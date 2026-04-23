// Source/DungeonForged/Public/Interaction/UDFInteractionPromptWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UDFInteractionPromptWidget.generated.h"

class UImage;
class UTextBlock;

UCLASS()
class DUNGEONFORGED_API UDFInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DF|Interaction")
	void UpdatePrompt(const FText& InAction, UTexture2D* InKeyIcon, const FText& InSubHint = FText::GetEmpty());

	/** Primary focus: full opacity; nearby but not focus: can dim. */
	UFUNCTION(BlueprintCallable, Category = "DF|Interaction")
	void SetPrimaryFocus(bool bIsPrimary);

	void PlayEnterAnimation();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ActionText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InteractorHint = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> KeyIcon = nullptr;

	/** 5px bob amplitude, ~2 Hz. */
	UPROPERTY(EditAnywhere, Category = "DF|Interaction")
	float BobAmplitude = 5.f;

	UPROPERTY(EditAnywhere, Category = "DF|Interaction")
	float BobFrequency = 2.f;

	void ApplyFadeIn();
	void BobTick();

	FTimerHandle BobTimerHandle;
	float BobPhase = 0.f;
};
