// Source/DungeonForged/Private/Events/UDFRandomEventWidget.cpp

#include "Events/UDFRandomEventWidget.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Characters/ADFPlayerState.h"
#include "Events/UDFEventChoiceButtonWidget.h"
#include "Events/UDFEventOutcomeWidget.h"
#include "Events/UDFRandomEventSubsystem.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void UDFRandomEventWidget::PresentEvent(
	const FDFRandomEventRow& Row, const FName SourceRowName, const bool bShowHintWhenMet)
{
	EventRowName = SourceRowName;
	for (TObjectPtr<UDFEventChoiceButtonWidget>& B : SpawnedChoiceButtons)
	{
		if (IsValid(B))
		{
			B->RemoveFromParent();
		}
	}
	SpawnedChoiceButtons.Reset();
	if (EventIllustration)
	{
		if (Row.EventIllustration)
		{
			EventIllustration->SetBrushFromTexture(Row.EventIllustration, true);
			EventIllustration->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			EventIllustration->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (EventTitle)
	{
		EventTitle->SetText(Row.EventTitle);
	}
	if (EventDescription)
	{
		EventDescription->SetText(Row.EventDescription);
	}
	if (!ChoiceListRoot || !ChoiceButtonClass)
	{
		return;
	}
	ChoiceListRoot->ClearChildren();
	UDFRandomEventSubsystem* const EvSub = GetWorld() ? GetWorld()->GetSubsystem<UDFRandomEventSubsystem>() : nullptr;
	ADFPlayerCharacter* const P = GetDFPlayerCharacter();
	for (int32 I = 0; I < Row.Choices.Num(); ++I)
	{
		UDFEventChoiceButtonWidget* const Btn = CreateWidget<UDFEventChoiceButtonWidget>(this, ChoiceButtonClass);
		if (!Btn)
		{
			continue;
		}
		const FDFEventChoice& Ch = Row.Choices[I];
		const bool bMet = !EvSub || EvSub->DoesPlayerMeetChoiceRequirements(Ch, P);
		FText Hint = FText::GetEmpty();
		if (!bMet)
		{
			if (Ch.RequiredAgility > 0.f)
			{
				Hint = FText::Format(
					NSLOCTEXT("DF", "EventNeedAgility", "Requires more than {0} Agility (Agility check)"),
					FText::AsNumber(FMath::RoundToInt(Ch.RequiredAgility)));
			}
			else if (Ch.RequiredTags.Num() > 0)
			{
				Hint = NSLOCTEXT("DF", "EventLockedTags", "Locked: missing required tags");
			}
			else
			{
				Hint = NSLOCTEXT("DF", "EventUnavailable", "Unavailable");
			}
		}
		const ESlateVisibility Idle =
			(bMet && bShowHintWhenMet) ? ESlateVisibility::Collapsed : ESlateVisibility::Hidden;
		Btn->Configure(I, Ch, Hint, bMet, Idle);
		Btn->OnChoiceClicked.AddDynamic(this, &UDFRandomEventWidget::HandleChoiceClicked);
		ChoiceListRoot->AddChild(Btn);
		SpawnedChoiceButtons.Add(Btn);
	}
}

void UDFRandomEventWidget::HandleChoiceClicked(int32 const ChoiceIndex, FDFEventChoice const Choice)
{
	(void)ChoiceIndex;
	CommitChoice(Choice);
}

void UDFRandomEventWidget::CommitChoice(const FDFEventChoice& Choice)
{
	ADFPlayerCharacter* const P = GetDFPlayerCharacter();
	ADFPlayerState* const PS = GetDFPlayerState();
	if (!IsValid(P) || !PS)
	{
		return;
	}
	if (!GetOwningLocalPlayer())
	{
		return;
	}
	PS->Server_ExecuteRandomEventChoice(Choice, EventRowName);
	RemoveFromParent();
	APlayerController* const PC = GetOwningPlayer();
	if (OutcomeWidgetClass && PC)
	{
		if (UDFEventOutcomeWidget* const O = CreateWidget<UDFEventOutcomeWidget>(PC, OutcomeWidgetClass))
		{
			O->ShowOutcomeAndClose(Choice.OutcomeText, 2.f);
			O->AddToViewport(1001);
		}
	}
}

void UDFRandomEventWidget::NativeDestruct()
{
	for (TObjectPtr<UDFEventChoiceButtonWidget>& B : SpawnedChoiceButtons)
	{
		if (IsValid(B))
		{
			B->OnChoiceClicked.RemoveAll(this);
		}
	}
	SpawnedChoiceButtons.Reset();
	Super::NativeDestruct();
}
