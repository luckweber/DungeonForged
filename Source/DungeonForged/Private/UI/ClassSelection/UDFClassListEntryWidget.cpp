// Source/DungeonForged/Private/UI/ClassSelection/UDFClassListEntryWidget.cpp
#include "UI/ClassSelection/UDFClassListEntryWidget.h"
#include "UI/ClassSelection/UDFClassSelectionWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Data/DFDataTableStructs.h"
#include "UI/ClassSelection/UDFClassSelectionSubsystem.h"
#include "Styling/SlateColor.h"

void UDFClassListEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (SelectButton)
	{
		SelectButton->OnClicked.AddDynamic(this, &UDFClassListEntryWidget::OnSelectPressed);
		SelectButton->OnHovered.AddDynamic(this, &UDFClassListEntryWidget::OnHovered);
		SelectButton->OnUnhovered.AddDynamic(this, &UDFClassListEntryWidget::OnUnhovered);
	}
	RefreshVisual();
}

void UDFClassListEntryWidget::InitializeEntry(
	const FName InClassRow, UDFClassSelectionWidget* const InOwner, const bool bInUnlocked, const bool bInSelected)
{
	ClassRow = InClassRow;
	OwnerScreen = InOwner;
	bUnlocked = bInUnlocked;
	bSelected = bInSelected;
	RefreshVisual();
}

void UDFClassListEntryWidget::RefreshVisual()
{
	if (UWorld* const W = GetWorld())
	{
		UDFClassSelectionSubsystem* const Sub = W->GetSubsystem<UDFClassSelectionSubsystem>();
		if (const FDFClassTableRow* const Row = Sub ? Sub->GetClassData(ClassRow) : nullptr)
		{
			if (ClassName) ClassName->SetText(Row->ClassName);
			if (ClassArchetype) ClassArchetype->SetText(Row->ClassArchetype);
			if (ClassPortrait)
			{
				if (Row->ClassPortrait)
				{
					ClassPortrait->SetBrushFromTexture(Row->ClassPortrait);
				}
				ClassPortrait->SetColorAndOpacity(
					bUnlocked ? FLinearColor::White : FLinearColor(0.4f, 0.4f, 0.4f, 1.f));
			}
		}
	}
	int32 PipsOn = 3;
	if (UWorld* const W = GetWorld())
	{
		if (const UDFClassSelectionSubsystem* const Sub = W->GetSubsystem<UDFClassSelectionSubsystem>())
		{
			if (const FDFClassTableRow* R = Sub->GetClassData(ClassRow))
			{
				PipsOn = FMath::Clamp(R->DifficultyPips, 0, 5);
			}
		}
	}
	UImage* const PipsW[] = { DifficultyPip0.Get(), DifficultyPip1.Get(), DifficultyPip2.Get(), DifficultyPip3.Get(), DifficultyPip4.Get() };
	for (int32 I = 0; I < 5; ++I)
	{
		if (PipsW[I])
		{
			PipsW[I]->SetVisibility(I < PipsOn ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}
	if (LockOverlay)
	{
		LockOverlay->SetVisibility(bUnlocked ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (UnlockHintText)
	{
		if (UWorld* const W = GetWorld())
		{
			if (UDFClassSelectionSubsystem* const Sub = W->GetSubsystem<UDFClassSelectionSubsystem>())
			{
				UnlockHintText->SetText(
					bUnlocked ? FText::GetEmpty() : Sub->GetUnlockConditionText(ClassRow));
			}
		}
		UnlockHintText->SetVisibility(bUnlocked ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (SelectButton)
	{
		SelectButton->SetIsEnabled(bUnlocked);
	}
	if (SelectionBorder)
	{
		SelectionBorder->SetVisibility(bSelected ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UDFClassListEntryWidget::OnSelectPressed()
{
	if (OwnerScreen)
	{
		OwnerScreen->SelectClassByRow(ClassRow);
	}
}

void UDFClassListEntryWidget::OnHovered()
{
	if (ClassName) ClassName->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.95f, 0.5f, 1.f)));
}

void UDFClassListEntryWidget::OnUnhovered()
{
	if (ClassName) ClassName->SetColorAndOpacity(FSlateColor(FLinearColor::White));
}
