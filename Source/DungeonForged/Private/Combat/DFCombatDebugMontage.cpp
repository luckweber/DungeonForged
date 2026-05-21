// Source/DungeonForged/Private/Combat/DFCombatDebugMontage.cpp
#include "Combat/DFCombatDebug.h"

#if !UE_BUILD_SHIPPING

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "DungeonForgedModule.h"

namespace
{
UAnimMontage* ResolveMontageToSample(UAnimInstance* const AnimInstance, UAnimMontage* const PreferredMontage)
{
	if (!AnimInstance)
	{
		return nullptr;
	}
	if (PreferredMontage && AnimInstance->Montage_IsActive(PreferredMontage))
	{
		return PreferredMontage;
	}
	return AnimInstance->GetCurrentActiveMontage();
}
} // namespace

DFCombatDebug::FMontagePlaybackSample DFCombatDebug::SampleMontagePlayback(
	UAnimInstance* const AnimInstance, UAnimMontage* const PreferredMontage)
{
	FMontagePlaybackSample Out;
	UAnimMontage* const M = ResolveMontageToSample(AnimInstance, PreferredMontage);
	if (!AnimInstance || !M)
	{
		return Out;
	}

	Out.bValid = true;
	Out.MontageName = M->GetName();
	Out.PositionSec = AnimInstance->Montage_GetPosition(M);
	Out.FrameRate = static_cast<float>(M->GetSamplingFrameRate().AsDecimal());
	if (Out.FrameRate <= KINDA_SMALL_NUMBER)
	{
		Out.FrameRate = 30.f;
	}
	Out.Frame = FMath::Max(0, FMath::RoundToInt(Out.PositionSec * Out.FrameRate));
	Out.LengthSec = M->GetPlayLength();
	Out.AssetBlendIn = M->BlendIn.GetBlendTime();
	Out.AssetBlendOut = M->BlendOut.GetBlendTime();
	return Out;
}

FString DFCombatDebug::FormatMontagePlayback(const FMontagePlaybackSample& Sample)
{
	if (!Sample.bValid)
	{
		return TEXT("montage: (none)");
	}
	const int32 TotalFrames = FMath::Max(1, FMath::RoundToInt(Sample.LengthSec * Sample.FrameRate));
	return FString::Printf(
		TEXT("%s t=%.3fs fr=%d/%d (%.0ffps) assetBlendIn=%.2f assetBlendOut=%.2f"),
		*Sample.MontageName,
		Sample.PositionSec,
		Sample.Frame,
		TotalFrames,
		Sample.FrameRate,
		Sample.AssetBlendIn,
		Sample.AssetBlendOut);
}

void DFCombatDebug::LogComboMontageEvent(const TCHAR* const Event, UAnimInstance* const AnimInstance,
	UAnimMontage* const Montage, const float RuntimeBlendIn)
{
	if (!IsChannelEnabled(EChannel::Combo))
	{
		return;
	}
	const FMontagePlaybackSample Sample = SampleMontagePlayback(AnimInstance, Montage);
	if (RuntimeBlendIn >= 0.f)
	{
		UE_LOG(LogDungeonForged, Log, TEXT("[Combo|Debug] %s | %s | runtimeBlendIn=%.3f"),
			Event, *FormatMontagePlayback(Sample), RuntimeBlendIn);
	}
	else
	{
		UE_LOG(LogDungeonForged, Log, TEXT("[Combo|Debug] %s | %s"), Event, *FormatMontagePlayback(Sample));
	}
}

#endif // !UE_BUILD_SHIPPING
