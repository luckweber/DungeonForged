// Source/DungeonForged/Private/UI/Status/UDFStatusEffectIconWidget.cpp
#include "UI/Status/UDFStatusEffectIconWidget.h"
#include "UI/Status/UDFStatusEffectBarWidget.h"
#include "UI/Status/UDFEnemyDebuffStatusBarWidget.h"
#include "UI/Status/UDFStatusEffectTooltipWidget.h"
#include "UI/Status/UDFStatusLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UDFStatusEffectIconWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (HoverArea)
	{
		HoverArea->OnHovered.AddDynamic(this, &UDFStatusEffectIconWidget::OnHoverAreaHovered);
		HoverArea->OnUnhovered.AddDynamic(this, &UDFStatusEffectIconWidget::OnHoverAreaUnhovered);
	}
}

void UDFStatusEffectIconWidget::NativeDestruct()
{
	if (HoverArea)
	{
		HoverArea->OnHovered.RemoveDynamic(this, &UDFStatusEffectIconWidget::OnHoverAreaHovered);
		HoverArea->OnUnhovered.RemoveDynamic(this, &UDFStatusEffectIconWidget::OnHoverAreaUnhovered);
	}
	StopAll();
	Super::NativeDestruct();
}

void UDFStatusEffectIconWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* const Animation)
{
	if (Animation == FadeOutAnim)
	{
		FinishFadeAndReturn();
	}
	Super::OnAnimationFinished_Implementation(Animation);
}

void UDFStatusEffectIconWidget::ApplyDisplayData(const FDFStatusEffectDisplayData& Data)
{
	if (IconImage && Data.Icon)
	{
		IconImage->SetBrushFromTexture(Data.Icon, false);
	}
	if (BorderImage)
	{
		BorderImage->SetColorAndOpacity(Data.BorderColor);
	}
	bUseDurationUi = Data.bShowDuration;
	if (DurationText)
	{
		DurationText->SetVisibility(
			Data.bShowDuration ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (DurationBar)
	{
		DurationBar->SetVisibility(
			Data.bShowDuration ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UDFStatusEffectIconWidget::InitializeForPlayerBar(
	const FGameplayTag InDisplayKey,
	UAbilitySystemComponent* const InASC,
	const FActiveGameplayEffectHandle InHandle,
	const FDFStatusEffectDisplayData& Data,
	UDFStatusEffectBarWidget* const InHost,
	const UDFStatusLibrary* const InLibrary)
{
	StopAll();
	DisplayKey = InDisplayKey;
	WatchedASC = InASC;
	ActiveHandle = InHandle;
	CachedData = Data;
	DisplayLibrary = InLibrary;
	PlayerHost = InHost;
	EnemyHost = nullptr;
	ApplyDisplayData(Data);
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().SetTimer(DurationTimer, this, &UDFStatusEffectIconWidget::TickDuration, 0.1f, true);
	}
	TickDuration();
}

void UDFStatusEffectIconWidget::InitializeForEnemyBar(
	const FGameplayTag InDisplayKey,
	UAbilitySystemComponent* const InASC,
	const FActiveGameplayEffectHandle InHandle,
	const FDFStatusEffectDisplayData& Data,
	UDFEnemyDebuffStatusBarWidget* const InHost,
	const UDFStatusLibrary* const InLibrary)
{
	StopAll();
	DisplayKey = InDisplayKey;
	WatchedASC = InASC;
	ActiveHandle = InHandle;
	CachedData = Data;
	DisplayLibrary = InLibrary;
	PlayerHost = nullptr;
	EnemyHost = InHost;
	ApplyDisplayData(Data);
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().SetTimer(DurationTimer, this, &UDFStatusEffectIconWidget::TickDuration, 0.1f, true);
	}
	TickDuration();
}

void UDFStatusEffectIconWidget::SetDesiredIconSize(const float SideLength)
{
	SetDesiredSizeInViewport(FVector2D(SideLength, SideLength));
}

void UDFStatusEffectIconWidget::StopAll()
{
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(DurationTimer);
	}
	HideTooltip();
}

void UDFStatusEffectIconWidget::ResetForPool()
{
	DisplayKey = FGameplayTag::EmptyTag;
	ActiveHandle = FActiveGameplayEffectHandle();
	WatchedASC = nullptr;
	StopAll();
}

void UDFStatusEffectIconWidget::PlayFadeOutAndRequestReturn()
{
	StopAll();
	if (FadeOutAnim)
	{
		PlayAnimation(FadeOutAnim);
	}
	else
	{
		FinishFadeAndReturn();
	}
}

void UDFStatusEffectIconWidget::TickDuration()
{
	if (!WatchedASC.IsValid() || !ActiveHandle.IsValid())
	{
		return;
	}
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	const FActiveGameplayEffect* const Active = WatchedASC->GetActiveGameplayEffect(ActiveHandle);
	if (!Active)
	{
		PlayFadeOutAndRequestReturn();
		return;
	}
	const float Now = W->GetTimeSeconds();
	const float Remaining = Active->GetTimeRemaining(Now);
	const float Total = Active->GetDuration();
	TotalDurationCached = Total;
	UpdateDurationDisplay(Remaining, Total);
}

void UDFStatusEffectIconWidget::UpdateDurationDisplay(const float Remaining, const float Total)
{
	if (!bUseDurationUi)
	{
		return;
	}
	const bool bInfinite = (Total <= 0.f) || (Total >= 1.e20f);
	if (DurationText)
	{
		if (bInfinite)
		{
			DurationText->SetText(NSLOCTEXT("DFStatus", "Infinite", "∞"));
			DurationText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		}
		else
		{
			const FString S = FString::SanitizeFloat(FMath::Max(Remaining, 0.f), 1) + TEXT("s");
			DurationText->SetText(FText::FromString(S));
			const FLinearColor C = (Remaining > 0.f && Remaining < 3.f)
				                       ? FLinearColor(1.f, 0.2f, 0.2f)
				                       : FLinearColor::White;
			DurationText->SetColorAndOpacity(FSlateColor(C));
		}
	}
	if (DurationBar && !bInfinite)
	{
		const float P = Total > KINDA_SMALL_NUMBER ? FMath::Clamp(Remaining / Total, 0.f, 1.f) : 0.f;
		DurationBar->SetPercent(P);
	}
}

void UDFStatusEffectIconWidget::OnHoverAreaHovered()
{
	ShowTooltip();
}

void UDFStatusEffectIconWidget::OnHoverAreaUnhovered()
{
	HideTooltip();
}

void UDFStatusEffectIconWidget::ShowTooltip()
{
	HideTooltip();
	const TSubclassOf<UDFStatusEffectTooltipWidget> TtClass = PlayerHost.IsValid() && PlayerHost->TooltipWidgetClass
		                                                            ? PlayerHost->TooltipWidgetClass
		                                                            : (EnemyHost.IsValid() ? EnemyHost->TooltipWidgetClass
		                                                                                    : nullptr);
	if (!TtClass)
	{
		return;
	}
	if (UDFStatusEffectTooltipWidget* const T = CreateWidget<UDFStatusEffectTooltipWidget>(this, TtClass))
	{
		FText TimeLine = FText::GetEmpty();
		if (bUseDurationUi && WatchedASC.IsValid() && ActiveHandle.IsValid() && GetWorld())
		{
			if (const FActiveGameplayEffect* const A = WatchedASC->GetActiveGameplayEffect(ActiveHandle))
			{
				const float R = A->GetTimeRemaining(GetWorld()->GetTimeSeconds());
				TimeLine = FText::Format(
					NSLOCTEXT("DFStatus", "TimeRemaining", "Remaining: {0}s"),
					FText::AsNumber(FMath::Max(0.f, R)));
			}
		}
		T->SetTooltipContent(CachedData.DisplayName, CachedData.Description, TimeLine);
		ActiveTooltip = T;
		T->AddToViewport(2000);
		const FVector2D Pix = UWidgetLayoutLibrary::GetMousePositionOnViewport(this);
		T->SetPositionInViewport(Pix + FVector2D(12.f, 12.f), false);
	}
}

void UDFStatusEffectIconWidget::HideTooltip()
{
	if (ActiveTooltip)
	{
		ActiveTooltip->RemoveFromParent();
		ActiveTooltip = nullptr;
	}
}

void UDFStatusEffectIconWidget::FinishFadeAndReturn()
{
	if (PlayerHost.IsValid())
	{
		PlayerHost->ReleaseIconToPool(this);
	}
	else if (EnemyHost.IsValid())
	{
		EnemyHost->ReleaseIconToPool(this);
	}
}
