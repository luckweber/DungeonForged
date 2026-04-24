// Source/DungeonForged/Public/Events/UDFEventOutcomeWidget.h
#pragma once

#include "CoreMinimal.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFEventOutcomeWidget.generated.h"

class UTextBlock;

/** Outcome line shown after a random-event pick (`WBP_EventOutcome`). */
UCLASS(Abstract, Blueprintable, BlueprintType)
class DUNGEONFORGED_API UDFEventOutcomeWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	/** Fills text and removes itself after `DisplaySeconds` (default 2s). */
	UFUNCTION(BlueprintCallable, Category = "DF|Events|UI")
	void ShowOutcomeAndClose(const FText& Outcome, float DisplaySeconds = 2.f);

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> OutcomeText = nullptr;

	FTimerHandle CloseTimer;
};
