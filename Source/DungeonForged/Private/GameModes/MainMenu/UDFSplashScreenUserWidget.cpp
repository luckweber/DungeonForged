// Source/DungeonForged/Private/GameModes/MainMenu/UDFSplashScreenUserWidget.cpp
#include "GameModes/MainMenu/UDFSplashScreenUserWidget.h"
#include "GameModes/MainMenu/ADFMainMenuHUD.h"
#include "GameFramework/HUD.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
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
	if (APlayerController* const PC = GetOwningPlayer())
	{
		SetUserFocus(PC);
		SetKeyboardFocus();
	}
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
	SchedulePhaseTimers();
}

void UDFSplashScreenUserWidget::OnSplashIndexChanged_Implementation(int32 const Index, UTexture2D* const Texture) {}

float UDFSplashScreenUserWidget::GetDefaultHold() const
{
	return 1.f;
}

FDFSplashPhaseConfig UDFSplashScreenUserWidget::GetOrCreatePhaseConfig(int32 const Index) const
{
	FDFSplashPhaseConfig C;
	if (PhaseConfig.IsValidIndex(Index))
	{
		C = PhaseConfig[Index];
	}
	else
	{
		// Padrão ~4s total: 1+2: UE/Estúdio 1,2s in / 1,0s hold / 0,8s out; 3: title card 1,5s hold, out mais curto
		if (Index == 2)
		{
			C.FadeInSeconds = 1.2f;
			C.HoldSeconds = 1.5f;
			C.FadeOutSeconds = 0.5f;
		}
		else
		{
			C.FadeInSeconds = 1.2f;
			C.HoldSeconds = 1.0f;
			C.FadeOutSeconds = 0.8f;
		}
	}
	if (HoldDurations.IsValidIndex(Index) && HoldDurations[Index] > 0.01f)
	{
		C.HoldSeconds = HoldDurations[Index];
	}
	if (C.HoldSeconds <= 0.01f)
	{
		C.HoldSeconds = GetDefaultHold();
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
	SplashLinearFadeAlpha = 0.f;
	ApplySplashVisibleAlpha(0.f, CurrentSplash);
	FTimerManager& TM = W->GetTimerManager();
	TM.SetTimer(PhaseTimer1, this, &UDFSplashScreenUserWidget::SplashTickFadeIn, SplashFadeTimeStep, true, 0.f);
}

void UDFSplashScreenUserWidget::SplashTickFadeIn()
{
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	const FDFSplashPhaseConfig P = GetOrCreatePhaseConfig(CurrentSplash);
	if (P.FadeInSeconds <= 1.e-3f)
	{
		ApplySplashVisibleAlpha(1.f, CurrentSplash);
		W->GetTimerManager().ClearTimer(PhaseTimer1);
		OnFadeInFinishedStartHold();
		return;
	}
	const float Step = SplashFadeTimeStep / P.FadeInSeconds;
	SplashLinearFadeAlpha = FMath::Min(1.f, SplashLinearFadeAlpha + Step);
	ApplySplashVisibleAlpha(SplashLinearFadeAlpha, CurrentSplash);
	if (SplashLinearFadeAlpha >= 1.f - 1.e-3f)
	{
		W->GetTimerManager().ClearTimer(PhaseTimer1);
		OnFadeInFinishedStartHold();
	}
}

void UDFSplashScreenUserWidget::OnFadeInFinishedStartHold()
{
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
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	if (P.FadeOutSeconds <= 1.e-3f)
	{
		ApplySplashVisibleAlpha(0.f, CurrentSplash);
		OnFadeOutEnd();
		return;
	}
	SplashLinearFadeAlpha = 0.f;
	W->GetTimerManager().SetTimer(PhaseTimer3, this, &UDFSplashScreenUserWidget::SplashTickFadeOut, SplashFadeTimeStep, true, 0.f);
}

void UDFSplashScreenUserWidget::SplashTickFadeOut()
{
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	const FDFSplashPhaseConfig P = GetOrCreatePhaseConfig(CurrentSplash);
	if (P.FadeOutSeconds <= 1.e-3f)
	{
		W->GetTimerManager().ClearTimer(PhaseTimer3);
		OnFadeOutEnd();
		return;
	}
	const float Step = SplashFadeTimeStep / P.FadeOutSeconds;
	SplashLinearFadeAlpha = FMath::Min(1.f, SplashLinearFadeAlpha + Step);
	ApplySplashVisibleAlpha(1.f - SplashLinearFadeAlpha, CurrentSplash);
	if (SplashLinearFadeAlpha >= 1.f - 1.e-3f)
	{
		W->GetTimerManager().ClearTimer(PhaseTimer3);
		OnFadeOutEnd();
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
