// Source/DungeonForged/Private/GameModes/Run/ADFRunHUD.cpp
#include "GameModes/Run/ADFRunHUD.h"
#include "GameModes/Run/ADFRunGameState.h"
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
	WBP_BossHealthBar = AddIf(WBP_BossHealthBarClass, Z_Boss);
	WBP_LockOnIndicator = AddIf(WBP_LockOnIndicatorClass, Z_LockOn);
	WBP_FloorCounter = AddIf(WBP_FloorCounterClass, Z_Floor);
	WBP_KillCounter = AddIf(WBP_KillCounterClass, Z_Kill);
	ShowBossHUD(false);
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
	// Boss bar: hidden in non-boss (handled separately).
}

void ADFRunHUD::ShowBossHUD(const bool bShow)
{
	if (WBP_BossHealthBar)
	{
		WBP_BossHealthBar->SetVisibility(
			bShow ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void ADFRunHUD::OnRunPhaseChanged(ERunPhase NewPhase, ERunPhase /*OldPhase*/)
{
	switch (NewPhase)
	{
	case ERunPhase::BetweenFloors: SetCombatWidgetsVisible(false);
		return;
	case ERunPhase::BossEncounter: SetCombatWidgetsVisible(true);
		ShowBossHUD(true);
		return;
	case ERunPhase::InCombat:
		SetCombatWidgetsVisible(true);
		ShowBossHUD(false);
		return;
	case ERunPhase::Victory:
	case ERunPhase::Defeat: SetCombatWidgetsVisible(false);
		return;
	default: return;
	}
}
