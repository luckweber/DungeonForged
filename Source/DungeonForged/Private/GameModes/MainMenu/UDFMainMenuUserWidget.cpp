// Source/DungeonForged/Private/GameModes/MainMenu/UDFMainMenuUserWidget.cpp
#include "GameModes/MainMenu/UDFMainMenuUserWidget.h"
#include "GameModes/MainMenu/ADFMainMenuHUD.h"
#include "GameModes/MainMenu/UDFConfirmDialogUserWidget.h"
#include "GameModes/MainMenu/DFMainMenuTypes.h"
#include "Run/UDFSaveSlotManagerSubsystem.h"
#include "Run/DFSaveGame.h"
#include "World/UDFWorldTransitionSubsystem.h"
#include "World/DFWorldTypes.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"

void UDFMainMenuUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (ContinueAdventureButton)
	{
		ContinueAdventureButton->OnClicked.AddDynamic(this, &UDFMainMenuUserWidget::OnContinueAdventure);
	}
	if (NewAdventureButton)
	{
		NewAdventureButton->OnClicked.AddDynamic(this, &UDFMainMenuUserWidget::OnNewAdventure);
	}
	if (ManageProfilesButton)
	{
		ManageProfilesButton->OnClicked.AddDynamic(this, &UDFMainMenuUserWidget::OnManageProfiles);
	}
	if (OptionsButton)
	{
		OptionsButton->OnClicked.AddDynamic(this, &UDFMainMenuUserWidget::OnOptions);
	}
	if (AchievementsButton)
	{
		AchievementsButton->OnClicked.AddDynamic(this, &UDFMainMenuUserWidget::OnAchievements);
	}
	if (CreditsButton)
	{
		CreditsButton->OnClicked.AddDynamic(this, &UDFMainMenuUserWidget::OnCredits);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UDFMainMenuUserWidget::OnQuit);
	}
	if (SubtitleText)
	{
		SubtitleText->SetText(NSLOCTEXT("MainMenu", "Tagline2", "Um Roguelike ARPG"));
	}
	if (VersionText)
	{
		FString Ver = TEXT("0.1.0");
		(void)GConfig->GetString(
			TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("ProjectVersion"), Ver, GGameIni);
		VersionText->SetText(FText::FromString(FString::Printf(
			TEXT("v%s | UE 5.4"), *Ver)));
	}
	if (CopyrightText)
	{
		CopyrightText->SetText(
			NSLOCTEXT("MainMenu", "Copyright", "2026 Your Studio. All rights reserved"));
	}
	RefreshForCurrentSaveState();
}

void UDFMainMenuUserWidget::NativeDestruct()
{
	if (ContinueAdventureButton)
	{
		ContinueAdventureButton->OnClicked.RemoveAll(this);
	}
	if (NewAdventureButton)
	{
		NewAdventureButton->OnClicked.RemoveAll(this);
	}
	if (ManageProfilesButton)
	{
		ManageProfilesButton->OnClicked.RemoveAll(this);
	}
	if (OptionsButton)
	{
		OptionsButton->OnClicked.RemoveAll(this);
	}
	if (AchievementsButton)
	{
		AchievementsButton->OnClicked.RemoveAll(this);
	}
	if (CreditsButton)
	{
		CreditsButton->OnClicked.RemoveAll(this);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UDFMainMenuUserWidget::RefreshForCurrentSaveState()
{
	UGameInstance* const GI = GetGameInstance();
	UDFSaveSlotManagerSubsystem* const Slots = GI ? GI->GetSubsystem<UDFSaveSlotManagerSubsystem>() : nullptr;
	UDFSaveGame* const Meta = Slots ? Slots->GetActiveOrLegacyMetaSave() : nullptr;
	const bool bHasActiveRun = Meta && Meta->bHasActiveRun && (Slots->GetActiveSlotIndex() >= 0);
	if (ContinueAdventureButton)
	{
		ContinueAdventureButton->SetVisibility(
			bHasActiveRun ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (AchievementsButton)
	{
		const bool bHasMeta = Slots && Slots->HasAnyProfileOrLegacySave();
		AchievementsButton->SetVisibility(
			bHasMeta ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UDFMainMenuUserWidget::OnContinueAdventure()
{
	UGameInstance* const GI = GetGameInstance();
	if (!GI)
	{
		return;
	}
	UDFSaveSlotManagerSubsystem* const Slots = GI->GetSubsystem<UDFSaveSlotManagerSubsystem>();
	if (!Slots)
	{
		return;
	}
	if (Slots->GetActiveSlotIndex() < 0)
	{
		if (APlayerController* const PC = GetOwningPlayer())
		{
			if (ADFMainMenuHUD* const H = Cast<ADFMainMenuHUD>(PC->GetHUD()))
			{
				H->ShowSaveSlotLayer(EDFSlotScreenMode::SelectToPlay);
			}
		}
		return;
	}
	UDFSaveGame* S = Slots->GetActiveSave();
	if (!S || !S->bHasActiveRun)
	{
		return;
	}
	if (!S->IsCompatible())
	{
		return;
	}
	if (UDFWorldTransitionSubsystem* const T = GI->GetSubsystem<UDFWorldTransitionSubsystem>())
	{
		T->TravelToNexus(ETravelReason::FirstLaunch);
	}
}

void UDFMainMenuUserWidget::OnNewAdventure()
{
	if (APlayerController* const PC = GetOwningPlayer())
	{
		if (ADFMainMenuHUD* const H = Cast<ADFMainMenuHUD>(PC->GetHUD()))
		{
			H->ShowSaveSlotLayer(EDFSlotScreenMode::SelectToPlay);
		}
	}
}

void UDFMainMenuUserWidget::OnManageProfiles()
{
	if (APlayerController* const PC = GetOwningPlayer())
	{
		if (ADFMainMenuHUD* const H = Cast<ADFMainMenuHUD>(PC->GetHUD()))
		{
			H->ShowSaveSlotLayer(EDFSlotScreenMode::SelectToDelete);
		}
	}
}

void UDFMainMenuUserWidget::OnOptions()
{
	if (APlayerController* const PC = GetOwningPlayer())
	{
		if (ADFMainMenuHUD* const H = Cast<ADFMainMenuHUD>(PC->GetHUD()))
		{
			if (H->OptionsWidgetClass)
			{
				if (UUserWidget* const W = CreateWidget<UUserWidget>(PC, H->OptionsWidgetClass))
				{
					W->AddToViewport(30);
				}
			}
		}
	}
}

void UDFMainMenuUserWidget::OnAchievements()
{
	if (APlayerController* const PC = GetOwningPlayer())
	{
		if (ADFMainMenuHUD* const H = Cast<ADFMainMenuHUD>(PC->GetHUD()))
		{
			if (H->AchievementListWidgetClass)
			{
				if (UUserWidget* const W = CreateWidget<UUserWidget>(PC, H->AchievementListWidgetClass))
				{
					W->AddToViewport(30);
				}
			}
		}
	}
}

void UDFMainMenuUserWidget::OnCredits()
{
	if (APlayerController* const PC = GetOwningPlayer())
	{
		if (ADFMainMenuHUD* const H = Cast<ADFMainMenuHUD>(PC->GetHUD()))
		{
			H->ShowCredits();
		}
	}
}

void UDFMainMenuUserWidget::OnQuit()
{
	APlayerController* const PC = GetOwningPlayer();
	ADFMainMenuHUD* const H = PC ? Cast<ADFMainMenuHUD>(PC->GetHUD()) : nullptr;
	if (!H || !H->ConfirmWidgetClass || !PC)
	{
		UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, true);
		return;
	}
	UDFConfirmDialogUserWidget* const D = CreateWidget<UDFConfirmDialogUserWidget>(PC, H->ConfirmWidgetClass);
	if (!D)
	{
		UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, true);
		return;
	}
	D->ShowDialog(
		NSLOCTEXT("MainMenu", "QuitTitle", "Sair?"),
		NSLOCTEXT("MainMenu", "QuitBody", "Tem certeza que deseja sair?"),
		FSimpleDelegate::CreateWeakLambda(
			PC, [this, PC] { UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, true); }));
	H->ShowConfirmDialog(D);
}
