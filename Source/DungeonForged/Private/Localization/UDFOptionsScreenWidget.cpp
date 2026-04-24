// Source/DungeonForged/Private/Localization/UDFOptionsScreenWidget.cpp
#include "Localization/UDFOptionsScreenWidget.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/PanelWidget.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "InputCoreTypes.h"
#include "Localization/UDFAccessibilitySubsystem.h"
#include "Localization/UDFInputRemappingSubsystem.h"
#include "Localization/UDFKeyBindRowWidget.h"
#include "Localization/UDFLocalizationSubsystem.h"
#include "Localization/UDFUILanguageListEntry.h"

void UDFOptionsScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_TabAudio) { Button_TabAudio->OnClicked.AddDynamic(this, &UDFOptionsScreenWidget::OnTabButtonAudio); }
	if (Button_TabGraphics) { Button_TabGraphics->OnClicked.AddDynamic(this, &UDFOptionsScreenWidget::OnTabButtonGraphics); }
	if (Button_TabControls) { Button_TabControls->OnClicked.AddDynamic(this, &UDFOptionsScreenWidget::OnTabButtonControls); }
	if (Button_TabAccessibility) { Button_TabAccessibility->OnClicked.AddDynamic(this, &UDFOptionsScreenWidget::OnTabButtonAccessibility); }
	if (Button_TabLanguage) { Button_TabLanguage->OnClicked.AddDynamic(this, &UDFOptionsScreenWidget::OnTabButtonLanguage); }

	if (Slider_Master) { Slider_Master->OnValueChanged.AddDynamic(this, &UDFOptionsScreenWidget::OnMasterChanged); }
	if (Slider_Music) { Slider_Music->OnValueChanged.AddDynamic(this, &UDFOptionsScreenWidget::OnMusicChanged); }
	if (Slider_SFX) { Slider_SFX->OnValueChanged.AddDynamic(this, &UDFOptionsScreenWidget::OnSfxChanged); }
	if (Slider_Voice) { Slider_Voice->OnValueChanged.AddDynamic(this, &UDFOptionsScreenWidget::OnVoiceChanged); }

	if (Button_ResetKeybinds) { Button_ResetKeybinds->OnClicked.AddDynamic(this, &UDFOptionsScreenWidget::ResetKeybindsToDefaults); }

	if (Slider_UIFont) { Slider_UIFont->OnValueChanged.AddDynamic(this, &UDFOptionsScreenWidget::OnFontScaleSliderValue); }
	if (Check_HighContrast) { Check_HighContrast->OnCheckStateChanged.AddDynamic(this, &UDFOptionsScreenWidget::OnHighContrastToggled); }
	if (Check_ReduceMotion) { Check_ReduceMotion->OnCheckStateChanged.AddDynamic(this, &UDFOptionsScreenWidget::OnReduceMotionToggled); }
	if (Check_ColorBlind) { Check_ColorBlind->OnCheckStateChanged.AddDynamic(this, &UDFOptionsScreenWidget::OnColorBlindToggled); }
	if (Combo_ColorBlind) { Combo_ColorBlind->OnSelectionChanged.AddDynamic(this, &UDFOptionsScreenWidget::OnColorBlindComboSelectionChanged); }
	if (Button_ApplyAccessibility) { Button_ApplyAccessibility->OnClicked.AddDynamic(this, &UDFOptionsScreenWidget::ApplyAndSaveAccessibility); }

	RebuildColorBlindCombo();
	SyncAudioFromSubsystem();
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFInputRemappingSubsystem* const RS = GI->GetSubsystem<UDFInputRemappingSubsystem>())
		{
			RS->RegisterInputMappingContextForLocalPlayer(GetOwningLocalPlayer());
		}
	}
	BuildKeybindRows();

	if (LanguageTileView)
	{
		LanguageTileView->OnItemSelectionChanged().AddUObject(
			this, &UDFOptionsScreenWidget::OnLanguageItemSelected);
		LanguageTileView->ClearListItems();
		for (int32 I = 0; I < 4; ++I)
		{
			const EDFLanguage L = static_cast<EDFLanguage>(I);
			UDFUILanguageListEntry* const E = NewObject<UDFUILanguageListEntry>(this);
			E->Language = L;
			if (UGameInstance* const GI = GetGameInstance())
			{
				if (const UDFLocalizationSubsystem* const Loc = GI->GetSubsystem<UDFLocalizationSubsystem>())
				{
					const TArray<FText> Names = Loc->GetAvailableLanguageDisplayNames();
					if (Names.IsValidIndex(I))
					{
						E->DisplayName = Names[I];
					}
				}
			}
			LanguageTileView->AddItem(E);
		}
		RefreshLanguagePreview();
	}
}

FReply UDFOptionsScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (RebindTargetMapping == NAME_None)
	{
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}
	const FKey K = InKeyEvent.GetKey();
	if (K == EKeys::Escape)
	{
		RebindTargetMapping = NAME_None;
		return FReply::Handled();
	}
	if (ULocalPlayer* const LP = GetOwningLocalPlayer())
	{
		if (UGameInstance* const GI = GetGameInstance())
		{
			if (UDFInputRemappingSubsystem* const RS = GI->GetSubsystem<UDFInputRemappingSubsystem>())
			{
				RS->RemapKey(LP, RebindTargetMapping, K);
			}
		}
	}
	RebindTargetMapping = NAME_None;
	BuildKeybindRows();
	return FReply::Handled();
}

void UDFOptionsScreenWidget::ShowTabByIndex(const int32 Index)
{
	if (MainTabSwitcher)
	{
		MainTabSwitcher->SetActiveWidgetIndex(Index);
	}
}

void UDFOptionsScreenWidget::ResetKeybindsToDefaults()
{
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFInputRemappingSubsystem* const RS = GI->GetSubsystem<UDFInputRemappingSubsystem>())
		{
			RS->ResetToDefaults(GetOwningLocalPlayer());
		}
	}
	BuildKeybindRows();
}

void UDFOptionsScreenWidget::ApplyAndSaveAccessibility()
{
	UDFAccessibilitySubsystem* A11y = GetGameInstance() ? GetGameInstance()->GetSubsystem<UDFAccessibilitySubsystem>() : nullptr;
	if (!A11y)
	{
		return;
	}
	FDFAccessibilitySettings S = A11y->GetSettings();
	if (Slider_UIFont) { S.UIFontScale = FMath::Lerp(0.8f, 2.0f, Slider_UIFont->GetValue()); }
	if (Check_HighContrast) { S.bHighContrast = Check_HighContrast->IsChecked(); }
	if (Check_ReduceMotion) { S.bReduceMotion = Check_ReduceMotion->IsChecked(); }
	if (Check_ColorBlind) { S.bColorBlindMode = Check_ColorBlind->IsChecked(); }
	if (S.bColorBlindMode) { S.ColorBlindType = ParseColorBlindCombo(); }
	else { S.ColorBlindType = EDFColorBlindMode::Off; }
	A11y->ApplySettings(S, true);
}

void UDFOptionsScreenWidget::ApplyAudioFromSliders()
{
	UDFAccessibilitySubsystem* A11y = GetGameInstance() ? GetGameInstance()->GetSubsystem<UDFAccessibilitySubsystem>() : nullptr;
	if (!A11y)
	{
		return;
	}
	FDFAccessibilitySettings S = A11y->GetSettings();
	if (Slider_Master) { S.MasterVolume = static_cast<float>(Slider_Master->GetValue()); }
	if (Slider_Music) { S.MusicVolume = static_cast<float>(Slider_Music->GetValue()); }
	if (Slider_SFX) { S.SFXVolume = static_cast<float>(Slider_SFX->GetValue()); }
	if (Slider_Voice) { S.VoiceVolume = static_cast<float>(Slider_Voice->GetValue()); }
	A11y->ApplySettings(S, true);
}

void UDFOptionsScreenWidget::SyncAudioFromSubsystem()
{
	UDFAccessibilitySubsystem* A11y = GetGameInstance() ? GetGameInstance()->GetSubsystem<UDFAccessibilitySubsystem>() : nullptr;
	if (!A11y)
	{
		return;
	}
	const FDFAccessibilitySettings& S = A11y->GetSettings();
	if (Slider_Master) { Slider_Master->SetValue(S.MasterVolume); }
	if (Slider_Music) { Slider_Music->SetValue(S.MusicVolume); }
	if (Slider_SFX) { Slider_SFX->SetValue(S.SFXVolume); }
	if (Slider_Voice) { Slider_Voice->SetValue(S.VoiceVolume); }
	if (Slider_UIFont) { Slider_UIFont->SetValue((S.UIFontScale - 0.8f) / 1.2f); }
	if (Check_HighContrast) { Check_HighContrast->SetIsChecked(S.bHighContrast); }
	if (Check_ReduceMotion) { Check_ReduceMotion->SetIsChecked(S.bReduceMotion); }
	if (Check_ColorBlind) { Check_ColorBlind->SetIsChecked(S.bColorBlindMode); }
}

void UDFOptionsScreenWidget::RefreshLanguagePreview()
{
	if (Text_LanguagePreview)
	{
		Text_LanguagePreview->SetText(NSLOCTEXT("DF", "Options_Lang_Preview", "The quick brown fox jumps over the lazy dog."));
	}
}

void UDFOptionsScreenWidget::BuildKeybindRows()
{
	if (!Panel_KeyBinds)
	{
		return;
	}
	Panel_KeyBinds->ClearChildren();
	const TSubclassOf<UUserWidget> RowClass = KeyBindRowClass ? KeyBindRowClass : TSubclassOf<UUserWidget>(UDFKeyBindRowWidget::StaticClass());
	if (!RowClass)
	{
		return;
	}
	UGameInstance* const GI = GetGameInstance();
	UDFInputRemappingSubsystem* const RS = GI ? GI->GetSubsystem<UDFInputRemappingSubsystem>() : nullptr;
	for (const FName& N : KeyBindOrder)
	{
		UDFKeyBindRowWidget* const Row = CreateWidget<UDFKeyBindRowWidget>(this, RowClass);
		if (!Row)
		{
			continue;
		}
		Row->MappingName = N;
		Row->SetActionLabelText(FText::FromName(N));
		Row->OnRequestRebind.BindUObject(this, &UDFOptionsScreenWidget::BeginRebindForMapping);
		if (RS)
		{
			if (const FKey* K = RS->RemappedKeys.Find(N))
			{
				Row->SetCurrentKeyText(FText::AsCultureInvariant(K->ToString()));
			}
		}
		Panel_KeyBinds->AddChild(Row);
	}
}

void UDFOptionsScreenWidget::BeginRebindForMapping(const FName MappingName)
{
	RebindTargetMapping = MappingName;
	SetKeyboardFocus();
}

void UDFOptionsScreenWidget::OnFontScaleSliderValue(const float Value)
{
	UDFAccessibilitySubsystem* A11y = GetGameInstance() ? GetGameInstance()->GetSubsystem<UDFAccessibilitySubsystem>() : nullptr;
	if (!A11y || !Slider_UIFont)
	{
		return;
	}
	FDFAccessibilitySettings S = A11y->GetSettings();
	S.UIFontScale = FMath::Lerp(0.8f, 2.0f, Value);
	A11y->ApplySettings(S, false);
}

void UDFOptionsScreenWidget::OnColorBlindComboSelectionChanged(const FString Selection, ESelectInfo::Type /*SelectType*/)
{
	(void)Selection;
	UDFAccessibilitySubsystem* A11y = GetGameInstance() ? GetGameInstance()->GetSubsystem<UDFAccessibilitySubsystem>() : nullptr;
	if (!A11y)
	{
		return;
	}
	FDFAccessibilitySettings S = A11y->GetSettings();
	if (S.bColorBlindMode)
	{
		S.ColorBlindType = ParseColorBlindCombo();
		A11y->ApplySettings(S, false);
	}
}

void UDFOptionsScreenWidget::OnHighContrastToggled(const bool bOn)
{
	UDFAccessibilitySubsystem* A11y = GetGameInstance() ? GetGameInstance()->GetSubsystem<UDFAccessibilitySubsystem>() : nullptr;
	if (!A11y) { return; }
	FDFAccessibilitySettings S = A11y->GetSettings();
	S.bHighContrast = bOn;
	A11y->ApplySettings(S, false);
}

void UDFOptionsScreenWidget::OnReduceMotionToggled(const bool bOn)
{
	UDFAccessibilitySubsystem* A11y = GetGameInstance() ? GetGameInstance()->GetSubsystem<UDFAccessibilitySubsystem>() : nullptr;
	if (!A11y) { return; }
	FDFAccessibilitySettings S = A11y->GetSettings();
	S.bReduceMotion = bOn;
	A11y->ApplySettings(S, false);
}

void UDFOptionsScreenWidget::OnColorBlindToggled(const bool bOn)
{
	UDFAccessibilitySubsystem* A11y = GetGameInstance() ? GetGameInstance()->GetSubsystem<UDFAccessibilitySubsystem>() : nullptr;
	if (!A11y) { return; }
	FDFAccessibilitySettings S = A11y->GetSettings();
	S.bColorBlindMode = bOn;
	S.ColorBlindType = bOn ? ParseColorBlindCombo() : EDFColorBlindMode::Off;
	A11y->ApplySettings(S, false);
}

void UDFOptionsScreenWidget::OnLanguageItemSelected(UObject* const Item)
{
	if (UDFUILanguageListEntry* const E = Cast<UDFUILanguageListEntry>(Item))
	{
		SetPendingLanguage(E->Language);
	}
}

void UDFOptionsScreenWidget::SetPendingLanguage(const EDFLanguage L)
{
	PendingLanguage = L;
	RefreshLanguagePreview();
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFLocalizationSubsystem* const Loc = GI->GetSubsystem<UDFLocalizationSubsystem>())
		{
			Loc->SetLanguage(L);
		}
	}
}

void UDFOptionsScreenWidget::OnMasterChanged(float /*Value*/)
{
	ApplyAudioFromSliders();
}
void UDFOptionsScreenWidget::OnMusicChanged(float /*Value*/)
{
	ApplyAudioFromSliders();
}
void UDFOptionsScreenWidget::OnSfxChanged(float /*Value*/)
{
	ApplyAudioFromSliders();
}
void UDFOptionsScreenWidget::OnVoiceChanged(float /*Value*/)
{
	ApplyAudioFromSliders();
}

void UDFOptionsScreenWidget::RebuildColorBlindCombo()
{
	if (!Combo_ColorBlind)
	{
		return;
	}
	Combo_ColorBlind->ClearOptions();
	Combo_ColorBlind->AddOption(TEXT("Off"));
	Combo_ColorBlind->AddOption(TEXT("Protanopia"));
	Combo_ColorBlind->AddOption(TEXT("Deuteranopia"));
	Combo_ColorBlind->AddOption(TEXT("Tritanopia"));
	Combo_ColorBlind->SetSelectedOption(TEXT("Off"));
}

EDFColorBlindMode UDFOptionsScreenWidget::ParseColorBlindCombo()
{
	if (!Combo_ColorBlind)
	{
		return EDFColorBlindMode::Off;
	}
	const FString S = Combo_ColorBlind->GetSelectedOption();
	if (S == TEXT("Protanopia")) { return EDFColorBlindMode::Protanopia; }
	if (S == TEXT("Deuteranopia")) { return EDFColorBlindMode::Deuteranopia; }
	if (S == TEXT("Tritanopia")) { return EDFColorBlindMode::Tritanopia; }
	return EDFColorBlindMode::Off;
}
