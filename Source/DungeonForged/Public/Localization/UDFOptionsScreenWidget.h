// Source/DungeonForged/Public/Localization/UDFOptionsScreenWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Localization/DFAccessibilityData.h"
#include "Localization/DFLocalizationTypes.h"
#include "UI/UDFUserWidgetBase.h"

#include "UDFOptionsScreenWidget.generated.h"

class UButton;
class UCheckBox;
class UComboBoxString;
class UListView;
class UPanelWidget;
class USlider;
class UTextBlock;
class UTileView;
class UUserWidget;
class UWidget;
class UWidgetSwitcher;

/** Matches @c MainTabSwitcher child order (Audio=0 … Language=4). */
UENUM(BlueprintType)
enum class EDFOptionsTab : uint8
{
	Audio         UMETA(DisplayName = "Audio"),
	Graphics      UMETA(DisplayName = "Graphics"),
	Controls      UMETA(DisplayName = "Controls"),
	Accessibility UMETA(DisplayName = "Accessibility"),
	Language      UMETA(DisplayName = "Language"),
};

/** Fired when the active options tab changes (@c ShowTabByIndex / tab buttons). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDFOptionsTabChanged, int32, TabIndex, EDFOptionsTab, Tab);

UCLASS(Blueprintable, BlueprintType)
class DUNGEONFORGED_API UDFOptionsScreenWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	/** Mappable `MappingName` values, same order as rows to spawn (WBP_KeyBindRow class). */
	UPROPERTY(EditAnywhere, Category = "DF|Options|Controls")
	TArray<FName> KeyBindOrder;

	UPROPERTY(EditAnywhere, Category = "DF|Options|Controls")
	TSubclassOf<UUserWidget> KeyBindRowClass;

	UFUNCTION(BlueprintCallable, Category = "DF|Options|Tabs")
	void ShowTabByIndex(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "DF|Options|Tabs")
	void ShowTab(EDFOptionsTab Tab);

	/**
	 * Sets active tab index, updates @c MainTabSwitcher, and fires @c OnTabChanged when the index changes.
	 * Callable from WBP Event Graph (custom tab buttons, animations, etc.).
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|Options|Tabs", meta = (AdvancedDisplay = "bNotifyEvenIfUnchanged"))
	void SetCurrentTabIndex(int32 Index, bool bNotifyEvenIfUnchanged = false);

	/** Index of the active tab (same as @c MainTabSwitcher active index). */
	UFUNCTION(BlueprintPure, Category = "DF|Options|Tabs")
	int32 GetCurrentTabIndex() const { return CurrentTabIndex; }

	UFUNCTION(BlueprintPure, Category = "DF|Options|Tabs")
	EDFOptionsTab GetCurrentTab() const { return static_cast<EDFOptionsTab>(CurrentTabIndex); }

	/** Read-only mirror for UMG bindings / Blueprint (updated by @c ShowTabByIndex). */
	UPROPERTY(BlueprintReadOnly, Category = "DF|Options|Tabs")
	int32 CurrentTabIndex = 0;

	/** Assign in WBP Designer: Event Graph → On Tab Changed. */
	UPROPERTY(BlueprintAssignable, Category = "DF|Options|Tabs|Events")
	FOnDFOptionsTabChanged OnTabChanged;

	UFUNCTION(BlueprintCallable, Category = "DF|Options|Tabs")
	void CloseOptions();

	UFUNCTION(BlueprintCallable, Category = "DF|Options|Controls")
	void ResetKeybindsToDefaults();

	/** Apply accessibility + write save (Accessibility tab). */
	UFUNCTION(BlueprintCallable, Category = "DF|Options|Accessibility")
	void ApplyAndSaveAccessibility();

	UFUNCTION(BlueprintCallable, Category = "DF|Options|Audio")
	void ApplyAudioFromSliders();

	UFUNCTION(BlueprintCallable, Category = "DF|Options|Audio")
	void SyncAudioFromSubsystem();

	UFUNCTION(BlueprintCallable, Category = "DF|Options|Language")
	void RefreshLanguagePreview();

	UFUNCTION(BlueprintCallable, Category = "DF|Options|Controls")
	void BeginRebindForMapping(FName MappingName);

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UFUNCTION()
	void OnFontScaleSliderValue(float Value);
	UFUNCTION()
	void OnColorBlindComboSelectionChanged(FString Selection, ESelectInfo::Type SelectType);
	UFUNCTION()
	void OnHighContrastToggled(bool bOn);
	UFUNCTION()
	void OnReduceMotionToggled(bool bOn);
	UFUNCTION()
	void OnColorBlindToggled(bool bOn);
	UFUNCTION()
	void OnLanguageItemSelected(UObject* Item);
	UFUNCTION()
	void OnMasterChanged(float Value);
	UFUNCTION()
	void OnMusicChanged(float Value);
	UFUNCTION()
	void OnSfxChanged(float Value);
	UFUNCTION()
	void OnVoiceChanged(float Value);
	UFUNCTION()
	void OnCameraShakeIntensityChanged(float Value);
	UFUNCTION()
	void OnHitStopIntensityChanged(float Value);
	UFUNCTION()
	void OnShowDamageNumbersToggled(bool bOn);

	UFUNCTION()
	void OnTabButtonAudio() { ShowTabByIndex(0); }
	UFUNCTION()
	void OnTabButtonGraphics() { ShowTabByIndex(1); }
	UFUNCTION()
	void OnTabButtonControls() { ShowTabByIndex(2); }
	UFUNCTION()
	void OnTabButtonAccessibility() { ShowTabByIndex(3); }
	UFUNCTION()
	void OnTabButtonLanguage() { ShowTabByIndex(4); }

	UFUNCTION()
	void OnBackButtonClicked();

	int32 ClampTabIndex(int32 Index) const;

	/** Override in WBP (Event Graph → Override Functions) for tab highlight / SFX. */
	UFUNCTION(BlueprintImplementableEvent, Category = "DF|Options|Tabs|Events")
	void OnTabIndexChanged(int32 NewTabIndex, EDFOptionsTab NewTab);

	void NotifyTabChanged();

	void BuildKeybindRows();
	void RebuildColorBlindCombo();
	EDFColorBlindMode ParseColorBlindCombo();
	void SetPendingLanguage(EDFLanguage L);
	void SyncAccessibilityFromSubsystem();

	/* --- Layout (WBP_BP_OptionsScreen names must match) --- */

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> MainTabSwitcher = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_TabAudio = nullptr;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_TabGraphics = nullptr;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_TabControls = nullptr;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_TabAccessibility = nullptr;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_TabLanguage = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Back = nullptr;

	/* Audio */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> Slider_Master = nullptr;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> Slider_Music = nullptr;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> Slider_SFX = nullptr;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> Slider_Voice = nullptr;

	/* Controls */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> Panel_KeyBinds = nullptr;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_ResetKeybinds = nullptr;

	/* Accessibility */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> Slider_UIFont = nullptr;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> Check_HighContrast = nullptr;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> Check_ReduceMotion = nullptr;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> Check_ColorBlind = nullptr;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> Combo_ColorBlind = nullptr;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_ApplyAccessibility = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> Slider_CameraShakeIntensity = nullptr;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> Slider_HitStopIntensity = nullptr;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> Check_ShowDamageNumbers = nullptr;

	/* Language */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTileView> LanguageTileView = nullptr;

	/** e.g. LOCTABLE sample line for the pending culture. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_LanguagePreview = nullptr;

	UPROPERTY(Transient)
	FName RebindTargetMapping = NAME_None;

	/** Staged for preview in the language tab. */
	EDFLanguage PendingLanguage = EDFLanguage::English;
};
