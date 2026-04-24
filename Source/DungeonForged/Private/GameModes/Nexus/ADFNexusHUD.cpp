// Source/DungeonForged/Private/GameModes/Nexus/ADFNexusHUD.cpp
#include "GameModes/Nexus/ADFNexusHUD.h"
#include "GameModes/Nexus/ADFNexusGameState.h"
#include "GameModes/Nexus/UDFNexusHUDWidget.h"
#include "GameModes/Nexus/UDFNexusUnlockNotificationWidget.h"
#include "Interaction/UDFInteractionPromptWidget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

ADFNexusHUD::ADFNexusHUD()
	: bNotificationShowing(false)
{
}

void ADFNexusHUD::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (APlayerController* const PC = PlayerOwner)
	{
		if (NexusRootWidgetClass)
		{
			RootWidget = CreateWidget<UDFNexusHUDWidget>(PC, NexusRootWidgetClass);
			if (RootWidget)
			{
				RootWidget->AddToViewport(0);
				if (ADFNexusGameState* const GS = GetWorld() ? GetWorld()->GetGameState<ADFNexusGameState>() : nullptr)
				{
					RootWidget->SetMetaInfo(
						GS->MetaLevel,
						GS->TotalRunsCompleted,
						GS->TotalRunsWon,
						GS->GetNexusXPFillRatio());
				}
			}
		}
		if (InteractionPromptClass)
		{
			InteractionLayer = CreateWidget<UDFInteractionPromptWidget>(PC, InteractionPromptClass);
			if (InteractionLayer)
			{
				InteractionLayer->AddToViewport(24);
			}
		}
	}
}

void ADFNexusHUD::QueueUnlockNotificationForEntry(const FDFPendingUnlockEntry& Entry)
{
	NotificationQueue.Add(Entry);
	if (!bNotificationShowing)
	{
		DequeueAndShowNextNotification();
	}
}

void ADFNexusHUD::DequeueAndShowNextNotification()
{
	if (GetNetMode() == NM_DedicatedServer || !PlayerOwner)
	{
		return;
	}
	if (NotificationQueue.Num() == 0)
	{
		bNotificationShowing = false;
		return;
	}
	bNotificationShowing = true;
	const FDFPendingUnlockEntry E = NotificationQueue[0];
	NotificationQueue.RemoveAt(0);
	if (!UnlockNotificationClass)
	{
		if (UWorld* const W = GetWorld())
		{
			W->GetTimerManager().SetTimer(NotificationChainTimer, this, &ADFNexusHUD::OnNotificationChainStep, 0.1f, false);
		}
		return;
	}
	UDFNexusUnlockNotificationWidget* const Widget = CreateWidget<UDFNexusUnlockNotificationWidget>(PlayerOwner, UnlockNotificationClass);
	if (Widget)
	{
		FText T = NSLOCTEXT("DFNexus", "Unlock", "Novo desbloqueio");
		FText N = FText::GetEmpty();
		switch (E.Type)
		{
		case ENexusPendingUnlockType::UnlockClass: T = NSLOCTEXT("DFNexus", "UnlockClass", "Nova classe!"); N = FText::FromName(E.ClassRow); break;
		case ENexusPendingUnlockType::UnlockNPC: T = NSLOCTEXT("DFNexus", "UnlockNPC", "NPC!"); N = FText::FromName(E.NPCId); break;
		case ENexusPendingUnlockType::UnlockUpgrade: T = NSLOCTEXT("DFNexus", "UnlockUp", "Upgrade!"); N = FText::FromName(E.UpgradeRow); break;
		default: break;
		}
		Widget->SetUnlockContent(T, N, nullptr);
		Widget->AddToViewport(30);
		Widget->PlayShowThenHide(4.f);
	}
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().SetTimer(NotificationChainTimer, this, &ADFNexusHUD::OnNotificationChainStep, 4.1f, false);
	}
}

void ADFNexusHUD::OnNotificationChainStep()
{
	bNotificationShowing = false;
	DequeueAndShowNextNotification();
}
