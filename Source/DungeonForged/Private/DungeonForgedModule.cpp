// Source/DungeonForged/Private/DungeonForgedModule.cpp

#include "DungeonForgedModule.h"

#include "Blueprint/UserWidget.h"
#include "GAS/DFGameplayCueRegistration.h"
#include "GAS/DFGameplayTags.h"
#include "Misc/CoreDelegates.h"

DEFINE_LOG_CATEGORY(LogDungeonForged);
DEFINE_LOG_CATEGORY(LogDFTuning);
DEFINE_LOG_CATEGORY(LogDFFeel);
DEFINE_LOG_CATEGORY(LogDFDeath);
DEFINE_LOG_CATEGORY(LogDFAI);

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
