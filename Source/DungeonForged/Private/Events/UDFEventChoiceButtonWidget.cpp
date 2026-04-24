// Source/DungeonForged/Private/Events/UDFEventChoiceButtonWidget.cpp

#include "Events/UDFEventChoiceButtonWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UDFEventChoiceButtonWidget::Configure(
	const int32 InIndex,
	const FDFEventChoice& InChoice,
	const FText& InHint,
	const bool bInEnabled,
	const ESlateVisibility InHintWhenIdleVisibility)
{
	Index = InIndex;
	ChoicePayload = InChoice;
	bPayloadEnabled = bInEnabled;
	HintIdleVis = InHintWhenIdleVisibility;
	if (ChoiceText)
	{
		ChoiceText->SetText(InChoice.ChoiceText);
	}
	if (OutcomeHint)
	{
		OutcomeHint->SetText(InHint);
		OutcomeHint->SetVisibility(bInEnabled ? HintIdleVis : ESlateVisibility::Visible);
	}
	if (MainButton)
	{
		MainButton->SetIsEnabled(bInEnabled);
	}
}

void UDFEventChoiceButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (MainButton)
	{
		MainButton->OnClicked.AddDynamic(this, &UDFEventChoiceButtonWidget::HandleClicked);
		MainButton->OnHovered.AddDynamic(this, &UDFEventChoiceButtonWidget::HandleHovered);
		MainButton->OnUnhovered.AddDynamic(this, &UDFEventChoiceButtonWidget::HandleUnhovered);
	}
}

void UDFEventChoiceButtonWidget::NativeDestruct()
{
	if (MainButton)
	{
		MainButton->OnClicked.RemoveAll(this);
		MainButton->OnHovered.RemoveAll(this);
		MainButton->OnUnhovered.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UDFEventChoiceButtonWidget::HandleClicked()
{
	if (bPayloadEnabled)
	{
		OnChoiceClicked.Broadcast(Index, ChoicePayload);
	}
}

void UDFEventChoiceButtonWidget::HandleHovered()
{
	if (OutcomeHint)
	{
		OutcomeHint->SetVisibility(ESlateVisibility::Visible);
	}
}

void UDFEventChoiceButtonWidget::HandleUnhovered()
{
	if (OutcomeHint)
	{
		OutcomeHint->SetVisibility(bPayloadEnabled ? HintIdleVis : ESlateVisibility::Visible);
	}
}
