// Source/DungeonForged/Public/Combat/DFCombatDebug.h
#pragma once

#include "CoreMinimal.h"

class UAnimInstance;
class UAnimMontage;

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

#if !UE_BUILD_SHIPPING
/** Snapshot of montage playback for on-screen / log combo debug. */
struct FMontagePlaybackSample
{
	bool bValid = false;
	FString MontageName;
	float PositionSec = 0.f;
	int32 Frame = 0;
	float FrameRate = 30.f;
	float LengthSec = 0.f;
	float AssetBlendIn = 0.f;
	float AssetBlendOut = 0.f;
};

FMontagePlaybackSample SampleMontagePlayback(UAnimInstance* AnimInstance, UAnimMontage* PreferredMontage = nullptr);
FString FormatMontagePlayback(const FMontagePlaybackSample& Sample);
void LogComboMontageEvent(const TCHAR* Event, UAnimInstance* AnimInstance, UAnimMontage* Montage,
	float RuntimeBlendIn = -1.f);
#endif
} // namespace DFCombatDebug
