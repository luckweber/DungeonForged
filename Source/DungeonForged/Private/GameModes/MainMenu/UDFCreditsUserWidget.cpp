// Source/DungeonForged/Private/GameModes/MainMenu/UDFCreditsUserWidget.cpp
#include "GameModes/MainMenu/UDFCreditsUserWidget.h"
#include "GameModes/MainMenu/ADFMainMenuHUD.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "GameFramework/PlayerController.h"

void UDFCreditsUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	if (APlayerController* const Pc = GetOwningPlayer())
	{
		SetUserFocus(Pc);
	}
	if (BackButton)
	{
		BackButton->OnClicked.AddDynamic(this, &UDFCreditsUserWidget::OnBackClicked);
	}
	if (SkipButton)
	{
		SkipButton->OnClicked.AddDynamic(this, &UDFCreditsUserWidget::OnSkipClicked);
	}
}

void UDFCreditsUserWidget::NativeDestruct()
{
	if (BackButton)
	{
		BackButton->OnClicked.RemoveAll(this);
	}
	if (SkipButton)
	{
		SkipButton->OnClicked.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UDFCreditsUserWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!IsVisible() || !CreditsScroll)
	{
		return;
	}
	const float Mult = bSkipSpeedMultiplier ? 4.f : 1.f;
	const float NewOffset = CreditsScroll->GetScrollOffset() + (AutoScrollSpeed * Mult * InDeltaTime);
	const float Limit = CreditsScroll->GetScrollOffsetOfEnd();
	CreditsScroll->SetScrollOffset(FMath::Min(NewOffset, Limit));
	if (NewOffset >= Limit - 0.5f)
	{
		bReachedEnd = true;
	}
	if (bReachedEnd && AutoCloseHoldSeconds > 0.f)
	{
		TimeAtEndSeconds += InDeltaTime;
		if (TimeAtEndSeconds >= AutoCloseHoldSeconds)
		{
			OnBackClicked();
		}
	}
}

void UDFCreditsUserWidget::OnBackClicked()
{
	APlayerController* const Pc = GetOwningPlayer();
	ADFMainMenuHUD* const H = Pc ? Cast<ADFMainMenuHUD>(Pc->GetHUD()) : nullptr;
	RemoveFromParent();
	if (H) { H->RestoreMainMenuFocus(); }
}

void UDFCreditsUserWidget::OnSkipClicked()
{
	JumpScrollToEnd();
}

void UDFCreditsUserWidget::JumpScrollToEnd()
{
	if (CreditsScroll)
	{
		CreditsScroll->ScrollToEnd();
	}
}

FReply UDFCreditsUserWidget::NativeOnKeyDown(
	const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	(void)InKeyEvent;
	bSkipSpeedMultiplier = true;
	return FReply::Unhandled();
}

FReply UDFCreditsUserWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	(void)InKeyEvent;
	bSkipSpeedMultiplier = false;
	return FReply::Unhandled();
}
