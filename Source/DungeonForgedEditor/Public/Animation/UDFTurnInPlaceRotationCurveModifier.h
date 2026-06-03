// Copyright DungeonForged.

#pragma once

#include "AnimationModifier.h"
#include "UDFTurnInPlaceRotationCurveModifier.generated.h"

/**
 * Editor-only: bakes ALS-style RotationYawSpeed on Turn_* sequences (root bone twist rate).
 * Runtime reads the curve via GetCurveValue in PhysicsRotation — not a synthetic 0→180 Rotation curve.
 */
UCLASS(Blueprintable, meta = (DisplayName = "DF Turn In Place Rotation Curve"))
class DUNGEONFORGEDEDITOR_API UUDFTurnInPlaceRotationCurveModifier : public UAnimationModifier
{
	GENERATED_BODY()

public:
	/** Must match TurnInPlaceRotationCurveName on UUDFAnimInstance (empty = RotationYawSpeed). */
	UPROPERTY(EditAnywhere, Category = "Turn In Place")
	FName CurveName = FName(TEXT("RotationYawSpeed"));

	/** Bone used to derive yaw speed (ALS uses skeleton root). */
	UPROPERTY(EditAnywhere, Category = "Turn In Place")
	FName RootBoneName = FName(TEXT("root"));

	/** If root track has no yaw, bake speed from total turn angle / clip length (parsed from asset name). */
	UPROPERTY(EditAnywhere, Category = "Turn In Place")
	bool bFallbackSyntheticYawSpeedIfNoRootMotion = true;

	UPROPERTY(EditAnywhere, Category = "Turn In Place")
	bool bEaseInOut = true;

	UPROPERTY(EditAnywhere, Category = "Turn In Place")
	bool bAutoDetectYawFromAssetName = true;

	UPROPERTY(EditAnywhere, Category = "Turn In Place", meta = (EditCondition = "!bAutoDetectYawFromAssetName"))
	float ManualTotalYawDegrees = 90.f;

	virtual void OnApply_Implementation(UAnimSequence* AnimationSequence) override;
	virtual void OnRevert_Implementation(UAnimSequence* AnimationSequence) override;

protected:
	virtual int32 GetNativeClassRevision() const override { return 2; }
};
