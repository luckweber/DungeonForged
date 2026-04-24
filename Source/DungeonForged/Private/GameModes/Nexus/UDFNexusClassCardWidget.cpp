// Source/DungeonForged/Private/GameModes/Nexus/UDFNexusClassCardWidget.cpp
#include "GameModes/Nexus/UDFNexusClassCardWidget.h"
#include "GameModes/Nexus/UDFNexusClassListObject.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"

void UDFNexusClassCardWidget::SetClassData(
	const FName& InRow,
	const FText& Name,
	const FText& Blurb,
	const bool bLocked,
	const FText& LockText)
{
	ClassRow = InRow;
	if (ClassName)
	{
		ClassName->SetText(bLocked && !LockText.IsEmpty() ? LockText : Name);
	}
	if (ClassBlurb)
	{
		ClassBlurb->SetText(Blurb);
	}
	if (LockOverlay)
	{
		LockOverlay->SetVisibility(bLocked ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UDFNexusClassCardWidget::NativeOnListItemObjectSet(UObject* const ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	if (UDFNexusClassListObject* const O = Cast<UDFNexusClassListObject>(ListItemObject))
	{
		SetClassData(O->ClassRow, O->Name, O->Blurb, O->bLocked, O->LockHint);
	}
}
