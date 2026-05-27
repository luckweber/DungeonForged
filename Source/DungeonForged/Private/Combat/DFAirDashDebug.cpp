// Source/DungeonForged/Private/Combat/DFAirDashDebug.cpp
#include "Combat/DFAirDashDebug.h"
#include "Combat/DFDodgeDebug.h"

#include "Animation/AnimMontage.h"
#include "Characters/UDFCharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "DungeonForgedModule.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarDF_DebugAirDash(
	TEXT("df.DebugAirDash"),
	0,
	TEXT("DungeonForged air dash debug.\n")
	TEXT(" 0: Off\n")
	TEXT(" 1: Output Log [AirDash|...] (activate, direction, montage, CanActivate blocks)\n")
	TEXT(" 2: Log + on-screen text + planar direction arrow\n")
	TEXT(" 3: Level 2 + per-tick world path dots (actual displacement)"),
	ECVF_Cheat);
#endif

bool DFAirDashDebug::IsLogEnabled()
{
#if !UE_BUILD_SHIPPING
	return CVarDF_DebugAirDash.GetValueOnGameThread() >= 1;
#else
	return false;
#endif
}

bool DFAirDashDebug::IsDrawEnabled()
{
#if !UE_BUILD_SHIPPING
	return CVarDF_DebugAirDash.GetValueOnGameThread() >= 2;
#else
	return false;
#endif
}

bool DFAirDashDebug::IsTraceEnabled()
{
#if !UE_BUILD_SHIPPING
	return CVarDF_DebugAirDash.GetValueOnGameThread() >= 3;
#else
	return false;
#endif
}

const TCHAR* DFAirDashDebug::FinishReasonName(const int32 Reason)
{
	switch (Reason)
	{
	case 0:
		return TEXT("WaitDelay");
	case 1:
		return TEXT("MontageCompleted");
	case 2:
		return TEXT("MontageInterrupted");
	case 3:
		return TEXT("MontageCancelled");
	default:
		return TEXT("Unknown");
	}
}

void DFAirDashDebug::Log(const TCHAR* const Message)
{
	if (!IsLogEnabled() || !Message)
	{
		return;
	}
	UE_LOG(LogDungeonForged, Log, TEXT("[AirDash] %s"), Message);
}

void DFAirDashDebug::Logf(const TCHAR* const Format, ...)
{
	if (!IsLogEnabled() || !Format)
	{
		return;
	}
	va_list Args;
	va_start(Args, Format);
	TCHAR Buffer[1024];
	const TCHAR* Fmt = Format;
	FCString::GetVarArgs(Buffer, UE_ARRAY_COUNT(Buffer), Fmt, Args);
	va_end(Args);
	UE_LOG(LogDungeonForged, Log, TEXT("[AirDash] %s"), Buffer);
}

void DFAirDashDebug::DrawPlanarArrow(UWorld* const World, const FVector& Start, const FVector& DirWorld,
	const float Distance, const FColor& Color, const float Duration)
{
	if (!IsDrawEnabled() || !World)
	{
		return;
	}
	const FVector Dir = DirWorld.GetSafeNormal2D();
	const FVector End = Start + Dir * Distance;
	DrawDebugDirectionalArrow(World, Start, End, 28.f, Color, false, Duration, 0, 2.5f);
	DrawDebugLine(World, Start, Start + FVector(0.f, 0.f, 80.f), FColor::Cyan, false, Duration, 0, 1.f);
}

void DFAirDashDebug::DrawPathPoint(UWorld* const World, const FVector& Location, const FColor& Color,
	const float Duration)
{
	if (!IsTraceEnabled() || !World)
	{
		return;
	}
	DrawDebugSphere(World, Location, 8.f, 8, Color, false, Duration, 0, 1.f);
}

void DFAirDashDebug::DumpCanActivateFail(const UDFCharacterMovementComponent* const CMC, const TCHAR* const Reason)
{
	if (!IsLogEnabled())
	{
		return;
	}
	if (!CMC)
	{
		Logf(TEXT("CanActivate FAIL %s (no CMC)"), Reason ? Reason : TEXT("?"));
		return;
	}
	Logf(TEXT("CanActivate FAIL %s falling=%d airDashUsed=%d vel=%s"),
		Reason ? Reason : TEXT("?"),
		CMC->IsFalling() ? 1 : 0,
		CMC->bAirDodgeUsedThisJump ? 1 : 0,
		*CMC->Velocity.ToCompactString());
}

void DFAirDashDebug::DumpActivate(const ACharacter* const Char, const UDFCharacterMovementComponent* const CMC,
	const EDFDodgeDirection Direction, UAnimMontage* const Montage, const FVector& DirWorld, const float DashDist,
	const float DashDur, const bool bMontageRM, const bool bUseAnimRM, const bool bProgrammatic,
	const bool bLockAltitude)
{
	if (!IsLogEnabled())
	{
		return;
	}
	const FVector Loc = Char ? Char->GetActorLocation() : FVector::ZeroVector;
	Logf(TEXT("Activate dir=%s armedLoc=%s dirWorld=%s dist=%.0f dur=%.2fs montage=%s len=%.2fs"),
		DFDodgeDebug::DirectionName(Direction),
		*Loc.ToCompactString(),
		*DirWorld.ToCompactString(),
		DashDist,
		DashDur,
		Montage ? *Montage->GetName() : TEXT("(none)"),
		Montage ? Montage->GetPlayLength() : 0.f);
	Logf(TEXT("Activate RM montageRM=%d useAnimRM=%d programmatic=%d lockAltitude=%d gravity=%.2f vel=%s"),
		bMontageRM ? 1 : 0,
		bUseAnimRM ? 1 : 0,
		bProgrammatic ? 1 : 0,
		bLockAltitude ? 1 : 0,
		CMC ? CMC->GravityScale : -1.f,
		CMC ? *CMC->Velocity.ToCompactString() : TEXT("(null)"));
}

void DFAirDashDebug::DumpMontageEvent(const TCHAR* const Event, UAnimMontage* const Montage,
	const float MontagePosition, const float MontageLength)
{
	if (!IsLogEnabled())
	{
		return;
	}
	Logf(TEXT("Montage %s name=%s pos=%.2f/%.2f (%.0f%%)"),
		Event ? Event : TEXT("?"),
		Montage ? *Montage->GetName() : TEXT("(none)"),
		MontagePosition,
		MontageLength,
		MontageLength > 0.f ? (100.f * MontagePosition / MontageLength) : 0.f);
}

void DFAirDashDebug::DumpMontageSlot(UAnimMontage* const Montage, const FName ExpectedSlot)
{
	if (!IsLogEnabled())
	{
		return;
	}
	const FName SlotName = DFGetMontagePrimarySlotName(Montage);
	Logf(TEXT("Montage slot='%s' expected='%s' group='%s' %s"),
		*SlotName.ToString(),
		*ExpectedSlot.ToString(),
		Montage ? *Montage->GetGroupName().ToString() : TEXT("(none)"),
		(SlotName == ExpectedSlot) ? TEXT("OK") : TEXT("MISMATCH — set montage slot to match AnimBP"));
	if (SlotName == FName(TEXT("DefaultSlot")) && ExpectedSlot == FName(TEXT("FullBody")))
	{
		Log(TEXT("Tip: move air dash montages to FullBody slot so they override jump locomotion in ABP_JSHeroCharacter"));
	}
}

void DFAirDashDebug::DumpMontagePlayback(UAnimMontage* const Montage, const float MontagePlayReturn,
	const bool bIsPlaying, const float MontagePosition)
{
	if (!IsLogEnabled())
	{
		return;
	}
	Logf(TEXT("Montage_Play ret=%.2f playing=%d pos=%.2f name=%s"),
		MontagePlayReturn,
		bIsPlaying ? 1 : 0,
		MontagePosition,
		Montage ? *Montage->GetName() : TEXT("(none)"));
}

void DFAirDashDebug::DumpFinish(const TCHAR* const Reason, const ACharacter* const Char,
	const UDFCharacterMovementComponent* const CMC, const float MontagePosition, const float MontageLength)
{
	if (!IsLogEnabled())
	{
		return;
	}
	const FVector Loc = Char ? Char->GetActorLocation() : FVector::ZeroVector;
	Logf(TEXT("Finish reason=%s loc=%s montagePos=%.2f/%.2f vel=%s"),
		Reason ? Reason : TEXT("?"),
		*Loc.ToCompactString(),
		MontagePosition,
		MontageLength,
		CMC ? *CMC->Velocity.ToCompactString() : TEXT("(null)"));
}
