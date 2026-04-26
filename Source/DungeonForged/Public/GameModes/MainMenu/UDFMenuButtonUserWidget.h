// Source/DungeonForged/Public/GameModes/MainMenu/UDFMenuButtonUserWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UDFMenuButtonUserWidget.generated.h"

class UButton;
class UTextBlock;
class UWidgetAnimation;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDFMenuButtonClicked);

/**
 * WBP_MenuButton: parchment-style hit area + label + optional sub-label.
 * Plays UMG animations and optional 2D sounds on interaction.
 */
UCLASS()
class DUNGEONFORGED_API UDFMenuButtonUserWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	/** Label on the primary line. */
	UFUNCTION(BlueprintCallable, Category = "DF|MainMenu|Button")
	void SetButtonLabelText(FText const& InText);

	/** Optional secondary line. */
	UFUNCTION(BlueprintCallable, Category = "DF|MainMenu|Button")
	void SetSubLabelText(FText const& InText, bool bShow = true);

	UPROPERTY(BlueprintAssignable, Category = "DF|MainMenu|Button")
	FOnDFMenuButtonClicked OnMenuButtonClicked;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION() void OnBtnHovered();
	UFUNCTION() void OnBtnUnhovered();
	UFUNCTION() void OnBtnPressed();
	UFUNCTION() void OnBtnReleased();
	UFUNCTION() void OnBtnClicked();

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UButton> Button = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ButtonLabel = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SubLabel = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim, AllowPrivateAccess), BlueprintReadOnly, Category = "DF|MainMenu|Button")
	TObjectPtr<UWidgetAnimation> HoverAnim = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim, AllowPrivateAccess), BlueprintReadOnly, Category = "DF|MainMenu|Button")
	TObjectPtr<UWidgetAnimation> PressAnim = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|MainMenu|Button|Audio")
	TObjectPtr<USoundBase> HoverSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|MainMenu|Button|Audio")
	TObjectPtr<USoundBase> ClickSound = nullptr;
};
