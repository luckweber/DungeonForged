// Source/DungeonForged/Public/Combat/DFCombatDebug.h
#pragma once

#include "CoreMinimal.h"

/** Shared cheat CVars for combat debug drawing (combo, heavy, warp, aim). */
namespace DFCombatDebug
{
enum class EChannel : uint8
{
	Combo = 1 << 0,
	Heavy = 1 << 1,
	Warp  = 1 << 2,
	Aim   = 1 << 3,
	Trail = 1 << 4,
	EnemyMelee = 1 << 5,
	All   = Combo | Heavy | Warp | Aim | Trail | EnemyMelee,
};

/** Returns mask from df.DebugCombat (0 = off). */
int32 GetDebugCombatMask();

bool IsChannelEnabled(EChannel Channel);
} // namespace DFCombatDebug
