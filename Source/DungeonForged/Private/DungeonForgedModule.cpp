// Source/DungeonForged/Private/DungeonForgedModule.cpp

#include "DungeonForgedModule.h"

#include "Blueprint/UserWidget.h"
#include "GAS/DFGameplayCueRegistration.h"
#include "GAS/DFGameplayTags.h"
#include "Misc/CoreDelegates.h"

DEFINE_LOG_CATEGORY(LogDungeonForged);

void DFPrepareWidgetForUIModeFocus(UUserWidget* const Widget)
{
	if (!Widget)
	{
		return;
	}
	Widget->SetIsFocusable(true);
	Widget->SynchronizeProperties();
}

void FDungeonForgedModule::StartupModule()
{
	FDFGameplayTags::RegisterGameplayTags();

	// GAS / IConsoleManager are not safe during module StartupModule (editor crash). Defer cues.
	FCoreDelegates::OnPostEngineInit.AddLambda([]()
	{
		DFGameplayCueRegistration::RegisterNativeGameplayCues();
	});
}

void FDungeonForgedModule::ShutdownModule() {}
