// Source/DungeonForged/Private/Events/UDFEventOutcomeWidget.cpp

#include "Events/UDFEventOutcomeWidget.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UDFEventOutcomeWidget::ShowOutcomeAndClose(const FText& Outcome, float const DisplaySeconds)
{
	if (OutcomeText)
	{
		OutcomeText->SetText(Outcome);
	}
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			CloseTimer,
			FTimerDelegate::CreateWeakLambda(
				this,
				[this]()
				{
					RemoveFromParent();
				}),
			FMath::Max(0.1f, DisplaySeconds),
			false);
	}
}

void UDFEventOutcomeWidget::NativeDestruct()
{
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(CloseTimer);
	}
	Super::NativeDestruct();
}
