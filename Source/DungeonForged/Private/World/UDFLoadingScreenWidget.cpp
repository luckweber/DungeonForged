// Source/DungeonForged/Private/World/UDFLoadingScreenWidget.cpp
#include "World/UDFLoadingScreenWidget.h"
#include "Animation/WidgetAnimation.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UDFLoadingScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ResetForTravel();
	if (LogoBreathAnim && !bBreathStarted)
	{
		bBreathStarted = true;
		PlayAnimation(LogoBreathAnim, 0.f, 0);
	}
	else if (FadeInAnim)
	{
		PlayAnimation(FadeInAnim, 0.f, 1);
	}
}

void UDFLoadingScreenWidget::NativeTick(const FGeometry& MyGeometry, float const InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!LoadingBar)
	{
		return;
	}
	// Smooth approach; slow "stutter" band before snap
	if (DisplayedProgress < 0.84f)
	{
		DisplayedProgress = FMath::FInterpTo(DisplayedProgress, FMath::Min(TargetProgress, 0.9f), InDeltaTime, 2.5f);
	}
	else if (DisplayedProgress < 0.9f && !bSnapRequest)
	{
		StutterTimer += InDeltaTime;
		const float Wobble = FMath::Sin(StutterTimer * 12.f) * 0.002f;
		DisplayedProgress = FMath::Clamp(0.85f + Wobble, 0.f, 0.9f);
	}
	else if (bSnapRequest)
	{
		DisplayedProgress = FMath::FInterpTo(DisplayedProgress, 1.f, InDeltaTime, 6.f);
	}
	LoadingBar->SetPercent(FMath::Clamp(DisplayedProgress, 0.f, 1.f));
}

void UDFLoadingScreenWidget::ResetForTravel()
{
	TargetProgress = 0.f;
	DisplayedProgress = 0.f;
	StutterTimer = 0.f;
	bSnapRequest = false;
	if (LoadingBar)
	{
		LoadingBar->SetPercent(0.f);
	}
	SetRootVisualAlpha(1.f);
}

void UDFLoadingScreenWidget::SetLoadingProgress(const float Pct, const bool bSnapComplete)
{
	TargetProgress = FMath::Clamp(Pct, 0.f, 1.f);
	if (bSnapComplete)
	{
		bSnapRequest = true;
	}
}

void UDFLoadingScreenWidget::SetLoadingTitleText(const FText& T)
{
	if (LoadingTitle)
	{
		LoadingTitle->SetText(T);
	}
}

void UDFLoadingScreenWidget::SetFlavorText(const FText& T)
{
	if (FlavorText)
	{
		FlavorText->SetText(T);
	}
}

void UDFLoadingScreenWidget::SetTip(const FText& Label, const FText& Body)
{
	if (TipLabel)
	{
		TipLabel->SetText(Label);
	}
	if (TipText)
	{
		TipText->SetText(Body);
	}
}

void UDFLoadingScreenWidget::SetFloorCopy(
	const int32 Floor, const int32 MaxFloors, const FText& DifficultyLine)
{
	if (FloorNumber)
	{
		FloorNumber->SetText(FText::Format(
			NSLOCTEXT("DF", "LoadingFloorFmt", "ANDAR {0} / {1}"),
			FText::AsNumber(Floor),
			FText::AsNumber(MaxFloors)));
	}
	if (FloorDifficulty)
	{
		FloorDifficulty->SetText(DifficultyLine);
	}
}

void UDFLoadingScreenWidget::SetRunProgress(const float Pct01)
{
	if (RunProgress)
	{
		RunProgress->SetPercent(FMath::Clamp(Pct01, 0.f, 1.f));
	}
}

void UDFLoadingScreenWidget::SetEnemyPreviewTextures(
	const int32 Count, UTexture2D* A, UTexture2D* B, UTexture2D* C)
{
	if (!EnemyPreview)
	{
		return;
	}
	const TArray<UTexture2D*> Ts = {A, B, C};
	int32 Idx = 0;
	const int32 N = FMath::Min(EnemyPreview->GetChildrenCount(), 3);
	for (int32 i = 0; i < N && Idx < FMath::Min(Count, 3); ++i)
	{
		if (UImage* const Img = Cast<UImage>(EnemyPreview->GetChildAt(i)))
		{
			if (Ts[Idx])
			{
				Img->SetBrushFromTexture(Ts[Idx], false);
				Img->SetVisibility(ESlateVisibility::Visible);
			}
			++Idx;
		}
	}
}

void UDFLoadingScreenWidget::SetRootVisualAlpha(const float Alpha)
{
	SetRenderOpacity(FMath::Clamp(Alpha, 0.f, 1.f));
}

void UDFLoadingScreenWidget::PlayFadeOutAnimation()
{
	if (FadeOutAnim)
	{
		PlayAnimation(FadeOutAnim, 0.f, 1, EUMGSequencePlayMode::Forward, 1.f);
	}
}
