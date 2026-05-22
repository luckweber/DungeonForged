// Source/DungeonForged/Private/Combat/DFDodgeDebug.cpp
#include "Combat/DFDodgeDebug.h"

#include "AbilitySystemComponent.h"
#include "Characters/UDFCharacterMovementComponent.h"
#include "DungeonForgedModule.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarDF_DebugDodge(
	TEXT("df.DebugDodge"),
	0,
	TEXT("DungeonForged dodge debug.\n")
	TEXT(" 0: Off\n")
	TEXT(" 1: Output Log [Dodge|...] (activate, direction, montage, CMC blocks)\n")
	TEXT(" 2: Log + on-screen text + world direction arrow"),
	ECVF_Cheat);
#endif

bool DFDodgeDebug::IsLogEnabled()
{
#if !UE_BUILD_SHIPPING
	return CVarDF_DebugDodge.GetValueOnGameThread() >= 1;
#else
	return false;
#endif
}

bool DFDodgeDebug::IsDrawEnabled()
{
#if !UE_BUILD_SHIPPING
	return CVarDF_DebugDodge.GetValueOnGameThread() >= 2;
#else
	return false;
#endif
}

const TCHAR* DFDodgeDebug::DirectionName(const EDFDodgeDirection Dir)
{
	switch (Dir)
	{
	case EDFDodgeDirection::Forward:
		return TEXT("Forward");
	case EDFDodgeDirection::ForwardRight:
		return TEXT("ForwardRight");
	case EDFDodgeDirection::Right:
		return TEXT("Right");
	case EDFDodgeDirection::BackwardRight:
		return TEXT("BackwardRight");
	case EDFDodgeDirection::Backward:
		return TEXT("Backward");
	case EDFDodgeDirection::BackwardLeft:
		return TEXT("BackwardLeft");
	case EDFDodgeDirection::Left:
		return TEXT("Left");
	case EDFDodgeDirection::ForwardLeft:
		return TEXT("ForwardLeft");
	default:
		return TEXT("Unknown");
	}
}

void DFDodgeDebug::Log(const TCHAR* const Message)
{
	if (!IsLogEnabled() || !Message)
	{
		return;
	}
	UE_LOG(LogDungeonForged, Log, TEXT("%s"), Message);
}

void DFDodgeDebug::Logf(const TCHAR* const Format, ...)
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
	UE_LOG(LogDungeonForged, Log, TEXT("[Dodge] %s"), Buffer);
}

void DFDodgeDebug::DrawDodgeArrow(UWorld* const World, const FVector& Start, const FVector& DirWorld,
	const float Distance, const FColor& Color, const float Duration)
{
	if (!IsDrawEnabled() || !World)
	{
		return;
	}
	const FVector Dir = DirWorld.GetSafeNormal();
	const FVector End = Start + Dir * Distance;
	DrawDebugDirectionalArrow(World, Start, End, 24.f, Color, false, Duration, 0, 2.f);
}

void DFDodgeDebug::DumpLocalDodgeState(UDFCharacterMovementComponent* const CMC, UAbilitySystemComponent* const ASC)
{
	if (!CMC)
	{
		Log(TEXT("[Dodge|Dump] no UDFCharacterMovementComponent"));
		return;
	}
	const float CdRem = CMC->GetDodgeCooldownRemaining();
	Logf(TEXT("[Dodge|Dump] dir=%s bDodging=%d cdRem=%.2f dist=%.0f dur=%.2f iframe=%.2f"),
		DirectionName(CMC->LastDodgeDirection),
		CMC->bIsDodging ? 1 : 0,
		CdRem,
		CMC->DodgeDistance,
		CMC->DodgeDuration,
		CMC->IFrameDuration);
	if (ASC)
	{
		Logf(TEXT("[Dodge|Dump] tags dodging=%d invuln=%d exhausted=%d stamina=%.1f"),
			ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Dodging) ? 1 : 0,
			ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Invulnerable) ? 1 : 0,
			ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Exhausted) ? 1 : 0,
			ASC->GetNumericAttribute(UDFAttributeSet::GetStaminaAttribute()));
	}
}
