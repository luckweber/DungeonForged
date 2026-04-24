// Source/DungeonForged/Public/Events/UDFRandomEventWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Events/DFEventData.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFRandomEventWidget.generated.h"

class UImage;
class UPanelWidget;
class UTexture2D;
class UDFEventChoiceButtonWidget;
class UDFEventOutcomeWidget;
class ADFPlayerState;

/**
 * Full-screen roguelike event card. Blueprint: **WBP_RandomEvent** — child button class **WBP_EventChoiceButton**.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class DUNGEONFORGED_API UDFRandomEventWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	/**
	 * Binds image/title/body and spawns 2–4 `ChoiceButtonClass` rows under `ChoiceListRoot`.
	 * @param bShowHintWhenMet When true, `OutcomeHint` on buttons uses `SelfHitTestInvisible` when idle (hint on hover only).
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|Events|UI")
	void PresentEvent(const FDFRandomEventRow& Row, FName SourceRowName, bool bShowHintWhenMet = true);

	/** C++: assign `WBP_EventChoiceButton` (native `UDFEventChoiceButtonWidget`). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Events|UI")
	TSubclassOf<UDFEventChoiceButtonWidget> ChoiceButtonClass = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Events|UI")
	TSubclassOf<UDFEventOutcomeWidget> OutcomeWidgetClass = nullptr;

protected:
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleChoiceClicked(int32 ChoiceIndex, FDFEventChoice Choice);

	void CommitChoice(const FDFEventChoice& Choice);

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UImage> EventIllustration = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<class UTextBlock> EventTitle = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<class UTextBlock> EventDescription = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UPanelWidget> ChoiceListRoot = nullptr;

	FName EventRowName = NAME_None;
	TArray<TObjectPtr<UDFEventChoiceButtonWidget>> SpawnedChoiceButtons;
};
