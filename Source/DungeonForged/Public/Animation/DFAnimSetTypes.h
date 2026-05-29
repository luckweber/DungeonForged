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

	bool IsValid() const
	{
		return IdleAnimation != nullptr || MovementBlendSpace != nullptr || StrafeBlendSpace != nullptr
			|| JumpSet.IsValid() || JumpStartAnim != nullptr || JumpLoopAnim != nullptr;
	}
};
