// Source/DungeonForged/Private/GameModes/Nexus/UDFNexusClassCardWidget.cpp
#include "GameModes/Nexus/UDFNexusClassCardWidget.h"
#include "GameModes/Nexus/UDFNexusClassListObject.h"
#include "Blueprint/IUserListEntry.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Blueprint/UserWidget.h"

namespace
{

static bool BrushHasImage(const FSlateBrush& B)
{
	return B.GetResourceObject() != nullptr;
}

} // namespace

void UDFNexusClassCardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshCardFrame();
}

FSlateBrush UDFNexusClassCardWidget::ResolveFrameBrush() const
{
	if (bListEntrySelected && BrushHasImage(FrameBrushSelected))
	{
		return FrameBrushSelected;
	}
	if (bListEntryHovered && BrushHasImage(FrameBrushHover))
	{
		return FrameBrushHover;
	}
	if (BrushHasImage(FrameBrushNormal))
	{
		return FrameBrushNormal;
	}
	if (bListEntrySelected && BrushHasImage(FrameBrushHover))
	{
		return FrameBrushHover;
	}
	if (bListEntryHovered && BrushHasImage(FrameBrushSelected))
	{
		return FrameBrushSelected;
	}
	if (BrushHasImage(FrameBrushSelected))
	{
		return FrameBrushSelected;
	}
	if (BrushHasImage(FrameBrushHover))
	{
		return FrameBrushHover;
	}
	return FSlateBrush();
}

void UDFNexusClassCardWidget::RefreshCardFrame()
{
	if (!Border_icone)
	{
		return;
	}
	if (!BrushHasImage(FrameBrushNormal) && !BrushHasImage(FrameBrushHover) && !BrushHasImage(FrameBrushSelected))
	{
		return;
	}

	const FSlateBrush R = ResolveFrameBrush();
	if (!BrushHasImage(R))
	{
		return;
	}
	Border_icone->SetBrush(R);
}

void UDFNexusClassCardWidget::NativeOnItemSelectionChanged(const bool bIsSelected)
{
	IUserListEntry::NativeOnItemSelectionChanged(bIsSelected);
	bListEntrySelected = bIsSelected;
	RefreshCardFrame();
}

void UDFNexusClassCardWidget::NativeOnMouseEnter(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	bListEntryHovered = true;
	RefreshCardFrame();
}

void UDFNexusClassCardWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	bListEntryHovered = false;
	RefreshCardFrame();
}

void UDFNexusClassCardWidget::SetClassData(
	const FName& InRow,
	const FText& Name,
	const bool bLocked,
	const FText& LockText,
	UTexture2D* const Portrait)
{
	ClassRow = InRow;
	if (ClassName)
	{
		ClassName->SetText(bLocked && !LockText.IsEmpty() ? LockText : Name);
	}
	if (ClassArt)
	{
		if (Portrait)
		{
			ClassArt->SetBrushFromTexture(Portrait, false);
			ClassArt->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			ClassArt->SetBrushResourceObject(nullptr);
			ClassArt->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (LockOverlay)
	{
		LockOverlay->SetVisibility(bLocked ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	RefreshCardFrame();
}

void UDFNexusClassCardWidget::NativeOnListItemObjectSet(UObject* const ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	if (UDFNexusClassListObject* const O = Cast<UDFNexusClassListObject>(ListItemObject))
	{
		SetClassData(O->ClassRow, O->Name, O->bLocked, O->LockHint, O->ClassPortrait);
	}
}
