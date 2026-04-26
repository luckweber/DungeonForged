// Source/DungeonForged/Private/GameModes/MainMenu/UDFMenuButtonUserWidget.cpp
#include "GameModes/MainMenu/UDFMenuButtonUserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/WidgetAnimation.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"

void UDFMenuButtonUserWidget::SetButtonLabelText(FText const& InText)
{
	if (ButtonLabel)
	{
		ButtonLabel->SetText(InText);
	}
}

void UDFMenuButtonUserWidget::SetSubLabelText(FText const& InText, bool const bShow)
{
	if (SubLabel)
	{
		SubLabel->SetText(InText);
		SubLabel->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UDFMenuButtonUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button)
	{
		Button->OnClicked.AddDynamic(this, &UDFMenuButtonUserWidget::OnBtnClicked);
		Button->OnPressed.AddDynamic(this, &UDFMenuButtonUserWidget::OnBtnPressed);
		Button->OnReleased.AddDynamic(this, &UDFMenuButtonUserWidget::OnBtnReleased);
		Button->OnHovered.AddDynamic(this, &UDFMenuButtonUserWidget::OnBtnHovered);
		Button->OnUnhovered.AddDynamic(this, &UDFMenuButtonUserWidget::OnBtnUnhovered);
	}
}

void UDFMenuButtonUserWidget::NativeDestruct()
{
	if (Button)
	{
		Button->OnClicked.RemoveAll(this);
		Button->OnPressed.RemoveAll(this);
		Button->OnReleased.RemoveAll(this);
		Button->OnHovered.RemoveAll(this);
		Button->OnUnhovered.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UDFMenuButtonUserWidget::OnBtnHovered()
{
	if (HoverAnim)
	{
		PlayAnimation(HoverAnim, 0.f, 1, EUMGSequencePlayMode::Forward, 1.f, false);
	}
	if (HoverSound)
	{
		UGameplayStatics::PlaySound2D(this, HoverSound, 0.4f, 1.f, 0.f, nullptr, nullptr, false);
	}
}

void UDFMenuButtonUserWidget::OnBtnUnhovered() {}

void UDFMenuButtonUserWidget::OnBtnPressed()
{
	if (PressAnim)
	{
		PlayAnimation(PressAnim, 0.f, 1, EUMGSequencePlayMode::Forward, 1.f, false);
	}
}

void UDFMenuButtonUserWidget::OnBtnReleased() {}

void UDFMenuButtonUserWidget::OnBtnClicked()
{
	if (ClickSound)
	{
		UGameplayStatics::PlaySound2D(this, ClickSound, 0.6f, 1.f, 0.f, nullptr, nullptr, false);
	}
	OnMenuButtonClicked.Broadcast();
}
