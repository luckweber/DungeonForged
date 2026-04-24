// Source/DungeonForged/Private/GameModes/Run/UDFVictoryScreenWidget.cpp
#include "GameModes/Run/UDFVictoryScreenWidget.h"
#include "GameModes/Run/ADFRunPlayerController.h"
#include "Audio/UDFMusicManagerSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/ProgressBar.h"

void UDFVictoryScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UWorld* const W = GetWorld())
	{
		if (UDFMusicManagerSubsystem* const Mus = W->GetSubsystem<UDFMusicManagerSubsystem>())
		{
			Mus->SetMusicState(EMusicState::Victory);
		}
	}
	if (GoldCelebrationNiagara.IsValid() || !GoldCelebrationNiagara.IsNull())
	{
		if (UWorld* const W = GetWorld())
		{
			if (UNiagaraSystem* const NS = GoldCelebrationNiagara.Get())
			{
				const FVector Loc = UGameplayStatics::GetPlayerPawn(this, 0) ? UGameplayStatics::GetPlayerPawn(
					this, 0)->GetActorLocation() + FVector(0, 0, 200) : FVector::ZeroVector;
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					W, NS, Loc, FRotator::ZeroRotator, FVector(1), true, true, ENCPoolMethod::None, true);
			}
		}
	}
	if (ReturnNexus)
	{
		ReturnNexus->OnClicked.AddDynamic(this, &UDFVictoryScreenWidget::HandleReturnNexus);
	}
	if (PlayAgain)
	{
		PlayAgain->OnClicked.AddDynamic(this, &UDFVictoryScreenWidget::HandlePlayAgain);
	}
}

void UDFVictoryScreenWidget::NativeDestruct()
{
	if (ReturnNexus)
	{
		ReturnNexus->OnClicked.RemoveDynamic(this, &UDFVictoryScreenWidget::HandleReturnNexus);
	}
	if (PlayAgain)
	{
		PlayAgain->OnClicked.RemoveDynamic(this, &UDFVictoryScreenWidget::HandlePlayAgain);
	}
	Super::NativeDestruct();
}

void UDFVictoryScreenWidget::SetSummary(const FDFRunSummary& Summary)
{
	if (TimeText)
	{
		const int32 TotalS = FMath::Max(0, FMath::RoundToInt(Summary.TimeSeconds));
		const int32 M = TotalS / 60;
		const int32 Ss = TotalS % 60;
		TimeText->SetText(FText::FromString(FString::Printf(
			TEXT("Tempo: %02d:%02d"), M, Ss)));
	}
	if (KillsText)
	{
		KillsText->SetText(FText::Format(
			NSLOCTEXT("Run", "VictoryKills", "Inimigos: {0}"), FText::AsNumber(Summary.Kills)));
	}
	if (GoldText)
	{
		GoldText->SetText(FText::Format(
			NSLOCTEXT("Run", "VictoryGold", "Ouro: {0}"), FText::AsNumber(Summary.Gold)));
	}
	if (TitleText)
	{
		TitleText->SetText(NSLOCTEXT("Run", "VictoryTitle", "VITÓRIA!"));
	}
	(void)Summary.AbilitiesCollected;
	(void)Summary.ClassName;
}

void UDFVictoryScreenWidget::HandleReturnNexus()
{
	if (ADFRunPlayerController* const DPC = Cast<ADFRunPlayerController>(GetOwningPlayer()))
	{
		DPC->RequestReturnToNexus(ERunNexusTravelReason::Victory);
	}
	RemoveFromParent();
}

void UDFVictoryScreenWidget::HandlePlayAgain()
{
	RemoveFromParent();
	// WBP: travel to a fresh run or open character select.
}
