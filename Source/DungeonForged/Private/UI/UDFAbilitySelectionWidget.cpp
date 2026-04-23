// Source/DungeonForged/Private/UI/UDFAbilitySelectionWidget.cpp
#include "UI/UDFAbilitySelectionWidget.h"
#include "UI/UDFAbilityCardWidget.h"
#include "UI/UDFAbilitySelectionSubsystem.h"
#include "Characters/ADFPlayerState.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

void UDFAbilitySelectionWidget::InitializeOffer(ADFPlayerState* InPlayerState, const int32 FloorCleared, const int32 InSkipGold, const int32 InOfferId, const TArray<FDFAbilityRolledChoice>& InChoices, const float InTimerSeconds)
{
	OwnerPlayerState = InPlayerState;
	OfferId = InOfferId;
	TimerSeconds = FMath::Max(0, FMath::RoundToInt(InTimerSeconds));

	if (TitleText)
	{
		TitleText->SetText(FText::FromString(TEXT("Choose an ability")));
	}
	if (SubtitleText)
	{
		SubtitleText->SetText(FText::Format(
			FText::FromString(TEXT("Floor {0} cleared — pick one reward or take gold.")), FText::AsNumber(FloorCleared)));
	}
	if (SkipButtonLabel)
	{
		SkipButtonLabel->SetText(
			FText::Format(FText::FromString(TEXT("Skip ( +{0} gold )")), FText::AsNumber(InSkipGold)));
	}
	else if (SkipButton)
	{
		SkipButton->SetToolTipText(
			FText::Format(FText::FromString(TEXT("Skip: +{0} gold")), FText::AsNumber(InSkipGold)));
	}

	UDFAbilityCardWidget* Cards[] = {Card0, Card1, Card2};
	for (int32 i = 0; i < 3; ++i)
	{
		if (Cards[i])
		{
			if (InChoices.IsValidIndex(i))
			{
				Cards[i]->SetVisibility(ESlateVisibility::Visible);
				Cards[i]->SetupCard(InChoices[i].Data, InChoices[i].RowName);
			}
			else
			{
				Cards[i]->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}

	if (UWorld* W = GetWorld())
	{
		if (UDFAbilitySelectionSubsystem* Sub = W->GetSubsystem<UDFAbilitySelectionSubsystem>())
		{
			Sub->RegisterActiveSelectionWidget(this);
		}
	}

	UnbindLocalEvents();
	if (SkipButton)
	{
		SkipButton->OnClicked.AddDynamic(this, &UDFAbilitySelectionWidget::HandleSkipClicked);
	}
	if (Card0)
	{
		Card0->OnCardSelectClicked.AddDynamic(this, &UDFAbilitySelectionWidget::HandleCard0Clicked);
	}
	if (Card1)
	{
		Card1->OnCardSelectClicked.AddDynamic(this, &UDFAbilitySelectionWidget::HandleCard1Clicked);
	}
	if (Card2)
	{
		Card2->OnCardSelectClicked.AddDynamic(this, &UDFAbilitySelectionWidget::HandleCard2Clicked);
	}

	ApplyTimerVisual(static_cast<float>(TimerSeconds));
	if (TimerSeconds > 0)
	{
		if (UWorld* W = GetWorld())
		{
			UnbindAndClearTimer();
			W->GetTimerManager().SetTimer(
				CountdownHandle,
				this,
				&UDFAbilitySelectionWidget::OnTimerExpired,
				static_cast<float>(TimerSeconds),
				false);
		}
	}
	else if (TimerBar)
	{
		TimerBar->SetPercent(0.f);
	}
}

void UDFAbilitySelectionWidget::NativeDestruct()
{
	UnbindAndClearTimer();
	UnbindLocalEvents();
	if (UWorld* W = GetWorld())
	{
		if (UDFAbilitySelectionSubsystem* Sub = W->GetSubsystem<UDFAbilitySelectionSubsystem>())
		{
			Sub->UnregisterActiveSelectionWidget(this);
		}
	}
	Super::NativeDestruct();
}

void UDFAbilitySelectionWidget::UnbindLocalEvents()
{
	if (SkipButton)
	{
		SkipButton->OnClicked.RemoveAll(this);
	}
	if (Card0) Card0->OnCardSelectClicked.RemoveAll(this);
	if (Card1) Card1->OnCardSelectClicked.RemoveAll(this);
	if (Card2) Card2->OnCardSelectClicked.RemoveAll(this);
}

void UDFAbilitySelectionWidget::UnbindAndClearTimer()
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(CountdownHandle);
	}
}

void UDFAbilitySelectionWidget::ApplyTimerVisual(const float Remaining) const
{
	if (TimerBar)
	{
		const float T = static_cast<float>(FMath::Max(0, TimerSeconds));
		TimerBar->SetPercent((T > 0.f) ? (Remaining / T) : 0.f);
	}
	if (TimerText)
	{
		TimerText->SetText(FText::AsNumber(FMath::CeilToInt(Remaining)));
	}
}

void UDFAbilitySelectionWidget::OnTimerExpired()
{
	HandleSkipClicked();
}

void UDFAbilitySelectionWidget::HandleSkipClicked()
{
	RequestFinish(true, NAME_None);
}

void UDFAbilitySelectionWidget::HandleCard0Clicked()
{
	if (Card0) RequestFinish(false, Card0->GetRowName());
}

void UDFAbilitySelectionWidget::HandleCard1Clicked()
{
	if (Card1) RequestFinish(false, Card1->GetRowName());
}

void UDFAbilitySelectionWidget::HandleCard2Clicked()
{
	if (Card2) RequestFinish(false, Card2->GetRowName());
}

void UDFAbilitySelectionWidget::RequestFinish(const bool bSkipped, FName RowName)
{
	if (bRequestInFlight)
	{
		return;
	}
	ADFPlayerState* PS = OwnerPlayerState.Get();
	if (!PS)
	{
		if (APlayerController* const PC = GetOwningPlayer())
		{
			PS = PC->GetPlayerState<ADFPlayerState>();
		}
	}
	if (!PS)
	{
		return;
	}
	bRequestInFlight = true;
	UnbindAndClearTimer();
	PS->Server_FinishAbilitySelection(OfferId, bSkipped, RowName);
}
