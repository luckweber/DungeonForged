// Source/DungeonForged/Public/Animation/DFAnimSetTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/BlendSpace.h"
#include "Animation/UDFLocomotionTypes.h"
#include "DFAnimSetTypes.generated.h"

/**
 * Per-direction jump animations (5 starts + 1 loop + 5 lands).
 * Direction is snapped to cardinals at takeoff (see UUDFAnimInstance::LastJumpDirection).
 */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FUDJumpAnimSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Start")
	TObjectPtr<UAnimSequenceBase> Start_Idle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Start")
	TObjectPtr<UAnimSequenceBase> Start_Forward;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Start")
	TObjectPtr<UAnimSequenceBase> Start_Backward;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Start")
	TObjectPtr<UAnimSequenceBase> Start_Left;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Start")
	TObjectPtr<UAnimSequenceBase> Start_Right;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Loop")
	TObjectPtr<UAnimSequenceBase> Loop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Land")
	TObjectPtr<UAnimSequenceBase> Land_Idle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Land")
	TObjectPtr<UAnimSequenceBase> Land_Forward;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Land")
	TObjectPtr<UAnimSequenceBase> Land_Backward;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Land")
	TObjectPtr<UAnimSequenceBase> Land_Left;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Land")
	TObjectPtr<UAnimSequenceBase> Land_Right;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Double")
	TObjectPtr<UAnimSequenceBase> DoubleJump_Start;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Double")
	TObjectPtr<UAnimSequenceBase> DoubleJump_Loop;

	UAnimSequenceBase* ResolveStart(const EDFMovementDirection Dir) const;
	UAnimSequenceBase* ResolveLand(const EDFMovementDirection Dir) const;

	bool IsValid() const
	{
		return Start_Idle || Start_Forward || Start_Backward || Start_Left || Start_Right || Loop;
	}
};

/**
 * Per-direction Start / Loop / Stop bundle for a single gait (walk or run).
 * Fill the 8 directions you have authored; the resolver falls back through
 * cardinals → Forward → null automatically.
 */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FUDLocomotionAnimSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Start")
	TObjectPtr<UAnimSequenceBase> Start_F;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Start")
	TObjectPtr<UAnimSequenceBase> Start_FR_45;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Start")
	TObjectPtr<UAnimSequenceBase> Start_R_90;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Start")
	TObjectPtr<UAnimSequenceBase> Start_BR_135;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Start")
	TObjectPtr<UAnimSequenceBase> Start_B_180;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Start")
	TObjectPtr<UAnimSequenceBase> Start_BL_135;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Start")
	TObjectPtr<UAnimSequenceBase> Start_L_90;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Start")
	TObjectPtr<UAnimSequenceBase> Start_FL_45;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Loop")
	TObjectPtr<UAnimSequenceBase> Loop_F;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Loop")
	TObjectPtr<UAnimSequenceBase> Loop_FR_45;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Loop")
	TObjectPtr<UAnimSequenceBase> Loop_R_90;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Loop")
	TObjectPtr<UAnimSequenceBase> Loop_BR_135;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Loop")
	TObjectPtr<UAnimSequenceBase> Loop_B_180;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Loop")
	TObjectPtr<UAnimSequenceBase> Loop_BL_135;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Loop")
	TObjectPtr<UAnimSequenceBase> Loop_L_90;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Loop")
	TObjectPtr<UAnimSequenceBase> Loop_FL_45;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Stop")
	TObjectPtr<UAnimSequenceBase> Stop_F;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Stop")
	TObjectPtr<UAnimSequenceBase> Stop_FR_45;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Stop")
	TObjectPtr<UAnimSequenceBase> Stop_R_90;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Stop")
	TObjectPtr<UAnimSequenceBase> Stop_BR_135;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Stop")
	TObjectPtr<UAnimSequenceBase> Stop_B_180;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Stop")
	TObjectPtr<UAnimSequenceBase> Stop_BL_135;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Stop")
	TObjectPtr<UAnimSequenceBase> Stop_L_90;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loco|Stop")
	TObjectPtr<UAnimSequenceBase> Stop_FL_45;

	UAnimSequenceBase* ResolveStart(EDFMovementDirection Dir) const;
	UAnimSequenceBase* ResolveLoop(EDFMovementDirection Dir) const;
	UAnimSequenceBase* ResolveStop(EDFMovementDirection Dir) const;

	bool IsValid() const
	{
		return Start_F || Loop_F || Stop_F
			|| Start_B_180 || Loop_B_180 || Stop_B_180;
	}
};

/**
 * Turn-in-place clips (90° / 180° left & right). Idle loop uses FUDAnimSet::IdleAnimation
 * (which Anim Set / linked layer is active defines unarmed vs armed idle).
 */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FUDTurnInPlaceAnimSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn|90")
	TObjectPtr<UAnimSequenceBase> Turn_90_L;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn|90")
	TObjectPtr<UAnimSequenceBase> Turn_90_R;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn|180")
	TObjectPtr<UAnimSequenceBase> Turn_180_L;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn|180")
	TObjectPtr<UAnimSequenceBase> Turn_180_R;

	/** bTurnRight: true = Turn_*_R, false = Turn_*_L. bUse180: pick 180 vs 90 set. */
	UAnimSequenceBase* ResolveTurn(const bool bUse180, const bool bTurnRight) const;

	bool IsValid() const
	{
		return Turn_90_L || Turn_90_R || Turn_180_L || Turn_180_R;
	}
};

/**
 * Animation bundle for idle / locomotion / jump driven by AnimGraph.
 */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FUDAnimSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Idle")
	TObjectPtr<UAnimSequenceBase> IdleAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Locomotion")
	TObjectPtr<UBlendSpace> MovementBlendSpace;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Locomotion")
	TObjectPtr<UBlendSpace> StrafeBlendSpace;

	/** Optional: per-direction Start/Loop/Stop for walk. Falls back to MovementBlendSpace if empty. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Locomotion|Directional")
	FUDLocomotionAnimSet WalkSet;

	/** Optional: per-direction Start/Loop/Stop for run. Falls back to MovementBlendSpace if empty. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Locomotion|Directional")
	FUDLocomotionAnimSet RunSet;

	/** Turn-in-place clips only (09_Turn). Idle = IdleAnimation above. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Turn")
	FUDTurnInPlaceAnimSet TurnSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Jump")
	FUDJumpAnimSet JumpSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Jump|Legacy")
	TObjectPtr<UAnimSequenceBase> JumpStartAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Jump|Legacy")
	TObjectPtr<UAnimSequenceBase> JumpLoopAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Jump|Legacy")
	TObjectPtr<UAnimSequenceBase> JumpLandAnim;

	UBlendSpace* ResolveLocomotionBS(const bool bStrafe) const
	{
		if (bStrafe && StrafeBlendSpace)
		{
			return StrafeBlendSpace;
		}
		return MovementBlendSpace;
	}

	UAnimSequenceBase* ResolveJumpStart(const EDFMovementDirection Dir) const;
	UAnimSequenceBase* ResolveJumpLand(const EDFMovementDirection Dir) const;
	UAnimSequenceBase* ResolveJumpLoop() const;
	UAnimSequenceBase* ResolveJumpDoubleStart() const;
	UAnimSequenceBase* ResolveJumpDoubleLoop() const;

	UAnimSequenceBase* ResolveLocomotionStart(EDFGait Gait, EDFMovementDirection Dir) const;
	UAnimSequenceBase* ResolveLocomotionLoop(EDFGait Gait, EDFMovementDirection Dir) const;
	UAnimSequenceBase* ResolveLocomotionStop(EDFGait Gait, EDFMovementDirection Dir) const;
	UAnimSequenceBase* ResolveLocomotionIdle() const;
	UAnimSequenceBase* ResolveLocomotionTurn(const bool bUse180, const bool bTurnRight) const;

	/** Copy any missing Turn_* slots from Source (layer inherits main Turn Set). */
	void MergeTurnSetFrom(const FUDTurnInPlaceAnimSet& Source);

	/**
	 * When Turn Set is empty but IdleAnimation is set, try loading Turn_90/180_* from
	 * sibling folders (09_Turn/01_Turn next to the idle package path).
	 */
	bool TryAutoFillTurnSetFromIdlePackagePaths();

	/** Load Turn_* from standard Sword_and_Shield / Fab paths when idle-relative lookup fails. */
	bool TryAutoFillTurnSetFromKnownContentPaths();

	bool IsValid() const
	{
		return IdleAnimation != nullptr || MovementBlendSpace != nullptr || StrafeBlendSpace != nullptr
			|| JumpSet.IsValid() || JumpStartAnim != nullptr || JumpLoopAnim != nullptr;
	}
};
