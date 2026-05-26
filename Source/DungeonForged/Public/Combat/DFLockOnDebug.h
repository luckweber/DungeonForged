// Source/DungeonForged/Public/Combat/DFLockOnDebug.h
#pragma once

#include "CoreMinimal.h"

class UDFLockOnComponent;

namespace DFLockOnDebug
{
bool IsLogEnabled();
bool IsDrawEnabled();

void Log(const TCHAR* Message);
void Logf(const TCHAR* Format, ...);

void DrawLockOnDebug(UDFLockOnComponent* LockOn, const UWorld* World, AActor* Owner);

/** Log + on-screen (mode 2): bShouldStrafe, active BS, Speed/Direction, movement wedge. */
void DumpLocomotionAnimState(const AActor* Owner, bool bForceLog = false);
} // namespace DFLockOnDebug
