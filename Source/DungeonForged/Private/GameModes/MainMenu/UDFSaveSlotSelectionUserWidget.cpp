// Source/DungeonForged/Private/GameModes/MainMenu/UDFSaveSlotSelectionUserWidget.cpp
#include "GameModes/MainMenu/UDFSaveSlotSelectionUserWidget.h"
#include "GameModes/MainMenu/UDFSaveSlotCardUserWidget.h"
#include "GameModes/MainMenu/ADFMainMenuHUD.h"
#include "GameModes/MainMenu/UDFConfirmDialogUserWidget.h"
#include "Run/UDFSaveSlotManagerSubsystem.h"
#include "Run/DFSaveGame.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Components/HorizontalBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/UserWidget.h"

void UDFSaveSlotSelectionUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (BackButton)
	{
		BackButton->OnClicked.AddDynamic(this, &UDFSaveSlotSelectionUserWidget::OnBackClicked);
	}
}

void UDFSaveSlotSelectionUserWidget::NativeDestruct()
{
	if (BackButton)
	{
		BackButton->OnClicked.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UDFSaveSlotSelectionUserWidget::OnBackClicked()
{
	if (APlayerController* const PC = GetOwningPlayer())
	{
		if (ADFMainMenuHUD* const H = Cast<ADFMainMenuHUD>(PC->GetHUD()))
		{
			H->HideSaveSlotLayer();
		}
	}
}

void UDFSaveSlotSelectionUserWidget::UpdateTitle() const
{
	if (!TitleText)
	{
		return;
	}
	TitleText->SetText(
		CurrentMode == EDFSlotScreenMode::SelectToDelete
			? NSLOCTEXT("MainMenu", "SlotTitleManage", "Gerenciar Perfis")
			: NSLOCTEXT("MainMenu", "SlotTitleSelect", "Selecionar Perfil"));
}

void UDFSaveSlotSelectionUserWidget::RefreshFromSubsystem()
{
	UGameInstance* const Gi = GetGameInstance();
	if (UDFSaveSlotManagerSubsystem* const S = Gi ? Gi->GetSubsystem<UDFSaveSlotManagerSubsystem>() : nullptr)
	{
		S->LoadAllSlotHeaders();
	}
	UpdateTitle();
	RebuildCardWidgets();
}

void UDFSaveSlotSelectionUserWidget::RebuildCardWidgets()
{
	if (!SlotRow)
	{
		return;
	}
	SlotRow->ClearChildren();
	APlayerController* const Pc = GetOwningPlayer();
	UGameInstance* const Gi = GetGameInstance();
	UDFSaveSlotManagerSubsystem* const Slots = Gi ? Gi->GetSubsystem<UDFSaveSlotManagerSubsystem>() : nullptr;
	if (!SlotCardClass || !Pc || !Slots)
	{
		return;
	}
	for (int32 I = 0; I < 3; ++I)
	{
		UDFSaveSlotCardUserWidget* const Card = CreateWidget<UDFSaveSlotCardUserWidget>(Pc, SlotCardClass);
		if (!Card)
		{
			continue;
		}
		UDFSaveGame* D = Slots->GetSlotData(I);
		const bool bEmpty = Slots->IsSlotEmpty(I);
		Card->SetupForSlot(I, CurrentMode, bEmpty ? nullptr : D, bEmpty);
		SlotRow->AddChild(Card);
	}
}

void UDFSaveSlotSelectionUserWidget::OnProfileSlotPickedForPlay(int32 const SlotIndex)
{
	UGameInstance* const Gi = GetGameInstance();
	UDFSaveSlotManagerSubsystem* const Slots = Gi ? Gi->GetSubsystem<UDFSaveSlotManagerSubsystem>() : nullptr;
	if (!Slots)
	{
		return;
	}
	Slots->SelectSlot(SlotIndex);
	(void)Slots->SaveActiveSlot();
	if (APlayerController* const Pc = GetOwningPlayer())
	{
		if (ADFMainMenuHUD* const H = Cast<ADFMainMenuHUD>(Pc->GetHUD()))
		{
			H->HideSaveSlotLayer();
		}
	}
}

void UDFSaveSlotSelectionUserWidget::OnRequestDeleteSlot(int32 const Index)
{
	UGameInstance* const Gi = GetGameInstance();
	UDFSaveSlotManagerSubsystem* const Slots = Gi ? Gi->GetSubsystem<UDFSaveSlotManagerSubsystem>() : nullptr;
	if (!Slots)
	{
		return;
	}
	APlayerController* const Pc = GetOwningPlayer();
	ADFMainMenuHUD* const H = Pc ? Cast<ADFMainMenuHUD>(Pc->GetHUD()) : nullptr;
	if (!H || !H->ConfirmWidgetClass)
	{
		Slots->DeleteSlot(Index);
		RefreshFromSubsystem();
		return;
	}
	UDFConfirmDialogUserWidget* const D = CreateWidget<UDFConfirmDialogUserWidget>(Pc, H->ConfirmWidgetClass);
	if (!D)
	{
		Slots->DeleteSlot(Index);
		RefreshFromSubsystem();
		return;
	}
	const int32 Captured = Index;
	D->ShowDialog(
		NSLOCTEXT("MainMenu", "DelProfileTitle", "Apagar Perfil?"),
		NSLOCTEXT("MainMenu", "DelProfileBody", "Todo o progresso deste perfil sera perdido."),
		FSimpleDelegate::CreateWeakLambda(
			this, [this, Captured]() { ExecuteDeleteAfterConfirm(Captured); }));
	H->ShowConfirmDialog(D);
}

void UDFSaveSlotSelectionUserWidget::ExecuteDeleteAfterConfirm(int32 const SlotIndex)
{
	UGameInstance* const Gi = GetGameInstance();
	UDFSaveSlotManagerSubsystem* const Slots = Gi ? Gi->GetSubsystem<UDFSaveSlotManagerSubsystem>() : nullptr;
	if (Slots)
	{
		Slots->DeleteSlot(SlotIndex);
	}
	RefreshFromSubsystem();
}
