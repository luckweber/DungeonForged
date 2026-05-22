// Source/DungeonForged/Public/Combat/DFDodgeDebug.h
#pragma once

#include "Combat/DFDodgeTypes.h"

class UAbilitySystemComponent;
class UDFCharacterMovementComponent;
class UWorld;

namespace DFDodgeDebug
{
/** df.DebugDodge mask: 0 off, 1 Output Log, 2 on-screen + world arrows. */
bool IsLogEnabled();
bool IsDrawEnabled();

const TCHAR* DirectionName(EDFDodgeDirection Dir);

void Log(const TCHAR* Message);
void Logf(const TCHAR* Format, ...);

void DrawDodgeArrow(UWorld* World, const FVector& Start, const FVector& DirWorld, float Distance, const FColor& Color,
	float Duration = 0.35f);

void DumpLocalDodgeState(UDFCharacterMovementComponent* CMC, UAbilitySystemComponent* ASC);
} // namespace DFDodgeDebug
