// Source/DungeonForged/Private/DungeonForgedModule.cpp

#include "DungeonForgedModule.h"
#include "Blueprint/UserWidget.h"

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

void FDungeonForgedModule::StartupModule() {}

void FDungeonForgedModule::ShutdownModule() {}
