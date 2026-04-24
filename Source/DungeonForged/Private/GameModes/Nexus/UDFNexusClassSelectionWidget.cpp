// Source/DungeonForged/Private/GameModes/Nexus/UDFNexusClassSelectionWidget.cpp
#include "GameModes/Nexus/UDFNexusClassSelectionWidget.h"
#include "GameModes/Nexus/ADFNexusPlayerController.h"
#include "GameModes/Nexus/UDFNexusClassListObject.h"
#include "Components/TileView.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

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
}

void UDFNexusClassSelectionWidget::ConfirmAndTravel()
{
	if (CurrentSelected.IsNone())
	{
		return;
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
