// Source/DungeonForged/Private/Combat/DFCombatDebug.cpp
#include "Combat/DFCombatDebug.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarDF_DebugCombat(
	TEXT("df.DebugCombat"),
	0,
	TEXT("DungeonForged: debug draw for melee combo / heavy / warp / aim.\n")
	TEXT(" 0: Off\n")
	TEXT(" 1: Combo (step, window, montage t/fr, chain blend; Output Log [Combo|Debug])\n")
	TEXT(" 2: Heavy charge + montage\n")
	TEXT(" 4: Motion warp targets (also enable bDrawDebug on ANS_DFMeleeWarp)\n")
	TEXT(" 8: Melee aim cone\n")
	TEXT(" 16: Weapon trail VFX (SpawnTrailVFX notify)\n")
	TEXT(" 32: Enemy melee hit sphere (UDFAbility_Enemy_Melee)\n")
	TEXT(" 63: All"),
	ECVF_Cheat);

int32 DFCombatDebug::GetDebugCombatMask()
{
	return CVarDF_DebugCombat.GetValueOnGameThread();
}

bool DFCombatDebug::IsChannelEnabled(const EChannel Channel)
{
	const int32 Mask = GetDebugCombatMask();
	if (Mask <= 0)
	{
		return false;
	}
	return (Mask & static_cast<int32>(Channel)) != 0;
}
#else
int32 DFCombatDebug::GetDebugCombatMask()
{
	return 0;
}

bool DFCombatDebug::IsChannelEnabled(const EChannel Channel)
{
	(void)Channel;
	return false;
}
#endif
