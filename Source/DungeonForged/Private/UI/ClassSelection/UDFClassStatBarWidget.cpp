// Source/DungeonForged/Private/UI/ClassSelection/UDFClassStatBarWidget.cpp
#include "UI/ClassSelection/UDFClassStatBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UDFClassStatBarWidget::SetData(
	const FText& Label, const float NormalizedFill, const int32 ValueDisplay, const FLinearColor FillColor)
{
	if (StatLabel) StatLabel->SetText(Label);
	if (StatBar)
	{
		StatBar->SetPercent(FMath::Clamp(NormalizedFill, 0.f, 1.f));
		StatBar->SetFillColorAndOpacity(FillColor);
	}
	if (StatValue) StatValue->SetText(FText::AsNumber(ValueDisplay));
}
