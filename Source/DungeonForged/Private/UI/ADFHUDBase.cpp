// Source/DungeonForged/Private/UI/ADFHUDBase.cpp
#include "UI/ADFHUDBase.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"

ADFHUDBase::ADFHUDBase()
	: MainHUDWidgetClass(nullptr)
	, MainHUDWidget(nullptr)
{
}

void ADFHUDBase::BeginPlay()
{
	Super::BeginPlay();

	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (!MainHUDWidgetClass)
	{
		return;
	}
	APlayerController* const PC = GetOwningPlayerController();
	if (!IsValid(PC))
	{
		return;
	}
	MainHUDWidget = CreateWidget<UUserWidget>(PC, MainHUDWidgetClass);
	if (IsValid(MainHUDWidget))
	{
		MainHUDWidget->SetVisibility(ESlateVisibility::Visible);
		MainHUDWidget->AddToViewport(0);
	}
}

void ADFHUDBase::ShowHUD()
{
	Super::ShowHUD();
	if (IsValid(MainHUDWidget))
	{
		MainHUDWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void ADFHUDBase::HideHUD()
{
	if (IsValid(MainHUDWidget))
	{
		MainHUDWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}
