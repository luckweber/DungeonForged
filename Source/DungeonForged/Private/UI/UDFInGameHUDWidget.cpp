// Source/DungeonForged/Private/UI/UDFInGameHUDWidget.cpp
#include "UI/UDFInGameHUDWidget.h"
#include "Characters/ADFPlayerState.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Animation/WidgetAnimation.h"

void UDFInGameHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (ADFPlayerState* const PS = GetDFPlayerState())
	{
		LastGoldShown = PS->GetReplicatedRunGold();
		PS->OnReplicatedRunGold.AddDynamic(this, &UDFInGameHUDWidget::HandleReplicatedRunGold);
		HandleReplicatedRunGold(LastGoldShown);
	}
}

void UDFInGameHUDWidget::NativeDestruct()
{
	if (ADFPlayerState* const PS = GetDFPlayerState())
	{
		PS->OnReplicatedRunGold.RemoveDynamic(this, &UDFInGameHUDWidget::HandleReplicatedRunGold);
	}
	Super::NativeDestruct();
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
