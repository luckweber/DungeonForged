// Source/DungeonForged/Private/GameModes/MainMenu/UDFSaveSlotSelectionUserWidget.cpp
#include "GameModes/MainMenu/UDFSaveSlotSelectionUserWidget.h"
#include "GameModes/MainMenu/UDFSaveSlotCardUserWidget.h"
#include "GameModes/MainMenu/ADFMainMenuHUD.h"
#include "GameModes/MainMenu/UDFConfirmDialogUserWidget.h"
#include "Run/UDFSaveSlotManagerSubsystem.h"
#include "Run/DFSaveGame.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Components/PanelWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/UserWidget.h"
#include "Logging/LogMacros.h"

void UDFSaveSlotSelectionUserWidget::ResolveWidgetBindings()
{
	if (!SlotRow && !UmgNameSlotRow.IsNone())
	{
		if (UWidget* const W = GetWidgetFromName(UmgNameSlotRow))
		{
			SlotRow = Cast<UPanelWidget>(W);
		}
	}
	if (!SlotCard0)
	{
		if (UWidget* const W = GetWidgetFromName(FName("SlotCard0")))
		{
			SlotCard0 = Cast<UDFSaveSlotCardUserWidget>(W);
		}
	}
	if (!SlotCard1)
	{
		if (UWidget* const W = GetWidgetFromName(FName("SlotCard1")))
		{
			SlotCard1 = Cast<UDFSaveSlotCardUserWidget>(W);
		}
	}
	if (!SlotCard2)
	{
		if (UWidget* const W = GetWidgetFromName(FName("SlotCard2")))
		{
			SlotCard2 = Cast<UDFSaveSlotCardUserWidget>(W);
		}
	}
}

void UDFSaveSlotSelectionUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (BackButton)
	{
		BackButton->OnClicked.AddDynamic(this, &UDFSaveSlotSelectionUserWidget::OnBackClicked);
	}
	if (UGameInstance* const Gi = GetGameInstance())
	{
		if (UDFSaveSlotManagerSubsystem* const S = Gi->GetSubsystem<UDFSaveSlotManagerSubsystem>())
		{
			S->OnSlotChanged.AddDynamic(this, &UDFSaveSlotSelectionUserWidget::HandleSlotChanged);
		}
	}
	RefreshFromSubsystem();
}

void UDFSaveSlotSelectionUserWidget::NativeDestruct()
{
	if (UGameInstance* const Gi = GetGameInstance())
	{
		if (UDFSaveSlotManagerSubsystem* const S = Gi->GetSubsystem<UDFSaveSlotManagerSubsystem>())
		{
			S->OnSlotChanged.RemoveAll(this);
		}
	}
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
	if (ManageHintText)
	{
		ManageHintText->SetText(
			NSLOCTEXT("MainMenu", "ManageSlotHint", "Selecione um perfil para apagá-lo"));
		ManageHintText->SetVisibility(
			CurrentMode == EDFSlotScreenMode::SelectToDelete
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
	}
}

void UDFSaveSlotSelectionUserWidget::RefreshFromSubsystem()
{
	ResolveWidgetBindings();
	UGameInstance* const Gi = GetGameInstance();
	if (UDFSaveSlotManagerSubsystem* const S = Gi ? Gi->GetSubsystem<UDFSaveSlotManagerSubsystem>() : nullptr)
	{
		S->LoadAllSlotHeaders();
	}
	UpdateTitle();
	if (SlotCard0 && SlotCard1 && SlotCard2)
	{
		SlotCard0->RefreshSlotData(0, CurrentMode);
		SlotCard1->RefreshSlotData(1, CurrentMode);
		SlotCard2->RefreshSlotData(2, CurrentMode);
	}
	else
	{
		RebuildCardWidgets();
	}
}

void UDFSaveSlotSelectionUserWidget::HandleSlotChanged(int32 const SlotIndex)
{
	// After Select/Save/Delete, LoadedSlots is already current — never call
	// LoadAllSlotHeaders from this path (it used to recurse via OnSlotChanged).
	if (SlotCard0 && SlotCard1 && SlotCard2 && SlotIndex >= 0 && SlotIndex <= 2)
	{
		UDFSaveSlotCardUserWidget* const Card = SlotIndex == 0
			 ? SlotCard0
			 : (SlotIndex == 1 ? SlotCard1 : SlotCard2);
		if (Card)
		{
			Card->RefreshSlotData(SlotIndex, CurrentMode);
		}
	}
	else
	{
		UpdateTitle();
		RebuildCardWidgets();
	}
}

void UDFSaveSlotSelectionUserWidget::RebuildCardWidgets()
{
	ResolveWidgetBindings();
	if (!SlotRow)
	{
		UE_LOG(
			LogTemp, Warning,
			TEXT("WBP_SaveSlotSelection: nenhum painel '%s'. Marque a caixa 'Is Variable' (UHorizontalBox/UWrapBox/UUniformGridPanel...) "
				 "ou coloque 3 cartões nomeados SlotCard0..2 no Designer."),
			*UmgNameSlotRow.ToString());
		return;
	}
	SlotRow->ClearChildren();
	APlayerController* const Pc = GetOwningPlayer();
	UGameInstance* const Gi = GetGameInstance();
	UDFSaveSlotManagerSubsystem* const Slots = Gi ? Gi->GetSubsystem<UDFSaveSlotManagerSubsystem>() : nullptr;
	if (!SlotCardClass || !Pc || !Slots)
	{
		UE_LOG(
			LogTemp, Warning,
			TEXT("WBP_SaveSlotSelection: SlotCardClass, PlayerController ou sub sistema de slots nulo; verifique a classe e o PIE."));
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
		(void)D;
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
