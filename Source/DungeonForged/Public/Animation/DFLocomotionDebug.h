// Source/DungeonForged/Public/Animation/DFLocomotionDebug.h
#pragma once

#include "CoreMinimal.h"

namespace DFLocomotionDebug
{
/** Level >= 1: Output Log [Loco] lines (gait, dir, transitions). */
bool IsLogEnabled();
/** Level >= 2: on-screen HUD (gait, dir, transition flags, resolved anims). */
bool IsHudEnabled();
/** Level >= 3: world-space direction arrows (facing + velocity). */
bool IsDrawEnabled();
/** Level >= 4: deep HUD + periodic log (anim RM/Distance curve, CMC caps, stride scale). */
bool IsVerboseEnabled();
} // namespace DFLocomotionDebug
