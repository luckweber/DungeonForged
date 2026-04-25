// Source/DungeonForged/Private/GameModes/MainMenu/UDFCreditsUserWidget.cpp
#include "GameModes/MainMenu/UDFCreditsUserWidget.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "GameFramework/PlayerController.h"

void UDFCreditsUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
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
	const float S = AutoScrollSpeed * (bSkipSpeedMultiplier ? 4.f : 1.f) * InDeltaTime;
	CreditsScroll->SetScrollOffset(CreditsScroll->GetScrollOffset() + S);
}

void UDFCreditsUserWidget::OnBackClicked()
{
	RemoveFromParent();
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
