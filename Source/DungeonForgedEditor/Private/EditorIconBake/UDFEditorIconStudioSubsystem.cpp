#include "EditorIconBake/UDFEditorIconStudioSubsystem.h"

#include "Editor.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "Misc/PackageName.h"
#include "Settings/UDFIconStudioWorkflowSettings.h"

void UDFEditorIconStudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	FEditorDelegates::OnMapOpened.AddUObject(this, &UDFEditorIconStudioSubsystem::HandleMapOpened);
}

void UDFEditorIconStudioSubsystem::Deinitialize()
{
	FEditorDelegates::OnMapOpened.RemoveAll(this);
	Super::Deinitialize();
}

void UDFEditorIconStudioSubsystem::HandleMapOpened(const FString& Filename, bool bLoadAsTemplate)
{
	const UDFIconStudioWorkflowSettings* Settings = GetDefault<UDFIconStudioWorkflowSettings>();
	if (!Settings || !Settings->bAutoOpenIconBakeEUWWhenMapOpens)
	{
		return;
	}

	const FSoftObjectPath MapPath = Settings->IconStudioPersistentMap.ToSoftObjectPath();
	if (!MapPath.IsValid())
	{
		return;
	}

	FString OpenedPackageName;
	if (!FPackageName::TryConvertFilenameToLongPackageName(Filename, OpenedPackageName))
	{
		if (FPackageName::IsValidLongPackageName(Filename))
		{
			OpenedPackageName = Filename;
		}
		else
		{
			return;
		}
	}

	const FString TargetPackageName = MapPath.GetLongPackageName();
	if (TargetPackageName.IsEmpty() || !OpenedPackageName.Equals(TargetPackageName, ESearchCase::IgnoreCase))
	{
		return;
	}

	if (!Settings->IconBakeEditorUtilityWidget.IsValid())
	{
		return;
	}

	UEditorUtilityWidgetBlueprint* EUWBP = Cast<UEditorUtilityWidgetBlueprint>(
		Settings->IconBakeEditorUtilityWidget.TryLoad());
	if (!EUWBP)
	{
		return;
	}

	if (!GEditor)
	{
		return;
	}

	if (UEditorUtilitySubsystem* UtilSubsys = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>())
	{
		UtilSubsys->SpawnAndRegisterTab(EUWBP);
	}
}
