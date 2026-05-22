// Source/DungeonForged/Public/Animation/DFAnimSetTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/BlendSpace.h"
#include "DFAnimSetTypes.generated.h"

/**
 * Animation bundle for idle / locomotion / jump driven by AnimGraph.
 *
 * Locomotion split:
 *   MovementBlendSpace  — 1-D (axis: Speed).
 *                         Used when bShouldStrafe == false (exploration / orient-to-movement).
 *                         Samples: Idle → Walk → Run → Sprint, all facing forward.
 *
 *   StrafeBlendSpace    — 2-D (axis X: Speed  0→max, axis Y: Direction  -180→180).
 *                         Used when bShouldStrafe == true  (combat / lock-on).
 *                         Needs 8-way samples at Direction = 0 / ±45 / ±90 / ±135 / 180.
 *                         If null, AnimBP falls back to MovementBlendSpace.
 *
 * AnimGraph usage (Break ActiveAnimSet node):
 *   if bShouldStrafe && StrafeBlendSpace != null
 *       BlendSpacePlayer(StrafeBlendSpace, X=Speed, Y=Direction)
 *   else
 *       BlendSpacePlayer(MovementBlendSpace, X=Speed)
 */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FUDAnimSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Idle")
	TObjectPtr<UAnimSequenceBase> IdleAnimation;

	/**
	 * Forward-facing locomotion: 1-D BlendSpace, axis = Speed (0 → MaxRunSpeed or higher).
	 * Samples: Idle (0) → Walk (~150) → Run (~540) → Sprint (~750).
	 * Used when NOT strafing (exploration, bShouldStrafe == false).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Locomotion")
	TObjectPtr<UBlendSpace> MovementBlendSpace;

	/**
	 * 8-way strafe locomotion: 2-D BlendSpace.
	 *   Axis X: Speed  (0 → MaxRunSpeed).
	 *   Axis Y: Direction  (-180 → 180, degrees relative to actor forward).
	 *
	 * Required samples (minimum viable — 4-way):
	 *   Direction  0°  → Forward  walk/run
	 *   Direction  90° → Strafe Right
	 *   Direction -90° → Strafe Left
	 *   Direction 180° → Backward walk
	 *
	 * Full 8-way adds ±45° (ForwardRight/Left) and ±135° (BackwardRight/Left).
	 *
	 * If null, AnimBP falls back to MovementBlendSpace for all movement.
	 * Used when bShouldStrafe == true  (combat proximity or lock-on active).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Locomotion")
	TObjectPtr<UBlendSpace> StrafeBlendSpace;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Jump")
	TObjectPtr<UAnimSequenceBase> JumpStartAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Jump")
	TObjectPtr<UAnimSequenceBase> JumpLoopAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Jump")
	TObjectPtr<UAnimSequenceBase> JumpLandAnim;

	/**
	 * Returns the correct locomotion blendspace for the current movement mode.
	 * @param bStrafe  true when in combat / lock-on (bShouldStrafe from AnimInstance).
	 */
	UBlendSpace* ResolveLocomotionBS(const bool bStrafe) const
	{
		if (bStrafe && StrafeBlendSpace)
		{
			return StrafeBlendSpace;
		}
		return MovementBlendSpace;
	}

	/** Idle or locomotion BS is enough to treat the set as usable. */
	bool IsValid() const
	{
		return IdleAnimation != nullptr || MovementBlendSpace != nullptr || StrafeBlendSpace != nullptr;
	}
};
