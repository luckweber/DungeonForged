// Source/DungeonForged/Private/UI/Combat/UDFCombatTextWidget.cpp
#include "UI/Combat/UDFCombatTextWidget.h"
#include "UI/Combat/UDFCombatTextSubsystem.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/SlateColor.h"
#include "TimerManager.h"
#include "Animation/WidgetAnimation.h"
#include "Animation/UMGSequencePlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UDFCombatTextWidget)

void UDFCombatTextWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UDFCombatTextWidget::NativeDestruct()
{
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(EndTimer);
		W->GetTimerManager().ClearTimer(FollowTimer);
	}
	Super::NativeDestruct();
}

void UDFCombatTextWidget::UpdateScreenPosition()
{
	if (!bInUse)
	{
		return;
	}
	APlayerController* const PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}
	bool bOnScreen = false;
	FVector2D Screen(0.f, 0.f);
	UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, WorldLocation, Screen, bOnScreen);
	Screen.X += ScreenScatterX;
	SetPositionInViewport(Screen, true);
}

void UDFCombatTextWidget::InitializeCombatText(
	const FString& InText,
	const ECombatTextType Type,
	const FVector InWorldLocation,
	const float InDuration,
	UDFCombatTextSubsystem* const InOwner)
{
	OwnerSubsystem = InOwner;
	WorldLocation = InWorldLocation
		+ FVector(FMath::FRandRange(-15.f, 15.f), FMath::FRandRange(-15.f, 15.f), FMath::FRandRange(0.f, 20.f));
	ScreenScatterX = FMath::FRandRange(-30.f, 30.f);
	if (DamageText)
	{
		DamageText->SetText(FText::FromString(InText));
	}
	ApplyStyleForType(Type);
	bInUse = true;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	{
		FWidgetTransform T;
		if (Type == ECombatTextType::Damage_Critical)
		{
			T.Angle = FMath::DegreesToRadians(FMath::FRandRange(-5.f, 5.f));
		}
		SetRenderTransform(T);
	}
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			FollowTimer, this, &UDFCombatTextWidget::UpdateScreenPosition, 1.f / 60.f, true);
		UpdateScreenPosition();
		float TEnd = InDuration;
		if (FloatAnimation)
		{
			PlayAnimation(FloatAnimation, 0.f, 1, EUMGSequencePlayMode::Forward, 1.f, false);
			TEnd = FMath::Max(TEnd, FloatAnimation->GetEndTime());
		}
		W->GetTimerManager().SetTimer(EndTimer, this, &UDFCombatTextWidget::OnFloatTimeElapsed, TEnd, false);
	}
}

void UDFCombatTextWidget::OnFloatTimeElapsed()
{
	ReturnToPool();
}

void UDFCombatTextWidget::ApplyStyleForType(const ECombatTextType Type) const
{
	if (!DamageText)
	{
		return;
	}
	FSlateFontInfo Font = DamageText->GetFont();
	auto Set = [this, &Font](FLinearColor const& C, int32 S, const bool bItal = false) {
		Font.Size = S;
		DamageText->SetColorAndOpacity(FSlateColor(C));
		DamageText->SetFont(Font);
		(void)bItal; // UTextBlock has no italic; designer can swap font in WBP
		(void)0;
	};
	switch (Type)
	{
	case ECombatTextType::Damage_Physical: Set(FLinearColor(1.f, 1.f, 1.f, 1.f), 28);
		return;
	case ECombatTextType::Damage_Magic: Set(FLinearColor(0.6f, 0.2f, 0.9f, 1.f), 28);
		return;
	case ECombatTextType::Damage_True: Set(FLinearColor(1.f, 0.35f, 0.35f, 1.f), 28);
		return;
	case ECombatTextType::Damage_Critical: Set(FLinearColor(1.f, 0.95f, 0.2f, 1.f), 42);
		return;
	case ECombatTextType::Damage_DoT: Set(FLinearColor(1.f, 0.45f, 0.1f, 1.f), 22, true);
		return;
	case ECombatTextType::Heal: Set(FLinearColor(0.2f, 0.9f, 0.3f, 1.f), 30);
		return;
	case ECombatTextType::Mana_Restore: Set(FLinearColor(0.2f, 0.4f, 1.f, 1.f), 28);
		return;
	case ECombatTextType::Miss: Set(FLinearColor(0.55f, 0.55f, 0.55f, 1.f), 24, true);
		return;
	case ECombatTextType::Dodge: Set(FLinearColor(1.f, 1.f, 1.f, 1.f), 24, true);
		return;
	case ECombatTextType::Block: Set(FLinearColor(0.5f, 0.5f, 0.5f, 1.f), 24);
		return;
	case ECombatTextType::Immune: Set(FLinearColor(1.f, 0.9f, 0.1f, 1.f), 24);
		return;
	case ECombatTextType::LevelUp: Set(FLinearColor(1.f, 0.75f, 0.1f, 1.f), 52);
		return;
	case ECombatTextType::XPGain: Set(FLinearColor(0.9f, 0.8f, 0.2f, 0.7f), 24);
		return;
	case ECombatTextType::GoldGain: Set(FLinearColor(1.f, 0.85f, 0.1f, 1.f), 28);
		return;
	case ECombatTextType::Status: Set(FLinearColor(0.6f, 0.2f, 0.8f, 1.f), 24, true);
		return;
	case ECombatTextType::Elemental_Weak: Set(FLinearColor(1.f, 0.9f, 0.1f, 1.f), 24, true);
		return;
	case ECombatTextType::Elemental_Resist: Set(FLinearColor(0.5f, 0.5f, 0.5f, 1.f), 22, true);
		return;
	case ECombatTextType::Elemental_Reaction: Set(FLinearColor(0.2f, 0.85f, 1.f, 1.f), 30, true);
		return;
	default: Set(FLinearColor::White, 28);
		return;
	}
}

void UDFCombatTextWidget::ReturnToPool()
{
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(EndTimer);
		W->GetTimerManager().ClearTimer(FollowTimer);
	}
	if (FloatAnimation)
	{
		StopAnimation(FloatAnimation);
	}
	bInUse = false;
	SetVisibility(ESlateVisibility::Collapsed);
	if (UDFCombatTextSubsystem* const S = OwnerSubsystem.Get())
	{
		S->ReturnToPool(this);
	}
}
