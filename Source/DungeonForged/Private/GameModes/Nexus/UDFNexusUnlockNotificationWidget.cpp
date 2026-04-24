// Source/DungeonForged/Private/GameModes/Nexus/UDFNexusUnlockNotificationWidget.cpp
#include "GameModes/Nexus/UDFNexusUnlockNotificationWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"
#include "Engine/Texture2D.h"
#include "Animation/WidgetAnimation.h"

void UDFNexusUnlockNotificationWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UDFNexusUnlockNotificationWidget::SetUnlockContent(
	const FText& Title, const FText& Name, UTexture2D* const OptionalIcon)
{
	if (TitleText)
	{
		TitleText->SetText(Title);
	}
	if (NameText)
	{
		NameText->SetText(Name);
	}
	if (UnlockIcon && OptionalIcon)
	{
		UnlockIcon->SetBrushFromTexture(OptionalIcon, false);
	}
}

void UDFNexusUnlockNotificationWidget::PlayShowThenHide(const float DisplaySeconds)
{
	if (SlideInAnim)
	{
		PlayAnimation(SlideInAnim, 0.f, 1, EUMGSequencePlayMode::Forward, 1.f);
	}
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().SetTimer(AutoHideHandle, this, &UDFNexusUnlockNotificationWidget::HideSelf, FMath::Max(0.2f, DisplaySeconds), false);
	}
}

void UDFNexusUnlockNotificationWidget::HideSelf()
{
	RemoveFromParent();
}
