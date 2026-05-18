// Source/DungeonForged/Public/GAS/DFGameplayCueRegistration.h
#pragma once

#include "CoreMinimal.h"

namespace DFGameplayCueRegistration
{
/** Registers C++ GameplayCue notifies with the runtime cue set (safe to call multiple times). */
DUNGEONFORGED_API void RegisterNativeGameplayCues();
}
