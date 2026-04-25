// Source/DungeonForged/Private/GameModes/MainMenu/UDFSplashScreenUserWidget.cpp
#include "GameModes/MainMenu/UDFSplashScreenUserWidget.h"
#include "GameModes/MainMenu/ADFMainMenuHUD.h"
#include "GameFramework/HUD.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Input/Events.h"
#include "TimerManager.h"

void UDFSplashScreenUserWidget::SetOwnerHUD(AHUD* const InHUD)
{
	OwnerHUD = InHUD;
}

void UDFSplashScreenUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ShownTimeSeconds = FPlatformTime::Seconds();
	SetIsFocusable(true);
}

void UDFSplashScreenUserWidget::StartSplashFlow()
{
	CurrentSplash = 0;
	PlayNextSplash();
}

FReply UDFSplashScreenUserWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	SkipSplashes();
	return FReply::Handled();
}

FReply UDFSplashScreenUserWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	SkipSplashes();
	return FReply::Handled();
}

void UDFSplashScreenUserWidget::SkipSplashes()
{
	if (FPlatformTime::Seconds() - ShownTimeSeconds < 0.5)
	{
		return;
	}
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(PhaseTimer1);
		W->GetTimerManager().ClearTimer(PhaseTimer2);
		W->GetTimerManager().ClearTimer(PhaseTimer3);
	}
	ShowMainMenu();
}

void UDFSplashScreenUserWidget::PlayNextSplash()
{
	if (SplashImages.IsEmpty() || CurrentSplash >= SplashImages.Num())
	{
		ShowMainMenu();
		return;
	}
	UTexture2D* const Tex = SplashImages[CurrentSplash];
	if (SplashImage)
	{
		SplashImage->SetBrushFromTexture(Tex, false);
	}
	if (CurrentSplash == 2 && SubtitleText)
	{
		SubtitleText->SetText(
			NSLOCTEXT("MainMenu", "Tagline", "Um Roguelike ARPG"));
	}
	OnSplashIndexChanged(CurrentSplash, Tex);
	if (CurrentSplash == 2)
	{
		OnTitleCardShown(Tex);
	}
	ApplySplashVisibleAlpha(1.f, CurrentSplash);
	SchedulePhaseTimers();
}

void UDFSplashScreenUserWidget::OnSplashIndexChanged_Implementation(int32 const Index, UTexture2D* const Texture) {}

FDFSplashPhaseConfig UDFSplashScreenUserWidget::GetOrCreatePhaseConfig(int32 const Index) const
{
	if (PhaseConfig.IsValidIndex(Index))
	{
		return PhaseConfig[Index];
	}
	FDFSplashPhaseConfig C;
	if (Index == 2)
	{
		C.FadeInSeconds = 1.2f;
		C.HoldSeconds = 1.5f;
		C.FadeOutSeconds = 0.5f;
	}
	return C;
}

void UDFSplashScreenUserWidget::ApplySplashVisibleAlpha_Implementation(
	float const Opacity, int32 const Index)
{
	if (SplashImage)
	{
		const FLinearColor L = SplashImage->GetColorAndOpacity();
		SplashImage->SetColorAndOpacity(FLinearColor(L.R, L.G, L.B, Opacity));
	}
}

void UDFSplashScreenUserWidget::SchedulePhaseTimers()
{
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	FTimerManager& TM = W->GetTimerManager();
	const FDFSplashPhaseConfig P = GetOrCreatePhaseConfig(CurrentSplash);
	ApplySplashVisibleAlpha(0.f, CurrentSplash);
	TM.SetTimer(PhaseTimer1, this, &UDFSplashScreenUserWidget::OnFadeInEnd, P.FadeInSeconds, false);
}

void UDFSplashScreenUserWidget::OnFadeInEnd()
{
	ApplySplashVisibleAlpha(1.f, CurrentSplash);
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	const FDFSplashPhaseConfig P = GetOrCreatePhaseConfig(CurrentSplash);
	W->GetTimerManager().SetTimer(PhaseTimer2, this, &UDFSplashScreenUserWidget::OnHoldEnd, P.HoldSeconds, false);
}

void UDFSplashScreenUserWidget::OnHoldEnd() { AdvanceAfterFadeInHold(); }

void UDFSplashScreenUserWidget::AdvanceAfterFadeInHold()
{
	const FDFSplashPhaseConfig P = GetOrCreatePhaseConfig(CurrentSplash);
	ApplySplashVisibleAlpha(0.f, CurrentSplash);
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(PhaseTimer3, this, &UDFSplashScreenUserWidget::OnFadeOutEnd, P.FadeOutSeconds, false);
	}
}

void UDFSplashScreenUserWidget::OnFadeOutEnd() { AdvanceAfterFadeOut(); }

void UDFSplashScreenUserWidget::AdvanceAfterFadeOut()
{
	++CurrentSplash;
	PlayNextSplash();
}

void UDFSplashScreenUserWidget::ShowMainMenu()
{
	RemoveFromParent();
	if (ADFMainMenuHUD* const H = Cast<ADFMainMenuHUD>(OwnerHUD.Get()))
	{
		H->ShowMainMenu();
	}
}
