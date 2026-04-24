// Source/DungeonForged/Private/UI/Status/UDFStatusEffectTooltipWidget.cpp
#include "UI/Status/UDFStatusEffectTooltipWidget.h"
#include "Components/TextBlock.h"

void UDFStatusEffectTooltipWidget::SetTooltipContent(
	const FText& Name,
	const FText& Description,
	const FText& TimeLine)
{
	if (NameText)
	{
		NameText->SetText(Name);
	}
	if (DescriptionText)
	{
		DescriptionText->SetText(Description);
	}
	if (TimeText)
	{
		TimeText->SetText(TimeLine);
	}
}
