// Source/DungeonForged/Public/Animation/DFTurnInPlaceDebug.h
#pragma once

#include "CoreMinimal.h"

namespace DFTurnInPlaceDebug
{
/** Level >= 1: Output Log [TIP] lines. */
bool IsLogEnabled();
/** Level >= 2: log + on-screen HUD (turn-only). */
bool IsHudEnabled();
/** Level >= 3: ground circle + body/aim arcs + turn direction. */
bool IsDrawEnabled();
/** Level >= 4: extra one-line [TIP|1] line (easy to paste in chat). */
bool IsVerboseEnabled();

float GetCircleRadiusCm();
} // namespace DFTurnInPlaceDebug
