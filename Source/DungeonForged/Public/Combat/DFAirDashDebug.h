// Source/DungeonForged/Public/Combat/DFAirDashDebug.h
#pragma once

#include "Combat/DFDodgeTypes.h"

class ACharacter;
class UAnimMontage;
class UDFCharacterMovementComponent;
class UWorld;

namespace DFAirDashDebug
{
/** df.DebugAirDash mask: 0 off, 1 log, 2 log+HUD+arrows, 3 + per-tick path trace. */
bool IsLogEnabled();
bool IsDrawEnabled();
bool IsTraceEnabled();

const TCHAR* FinishReasonName(int32 Reason);

void Log(const TCHAR* Message);
void Logf(const TCHAR* Format, ...);

void DrawPlanarArrow(UWorld* World, const FVector& Start, const FVector& DirWorld, float Distance,
	const FColor& Color, float Duration);

void DrawPathPoint(UWorld* World, const FVector& Location, const FColor& Color, float Duration = 0.15f);

void DumpCanActivateFail(const UDFCharacterMovementComponent* CMC, const TCHAR* Reason);

void DumpActivate(const ACharacter* Char, const UDFCharacterMovementComponent* CMC, EDFDodgeDirection Direction,
	UAnimMontage* Montage, const FVector& DirWorld, float DashDist, float DashDur, bool bMontageRM, bool bUseAnimRM,
	bool bProgrammatic, bool bLockAltitude);

void DumpMontageEvent(const TCHAR* Event, UAnimMontage* Montage, float MontagePosition, float MontageLength);

void DumpMontageSlot(UAnimMontage* Montage, FName ExpectedSlot);

void DumpMontagePlayback(UAnimMontage* Montage, float MontagePlayReturn, bool bIsPlaying, float MontagePosition);

void DumpFinish(const TCHAR* Reason, const ACharacter* Char, const UDFCharacterMovementComponent* CMC,
	float MontagePosition, float MontageLength);
} // namespace DFAirDashDebug
