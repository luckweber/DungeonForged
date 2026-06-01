// Source/DungeonForged/Private/Animation/DFLocomotionDebug.cpp
#include "Animation/DFLocomotionDebug.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarDF_DebugLocomotion(
	TEXT("df.DebugLocomotion"),
	0,
	TEXT("DungeonForged 8-way Start/Loop/Stop locomotion debug.\n")
	TEXT(" 0: Off\n")
	TEXT(" 1: Output Log [Loco] (gait, dir, transition flags)\n")
	TEXT(" 2: Log + on-screen HUD (gait, dir, transitions, resolved Start/Loop/Stop anims)\n")
	TEXT(" 3: Log + HUD + world direction arrows (blue=facing, green=velocity)\n")
	TEXT(" 4: Level 3 + deep HUD/log (loop anim Distance curve speed, root motion, MaxWalkSpeed, stride scale)"),
	ECVF_Cheat);
#endif

bool DFLocomotionDebug::IsLogEnabled()
{
#if !UE_BUILD_SHIPPING
	return CVarDF_DebugLocomotion.GetValueOnGameThread() >= 1;
#else
	return false;
#endif
}

bool DFLocomotionDebug::IsHudEnabled()
{
#if !UE_BUILD_SHIPPING
	return CVarDF_DebugLocomotion.GetValueOnGameThread() >= 2;
#else
	return false;
#endif
}

bool DFLocomotionDebug::IsDrawEnabled()
{
#if !UE_BUILD_SHIPPING
	return CVarDF_DebugLocomotion.GetValueOnGameThread() >= 3;
#else
	return false;
#endif
}

bool DFLocomotionDebug::IsVerboseEnabled()
{
#if !UE_BUILD_SHIPPING
	return CVarDF_DebugLocomotion.GetValueOnGameThread() >= 4;
#else
	return false;
#endif
}
