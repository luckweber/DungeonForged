// Source/DungeonForged/Private/Dungeon/Traps/UDFTrapDetectionComponent.cpp
#include "Dungeon/Traps/UDFTrapDetectionComponent.h"
#include "Dungeon/Traps/ADFTrapBase.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"

UDFTrapDetectionComponent::UDFTrapDetectionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDFTrapDetectionComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			DetectionTimer,
			this,
			&UDFTrapDetectionComponent::TickDetection,
			0.5f,
			true);
	}
}

void UDFTrapDetectionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(DetectionTimer);
	}
	for (auto& P : IndicatorByTrap)
	{
		if (P.Value)
		{
			P.Value->RemoveFromParent();
		}
	}
	IndicatorByTrap.Empty();
	Super::EndPlay(EndPlayReason);
}

void UDFTrapDetectionComponent::SetTrapHighlightEnabled(const bool bEnabled)
{
	bTrapHighlightEnabled = bEnabled;
	if (!bEnabled)
	{
		for (auto& P : IndicatorByTrap)
		{
			if (ADFTrapBase* T = P.Key.Get())
			{
				T->SetTrapHighlight(false);
			}
			if (P.Value)
			{
				P.Value->RemoveFromParent();
			}
		}
		IndicatorByTrap.Empty();
	}
}

void UDFTrapDetectionComponent::HideTrapHighlight(ADFTrapBase* const Trap)
{
	if (!Trap)
	{
		return;
	}
	Trap->SetTrapHighlight(false);
	for (auto It = IndicatorByTrap.CreateIterator(); It; ++It)
	{
		if (It.Key().Get() == Trap)
		{
			if (It.Value())
			{
				It.Value()->RemoveFromParent();
			}
			It.RemoveCurrent();
			break;
		}
	}
}

void UDFTrapDetectionComponent::TickDetection()
{
	if (!bTrapHighlightEnabled)
	{
		return;
	}
	APawn* const OwnerPawn = Cast<APawn>(GetOwner());
	UWorld* const W = GetWorld();
	if (!OwnerPawn || !W)
	{
		return;
	}
	const FVector O = OwnerPawn->GetActorLocation();
	TSet<ADFTrapBase*> Now;
	for (TActorIterator<ADFTrapBase> It(W); It; ++It)
	{
		ADFTrapBase* const Tr = *It;
		if (!Tr || !Tr->bIsHidden)
		{
			continue;
		}
		if (FVector::Dist(O, Tr->GetActorLocation()) > DetectionRadius)
		{
			continue;
		}
		Now.Add(Tr);
		Tr->SetTrapHighlight(true);
		if (TrapIndicatorClass)
		{
			if (APlayerController* const PC = Cast<APlayerController>(OwnerPawn->GetController()))
			{
				if (!IndicatorByTrap.Contains(Tr))
				{
					if (UUserWidget* Wgt = CreateWidget<UUserWidget>(PC, TrapIndicatorClass))
					{
						IndicatorByTrap.Add(Tr, Wgt);
						Wgt->AddToPlayerScreen(0);
					}
				}
			}
		}
	}
	for (auto It = IndicatorByTrap.CreateIterator(); It; ++It)
	{
		ADFTrapBase* K = It.Key().Get();
		if (!K || !Now.Contains(K))
		{
			if (K)
			{
				K->SetTrapHighlight(false);
			}
			if (It.Value())
			{
				It.Value()->RemoveFromParent();
			}
			It.RemoveCurrent();
		}
	}
}
