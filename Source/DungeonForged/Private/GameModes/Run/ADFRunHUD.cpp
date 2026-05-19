// Source/DungeonForged/Private/GameModes/Run/ADFRunHUD.cpp
#include "GameModes/Run/ADFRunHUD.h"
#include "Boss/ADFBossBase.h"
#include "DungeonForgedModule.h"
#include "GameModes/Run/ADFRunGameState.h"
#include "UI/UDFBossHealthBarWidget.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"

namespace
{
	constexpr int32 Z_HUD = 0;
	constexpr int32 Z_Minimap = 1;
	constexpr int32 Z_Status = 1;
	constexpr int32 Z_Boss = 2;
	constexpr int32 Z_LockOn = 3;
	constexpr int32 Z_Floor = 4;
	constexpr int32 Z_Kill = 4;
}

ADFRunHUD::ADFRunHUD() = default;

void ADFRunHUD::BeginPlay()
{
	Super::BeginPlay();
	CreateRunWidgets();
	if (APlayerController* const PC = GetOwningPlayerController())
	{
		if (UWorld* const W = GetWorld())
		{
			if (ADFRunGameState* const RGS = W->GetGameState<ADFRunGameState>())
			{
				RGS->OnPhaseChanged.AddDynamic(this, &ADFRunHUD::OnRunPhaseChanged);
				RGS->OnActiveBossChanged.AddDynamic(this, &ADFRunHUD::OnActiveBossChanged);
				SyncBossBarFromGameState();
			}
		}
	}
}

void ADFRunHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* const W = GetWorld())
	{
		if (ADFRunGameState* const RGS = W->GetGameState<ADFRunGameState>())
		{
			RGS->OnPhaseChanged.RemoveAll(this);
			RGS->OnActiveBossChanged.RemoveAll(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ADFRunHUD::CreateRunWidgets()
{
	APlayerController* const PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}
	auto AddIf = [PC, this](TSubclassOf<UUserWidget> Cls, int32 const Z) -> UUserWidget*
	{
		if (!Cls)
		{
			return nullptr;
		}
		if (UUserWidget* const Wg = CreateWidget<UUserWidget>(PC, Cls))
		{
			Wg->AddToViewport(Z);
			return Wg;
		}
		return nullptr;
	};

	WBP_HUD = AddIf(WBP_HUDClass, Z_HUD);
	WBP_Minimap = AddIf(WBP_MinimapClass, Z_Minimap);
	WBP_StatusEffectBar = AddIf(WBP_StatusEffectBarClass, Z_Status);
	if (UUserWidget* const BossWg = AddIf(WBP_BossHealthBarClass, Z_Boss))
	{
		WBP_BossHealthBar = Cast<UDFBossHealthBarWidget>(BossWg);
		if (!WBP_BossHealthBar)
		{
			DF_LOG(Warning,
				"ADFRunHUD: WBP_BossHealthBarClass must inherit UDFBossHealthBarWidget (BindWidget: BossHealthBar, BossNameText).");
		}
	}
	WBP_LockOnIndicator = AddIf(WBP_LockOnIndicatorClass, Z_LockOn);
	WBP_FloorCounter = AddIf(WBP_FloorCounterClass, Z_Floor);
	WBP_KillCounter = AddIf(WBP_KillCounterClass, Z_Kill);
	ClearBossHealthBar();
}

void ADFRunHUD::SetCombatWidgetsVisible(const bool bVisible)
{
	ESlateVisibility const V = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
	if (WBP_HUD) WBP_HUD->SetVisibility(V);
	if (WBP_Minimap) WBP_Minimap->SetVisibility(V);
	if (WBP_StatusEffectBar) WBP_StatusEffectBar->SetVisibility(V);
	if (WBP_LockOnIndicator) WBP_LockOnIndicator->SetVisibility(V);
	if (WBP_FloorCounter) WBP_FloorCounter->SetVisibility(V);
	if (WBP_KillCounter) WBP_KillCounter->SetVisibility(V);
}

void ADFRunHUD::ShowBossHUD(const bool bShow)
{
	if (WBP_BossHealthBar)
	{
		WBP_BossHealthBar->SetVisibility(
			bShow ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void ADFRunHUD::PresentBossHealthBar(ADFBossBase* const Boss)
{
	if (!Boss || !WBP_BossHealthBar)
	{
		return;
	}
	WBP_BossHealthBar->ShowForBoss(Boss, Boss->GetBossDisplayName());
	ShowBossHUD(true);
}

void ADFRunHUD::ClearBossHealthBar()
{
	if (WBP_BossHealthBar)
	{
		WBP_BossHealthBar->HideBossBar();
	}
	ShowBossHUD(false);
}

void ADFRunHUD::SyncBossBarFromGameState()
{
	if (UWorld* const W = GetWorld())
	{
		if (const ADFRunGameState* const RGS = W->GetGameState<ADFRunGameState>())
		{
			if (RGS->CurrentPhase == ERunPhase::BossEncounter && RGS->ActiveBoss)
			{
				PresentBossHealthBar(RGS->ActiveBoss);
				return;
			}
		}
	}
	ClearBossHealthBar();
}

void ADFRunHUD::OnActiveBossChanged(ADFBossBase* /*Boss*/)
{
	SyncBossBarFromGameState();
}

void ADFRunHUD::OnRunPhaseChanged(ERunPhase NewPhase, ERunPhase /*OldPhase*/)
{
	switch (NewPhase)
	{
	case ERunPhase::BetweenFloors:
		SetCombatWidgetsVisible(false);
		ClearBossHealthBar();
		return;
	case ERunPhase::BossEncounter:
		SetCombatWidgetsVisible(true);
		SyncBossBarFromGameState();
		return;
	case ERunPhase::InCombat:
		SetCombatWidgetsVisible(true);
		ClearBossHealthBar();
		return;
	case ERunPhase::Victory:
	case ERunPhase::Defeat:
		SetCombatWidgetsVisible(false);
		ClearBossHealthBar();
		return;
	default:
		return;
	}
}
