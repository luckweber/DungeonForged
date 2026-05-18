// Source/DungeonForged/Private/UI/UDFInGameHUDWidget.cpp
#include "UI/UDFInGameHUDWidget.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Characters/ADFPlayerState.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Animation/WidgetAnimation.h"
#include "AbilitySystemComponent.h"
#include "GAS/DFGameplayTags.h"
#include "UI/Combat/UDFDamageDirectionWidget.h"
#include "UI/UDFAbilityHotbarWidget.h"
#include "UI/UDFPlayerVitalsWidget.h"

void UDFInGameHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (ADFPlayerState* const PS = GetDFPlayerState())
	{
		LastGoldShown = PS->GetReplicatedRunGold();
		PS->OnReplicatedRunGold.AddDynamic(this, &UDFInGameHUDWidget::HandleReplicatedRunGold);
		HandleReplicatedRunGold(LastGoldShown);
	}
	if (ADFPlayerCharacter* const PC = GetDFPlayerCharacter())
	{
		PC->OnDamageTakenForUI.AddDynamic(this, &UDFInGameHUDWidget::OnPlayerDamageTaken);
	}
	BindCombatState();
	ApplyHUDOpacity(CurrentHUDOpacity);
}

void UDFInGameHUDWidget::NativeDestruct()
{
	if (ADFPlayerCharacter* const PC = GetDFPlayerCharacter())
	{
		PC->OnDamageTakenForUI.RemoveDynamic(this, &UDFInGameHUDWidget::OnPlayerDamageTaken);
	}
	if (ADFPlayerState* const PS = GetDFPlayerState())
	{
		PS->OnReplicatedRunGold.RemoveDynamic(this, &UDFInGameHUDWidget::HandleReplicatedRunGold);
	}
	UnbindCombatState();
	Super::NativeDestruct();
}

void UDFInGameHUDWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const float Speed = (TargetHUDOpacity > CurrentHUDOpacity)
		? (1.f / FMath::Max(HUDFadeInDuration, KINDA_SMALL_NUMBER))
		: (1.f / FMath::Max(HUDFadeOutDuration, KINDA_SMALL_NUMBER));
	CurrentHUDOpacity = FMath::FInterpConstantTo(CurrentHUDOpacity, TargetHUDOpacity, InDeltaTime, Speed);
	ApplyHUDOpacity(CurrentHUDOpacity);
}

void UDFInGameHUDWidget::BindCombatState()
{
	UAbilitySystemComponent* const ASC = GetAbilitySystemComponent();
	if (!ASC || !FDFGameplayTags::State_InCombat.IsValid())
	{
		return;
	}
	TargetHUDOpacity = ASC->HasMatchingGameplayTag(FDFGameplayTags::State_InCombat) ? 1.f : 0.f;
	CurrentHUDOpacity = TargetHUDOpacity;
	CombatTagDelegateHandle = ASC->RegisterGameplayTagEvent(
		FDFGameplayTags::State_InCombat, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UDFInGameHUDWidget::OnInCombatTagChanged);
}

void UDFInGameHUDWidget::UnbindCombatState()
{
	if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponent())
	{
		if (CombatTagDelegateHandle.IsValid())
		{
			ASC->UnregisterGameplayTagEvent(CombatTagDelegateHandle, FDFGameplayTags::State_InCombat);
			CombatTagDelegateHandle.Reset();
		}
	}
}

void UDFInGameHUDWidget::OnInCombatTagChanged(const FGameplayTag Tag, const int32 NewCount)
{
	(void)Tag;
	TargetHUDOpacity = NewCount > 0 ? 1.f : 0.f;
}

void UDFInGameHUDWidget::ApplyHUDOpacity(const float Opacity)
{
	if (Panel_FadeableHUD)
	{
		Panel_FadeableHUD->SetRenderOpacity(Opacity);
		return;
	}
	if (UDFPlayerVitalsWidget* const Vitals = PlayerVitals.Get())
	{
		Vitals->SetRenderOpacity(Opacity);
	}
	if (UDFAbilityHotbarWidget* const Hotbar = AbilityHotbar.Get())
	{
		Hotbar->SetRenderOpacity(Opacity);
	}
	if (UDFStatusEffectBarWidget* const Status = DFStatusEffectBar.Get())
	{
		Status->SetRenderOpacity(Opacity);
	}
}

void UDFInGameHUDWidget::OnPlayerDamageTaken(FVector DamageSourceWorldLocation, const float Intensity)
{
	if (DamageDirection)
	{
		DamageDirection->PulseFromWorldLocation(DamageSourceWorldLocation, Intensity);
	}
}

void UDFInGameHUDWidget::HandleReplicatedRunGold(int32 NewTotal)
{
	if (GoldText)
	{
		GoldText->SetText(FText::AsNumber(NewTotal));
	}
	if (NewTotal > LastGoldShown)
	{
		PlayGoldPulse();
	}
	LastGoldShown = NewTotal;
}

void UDFInGameHUDWidget::PlayGoldPulse()
{
	if (GoldChangePulseAnim)
	{
		PlayAnimation(GoldChangePulseAnim);
	}
}
