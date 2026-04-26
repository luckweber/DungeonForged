// Source/DungeonForged/Public/GameModes/MainMenu/UDFSaveSlotCardUserWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameModes/MainMenu/DFMainMenuTypes.h"
#include "UDFSaveSlotCardUserWidget.generated.h"

class UTextBlock;
class UImage;
class UButton;
class UProgressBar;
class UWrapBox;
class UWidget;
class UWidgetSwitcher;
class UDFSaveGame;
class UWidgetAnimation;
class USoundBase;

UCLASS(Blueprintable)
class DUNGEONFORGED_API UDFSaveSlotCardUserWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	/** Fill labels, images, and visibility from the save subsystem. */
	UFUNCTION(BlueprintCallable, Category = "DF|MainMenu|Slots")
	void RefreshSlotData(int32 InSlotIndex, EDFSlotScreenMode InMode);

	/** Legacy wrapper used by WBP. */
	UFUNCTION(BlueprintCallable, Category = "DF|MainMenu|Slots", meta = (DisplayName = "Setup For Slot (Legacy)"))
	void SetupForSlot(
		int32 InSlotIndex, EDFSlotScreenMode InMode, UDFSaveGame* InSaveOrNull, bool bEmptyOnDisk)
		{ (void)InSaveOrNull; RefreshSlotData(InSlotIndex, InMode); }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION() void OnPlayClicked();
	UFUNCTION() void OnNewRunClicked();
	UFUNCTION() void OnCreateClicked();
	UFUNCTION() void OnDeleteClicked();

	void ShowDeleteConfirm(UDFSaveGame* Data);
	void ShowNewRunAbandonConfirm(UDFSaveGame* Data);
	void AfterNewRunCleared();

	/** WBP: nomes de variáveis comuns; usado se @c meta=(BindWidget) faltar. */
	void ResolveOptionalWidgetNames();

	/** Borda 1: cinza = vazio, ouro = perfil com save (nome opcional "SlotBorderImage"). */
	void ApplySlotBorderState(bool bEmpty);

	/** Aplica @c StateSwitcher (se configurado) e fallback @c EmptyRoot/OccupiedRoot. */
	void ApplyStateSwitch(bool bEmpty);

	void ApplyEmptyState(bool bManage);
	void ApplyOccupiedState(UDFSaveGame* Data, bool bManage);

	int32 SlotIndex = 0;
	EDFSlotScreenMode Mode = EDFSlotScreenMode::SelectToPlay;

	/**
	 * Switcher recomendado: 1 filho “Empty” + 1 filho “Occupied”.
	 * O C++ alterna por @c SetActiveWidgetIndex, sem manipular visibilidade
	 * de cada widget — útil para alinhar 3 cartões com tamanho fixo.
	 */
	UPROPERTY(Transient, meta = (BindWidgetOptional)) TObjectPtr<UWidgetSwitcher> StateSwitcher = nullptr;

	/** Índices do switcher (Designer pode reordenar e o C++ continua válido). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|MainMenu|Slots|Switcher")
	int32 SwitcherEmptyIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|MainMenu|Slots|Switcher")
	int32 SwitcherOccupiedIndex = 1;

	// -- Occupied --
	UPROPERTY(Transient, meta = (BindWidgetOptional)) TObjectPtr<UImage> SlotBorderImage = nullptr;
	UPROPERTY(Transient, meta = (BindWidgetOptional)) TObjectPtr<UImage> ClassPortraitArt = nullptr;
	UPROPERTY(Transient, meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> SlotLabel = nullptr;
	UPROPERTY(Transient, meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ClassNameText = nullptr;
	UPROPERTY(Transient, meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MetaLevelText = nullptr;
	UPROPERTY(Transient, meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> MetaXPBar = nullptr;
	UPROPERTY(Transient, meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> LastFloorText = nullptr;
	UPROPERTY(Transient, meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TotalRunsText = nullptr;
	UPROPERTY(Transient, meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> PlayTimeText = nullptr;
	UPROPERTY(Transient, meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> LastPlayedText = nullptr;
	UPROPERTY(Transient, meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> IncompatibleVersionText = nullptr;
	UPROPERTY(Transient, meta = (BindWidgetOptional)) TObjectPtr<UWrapBox> UnlockedClassIcons = nullptr;
	UPROPERTY(Transient, meta = (BindWidgetOptional)) TObjectPtr<UButton> PlayButton = nullptr;
	UPROPERTY(Transient, meta = (BindWidgetOptional)) TObjectPtr<UButton> NewRunButton = nullptr;
	UPROPERTY(Transient, meta = (BindWidgetOptional)) TObjectPtr<UButton> DeleteButton = nullptr;
	UPROPERTY(Transient, meta = (BindWidgetOptional)) TObjectPtr<UImage> ActiveRunBadge = nullptr;
	UPROPERTY(Transient, meta = (BindWidgetOptional)) TObjectPtr<UWidget> OccupiedRoot = nullptr;

	// -- Empty --
	UPROPERTY(Transient, meta = (BindWidgetOptional)) TObjectPtr<UImage> EmptySlotArt = nullptr;
	UPROPERTY(Transient, meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> EmptyText = nullptr;
	UPROPERTY(Transient, meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> HintText = nullptr;
	UPROPERTY(Transient, meta = (BindWidgetOptional)) TObjectPtr<UButton> CreateButton = nullptr;
	UPROPERTY(Transient, meta = (BindWidgetOptional)) TObjectPtr<UWidget> EmptyRoot = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim, AllowPrivateAccess), BlueprintReadOnly, Category = "DF|MainMenu|Slots")
	TObjectPtr<UWidgetAnimation> CardRefreshAnim = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|MainMenu|Slots|Audio")
	TObjectPtr<USoundBase> DeleteSound = nullptr;
};
