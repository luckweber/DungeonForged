// Source/DungeonForged/Private/Localization/UDFKeyBindRowWidget.cpp
#include "Localization/UDFKeyBindRowWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UDFKeyBindRowWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (KeyButton)
	{
		KeyButton->OnClicked.AddDynamic(this, &UDFKeyBindRowWidget::HandleKeyButtonPressed);
	}
}

void UDFKeyBindRowWidget::NativeDestruct()
{
	if (KeyButton)
	{
		KeyButton->OnClicked.RemoveDynamic(this, &UDFKeyBindRowWidget::HandleKeyButtonPressed);
	}
	Super::NativeDestruct();
}

void UDFKeyBindRowWidget::SetCurrentKeyText(const FText& T)
{
	if (KeyNameText)
	{
		KeyNameText->SetText(T);
	}
}

void UDFKeyBindRowWidget::SetActionLabelText(const FText& T)
{
	if (ActionLabel)
	{
		ActionLabel->SetText(T);
	}
}

void UDFKeyBindRowWidget::HandleKeyButtonPressed()
{
	OnRequestRebind.ExecuteIfBound(MappingName);
}
