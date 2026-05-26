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

	bool IsValid() const
	{
		return IdleAnimation != nullptr || MovementBlendSpace != nullptr || StrafeBlendSpace != nullptr
			|| JumpSet.IsValid() || JumpStartAnim != nullptr || JumpLoopAnim != nullptr;
	}
};
