// Source/DungeonForged/Public/DungeonForgedModule.h
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

/** Global game module log. Use in any .cpp that includes this header after DEFINE in DungeonForgedModule.cpp. */
DECLARE_LOG_CATEGORY_EXTERN(LogDungeonForged, Log, All);

/** Literal de formato apenas (sem TEXT()); a macro aplica TEXT ao literal. */
#define DF_LOG(Verbosity, Format, ...) UE_LOG(LogDungeonForged, Verbosity, TEXT(Format), ##__VA_ARGS__)

#define DF_SCREEN(Color, Duration, Format, ...) \
	GEngine->AddOnScreenDebugMessage(-1, Duration, Color, FString::Printf(TEXT(Format), ##__VA_ARGS__))

class UUserWidget;

/** Chame imediatamente antes de FInputModeUIOnly::SetWidgetToFocus(W->TakeWidget()) para evitar erro "Non-Focusable widget SObjectWidget". */
DUNGEONFORGED_API void DFPrepareWidgetForUIModeFocus(UUserWidget* Widget);

class FDungeonForgedModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
