// Source/DungeonForged/Private/Interaction/UDFInteractionPromptWidget.cpp
#include "Interaction/UDFInteractionPromptWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"
#include "TimerManager.h"

void UDFInteractionPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			BobTimerHandle, this, &UDFInteractionPromptWidget::BobTick, 0.04f, true, 0.f);
	}
}

void UDFInteractionPromptWidget::NativeDestruct()
{
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(BobTimerHandle);
	}
	Super::NativeDestruct();
}

void UDFInteractionPromptWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ApplyFadeIn();
}

void UDFInteractionPromptWidget::BobTick()
{
	BobPhase += 0.04f * BobFrequency * 6.28318530718f;
	const float DY = FMath::Sin(BobPhase) * BobAmplitude;
	SetRenderTranslation(FVector2D(0.f, DY));
}

void UDFInteractionPromptWidget::UpdatePrompt(
	const FText& InAction, UTexture2D* InKeyIcon, const FText& InSubHint)
{
	if (ActionText)
	{
		ActionText->SetText(InAction);
	}
	if (InteractorHint)
	{
		InteractorHint->SetText(InSubHint);
		InteractorHint->SetVisibility(InSubHint.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (KeyIcon)
	{
		if (InKeyIcon)
		{
			KeyIcon->SetVisibility(ESlateVisibility::Visible);
			KeyIcon->SetBrushFromTexture(InKeyIcon, false);
		}
		else
		{
			KeyIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UDFInteractionPromptWidget::SetPrimaryFocus(const bool bIsPrimary)
{
	const float A = bIsPrimary ? 1.f : 0.45f;
	SetRenderOpacity(A);
}

void UDFInteractionPromptWidget::PlayEnterAnimation()
{
	ApplyFadeIn();
}

void UDFInteractionPromptWidget::ApplyFadeIn()
{
	SetRenderOpacity(1.f);
}
