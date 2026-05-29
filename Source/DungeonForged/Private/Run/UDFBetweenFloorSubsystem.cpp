// Source/DungeonForged/Private/Run/UDFBetweenFloorSubsystem.cpp

#include "Run/UDFBetweenFloorSubsystem.h"
#include "ADFDungeonManager.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Characters/ADFPlayerState.h"
#include "Events/UDFRandomEventSubsystem.h"
#include "Events/UDFRandomEventWidget.h"
#include "GameModes/Run/ADFRunGameMode.h"
#include "GameModes/Run/ADFRunGameState.h"
#include "GameModes/Run/ADFRunPlayerController.h"
#include "GAS/Effects/UDFGameplayEffectLibrary.h"
#include "GAS/UDFAttributeSet.h"
#include "Merchant/ADFMerchantActor.h"
#include "EngineUtils.h"
#include "Run/DFRunManager.h"
#include "UI/UDFAbilitySelectionSubsystem.h"
#include "World/UDFWorldTransitionSubsystem.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void UDFBetweenFloorSubsystem::StartBetweenFloorFlow(ADFRunGameMode* const GameMode)
{
	if (!IsAuthorityWorld() || !GameMode)
	{
		return;
	}
	if (bFlowActive)
	{
		return;
	}
	UGameInstance* const GI = GetWorld()->GetGameInstance();
	UDFDungeonManager* const DM = GI ? GI->GetSubsystem<UDFDungeonManager>() : nullptr;
	if (!DM)
	{
		return;
	}
	bFlowActive = true;
	OwningGameMode = GameMode;
	ClearedFloorNumber = DM->CurrentFloor;
	StepQueueIndex = INDEX_NONE;
	CurrentStep = EBetweenFloorStep::None;
	PendingEventRowName = NAME_None;
	bAwaitingClientStep = false;

	if (UGameInstance* const GI2 = GetWorld()->GetGameInstance())
	{
		if (UDFRunManager* const RM = GI2->GetSubsystem<UDFRunManager>())
		{
			RM->CaptureRunState();
		}
	}
	if (ADFRunGameState* const RGS = GameMode->GetGameState<ADFRunGameState>())
	{
		RGS->SetPhase(ERunPhase::BetweenFloors);
	}
	PauseForBetweenFloorUI();
	BuildStepQueue(ClearedFloorNumber);
	EnterNextStep();
}

void UDFBetweenFloorSubsystem::BuildStepQueue(const int32 ClearedFloor)
{
	StepQueue.Reset();
	UDFRandomEventSubsystem* const Ev = GetWorld()->GetSubsystem<UDFRandomEventSubsystem>();
	if (Ev && Ev->ShouldTriggerEvent(ClearedFloor))
	{
		FName RowName;
		if (const FDFRandomEventRow* const Row = Ev->RollEvent(ClearedFloor, RowName))
		{
			PendingEventRowName = RowName;
			PendingEventRow = *Row;
			StepQueue.Add(EBetweenFloorStep::Event);
		}
	}
	StepQueue.Add(EBetweenFloorStep::Rest);
	if (ShopEveryNFloors > 0 && ClearedFloor > 0 && (ClearedFloor % ShopEveryNFloors) == 0)
	{
		StepQueue.Add(EBetweenFloorStep::Shop);
	}
	StepQueue.Add(EBetweenFloorStep::Draft);
	StepQueue.Add(EBetweenFloorStep::AdvanceFloor);
}

void UDFBetweenFloorSubsystem::EnterNextStep()
{
	if (!IsAuthorityWorld())
	{
		return;
	}
	++StepQueueIndex;
	if (!StepQueue.IsValidIndex(StepQueueIndex))
	{
		FinishFlowAndAdvanceFloor();
		return;
	}
	EnterStep(StepQueue[StepQueueIndex]);
}

void UDFBetweenFloorSubsystem::EnterStep(const EBetweenFloorStep Step)
{
	CurrentStep = Step;
	bAwaitingClientStep = false;
	switch (Step)
	{
	case EBetweenFloorStep::Event:
		if (PendingEventRowName.IsNone() || !RandomEventWidgetClass)
		{
			EnterNextStep();
			return;
		}
		bAwaitingClientStep = true;
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (ADFRunPlayerController* const RPC = Cast<ADFRunPlayerController>(It->Get()))
			{
				RPC->Client_PresentBetweenFloorUI();
			}
		}
		break;
	case EBetweenFloorStep::Rest:
		ApplyRestHeal();
		if (UWorld* const W = GetWorld())
		{
			W->GetTimerManager().SetTimer(
				RestAdvanceTimer,
				this,
				&UDFBetweenFloorSubsystem::HandleRestTimerElapsed,
				FMath::Max(0.1f, RestAutoAdvanceSeconds),
				false);
		}
		break;
	case EBetweenFloorStep::Shop:
		{
			bool bHasMerchant = false;
			for (TActorIterator<ADFMerchantActor> It(GetWorld()); It; ++It)
			{
				bHasMerchant = true;
				It->GenerateStock();
				break;
			}
			if (!bHasMerchant)
			{
				EnterNextStep();
				return;
			}
			bAwaitingClientStep = true;
			for (FConstPlayerControllerIterator PIt = GetWorld()->GetPlayerControllerIterator(); PIt; ++PIt)
			{
				if (ADFRunPlayerController* const RPC = Cast<ADFRunPlayerController>(PIt->Get()))
				{
					RPC->Client_PresentBetweenFloorUI();
				}
			}
		}
		break;
	case EBetweenFloorStep::Draft:
		BeginDraftStep();
		break;
	case EBetweenFloorStep::AdvanceFloor:
		FinishFlowAndAdvanceFloor();
		break;
	default:
		EnterNextStep();
		break;
	}
}

void UDFBetweenFloorSubsystem::HandleRestTimerElapsed()
{
	if (CurrentStep == EBetweenFloorStep::Rest)
	{
		EnterNextStep();
	}
}

void UDFBetweenFloorSubsystem::ApplyRestHeal()
{
	if (RestHealFraction <= 0.f)
	{
		return;
	}
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ADFPlayerCharacter* const P = (*It)->GetPawn<ADFPlayerCharacter>())
		{
			if (UAbilitySystemComponent* const ASC = P->GetAbilitySystemComponent())
			{
				const float MaxH = ASC->GetNumericAttribute(
					UDFAttributeSet::GetMaxHealthAttribute());
				const float HealAmt = MaxH * RestHealFraction;
				if (HealAmt > 0.f)
				{
					const FGameplayEffectSpecHandle SpecH =
						UDFGameplayEffectLibrary::MakeHealEffect(HealAmt, P);
					if (SpecH.IsValid())
					{
						ASC->ApplyGameplayEffectSpecToSelf(*SpecH.Data);
					}
				}
			}
		}
	}
}

void UDFBetweenFloorSubsystem::BeginDraftStep()
{
	UGameInstance* const GI = GetWorld()->GetGameInstance();
	UDFDungeonManager* const DM = GI ? GI->GetSubsystem<UDFDungeonManager>() : nullptr;
	if (!DM)
	{
		EnterNextStep();
		return;
	}
	DM->bFloorOfferResolved = false;
	++DM->ActiveFloorOfferId;
	if (UDFAbilitySelectionSubsystem* const Sub = GetWorld()->GetSubsystem<UDFAbilitySelectionSubsystem>())
	{
		const TArray<FDFAbilityRolledChoice> Choices = Sub->RollAbilityChoices(3);
		const int32 FloorForUi = ClearedFloorNumber;
		const int32 SkipG = Sub->SkipGoldReward;
		if (Choices.Num() == 0)
		{
			DM->bFloorOfferResolved = true;
			EnterNextStep();
			return;
		}
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* const Pc = It->Get())
			{
				if (ADFPlayerState* const PState = Pc->GetPlayerState<ADFPlayerState>())
				{
					PState->Client_OpenAbilitySelectionScreen(
						FloorForUi, Choices, SkipG, DM->ActiveFloorOfferId, 30.f);
				}
			}
		}
	}
	else
	{
		DM->bFloorOfferResolved = true;
		EnterNextStep();
	}
}

void UDFBetweenFloorSubsystem::NotifyEventResolved()
{
	if (!bFlowActive || CurrentStep != EBetweenFloorStep::Event)
	{
		return;
	}
	bAwaitingClientStep = false;
	EnterNextStep();
}

void UDFBetweenFloorSubsystem::NotifyClientStepFinished(ADFRunPlayerController* const FromController)
{
	(void)FromController;
	if (!bFlowActive || !bAwaitingClientStep)
	{
		return;
	}
	if (CurrentStep != EBetweenFloorStep::Shop && CurrentStep != EBetweenFloorStep::Event)
	{
		return;
	}
	bAwaitingClientStep = false;
	EnterNextStep();
}

void UDFBetweenFloorSubsystem::NotifyDraftResolved(
	const bool bSkipped, const FName SelectedRowName, ADFPlayerState* const FromPlayerState)
{
	if (!bFlowActive || CurrentStep != EBetweenFloorStep::Draft)
	{
		return;
	}
	UGameInstance* const GI = GetWorld()->GetGameInstance();
	UDFDungeonManager* const DM = GI ? GI->GetSubsystem<UDFDungeonManager>() : nullptr;
	if (!DM || DM->bFloorOfferResolved)
	{
		return;
	}
	DM->bFloorOfferResolved = true;
	if (UWorld* const W = GetWorld())
	{
		if (UDFAbilitySelectionSubsystem* const Sub = W->GetSubsystem<UDFAbilitySelectionSubsystem>())
		{
			ADFPlayerCharacter* const P = FromPlayerState ? FromPlayerState->GetPawn<ADFPlayerCharacter>() : nullptr;
			if (bSkipped)
			{
				Sub->SkipSelection(P);
			}
			else if (!SelectedRowName.IsNone() && P)
			{
				Sub->GrantSelectedAbility(SelectedRowName, P);
			}
		}
		for (FConstPlayerControllerIterator It = W->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* const PCG = It->Get())
			{
				if (ADFPlayerState* const Ops = PCG->GetPlayerState<ADFPlayerState>())
				{
					Ops->Client_ResumeAfterAbilitySelection();
				}
			}
		}
	}
	EnterNextStep();
}

void UDFBetweenFloorSubsystem::FinishFlowAndAdvanceFloor()
{
	if (!IsAuthorityWorld())
	{
		return;
	}
	ResumeTimeAfterBetweenFloorUI();
	UGameInstance* const GI = GetWorld()->GetGameInstance();
	UDFDungeonManager* const DM = GI ? GI->GetSubsystem<UDFDungeonManager>() : nullptr;
	if (DM)
	{
		if (UGameInstance* const GI2 = GetWorld()->GetGameInstance())
		{
			if (UDFWorldTransitionSubsystem* const T = GI2->GetSubsystem<UDFWorldTransitionSubsystem>())
			{
				T->SaveCheckpoint(ECheckpointType::FloorComplete);
			}
		}
		DM->AdvanceToNextFloor();
	}
	if (ADFRunGameMode* const GM = OwningGameMode.Get())
	{
		if (ADFRunGameState* const RGS = GM->GetGameState<ADFRunGameState>())
		{
			RGS->SetPhase(ERunPhase::InCombat);
		}
	}
	bFlowActive = false;
	bAwaitingClientStep = false;
	CurrentStep = EBetweenFloorStep::None;
	StepQueue.Reset();
	StepQueueIndex = INDEX_NONE;
	OwningGameMode = nullptr;
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(RestAdvanceTimer);
	}
}

void UDFBetweenFloorSubsystem::PresentStepForLocalPlayer(ADFRunPlayerController* const PC)
{
	if (!PC || !bFlowActive)
	{
		return;
	}
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	switch (CurrentStep)
	{
	case EBetweenFloorStep::Event:
		if (RandomEventWidgetClass && !PendingEventRowName.IsNone())
		{
			if (UDFRandomEventWidget* const EvW = CreateWidget<UDFRandomEventWidget>(PC, RandomEventWidgetClass))
			{
				EvW->AddToViewport(15);
				EvW->PresentEvent(PendingEventRow, PendingEventRowName, true);
				PC->SetupInputModeUIForWidget(EvW, false);
			}
		}
		break;
	case EBetweenFloorStep::Shop:
		for (TActorIterator<ADFMerchantActor> It(W); It; ++It)
		{
			if (ADFPlayerCharacter* const Hero = PC->GetPawn<ADFPlayerCharacter>())
			{
				It->Interact(Hero);
			}
			break;
		}
		break;
	default:
		break;
	}
}

bool UDFBetweenFloorSubsystem::TryGetPendingEventRow(FDFRandomEventRow& OutRow) const
{
	if (PendingEventRowName.IsNone())
	{
		return false;
	}
	OutRow = PendingEventRow;
	return true;
}

void UDFBetweenFloorSubsystem::PauseForBetweenFloorUI()
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.0001f);
}

void UDFBetweenFloorSubsystem::ResumeTimeAfterBetweenFloorUI()
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f);
}

bool UDFBetweenFloorSubsystem::IsAuthorityWorld() const
{
	const UWorld* const W = GetWorld();
	return W && W->GetNetMode() != NM_Client;
}
