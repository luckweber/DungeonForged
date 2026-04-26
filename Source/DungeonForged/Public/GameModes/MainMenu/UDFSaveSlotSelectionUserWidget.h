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
	/** Garante @c SlotRow (e @c SlotCard0–2) se o nome no UMG não estiver alinhado ao @c meta=(BindWidget). */
	void ResolveWidgetBindings();
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

	/**
	 * Nome exato do @c UHorizontalBox no Designer (opção "é variável") se não usar @c meta=(BindWidget)
	 * com o identificador @c SlotRow. Padrão: "SlotRow".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|MainMenu|Slots|Binding")
	FName UmgNameSlotRow = FName("SlotRow");

	/** Alternativa: arraste 3 WBP_SaveSlotCard no Designer. */
	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UDFSaveSlotCardUserWidget> SlotCard0 = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UDFSaveSlotCardUserWidget> SlotCard1 = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UDFSaveSlotCardUserWidget> SlotCard2 = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ManageHintText = nullptr;

	UFUNCTION()
	void HandleSlotChanged(int32 SlotIndex);
};
