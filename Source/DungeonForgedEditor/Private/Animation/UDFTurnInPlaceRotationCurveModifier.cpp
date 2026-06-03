// Copyright DungeonForged.

#include "Animation/UDFTurnInPlaceRotationCurveModifier.h"

#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimSequence.h"
#include "AnimationBlueprintLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogDFTurnCurveModifier, Log, All);

namespace DFTurnCurveModifier
{
static bool ParseTurnYawDegreesFromAssetName(const FString& AssetName, float& OutTotalYawDeg)
{
	if (!AssetName.Contains(TEXT("Turn"), ESearchCase::IgnoreCase))
	{
		return false;
	}

	int32 Degrees = 90;
	if (AssetName.Contains(TEXT("180")))
	{
		Degrees = 180;
	}
	else if (AssetName.Contains(TEXT("90")))
	{
		Degrees = 90;
	}
	else
	{
		return false;
	}

	const bool bLeft = AssetName.Contains(TEXT("_L_"), ESearchCase::IgnoreCase)
		|| AssetName.Contains(TEXT("_L_Seq"), ESearchCase::IgnoreCase)
		|| AssetName.EndsWith(TEXT("_L"));
	const bool bRight = AssetName.Contains(TEXT("_R_"), ESearchCase::IgnoreCase)
		|| AssetName.Contains(TEXT("_R_Seq"), ESearchCase::IgnoreCase)
		|| AssetName.EndsWith(TEXT("_R"));

	if (bLeft == bRight)
	{
		return false;
	}

	OutTotalYawDeg = bLeft ? -static_cast<float>(Degrees) : static_cast<float>(Degrees);
	return true;
}

static float SmoothStepDerivative(const float Alpha)
{
	if (Alpha <= 0.f || Alpha >= 1.f)
	{
		return 0.f;
	}
	return 6.f * Alpha * (1.f - Alpha);
}

static void BuildSyntheticYawSpeedKeys(const float PlayLength, const float TotalYawDeg, const bool bEase,
	TArray<float>& OutTimes, TArray<float>& OutValues)
{
	OutTimes.Reset();
	OutValues.Reset();
	if (PlayLength <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const int32 NumKeys = bEase ? 32 : 2;
	for (int32 i = 0; i < NumKeys; ++i)
	{
		const float Alpha = static_cast<float>(i) / static_cast<float>(NumKeys - 1);
		const float T = Alpha * PlayLength;
		float Speed = TotalYawDeg / PlayLength;
		if (bEase)
		{
			Speed = (TotalYawDeg / PlayLength) * SmoothStepDerivative(Alpha);
		}
		OutTimes.Add(T);
		OutValues.Add(Speed);
	}
}
} // namespace DFTurnCurveModifier

void UUDFTurnInPlaceRotationCurveModifier::OnApply_Implementation(UAnimSequence* AnimationSequence)
{
	if (!AnimationSequence || CurveName.IsNone())
	{
		return;
	}

	const float PlayLength = AnimationSequence->GetPlayLength();
	const int32 NumKeys = AnimationSequence->GetNumberOfSampledKeys();
	if (PlayLength <= KINDA_SMALL_NUMBER || NumKeys < 2)
	{
		return;
	}

	const FFrameRate FrameRate = AnimationSequence->GetSamplingFrameRate();
	const float FrameRateDecimal = FrameRate.AsDecimal();
	const bool bForward = AnimationSequence->RateScale >= 0.f;
	const int32 BoneIndexOffset = bForward ? -1 : 0;
	const int32 NextBoneIndexOffset = bForward ? 0 : -1;

	const IAnimationDataModel* const DataModel = AnimationSequence->GetDataModel();
	if (!DataModel)
	{
		return;
	}

	UAnimationBlueprintLibrary::RemoveCurve(AnimationSequence, CurveName, false);
	UAnimationBlueprintLibrary::AddCurve(AnimationSequence, CurveName, ERawCurveTrackTypes::RCT_Float, false);
	UAnimationBlueprintLibrary::AddFloatCurveKey(AnimationSequence, CurveName, 0.f, 0.f);

	float MaxAbsSpeed = 0.f;
	for (int32 i = 1; i < NumKeys; ++i)
	{
		const FTransform RootTransform = DataModel->GetBoneTrackTransform(
			RootBoneName, i + BoneIndexOffset);
		const FTransform NextRootTransform = DataModel->GetBoneTrackTransform(
			RootBoneName, i + NextBoneIndexOffset);

		const float DeltaYaw = FMath::RadiansToDegrees(
			(NextRootTransform.GetRotation() * RootTransform.GetRotation().Inverse()).GetTwistAngle(FVector::UpVector));
		const float YawSpeed = DeltaYaw * FMath::Abs(AnimationSequence->RateScale) * FrameRateDecimal;
		MaxAbsSpeed = FMath::Max(MaxAbsSpeed, FMath::Abs(YawSpeed));

		const float Time = AnimationSequence->GetTimeAtFrame(i);
		UAnimationBlueprintLibrary::AddFloatCurveKey(AnimationSequence, CurveName, Time, YawSpeed);
	}

	if (bFallbackSyntheticYawSpeedIfNoRootMotion && MaxAbsSpeed < 1.f)
	{
		float TotalYawDeg = ManualTotalYawDegrees;
		if (bAutoDetectYawFromAssetName)
		{
			if (!DFTurnCurveModifier::ParseTurnYawDegreesFromAssetName(AnimationSequence->GetName(), TotalYawDeg))
			{
				UE_LOG(LogDFTurnCurveModifier, Warning,
					TEXT("[TIP Modifier] %s: no root yaw on '%s' and name not Turn_90/180_L/R — set ManualTotalYawDegrees."),
					*AnimationSequence->GetName(), *RootBoneName.ToString());
				return;
			}
		}

		UAnimationBlueprintLibrary::RemoveCurve(AnimationSequence, CurveName, false);
		TArray<float> Times;
		TArray<float> Values;
		DFTurnCurveModifier::BuildSyntheticYawSpeedKeys(PlayLength, TotalYawDeg, bEaseInOut, Times, Values);
		if (Times.Num() >= 2)
		{
			UAnimationBlueprintLibrary::AddCurve(AnimationSequence, CurveName, ERawCurveTrackTypes::RCT_Float, false);
			UAnimationBlueprintLibrary::AddFloatCurveKeys(AnimationSequence, CurveName, Times, Values);
		}

		UE_LOG(LogDFTurnCurveModifier, Log,
			TEXT("[TIP Modifier] %s: synthetic %s (no root yaw on '%s') %.0f° / %.2fs"),
			*AnimationSequence->GetName(), *CurveName.ToString(), *RootBoneName.ToString(), TotalYawDeg, PlayLength);
		return;
	}

	UE_LOG(LogDFTurnCurveModifier, Log,
		TEXT("[TIP Modifier] %s: %s from bone '%s' (peak %.0f°/s, %d keys)"),
		*AnimationSequence->GetName(), *CurveName.ToString(), *RootBoneName.ToString(), MaxAbsSpeed, NumKeys);
}

void UUDFTurnInPlaceRotationCurveModifier::OnRevert_Implementation(UAnimSequence* AnimationSequence)
{
	if (!AnimationSequence || CurveName.IsNone())
	{
		return;
	}

	UAnimationBlueprintLibrary::RemoveCurve(AnimationSequence, CurveName, false);
	// Legacy synthetic Rotation curve from older modifier revisions.
	UAnimationBlueprintLibrary::RemoveCurve(AnimationSequence, FName(TEXT("Rotation")), false);
}
