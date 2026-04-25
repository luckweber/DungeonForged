// Source/DungeonForged/Public/GameModes/MainMenu/UDFSaveSlotSelectionUserWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameModes/MainMenu/DFMainMenuTypes.h"
#include "UDFSaveSlotSelectionUserWidget.generated.h"

class UTextBlock;
class UButton;
class UHorizontalBox;
class UDFSaveSlotCardUserWidget;
class APlayerController;
class UDFSaveGame;

UCLASS(Blueprintable)
class DUNGEONFORGED_API UDFSaveSlotSelectionUserWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "DF|MainMenu|Slots")
	void SetScreenMode(EDFSlotScreenMode const Mode) { CurrentMode = Mode; }

	UFUNCTION(BlueprintCallable, Category = "DF|MainMenu|Slots")
	void RefreshFromSubsystem();

	/** Called from WBP of each card when a slot is chosen. */
	UFUNCTION(BlueprintCallable, Category = "DF|MainMenu|Slots")
	void OnProfileSlotPickedForPlay(int32 SlotIndex);

	/** Delete flow. */
	UFUNCTION(BlueprintCallable, Category = "DF|MainMenu|Slots")
	void OnRequestDeleteSlot(int32 const SlotIndex);

	UFUNCTION()
	void ExecuteDeleteAfterConfirm(int32 const SlotIndex);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void OnBackClicked();

	void UpdateTitle() const;
	void RebuildCardWidgets();

	EDFSlotScreenMode CurrentMode = EDFSlotScreenMode::SelectToPlay;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> SlotRow = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackButton = nullptr;

	/** Preenchido no Blueprint: WBP_SaveSlotCard x3, ou crie @a SlotCardClass C++. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|MainMenu|Slots")
	TSubclassOf<UDFSaveSlotCardUserWidget> SlotCardClass = nullptr;
};
