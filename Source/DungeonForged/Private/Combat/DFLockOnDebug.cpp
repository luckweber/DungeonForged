// Source/DungeonForged/Private/Combat/DFLockOnDebug.cpp
#include "Combat/DFLockOnDebug.h"

#include "Animation/UDFAnimInstance.h"
#include "Camera/UDFLockOnComponent.h"
#include "DungeonForgedModule.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarDF_DebugLockOn(
	TEXT("df.DebugLockOn"),
	0,
	TEXT("DungeonForged lock-on debug.\n")
	TEXT(" 0: Off\n")
	TEXT(" 1: Output Log [LockOn|...]\n")
	TEXT(" 2: World draw + on-screen locomotion BS (bShouldStrafe, active BS, Speed, Direction)"),
	ECVF_Cheat);

constexpr int32 GLockOnAnimScreenKey = 88001;
#endif

bool DFLockOnDebug::IsLogEnabled()
{
#if !UE_BUILD_SHIPPING
	return CVarDF_DebugLockOn.GetValueOnGameThread() >= 1;
#else
	return false;
#endif
}

bool DFLockOnDebug::IsDrawEnabled()
{
#if !UE_BUILD_SHIPPING
	return CVarDF_DebugLockOn.GetValueOnGameThread() >= 2;
#else
	return false;
#endif
}

void DFLockOnDebug::Log(const TCHAR* const Message)
{
	if (!IsLogEnabled() || !Message)
	{
		return;
	}
	UE_LOG(LogDungeonForged, Log, TEXT("[LockOn] %s"), Message);
}

void DFLockOnDebug::Logf(const TCHAR* const Format, ...)
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
	UE_LOG(LogDungeonForged, Log, TEXT("[LockOn] %s"), Buffer);
}

void DFLockOnDebug::DumpLocomotionAnimState(const AActor* const Owner, const bool bForceLog)
{
#if !UE_BUILD_SHIPPING
	if (!Owner || (!IsDrawEnabled() && !IsLogEnabled() && !bForceLog))
	{
		return;
	}

	FString Text = TEXT("[LockOn|Anim] ");
	const ACharacter* const Char = Cast<ACharacter>(Owner);
	const USkeletalMeshComponent* const Skel = Char ? Char->GetMesh() : nullptr;
	const UUDFAnimInstance* const Anim = Skel ? Cast<UUDFAnimInstance>(Skel->GetAnimInstance()) : nullptr;
	if (!Anim)
	{
		Text += TEXT("no UUDFAnimInstance on mesh");
	}
	else
	{
		Text += Anim->BuildLocomotionDebugString();
	}

	if (bForceLog || IsLogEnabled())
	{
		UE_LOG(LogDungeonForged, Log, TEXT("%s"), *Text);
	}
	if (IsDrawEnabled() && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(GLockOnAnimScreenKey, 0.f, FColor::Cyan, Text);
	}
#else
	(void)Owner;
	(void)bForceLog;
#endif
}

void DFLockOnDebug::DrawLockOnDebug(UDFLockOnComponent* const LockOn, const UWorld* const World, AActor* const Owner)
{
#if !UE_BUILD_SHIPPING
	if (!IsDrawEnabled() || !LockOn || !World || !Owner)
	{
		return;
	}
	const FVector Origin = Owner->GetActorLocation();
	const bool bLocked = LockOn->IsLockedOn();
	const float Range = LockOn->GetLockOnRange();
	const float ConeAngle = LockOn->GetLockOnAngle();
	DrawDebugSphere(World, Origin, Range, 24, bLocked ? FColor::Green : FColor::Yellow, false, -1.f, 0, 1.f);
	const float HalfConeRad = FMath::DegreesToRadians(ConeAngle * 0.5f);
	const FVector ConeEnd = Origin + Owner->GetActorForwardVector() * Range * FMath::Cos(HalfConeRad);
	DrawDebugDirectionalArrow(World, Origin, ConeEnd, 20.f, FColor::Orange, false, -1.f, 0, 2.f);
	if (bLocked)
	{
		if (AActor* const Target = LockOn->GetCurrentTarget())
		{
			DrawDebugLine(World, Origin, Target->GetActorLocation(), FColor::Red, false, -1.f, 0, 3.f);
		}
	}
	DumpLocomotionAnimState(Owner, false);
#else
	(void)LockOn;
	(void)World;
	(void)Owner;
#endif
}
