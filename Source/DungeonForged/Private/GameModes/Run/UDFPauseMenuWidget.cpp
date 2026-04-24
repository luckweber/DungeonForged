// Source/DungeonForged/Private/GameModes/Run/UDFPauseMenuWidget.cpp
#include "GameModes/Run/UDFPauseMenuWidget.h"
#include "GameModes/Run/ADFRunGameState.h"
#include "GameModes/Run/ADFRunPlayerController.h"
#include "World/UDFWorldTransitionSubsystem.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"

void UDFPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Resume)
	{
		Resume->OnClicked.AddDynamic(this, &UDFPauseMenuWidget::HandleResume);
	}
	if (Options)
	{
		Options->OnClicked.AddDynamic(this, &UDFPauseMenuWidget::HandleOptions);
	}
	if (AbandonRun)
	{
		AbandonRun->OnClicked.AddDynamic(this, &UDFPauseMenuWidget::HandleAbandonRun);
	}
	if (RunStatsText)
	{
		if (UWorld* const W = GetWorld())
		{
			if (const ADFRunGameState* const RGS = W->GetGameState<ADFRunGameState>())
			{
				const int32 TotalS = FMath::Max(0, FMath::RoundToInt(RGS->ElapsedRunTime));
				const int32 M = TotalS / 60;
				const int32 Ss = TotalS % 60;
				RunStatsText->SetText(FText::FromString(FString::Printf(
					TEXT("Andar %d | Kills %d | Ouro %d | Tempo %02d:%02d"),
					RGS->CurrentFloor,
					RGS->TotalKills,
					RGS->TotalGoldCollected,
					M,
					Ss)));
			}
		}
	}
}

void UDFPauseMenuWidget::NativeDestruct()
{
	if (Resume)
	{
		Resume->OnClicked.RemoveDynamic(this, &UDFPauseMenuWidget::HandleResume);
	}
	if (Options)
	{
		Options->OnClicked.RemoveDynamic(this, &UDFPauseMenuWidget::HandleOptions);
	}
	if (AbandonRun)
	{
		AbandonRun->OnClicked.RemoveDynamic(this, &UDFPauseMenuWidget::HandleAbandonRun);
	}
	Super::NativeDestruct();
}

void UDFPauseMenuWidget::HandleResume()
{
	RemoveFromParent();
	UGameplayStatics::SetGamePaused(GetWorld(), false);
	if (APlayerController* const PC = GetOwningPlayer())
	{
		if (ADFRunPlayerController* const DPC = Cast<ADFRunPlayerController>(PC))
		{
			DPC->SetupInputModeGameplay();
		}
	}
}

void UDFPauseMenuWidget::HandleOptions()
{
	if (!OptionsScreenClass || !GetOwningPlayer())
	{
		return;
	}
	OptionsScreenInstance = CreateWidget<UUserWidget>(GetOwningPlayer(), OptionsScreenClass);
	if (OptionsScreenInstance)
	{
		OptionsScreenInstance->AddToViewport(25);
	}
}

void UDFPauseMenuWidget::HandleAbandonRun()
{
	if (ADFRunPlayerController* const DPC = Cast<ADFRunPlayerController>(GetOwningPlayer()))
	{
		DPC->RequestReturnToNexus(ERunNexusTravelReason::Abandon);
	}
	RemoveFromParent();
}
