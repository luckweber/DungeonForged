// Source/DungeonForged/Private/GameModes/MainMenu/UDFConfirmDialogUserWidget.cpp
#include "GameModes/MainMenu/UDFConfirmDialogUserWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Animation/WidgetAnimation.h"

void UDFConfirmDialogUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (ConfirmButton)
	{
		ConfirmButton->OnClicked.AddDynamic(this, &UDFConfirmDialogUserWidget::OnConfirmClicked);
	}
	if (CancelButton)
	{
		CancelButton->OnClicked.AddDynamic(this, &UDFConfirmDialogUserWidget::OnCancelClicked);
	}
}

void UDFConfirmDialogUserWidget::NativeDestruct()
{
	if (ConfirmButton)
	{
		ConfirmButton->OnClicked.RemoveAll(this);
	}
	if (CancelButton)
	{
		CancelButton->OnClicked.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UDFConfirmDialogUserWidget::ShowDialog(FText const& Title, FText const& Body, FSimpleDelegate InOnConfirm)
{
	if (TitleText)
	{
		TitleText->SetText(Title);
	}
	if (BodyText)
	{
		BodyText->SetText(Body);
	}
	OnConfirmDelegate = InOnConfirm;
	PlayOpenPresentation();
}

void UDFConfirmDialogUserWidget::PlayOpenPresentation_Implementation()
{
	if (OpenAnim)
	{
		PlayAnimation(OpenAnim, 0.f, 1, EUMGSequencePlayMode::Forward, 1.f);
	}
}

void UDFConfirmDialogUserWidget::PlayClosePresentation_Implementation()
{
	RemoveFromParent();
}

void UDFConfirmDialogUserWidget::OnConfirmClicked()
{
	if (OnConfirmDelegate.IsBound())
	{
		OnConfirmDelegate.Execute();
	}
	OnConfirmDelegate.Unbind();
	PlayClosePresentation();
}

void UDFConfirmDialogUserWidget::OnCancelClicked()
{
	OnConfirmDelegate.Unbind();
	PlayClosePresentation();
}
