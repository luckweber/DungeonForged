// Source/DungeonForged/Private/GameModes/Nexus/ADFNexusHUD.cpp
#include "GameModes/Nexus/ADFNexusHUD.h"
#include "GameModes/Nexus/ADFNexusGameState.h"
#include "GameModes/Nexus/UDFNexusHUDWidget.h"
#include "GameModes/Nexus/UDFNexusUnlockNotificationWidget.h"
#include "Interaction/UDFInteractionPromptWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

namespace
{

static bool UnlockEntryShouldShowNotification(const FDFPendingUnlockEntry& E)
{
	switch (E.Type)
	{
	case ENexusPendingUnlockType::UnlockClass:
		return !E.ClassRow.IsNone();
	case ENexusPendingUnlockType::UnlockNPC:
		return !E.NPCId.IsNone();
	case ENexusPendingUnlockType::UnlockUpgrade:
		return !E.UpgradeRow.IsNone();
	default:
		return false;
	}
}

} // namespace

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

void ADFNexusHUD::SetRootWidgetHiddenForClassSelection(const bool InHidden)
{
	if (!RootWidget)
	{
		return;
	}
	if (InHidden)
	{
		if (!bRootHiddenForClassSelection)
		{
			CachedRootVisibilityBeforeClassSelection = RootWidget->GetVisibility();
			bRootHiddenForClassSelection = true;
		}
		RootWidget->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	if (bRootHiddenForClassSelection)
	{
		bRootHiddenForClassSelection = false;
		RootWidget->SetVisibility(CachedRootVisibilityBeforeClassSelection);
	}
}

void ADFNexusHUD::QueueUnlockNotificationForEntry(const FDFPendingUnlockEntry& Entry)
{
	if (!UnlockEntryShouldShowNotification(Entry))
	{
		return;
	}
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
	auto CollapseTray = [this]
		{
			if (RootWidget)
			{
				RootWidget->SetNotificationTrayVisible(false);
			}
		};

	if (NotificationQueue.Num() == 0)
	{
		bNotificationShowing = false;
		CollapseTray();
		return;
	}

	bNotificationShowing = true;

	while (NotificationQueue.Num() > 0 && !UnlockEntryShouldShowNotification(NotificationQueue[0]))
	{
		NotificationQueue.RemoveAt(0);
	}

	if (NotificationQueue.Num() == 0)
	{
		bNotificationShowing = false;
		CollapseTray();
		return;
	}

	const FDFPendingUnlockEntry E = NotificationQueue[0];
	NotificationQueue.RemoveAt(0);
	if (!UnlockNotificationClass)
	{
		if (UWorld* const W = GetWorld())
		{
			W->GetTimerManager().SetTimer(
				NotificationChainTimer, this, &ADFNexusHUD::OnNotificationChainStep, 0.1f, false);
		}
		return;
	}

	UDFNexusUnlockNotificationWidget* const Widget =
		CreateWidget<UDFNexusUnlockNotificationWidget>(PlayerOwner, UnlockNotificationClass);
	if (Widget)
	{
		FText T = NSLOCTEXT("DFNexus", "Unlock", "Novo desbloqueio");
		FText N = FText::GetEmpty();
		switch (E.Type)
		{
		case ENexusPendingUnlockType::UnlockClass:
			T = NSLOCTEXT("DFNexus", "UnlockClass", "Nova classe!");
			N =
				E.ClassRow.IsNone()
					? NSLOCTEXT("DFNexus", "UnlockGeneric", "(sem nome de linha no save)")
					: FText::FromName(E.ClassRow);
			break;
		case ENexusPendingUnlockType::UnlockNPC:
			T = NSLOCTEXT("DFNexus", "UnlockNPC", "NPC desbloqueado!");
			N =
				E.NPCId.IsNone()
					? NSLOCTEXT("DFNexus", "UnlockGenericNPC", "(sem ID do NPC no save)")
					: FText::FromName(E.NPCId);
			break;
		case ENexusPendingUnlockType::UnlockUpgrade:
			T = NSLOCTEXT("DFNexus", "UnlockUp", "Novo upgrade!");
			N =
				E.UpgradeRow.IsNone()
					? NSLOCTEXT("DFNexus", "UnlockGenericUp", "(sem nome de linha no save)")
					: FText::FromName(E.UpgradeRow);
			break;
		default:
			N = NSLOCTEXT("DFNexus", "UnlockUnknown", "(tipo invalido em PendingUnlocks)");
			break;
		}

		Widget->SetUnlockContent(T, N, nullptr);

		UOverlay* const Tray = RootWidget ? RootWidget->GetNotificationOverlay() : nullptr;
		if (Tray)
		{
			RootWidget->SetNotificationTrayVisible(true);
			Tray->AddChildToOverlay(Widget);
			if (UOverlaySlot* const OvSlot = Cast<UOverlaySlot>(Widget->Slot))
			{
				OvSlot->SetHorizontalAlignment(HAlign_Fill);
				OvSlot->SetVerticalAlignment(VAlign_Top);
				OvSlot->SetPadding(FMargin(0.f, 12.f));
			}
		}
		else
		{
			Widget->AddToViewport(30);
		}

		Widget->PlayShowThenHide(4.f);
	}
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			NotificationChainTimer, this, &ADFNexusHUD::OnNotificationChainStep, 4.1f, false);
	}
}

void ADFNexusHUD::OnNotificationChainStep()
{
	bNotificationShowing = false;
	DequeueAndShowNextNotification();
}
