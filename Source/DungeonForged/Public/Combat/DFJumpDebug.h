// Source/DungeonForged/Public/Combat/DFJumpDebug.h
#pragma once

#include "CoreMinimal.h"

namespace DFJumpDebug
{
bool IsLogEnabled();
bool IsHudEnabled();
/** Level >= 3: multi-line transition HUD + edge logs. */
bool IsTransitionHudEnabled();
/** Level >= 4: deep anim/SM timing snapshots (log + HUD). */
bool IsDeepLogEnabled();

void Log(const TCHAR* Message);
void Logf(const TCHAR* Format, ...);
/** Splits multiline text into separate [Jump|Deep] log lines. */
void LogBlock(const TCHAR* Header, const FString& Body);
} // namespace DFJumpDebug
