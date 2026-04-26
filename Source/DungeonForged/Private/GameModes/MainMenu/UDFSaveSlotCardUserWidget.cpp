// Source/DungeonForged/Private/GameModes/MainMenu/UDFSaveSlotCardUserWidget.cpp
#include "GameModes/MainMenu/UDFSaveSlotCardUserWidget.h"
#include "GameModes/MainMenu/ADFMainMenuHUD.h"
#include "GameModes/MainMenu/UDFConfirmDialogUserWidget.h"
#include "Data/DFDataTableStructs.h"
#include "UI/ClassSelection/UDFClassSelectionSubsystem.h"
#include "Run/DFSaveGame.h"
#include "Run/UDFSaveSlotManagerSubsystem.h"
#include "Run/DFRunManager.h"
#include "World/UDFWorldTransitionSubsystem.h"
#include "World/DFWorldTypes.h"
#include "Animation/WidgetAnimation.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WrapBox.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Styling/SlateColor.h"
#include "Blueprint/UserWidget.h"
#include "Misc/DateTime.h"

namespace
{
	FText DfFormatRelativeTime(FDateTime const& Last)
	{
		if (Last == FDateTime(0))
		{
			return NSLOCTEXT("MainMenu", "NeverPlayed", "Nunca");
		}
		FTimespan const Delta = FDateTime::Now() - Last;
		if (Delta.GetTotalMinutes() < 1.0)
		{
			return NSLOCTEXT("MainMenu", "JustNow", "Agora ha pouco");
		}
		if (Delta.GetTotalHours() < 1.0)
		{
			return FText::Format(
				NSLOCTEXT("MainMenu", "MinsAgo", "Há {0} minutos"), FText::AsNumber((int32)Delta.GetTotalMinutes()));
		}
		if (Delta.GetTotalDays() < 1.0)
		{
			return FText::Format(
				NSLOCTEXT("MainMenu", "HoursAgo", "Há {0} horas"), FText::AsNumber((int32)Delta.GetTotalHours()));
		}
		if ((int32)Delta.GetTotalDays() == 1)
		{
			return NSLOCTEXT("MainMenu", "Yesterday", "Jogado ontem");
		}
		return FText::Format(
			NSLOCTEXT("MainMenu", "DaysAgo", "Há {0} dias"), FText::AsNumber((int32)Delta.GetTotalDays()));
	}
}

void UDFSaveSlotCardUserWidget::ApplySlotBorderState(bool const bEmpty)
{
	if (!SlotBorderImage)
	{
		return;
	}
	SlotBorderImage->SetColorAndOpacity(
		bEmpty ? FLinearColor(0.3f, 0.3f, 0.3f, 1.f) : FLinearColor(1.f, 0.8f, 0.f, 1.f));
}

void UDFSaveSlotCardUserWidget::ResolveOptionalWidgetNames()
{
	if (!EmptyText)
	{
		if (UWidget* const W = GetWidgetFromName(FName("EmptyText")))
		{
			EmptyText = Cast<UTextBlock>(W);
		}
	}
	if (!EmptyRoot)
	{
		if (UWidget* const W = GetWidgetFromName(FName("EmptyRoot")))
		{
			EmptyRoot = W;
		}
	}
	if (!OccupiedRoot)
	{
		if (UWidget* const W = GetWidgetFromName(FName("OccupiedRoot")))
		{
			OccupiedRoot = W;
		}
	}
	if (!EmptySlotArt)
	{
		if (UWidget* const W = GetWidgetFromName(FName("EmptySlotArt")))
		{
			EmptySlotArt = Cast<UImage>(W);
		}
	}
	if (!SlotBorderImage)
	{
		if (UWidget* const W = GetWidgetFromName(FName("SlotBorderImage")))
		{
			SlotBorderImage = Cast<UImage>(W);
		}
	}
	if (!StateSwitcher)
	{
		if (UWidget* const W = GetWidgetFromName(FName("StateSwitcher")))
		{
			StateSwitcher = Cast<UWidgetSwitcher>(W);
		}
	}
}

void UDFSaveSlotCardUserWidget::ApplyStateSwitch(bool const bEmpty)
{
	if (StateSwitcher)
	{
		const int32 Idx = bEmpty ? SwitcherEmptyIndex : SwitcherOccupiedIndex;
		const int32 Clamped = FMath::Clamp(Idx, 0, FMath::Max(0, StateSwitcher->GetNumWidgets() - 1));
		StateSwitcher->SetActiveWidgetIndex(Clamped);
		return;
	}
	if (OccupiedRoot)
	{
		OccupiedRoot->SetVisibility(
			bEmpty ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
	if (EmptyRoot)
	{
		EmptyRoot->SetVisibility(
			bEmpty ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UDFSaveSlotCardUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ResolveOptionalWidgetNames();
	if (PlayButton) { PlayButton->OnClicked.AddDynamic(this, &UDFSaveSlotCardUserWidget::OnPlayClicked); }
	if (NewRunButton) { NewRunButton->OnClicked.AddDynamic(this, &UDFSaveSlotCardUserWidget::OnNewRunClicked); }
	if (CreateButton) { CreateButton->OnClicked.AddDynamic(this, &UDFSaveSlotCardUserWidget::OnCreateClicked); }
	if (DeleteButton) { DeleteButton->OnClicked.AddDynamic(this, &UDFSaveSlotCardUserWidget::OnDeleteClicked); }
}

void UDFSaveSlotCardUserWidget::NativeDestruct()
{
	if (PlayButton) { PlayButton->OnClicked.RemoveAll(this); }
	if (NewRunButton) { NewRunButton->OnClicked.RemoveAll(this); }
	if (CreateButton) { CreateButton->OnClicked.RemoveAll(this); }
	if (DeleteButton) { DeleteButton->OnClicked.RemoveAll(this); }
	Super::NativeDestruct();
}

void UDFSaveSlotCardUserWidget::ApplyEmptyState(bool const bManage)
{
	const FText PlaceholderEmpty = NSLOCTEXT("MainMenu", "EmptySaveSlotParen", "( PERFIL VAZIO )");
	if (EmptyText)
	{
		EmptyText->SetText(PlaceholderEmpty);
		EmptyText->SetVisibility(ESlateVisibility::Visible);
	}
	if (EmptySlotArt) { EmptySlotArt->SetVisibility(ESlateVisibility::Visible); }
	if (HintText)
	{
		HintText->SetText(NSLOCTEXT("MainMenu", "EmptyHint", "Clique em criar para começar"));
		HintText->SetVisibility(ESlateVisibility::Visible);
	}
	if (SlotLabel)
	{
		SlotLabel->SetText(
			EmptyText
			? FText::FromString(FString::Printf(TEXT("Perfil %d"), SlotIndex + 1))
			: FText::FromString(FString::Printf(
				TEXT("Perfil %d\n%s"), SlotIndex + 1, *PlaceholderEmpty.ToString())));
		SlotLabel->SetVisibility(ESlateVisibility::Visible);
	}
	else if (!EmptyText)
	{
		UE_LOG(
			LogTemp, Warning,
			TEXT("WBP_SaveSlotCard: slot vazio sem EmptyText/SlotLabel — adicione um TextBlock (nome 'EmptyText' ou 'SlotLabel')."));
	}
	// Sem switcher: garantimos que widgets do estado oposto fiquem ocultos.
	if (!StateSwitcher)
	{
		if (ClassPortraitArt) { ClassPortraitArt->SetVisibility(ESlateVisibility::Collapsed); }
		if (ClassNameText) { ClassNameText->SetVisibility(ESlateVisibility::Collapsed); }
		if (MetaLevelText) { MetaLevelText->SetVisibility(ESlateVisibility::Collapsed); }
		if (MetaXPBar) { MetaXPBar->SetVisibility(ESlateVisibility::Collapsed); }
		if (LastFloorText) { LastFloorText->SetVisibility(ESlateVisibility::Collapsed); }
		if (TotalRunsText) { TotalRunsText->SetVisibility(ESlateVisibility::Collapsed); }
		if (PlayTimeText) { PlayTimeText->SetVisibility(ESlateVisibility::Collapsed); }
		if (LastPlayedText) { LastPlayedText->SetVisibility(ESlateVisibility::Collapsed); }
		if (UnlockedClassIcons) { UnlockedClassIcons->SetVisibility(ESlateVisibility::Collapsed); }
		if (ActiveRunBadge) { ActiveRunBadge->SetVisibility(ESlateVisibility::Collapsed); }
		if (IncompatibleVersionText) { IncompatibleVersionText->SetVisibility(ESlateVisibility::Collapsed); }
	}
	if (CreateButton)
	{
		CreateButton->SetVisibility(bManage ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (PlayButton) { PlayButton->SetVisibility(ESlateVisibility::Collapsed); }
	if (NewRunButton) { NewRunButton->SetVisibility(ESlateVisibility::Collapsed); }
	if (DeleteButton) { DeleteButton->SetVisibility(ESlateVisibility::Collapsed); }
}

void UDFSaveSlotCardUserWidget::ApplyOccupiedState(UDFSaveGame* const Data, bool const bManage)
{
	check(Data);
	UDFRunManager* const RM = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDFRunManager>()
		: nullptr;
	if (SlotLabel)
	{
		SlotLabel->SetText(
			FText::FromString(FString::Printf(TEXT("Perfil %d"), SlotIndex + 1)));
		SlotLabel->SetVisibility(ESlateVisibility::Visible);
	}
	if (!StateSwitcher)
	{
		if (EmptySlotArt) { EmptySlotArt->SetVisibility(ESlateVisibility::Collapsed); }
		if (EmptyText) { EmptyText->SetVisibility(ESlateVisibility::Collapsed); }
		if (HintText) { HintText->SetVisibility(ESlateVisibility::Collapsed); }
	}
	if (FDFClassTableRow const* const ClassRow = RM ? RM->FindClassTableRow(Data->LastRunClass) : nullptr)
	{
		if (ClassNameText) { ClassNameText->SetText(ClassRow->ClassName); }
		if (ClassPortraitArt && ClassRow->ClassPortrait)
		{
			ClassPortraitArt->SetBrushFromTexture(ClassRow->ClassPortrait, false);
		}
	}
	else if (ClassNameText)
	{
		ClassNameText->SetText(FText::GetEmpty());
	}
	if (ClassNameText) { ClassNameText->SetVisibility(ESlateVisibility::Visible); }
	if (ClassPortraitArt) { ClassPortraitArt->SetVisibility(ESlateVisibility::Visible); }
	if (MetaLevelText)
	{
		MetaLevelText->SetText(
			FText::FromString(FString::Printf(TEXT("Nexus Nv. %d"), FMath::Max(1, Data->MetaLevel))));
		MetaLevelText->SetVisibility(ESlateVisibility::Visible);
	}
	if (MetaXPBar)
	{
		const float P = Data->GetNexusMetaXPFillRatio(
			RM && RM->NexusMetaLevelsTable ? RM->NexusMetaLevelsTable : nullptr);
		MetaXPBar->SetPercent(P);
		MetaXPBar->SetVisibility(ESlateVisibility::Visible);
	}
	if (LastFloorText)
	{
		if (Data->TotalWins > 0 && Data->bLastRunWasVictory)
		{
			LastFloorText->SetText(
				FText::FromString(
					FString::Printf(TEXT("\u2605 Vitoria no Andar %d"), Data->LastRunFloor)));
			LastFloorText->SetColorAndOpacity(
				FSlateColor(FLinearColor(0.95f, 0.8f, 0.2f, 1.f)));
		}
		else if (Data->bHasActiveRun)
		{
			LastFloorText->SetText(
				FText::FromString(FString::Printf(TEXT("Run ativa: Andar %d"), Data->LastRunFloor)));
			LastFloorText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		}
		else
		{
			LastFloorText->SetText(
				FText::FromString(FString::Printf(TEXT("Melhor andar: %d"), Data->BestFloorReached)));
			LastFloorText->SetColorAndOpacity(
				FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f, 1.f)));
		}
		LastFloorText->SetVisibility(ESlateVisibility::Visible);
	}
	if (TotalRunsText)
	{
		TotalRunsText->SetText(
			FText::FromString(FString::Printf(TEXT("%d runs - %d vitorias"), Data->TotalRuns, Data->TotalWins)));
		TotalRunsText->SetVisibility(ESlateVisibility::Visible);
	}
	if (PlayTimeText)
	{
		const int32 T = FMath::RoundToInt(Data->TotalPlayTimeSeconds);
		const int32 H = T / 3600;
		const int32 M = (T % 3600) / 60;
		PlayTimeText->SetText(FText::FromString(FString::Printf(TEXT("%dh %dmin jogadas"), H, M)));
		PlayTimeText->SetVisibility(ESlateVisibility::Visible);
	}
	if (LastPlayedText)
	{
		LastPlayedText->SetText(DfFormatRelativeTime(Data->LastPlayedDate));
		LastPlayedText->SetVisibility(ESlateVisibility::Visible);
	}
	if (IncompatibleVersionText)
	{
		const bool bWarn = !Data->IsCompatible();
		IncompatibleVersionText->SetVisibility(
			bWarn ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bWarn)
		{
			IncompatibleVersionText->SetText(
				FText::Format(
					NSLOCTEXT("MainMenu", "OldVersion", "Salvo em versao antiga ({0})"),
					FText::FromString(Data->GameVersion)));
		}
	}
	if (ActiveRunBadge)
	{
		ActiveRunBadge->SetVisibility(
			Data->bHasActiveRun ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (UnlockedClassIcons)
	{
		UnlockedClassIcons->SetVisibility(ESlateVisibility::Visible);
		UnlockedClassIcons->ClearChildren();
		if (RM && RM->ClassDataTable)
		{
			for (FName const ClassName : Data->UnlockedClasses)
			{
				if (FDFClassTableRow const* const R = RM->ClassDataTable->FindRow<FDFClassTableRow>(
					ClassName, TEXT("UnlockedClassIcon"), false))
				{
					if (R->ClassPortrait)
					{
						UImage* const Img = NewObject<UImage>(this);
						Img->SetBrushFromTexture(R->ClassPortrait, false);
						UnlockedClassIcons->AddChild(Img);
					}
				}
			}
		}
	}
	if (bManage)
	{
		if (PlayButton) { PlayButton->SetVisibility(ESlateVisibility::Collapsed); }
		if (NewRunButton) { NewRunButton->SetVisibility(ESlateVisibility::Collapsed); }
		if (CreateButton) { CreateButton->SetVisibility(ESlateVisibility::Collapsed); }
		if (DeleteButton) { DeleteButton->SetVisibility(ESlateVisibility::Visible); }
	}
	else
	{
		if (PlayButton) { PlayButton->SetVisibility(ESlateVisibility::Visible); }
		if (NewRunButton) { NewRunButton->SetVisibility(ESlateVisibility::Visible); }
		if (CreateButton) { CreateButton->SetVisibility(ESlateVisibility::Collapsed); }
		if (DeleteButton) { DeleteButton->SetVisibility(ESlateVisibility::Visible); }
	}
}

void UDFSaveSlotCardUserWidget::RefreshSlotData(int32 const InSlotIndex, EDFSlotScreenMode const InMode)
{
	SlotIndex = InSlotIndex;
	Mode = InMode;
	ResolveOptionalWidgetNames();
	UGameInstance* const GI = GetGameInstance();
	UDFSaveSlotManagerSubsystem* const Slots = GI ? GI->GetSubsystem<UDFSaveSlotManagerSubsystem>() : nullptr;
	if (!Slots)
	{
		return;
	}
	const bool bEmpty = Slots->IsSlotEmpty(SlotIndex);
	UDFSaveGame* const Data = Slots->GetSlotData(SlotIndex);
	const bool bManage = (Mode == EDFSlotScreenMode::SelectToDelete);
	const bool bIsEmpty = bEmpty || !Data;
	// O card raiz nunca pode ficar invisível mesmo no estado vazio.
	SetVisibility(ESlateVisibility::Visible);
	ApplySlotBorderState(bIsEmpty);
	ApplyStateSwitch(bIsEmpty);
	if (bIsEmpty)
	{
		ApplyEmptyState(bManage);
	}
	else
	{
		ApplyOccupiedState(Data, bManage);
	}
	if (CardRefreshAnim) { PlayAnimation(CardRefreshAnim, 0.f, 1, EUMGSequencePlayMode::Forward, 1.f, false); }
}

void UDFSaveSlotCardUserWidget::OnPlayClicked()
{
	UGameInstance* const GI = GetGameInstance();
	UDFSaveSlotManagerSubsystem* const Slots = GI ? GI->GetSubsystem<UDFSaveSlotManagerSubsystem>() : nullptr;
	if (!Slots || Mode == EDFSlotScreenMode::SelectToDelete)
	{
		return;
	}
	Slots->SelectSlot(SlotIndex);
	UDFSaveGame* S = Slots->GetActiveSave();
	if (!S)
	{
		return;
	}
	if (S->bHasActiveRun)
	{
		if (UDFWorldTransitionSubsystem* const T = GI->GetSubsystem<UDFWorldTransitionSubsystem>())
		{
			T->TravelToNexus(ETravelReason::FirstLaunch);
		}
		return;
	}
	AfterNewRunCleared();
}

void UDFSaveSlotCardUserWidget::AfterNewRunCleared()
{
	UWorld* const W = GetWorld();
	if (UDFClassSelectionSubsystem* const Sub = W ? W->GetSubsystem<UDFClassSelectionSubsystem>() : nullptr)
	{
		Sub->SetMainMenuClassPickDestination(EDFMainMenuClassPickDestination::RunDungeon);
		Sub->OpenClassSelection();
	}
}

void UDFSaveSlotCardUserWidget::OnNewRunClicked()
{
	UGameInstance* const GI = GetGameInstance();
	UDFSaveSlotManagerSubsystem* const Slots = GI ? GI->GetSubsystem<UDFSaveSlotManagerSubsystem>() : nullptr;
	if (!Slots || Mode == EDFSlotScreenMode::SelectToDelete)
	{
		return;
	}
	Slots->SelectSlot(SlotIndex);
	UDFSaveGame* S = Slots->GetActiveSave();
	if (!S)
	{
		return;
	}
	if (S->bHasActiveRun)
	{
		ShowNewRunAbandonConfirm(S);
		return;
	}
	S->ResetRunData();
	(void)Slots->SaveActiveSlot();
	AfterNewRunCleared();
}

void UDFSaveSlotCardUserWidget::OnCreateClicked()
{
	UGameInstance* const GI = GetGameInstance();
	UDFSaveSlotManagerSubsystem* const Slots = GI ? GI->GetSubsystem<UDFSaveSlotManagerSubsystem>() : nullptr;
	if (!Slots || Mode == EDFSlotScreenMode::SelectToDelete)
	{
		return;
	}
	Slots->SelectSlot(SlotIndex);
	if (UWorld* const W = GetWorld())
	{
		if (UDFClassSelectionSubsystem* const Sub = W->GetSubsystem<UDFClassSelectionSubsystem>())
		{
			Sub->SetMainMenuClassPickDestination(EDFMainMenuClassPickDestination::NexusFirstLaunch);
			Sub->OpenClassSelection();
		}
	}
}

void UDFSaveSlotCardUserWidget::OnDeleteClicked()
{
	UGameInstance* const GI = GetGameInstance();
	UDFSaveSlotManagerSubsystem* const Slots = GI ? GI->GetSubsystem<UDFSaveSlotManagerSubsystem>() : nullptr;
	if (!Slots)
	{
		return;
	}
	UDFSaveGame* const S = Slots->GetSlotData(SlotIndex);
	ShowDeleteConfirm(S);
}

void UDFSaveSlotCardUserWidget::ShowDeleteConfirm(UDFSaveGame* const Data)
{
	APlayerController* const PC = GetOwningPlayer();
	ADFMainMenuHUD* const H = PC ? Cast<ADFMainMenuHUD>(PC->GetHUD()) : nullptr;
	if (!H || !H->ConfirmWidgetClass || !Data)
	{
		return;
	}
	UDFConfirmDialogUserWidget* const D = CreateWidget<UDFConfirmDialogUserWidget>(PC, H->ConfirmWidgetClass);
	if (!D)
	{
		return;
	}
	const FText T = FText::Format(
		NSLOCTEXT("MainMenu", "DelTitle", "Apagar Perfil {0}?"), FText::AsNumber(SlotIndex + 1));
	const FText B = FText::Format(
		NSLOCTEXT("MainMenu", "DelBody", "Nexus Nv.{0} - {1} runs - {2} vitorias. Irreversivel."),
		FText::AsNumber(FMath::Max(1, Data->MetaLevel)),
		FText::AsNumber(Data->TotalRuns), FText::AsNumber(Data->TotalWins));
	UDFSaveSlotManagerSubsystem* const Slots = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDFSaveSlotManagerSubsystem>()
		: nullptr;
	const int32 Cap = SlotIndex;
	D->ShowDialog(
		T, B,
		FSimpleDelegate::CreateWeakLambda(
			this,
			[this, Cap, Slots]
			{
				if (Slots)
				{
					Slots->DeleteSlot(Cap);
				}
				if (DeleteSound)
				{
					UGameplayStatics::PlaySound2D(this, DeleteSound, 0.5f, 1.f, 0.f, nullptr, nullptr, false);
				}
				RefreshSlotData(Cap, Mode);
			}));
	H->ShowConfirmDialog(D);
}

void UDFSaveSlotCardUserWidget::ShowNewRunAbandonConfirm(UDFSaveGame* const Data)
{
	if (!Data)
	{
		return;
	}
	APlayerController* const PC = GetOwningPlayer();
	ADFMainMenuHUD* const H = PC ? Cast<ADFMainMenuHUD>(PC->GetHUD()) : nullptr;
	if (!H || !H->ConfirmWidgetClass)
	{
		return;
	}
	UDFConfirmDialogUserWidget* const D = CreateWidget<UDFConfirmDialogUserWidget>(PC, H->ConfirmWidgetClass);
	if (!D)
	{
		return;
	}
	UDFRunManager* const RM = GetGameInstance() ? GetGameInstance()->GetSubsystem<UDFRunManager>() : nullptr;
	FString ClassLine = Data->LastRunClass.ToString();
	if (RM)
	{
		if (FDFClassTableRow const* const R = RM->FindClassTableRow(Data->LastRunClass))
		{
			ClassLine = R->ClassName.ToString();
		}
	}
	const FText B = FText::Format(
		NSLOCTEXT("MainMenu", "AbandonRunBody", "Andar {0} como {1}. Iniciar nova run encerra esta. Meta-progresso e mantido."),
		FText::AsNumber(Data->LastRunFloor), FText::FromString(ClassLine));
	UGameInstance* const GI = GetGameInstance();
	UDFSaveSlotManagerSubsystem* const Slots = GI ? GI->GetSubsystem<UDFSaveSlotManagerSubsystem>() : nullptr;
	const int32 CapSlot = SlotIndex;
	D->ShowDialog(
		NSLOCTEXT("MainMenu", "AbandonRunTitle", "Abandonar run atual?"), B,
		FSimpleDelegate::CreateWeakLambda(
			this,
			[GI, CapSlot, Slots, this]
			{
				if (Slots) { Slots->SelectSlot(CapSlot); }
				UDFSaveGame* S = Slots ? Slots->GetActiveSave() : nullptr;
				if (S) { S->ResetRunData(); S->bHasActiveRun = false; S->LastRunClass = NAME_None; if (Slots) { (void)Slots->SaveActiveSlot(); } }
				if (UWorld* const W = this->GetWorld())
				{
					if (UDFClassSelectionSubsystem* const Sub = W->GetSubsystem<UDFClassSelectionSubsystem>())
					{
						Sub->SetMainMenuClassPickDestination(EDFMainMenuClassPickDestination::RunDungeon);
						Sub->OpenClassSelection();
					}
				}
			}));
	H->ShowConfirmDialog(D);
}
