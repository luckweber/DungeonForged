// Source/DungeonForged/Private/GameModes/Run/UDFDefeatScreenWidget.cpp
#include "GameModes/Run/UDFDefeatScreenWidget.h"
#include "GameModes/Run/ADFRunPlayerController.h"
#include "Audio/UDFMusicManagerSubsystem.h"
#include "Engine/DataTable.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"

void UDFDefeatScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UWorld* const W = GetWorld())
	{
		if (UDFMusicManagerSubsystem* const Mus = W->GetSubsystem<UDFMusicManagerSubsystem>())
		{
			Mus->SetMusicState(EMusicState::Death);
		}
	}
	if (BackgroundArt && DesaturationPostProcessMaterial)
	{
		UMaterialInstanceDynamic* const MID = UMaterialInstanceDynamic::Create(DesaturationPostProcessMaterial, this);
		if (MID)
		{
			BackgroundArt->SetBrushFromMaterial(MID);
		}
	}
	if (ReturnNexus)
	{
		ReturnNexus->OnClicked.AddDynamic(this, &UDFDefeatScreenWidget::HandleReturnNexus);
	}
	if (PlayAgain)
	{
		PlayAgain->OnClicked.AddDynamic(this, &UDFDefeatScreenWidget::HandlePlayAgain);
	}
}

void UDFDefeatScreenWidget::NativeDestruct()
{
	if (ReturnNexus)
	{
		ReturnNexus->OnClicked.RemoveDynamic(this, &UDFDefeatScreenWidget::HandleReturnNexus);
	}
	if (PlayAgain)
	{
		PlayAgain->OnClicked.RemoveDynamic(this, &UDFDefeatScreenWidget::HandlePlayAgain);
	}
	Super::NativeDestruct();
}

void UDFDefeatScreenWidget::SetDefeatData(
	const FDFRunSummary& Summary, const FText& DefeatCause, const FText& OptionalTip)
{
	if (TitleText)
	{
		TitleText->SetText(NSLOCTEXT("Run", "DefeatTitle", "VOCÊ MORREU"));
	}
	if (FloorText)
	{
		FloorText->SetText(FText::Format(
			NSLOCTEXT("Run", "DefeatFloor", "Chegou ao Andar {0}"),
			FText::AsNumber(Summary.FloorReached)));
	}
	if (CauseText)
	{
		CauseText->SetText(
			FText::Format(
				NSLOCTEXT("Run", "DefeatCauseFmt", "Derrotado por: {0}"), DefeatCause));
	}
	if (TipText)
	{
		if (OptionalTip.IsEmpty() && TipsTable)
		{
			const TArray<FName> Names = TipsTable->GetRowNames();
			if (Names.Num() > 0)
			{
				const FName Picked = Names[UKismetMathLibrary::RandomIntegerInRange(0, Names.Num() - 1)];
				// WBP/rows: if row has a text column, read in Blueprint; placeholder string:
				(void)Picked;
			}
		}
		if (!OptionalTip.IsEmpty())
		{
			TipText->SetText(OptionalTip);
		}
	}
	(void)Summary.Gold;
}

void UDFDefeatScreenWidget::HandleReturnNexus()
{
	if (ADFRunPlayerController* const DPC = Cast<ADFRunPlayerController>(GetOwningPlayer()))
	{
		DPC->RequestReturnToNexus(ERunNexusTravelReason::Defeat);
	}
	RemoveFromParent();
}

void UDFDefeatScreenWidget::HandlePlayAgain()
{
	RemoveFromParent();
}
