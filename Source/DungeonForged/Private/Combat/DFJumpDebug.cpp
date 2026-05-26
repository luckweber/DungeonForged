// Source/DungeonForged/Private/Combat/DFJumpDebug.cpp
#include "Combat/DFJumpDebug.h"

#include "DungeonForgedModule.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarDF_DebugJump(
	TEXT("df.DebugJump"),
	0,
	TEXT("DungeonForged jump debug.\n")
	TEXT(" 0: Off\n")
	TEXT(" 1: Output Log [Jump|...]\n")
	TEXT(" 2: Log + on-screen HUD (state)\n")
	TEXT(" 3: Log + transition HUD (SM rules + blend hints)\n")
	TEXT(" 4: Deep log (anim lengths, SM elapsed, montage, blends) + level 3"),
	ECVF_Cheat);
#endif

bool DFJumpDebug::IsTransitionHudEnabled()
{
#if !UE_BUILD_SHIPPING
	return CVarDF_DebugJump.GetValueOnGameThread() >= 3;
#else
	return false;
#endif
}

bool DFJumpDebug::IsDeepLogEnabled()
{
#if !UE_BUILD_SHIPPING
	return CVarDF_DebugJump.GetValueOnGameThread() >= 4;
#else
	return false;
#endif
}

bool DFJumpDebug::IsLogEnabled()
{
#if !UE_BUILD_SHIPPING
	return CVarDF_DebugJump.GetValueOnGameThread() >= 1;
#else
	return false;
#endif
}

bool DFJumpDebug::IsHudEnabled()
{
#if !UE_BUILD_SHIPPING
	return CVarDF_DebugJump.GetValueOnGameThread() >= 2;
#else
	return false;
#endif
}

void DFJumpDebug::Log(const TCHAR* const Message)
{
	if (!IsLogEnabled() || !Message)
	{
		return;
	}
	UE_LOG(LogDungeonForged, Log, TEXT("[Jump] %s"), Message);
}

void DFJumpDebug::Logf(const TCHAR* const Format, ...)
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
	UE_LOG(LogDungeonForged, Log, TEXT("[Jump] %s"), Buffer);
}

void DFJumpDebug::LogBlock(const TCHAR* const Header, const FString& Body)
{
	if (!IsDeepLogEnabled())
	{
		return;
	}
	if (Header && *Header)
	{
		Logf(TEXT("[Deep] %s"), Header);
	}
	TArray<FString> Lines;
	Body.ParseIntoArrayLines(Lines);
	for (const FString& Line : Lines)
	{
		if (!Line.IsEmpty())
		{
			UE_LOG(LogDungeonForged, Log, TEXT("[Jump|Deep] %s"), *Line);
		}
	}
}
