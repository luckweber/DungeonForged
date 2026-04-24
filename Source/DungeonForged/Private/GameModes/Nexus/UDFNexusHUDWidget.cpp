// Source/DungeonForged/Private/GameModes/Nexus/UDFNexusHUDWidget.cpp
#include "GameModes/Nexus/UDFNexusHUDWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Internationalization/Text.h"

void UDFNexusHUDWidget::SetMetaInfo(
	const int32 MetaLevel,
	const int32 TotalRuns,
	const int32 TotalWins,
	const float XPFill)
{
	if (MetaLevelText)
	{
		MetaLevelText->SetText(FText::Format(
			NSLOCTEXT("DFNexus", "MetaLevelFmt", "Nexus Nv. {0}"),
			FText::AsNumber(MetaLevel)));
	}
	if (RunStats)
	{
		RunStats->SetText(FText::Format(
			NSLOCTEXT("DFNexus", "RunStatsFmt", "Runs: {0} | Vitorias: {1}"),
			FText::AsNumber(TotalRuns),
			FText::AsNumber(TotalWins)));
	}
	if (MetaXPBar)
	{
		MetaXPBar->SetPercent(FMath::Clamp(XPFill, 0.f, 1.f));
	}
}
