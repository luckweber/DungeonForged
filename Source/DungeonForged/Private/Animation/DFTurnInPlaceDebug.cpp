// Source/DungeonForged/Private/Animation/DFTurnInPlaceDebug.cpp
#include "Animation/DFTurnInPlaceDebug.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarDF_DebugTurnInPlace(
	TEXT("df.DebugTurnInPlace"),
	0,
	TEXT("Turn-in-place debug (player only, separate from df.LocomotionDebug).\n")
	TEXT(" 0: Off\n")
	TEXT(" 1: Output Log [TIP]\n")
	TEXT(" 2: Log + on-screen HUD\n")
	TEXT(" 3: Log + HUD + world draw (circle, aim wedge, turn arc)\n")
	TEXT(" 4: Above + compact one-line [TIP|1] (best for sharing logs)"),
	ECVF_Cheat);

static TAutoConsoleVariable<float> CVarDF_TurnDebugCircleRadius(
	TEXT("df.TurnDebug.CircleRadius"),
	85.f,
	TEXT("Ground circle radius (cm) for df.TurnDebug draw mode."),
	ECVF_Cheat);
#endif

bool DFTurnInPlaceDebug::IsLogEnabled()
{
#if !UE_BUILD_SHIPPING
	return CVarDF_DebugTurnInPlace.GetValueOnGameThread() >= 1;
#else
	return false;
#endif
}

bool DFTurnInPlaceDebug::IsHudEnabled()
{
#if !UE_BUILD_SHIPPING
	return CVarDF_DebugTurnInPlace.GetValueOnGameThread() >= 2;
#else
	return false;
#endif
}

bool DFTurnInPlaceDebug::IsDrawEnabled()
{
#if !UE_BUILD_SHIPPING
	return CVarDF_DebugTurnInPlace.GetValueOnGameThread() >= 3;
#else
	return false;
#endif
}

bool DFTurnInPlaceDebug::IsVerboseEnabled()
{
#if !UE_BUILD_SHIPPING
	return CVarDF_DebugTurnInPlace.GetValueOnGameThread() >= 4;
#else
	return false;
#endif
}

float DFTurnInPlaceDebug::GetCircleRadiusCm()
{
#if !UE_BUILD_SHIPPING
	return FMath::Max(20.f, CVarDF_TurnDebugCircleRadius.GetValueOnGameThread());
#else
	return 85.f;
#endif
}
