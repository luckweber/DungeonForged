// Source/DungeonForged/Public/Events/UDFEventChoiceButtonWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Events/DFEventData.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFEventChoiceButtonWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDFEventChoiceButtonClicked, int32, ChoiceIndex, FDFEventChoice, Choice);

/**
 * One row in `WBP_RandomEvent` (choice label + optional hover hint for locks / risk).
 * Blueprint name: **WBP_EventChoiceButton** — `WidgetTree` names must match `BindWidget`.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class DUNGEONFORGED_API UDFEventChoiceButtonWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DF|Events|UI")
	void Configure(
		int32 InIndex,
		const FDFEventChoice& InChoice,
		const FText& InHint,
		bool bInEnabled,
		ESlateVisibility InHintWhenIdleVisibility = ESlateVisibility::Collapsed);

	UPROPERTY(BlueprintAssignable, Category = "DF|Events|UI")
	FOnDFEventChoiceButtonClicked OnChoiceClicked;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandleHovered();

	UFUNCTION()
	void HandleUnhovered();

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UButton> MainButton = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> ChoiceText = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> OutcomeHint = nullptr;

	int32 Index = 0;
	FDFEventChoice ChoicePayload;
	bool bPayloadEnabled = true;
	ESlateVisibility HintIdleVis = ESlateVisibility::Collapsed;
};
