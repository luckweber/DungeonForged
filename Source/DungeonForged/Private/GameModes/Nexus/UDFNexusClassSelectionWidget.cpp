// Source/DungeonForged/Private/GameModes/Nexus/UDFNexusClassSelectionWidget.cpp
#include "GameModes/Nexus/UDFNexusClassSelectionWidget.h"
#include "GameModes/Nexus/ADFNexusPlayerController.h"
#include "GameModes/Nexus/UDFNexusClassListObject.h"
#include "UI/ClassSelection/UDFClassSelectionSubsystem.h"
#include "Data/DFDataTableStructs.h"
#include "DungeonForgedModule.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TileView.h"
#include "Engine/DataTable.h"
#include "Engine/TextureRenderTarget2D.h"

namespace
{

static UDFNexusClassListObject* PickInitialTilePayload(const FName DesiredRow,
	const TArray<UDFNexusClassListObject*>& ItemsSorted,
	const UDFClassSelectionSubsystem* const Sub)
{
	if (!Sub)
	{
		return nullptr;
	}
	if (!DesiredRow.IsNone())
	{
		for (UDFNexusClassListObject* O : ItemsSorted)
		{
			if (O && O->ClassRow == DesiredRow && !O->bLocked)
			{
				return O;
			}
		}
	}
	for (UDFNexusClassListObject* O : ItemsSorted)
	{
		if (O && !O->bLocked)
		{
			return O;
		}
	}
	return nullptr;
}

} // namespace

void UDFNexusClassSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (StartRunButton)
	{
		StartRunButton->SetIsEnabled(false);
		StartRunButton->OnClicked.AddDynamic(this, &UDFNexusClassSelectionWidget::OnStartRunClicked);
	}
	if (UpgradesButton)
	{
		UpgradesButton->OnClicked.AddDynamic(this, &UDFNexusClassSelectionWidget::OnUpgradesClicked);
	}
	if (ClassTileView)
	{
		ClassTileView->OnItemSelectionChanged().AddUObject(this, &UDFNexusClassSelectionWidget::OnClassTileSelectionChanged);
	}
	RebuildClassTileItemsFromSubsystem();
	RefreshPreviewBrushFromSubsystem();
}

void UDFNexusClassSelectionWidget::RebuildClassTileItemsFromSubsystem()
{
	if (!ClassTileView)
	{
		return;
	}
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	UDFClassSelectionSubsystem* const Sub = W->GetSubsystem<UDFClassSelectionSubsystem>();
	UDataTable* const DT = Sub ? Sub->GetClassTable() : nullptr;
	if (!Sub || !DT)
	{
		DF_LOG(Warning,
			"[DF|Nexus|ClassTile] DT de classes ausente — verifique DFRunManager::ClassDataTable / Project Settings Dungeon Forged | "
			"Class Selection.");
		ClassTileView->ClearListItems();
		return;
	}
	TArray<FName> RowNames;
	DT->GetRowMap().GetKeys(RowNames);
	RowNames.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });

	TArray<UDFNexusClassListObject*> OwnedItems;
	OwnedItems.Reserve(RowNames.Num());

	for (const FName& RowName : RowNames)
	{
		const FDFClassTableRow* const Row = DT->FindRow<FDFClassTableRow>(RowName, TEXT("NexusTile"));
		if (!Row)
		{
			continue;
		}
		UDFNexusClassListObject* const O = NewObject<UDFNexusClassListObject>(this);
		O->ClassRow = RowName;
		O->Name = Row->ClassName;
		O->Blurb = Row->ClassDescription;
		O->bLocked = !Sub->IsClassUnlocked(RowName);
		O->LockHint = Sub->GetUnlockConditionText(RowName);
		OwnedItems.Add(O);
	}

	RefreshUnlockedList(OwnedItems);

	const FName SubSelection = Sub->GetSelectedClass();
	if (UDFNexusClassListObject* const Pick = PickInitialTilePayload(SubSelection, OwnedItems, Sub))
	{
		ClassTileView->SetSelectedItem(Pick);
		SetSelectedClassRow(Pick->ClassRow);
	}
}

void UDFNexusClassSelectionWidget::SetSelectedClassRow(const FName ClassRow)
{
	CurrentSelected = ClassRow;

	UDFClassSelectionSubsystem* Sub = nullptr;
	if (UWorld* const W = GetWorld())
	{
		Sub = W->GetSubsystem<UDFClassSelectionSubsystem>();
	}
	if (Sub)
	{
		Sub->SetSelectedClass(ClassRow);
		Sub->UpdatePreviewForClass(ClassRow);
	}

	RefreshPreviewBrushFromSubsystem();

	if (StartRunButton)
	{
		const bool bCanStart =
			ClassRow.IsNone() == false && Sub && Sub->IsClassUnlocked(ClassRow);
		StartRunButton->SetIsEnabled(bCanStart);
	}
}

void UDFNexusClassSelectionWidget::RefreshPreviewBrushFromSubsystem()
{
	if (!PreviewRenderTarget)
	{
		return;
	}
	UWorld* const W = GetWorld();
	UDFClassSelectionSubsystem* const Sub = W ? W->GetSubsystem<UDFClassSelectionSubsystem>() : nullptr;
	if (!Sub || !Sub->IsPreviewUsingSceneCapture())
	{
		PreviewRenderTarget->SetBrushResourceObject(nullptr);
		PreviewRenderTarget->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	if (UTextureRenderTarget2D* const RT = Sub->GetRenderTarget())
	{
		PreviewRenderTarget->SetBrushResourceObject(RT);
		PreviewRenderTarget->SetVisibility(ESlateVisibility::Visible);
	}
}

void UDFNexusClassSelectionWidget::OnClassTileSelectionChanged(UObject* const Item)
{
	if (!Item)
	{
		return;
	}
	UDFNexusClassListObject* const O = Cast<UDFNexusClassListObject>(Item);
	if (!O || O->bLocked)
	{
		return;
	}
	SetSelectedClassRow(O->ClassRow);
}

void UDFNexusClassSelectionWidget::OnStartRunClicked()
{
	ConfirmAndTravel();
}

void UDFNexusClassSelectionWidget::OnUpgradesClicked()
{
	OpenMetaUpgradesOverlay();
}

void UDFNexusClassSelectionWidget::RefreshUnlockedList(const TArray<UDFNexusClassListObject*>& InItems)
{
	if (ClassTileView)
	{
		ClassTileView->ClearListItems();
		for (UDFNexusClassListObject* O : InItems)
		{
			if (O)
			{
				ClassTileView->AddItem(O);
			}
		}
	}
	RefreshPreviewBrushFromSubsystem();
}

void UDFNexusClassSelectionWidget::ConfirmAndTravel()
{
	if (CurrentSelected.IsNone())
	{
		return;
	}
	if (UWorld* const W = GetWorld())
	{
		if (UDFClassSelectionSubsystem* const Sub = W->GetSubsystem<UDFClassSelectionSubsystem>())
		{
			if (!Sub->IsClassUnlocked(CurrentSelected))
			{
				return;
			}
		}
	}
	if (APlayerController* const PC = GetOwningPlayer())
	{
		if (ADFNexusPlayerController* const N = Cast<ADFNexusPlayerController>(PC))
		{
			N->Server_BeginRunWithClass(CurrentSelected);
		}
	}
	RemoveFromParent();
}

void UDFNexusClassSelectionWidget::OpenMetaUpgradesOverlay()
{
	// Designer hook: add WBP in Blueprint.
}
