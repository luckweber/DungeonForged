// Source/DungeonForged/Private/World/UDFLoadingScreenSubsystem.cpp
#include "World/UDFLoadingScreenSubsystem.h"
#include "World/UDFLoadingScreenWidget.h"
#include "World/UDFWorldTransitionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"
#include "HAL/PlatformTime.h"
#include "TimerManager.h"

void UDFLoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UDFLoadingScreenSubsystem::HandlePostLoadMapWithWorld);
}

void UDFLoadingScreenSubsystem::Deinitialize()
{
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UWorld* const W = GI->GetWorld())
		{
			W->GetTimerManager().ClearTimer(ProgressTimer);
			W->GetTimerManager().ClearTimer(MinTimeTimer);
			W->GetTimerManager().ClearTimer(FadeTimer);
		}
	}
	Super::Deinitialize();
}

void UDFLoadingScreenSubsystem::ShowLoadingScreen(
	const ETravelReason Reason, const int32 NextFloorNumber, const int32 MaxFloors)
{
	UGameInstance* const GI = GetGameInstance();
	if (!GI)
	{
		return;
	}
	if (!LoadingScreenClass)
	{
		if (UDFWorldTransitionSubsystem* const T = GI->GetSubsystem<UDFWorldTransitionSubsystem>())
		{
			T->NotifyLoadingFinished();
		}
		return;
	}
	ShownReason = Reason;
	LoadingStartTime = FPlatformTime::Seconds();
	APlayerController* PC = nullptr;
	if (UWorld* const W = GI->GetWorld())
	{
		PC = W->GetFirstPlayerController();
	}
	if (!PC)
	{
		PC = UGameplayStatics::GetPlayerController(GI, 0);
	}
	if (!PC)
	{
		return;
	}
	ActiveLoadingScreen = CreateWidget<UUserWidget>(PC, LoadingScreenClass);
	ActiveLoadingCxx = Cast<UDFLoadingScreenWidget>(ActiveLoadingScreen);
	if (ActiveLoadingCxx)
	{
		ActiveLoadingCxx->ResetForTravel();
		switch (Reason)
		{
		case ETravelReason::NewRun:
			ActiveLoadingCxx->SetLoadingTitleText(
				NSLOCTEXT("DF", "LoadTitleNewRun", "Gerando Dungeon..."));
			ActiveLoadingCxx->SetFlavorText(
				NSLOCTEXT("DF", "LoadFlavorNewRun", "A masmorra desperta; cada passo forja a lenda."));
			break;
		case ETravelReason::NextFloor:
			ActiveLoadingCxx->SetLoadingTitleText(
				FText::Format(
					NSLOCTEXT("DF", "LoadTitleFloor", "Andar {0}..."),
					FText::AsNumber(NextFloorNumber)));
			ActiveLoadingCxx->SetFloorCopy(NextFloorNumber, MaxFloors, FText::GetEmpty());
			ActiveLoadingCxx->SetRunProgress(
				static_cast<float>(NextFloorNumber) / static_cast<float>(FMath::Max(1, MaxFloors)));
			ActiveLoadingCxx->SetFlavorText(
				NSLOCTEXT("DF", "LoadFlavorFloor", "Subindo — os corredores mudam, mas a procura nao cessa."));
			break;
		case ETravelReason::Victory:
			ActiveLoadingCxx->SetLoadingTitleText(
				NSLOCTEXT("DF", "LoadTitleVictory", "Retornando ao Nexus..."));
			ActiveLoadingCxx->SetFlavorText(
				NSLOCTEXT("DF", "LoadFlavorVictory", "O portal brilha; a forja do hub aguarda teus trofeus."));
			break;
		case ETravelReason::Defeat:
		case ETravelReason::AbandonRun:
			ActiveLoadingCxx->SetLoadingTitleText(
				NSLOCTEXT("DF", "LoadTitleReturn", "Retornando ao Nexus..."));
			ActiveLoadingCxx->SetFlavorText(
				NSLOCTEXT("DF", "LoadFlavorDefeat", "Toda queda e runa; levanta e forja de novo."));
			break;
		case ETravelReason::FirstLaunch:
		default:
			ActiveLoadingCxx->SetLoadingTitleText(
				NSLOCTEXT("DF", "LoadTitleDefault", "Carregando..."));
			ActiveLoadingCxx->SetFlavorText(FText::GetEmpty());
			break;
		}
		ActiveLoadingCxx->SetTip(
			NSLOCTEXT("DF", "LoadTipLabel", "Dica"),
			NSLOCTEXT("DF", "LoadTipBody", "Use o terreno. Quebrar a linha de visao nega o encanto inimigo."));
	}
	ActiveLoadingScreen->AddToViewport(100);
	FInputModeUIOnly M;
	PC->SetInputMode(M);
	PC->SetShowMouseCursor(false);
	// 0 -> 0.9 over 1.5s in steps
	LerpStepIndex = 0;
	if (UWorld* const W = GI->GetWorld())
	{
		ProgressTickDelegate = FTimerDelegate::CreateUObject(this, &UDFLoadingScreenSubsystem::LerpProgressStep);
		W->GetTimerManager().SetTimer(ProgressTimer, ProgressTickDelegate, 0.05f, true);
	}
}

void UDFLoadingScreenSubsystem::LerpProgressStep()
{
	UGameInstance* const GI = GetGameInstance();
	if (!ActiveLoadingCxx || !GI)
	{
		return;
	}
	++LerpStepIndex;
	const float Alpha = FMath::Clamp(static_cast<float>(LerpStepIndex) / static_cast<float>(LerpStepCount), 0.f, 1.f);
	ActiveLoadingCxx->SetLoadingProgress(Alpha * 0.9f, false);
	if (LerpStepIndex >= LerpStepCount)
	{
		if (UWorld* const W = GI->GetWorld())
		{
			W->GetTimerManager().ClearTimer(ProgressTimer);
		}
	}
}

void UDFLoadingScreenSubsystem::HandlePostLoadMapWithWorld(UWorld* const LoadedWorld)
{
	if (!ActiveLoadingScreen)
	{
		return;
	}
	if (!LoadedWorld || !GetGameInstance() || GetGameInstance()->GetWorld() != LoadedWorld)
	{
		return;
	}
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UWorld* const W = GI->GetWorld())
		{
			W->GetTimerManager().ClearTimer(ProgressTimer);
		}
	}
	if (ActiveLoadingCxx)
	{
		ActiveLoadingCxx->SetLoadingProgress(1.f, true);
	}
	// Slight delay so percent paints before hide pipeline
	FTimerHandle H;
	LoadedWorld->GetTimerManager().SetTimer(
		H, FTimerDelegate::CreateUObject(this, &UDFLoadingScreenSubsystem::HideLoadingScreen), 0.05f, false);
}

void UDFLoadingScreenSubsystem::HideLoadingScreen()
{
	UGameInstance* const GI = GetGameInstance();
	if (!GI)
	{
		return;
	}
	const double Elapsed = FPlatformTime::Seconds() - LoadingStartTime;
	const float Rem = FMath::Max(0.f, static_cast<float>(MinLoadingTime) - static_cast<float>(Elapsed));
	if (UWorld* const W = GI->GetWorld())
	{
		if (Rem > 0.01f)
		{
			W->GetTimerManager().SetTimer(
				MinTimeTimer,
				FTimerDelegate::CreateUObject(this, &UDFLoadingScreenSubsystem::FinishHideAfterMinTime),
				Rem,
				false);
			return;
		}
	}
	FinishHideAfterMinTime();
}

void UDFLoadingScreenSubsystem::FinishHideAfterMinTime()
{
	UGameInstance* const GI = GetGameInstance();
	if (!GI)
	{
		return;
	}
	if (!ActiveLoadingScreen)
	{
		if (UDFWorldTransitionSubsystem* const T = GI->GetSubsystem<UDFWorldTransitionSubsystem>())
		{
			T->NotifyLoadingFinished();
		}
		return;
	}
	// 0.5s fade: animation length in asset; playback at normal speed
	if (UDFLoadingScreenWidget* const Cxx = ActiveLoadingCxx)
	{
		Cxx->PlayFadeOutAnimation();
	}
	if (UWorld* const W = GI->GetWorld())
	{
		W->GetTimerManager().SetTimer(
			FadeTimer,
			FTimerDelegate::CreateUObject(this, &UDFLoadingScreenSubsystem::OnFadeOutRemoveWidget),
			0.5f,
			false);
	}
}

void UDFLoadingScreenSubsystem::OnFadeOutRemoveWidget()
{
	if (ActiveLoadingScreen)
	{
		ActiveLoadingScreen->RemoveFromParent();
	}
	ActiveLoadingScreen = nullptr;
	ActiveLoadingCxx = nullptr;
	if (UGameInstance* const G = GetGameInstance())
	{
		if (UDFWorldTransitionSubsystem* const T = G->GetSubsystem<UDFWorldTransitionSubsystem>())
		{
			T->NotifyLoadingFinished();
		}
	}
}
