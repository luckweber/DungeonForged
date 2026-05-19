// Source/DungeonForged/Public/Combat/AN/ANS_DFMeleeWarp.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_DFMeleeWarp.generated.h"

class USkeletalMeshComponent;
class UAnimSequenceBase;
struct FAnimNotifyEventReference;

/**
 * AnimNotifyState that marks a Motion Warping window on a melee montage.
 *
 * Author flow:
 *   1. Drop this notify state on the montage section where you want the attacker to rotate/lunge
 *      toward the target (typically: end of windup → start of swing).
 *   2. The AnimGraph of the owning character must contain a `Motion Warping` node *after* the
 *      slot used by the montage, otherwise the warp target is ignored.
 *   3. The montage section must contribute root motion (translation/rotation) for the warp
 *      to actually steer the actor. For "rotation only" enemy windups, set @c bRotationOnly
 *      and animate a small in-place pivot in the montage.
 *
 * Target resolution chain (from @c UDFMeleeAimComponent on the owner):
 *   ManualTarget → Player LockOn → AI Blackboard TargetActor → Soft cone sweep.
 *
 * The warp target is added with @c WarpTargetName so the Motion Warping anim node looks it up.
 * Stop position is @c DesiredStopDistance behind the target (so the swing reach actually lands).
 */
UCLASS(Blueprintable, meta = (DisplayName = "DF Melee Warp Target"))
class DUNGEONFORGED_API UANS_DFMeleeWarp : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UANS_DFMeleeWarp();

	/** Sync point name. Match this to the Motion Warping anim node's @c WarpTargetName. */
	UPROPERTY(EditAnywhere, Category = "DF|Warp")
	FName WarpTargetName = TEXT("MeleeTarget");

	/** Desired stop distance from the target's actor location (cm). Use ~ swing reach. */
	UPROPERTY(EditAnywhere, Category = "DF|Warp", meta = (ClampMin = "0.0"))
	float DesiredStopDistance = 150.f;

	/** If > 0, ignores targets farther than this (no warp). 0 = no cap. */
	UPROPERTY(EditAnywhere, Category = "DF|Warp", meta = (ClampMin = "0.0"))
	float MaxWarpDistance = 800.f;

	/**
	 * If true, the warp target's location is forced to the owner's current location
	 * (only the rotation actually warps). Use this for enemy "face the player" windups
	 * — fixes the "attacking with back to the player" bug — without sliding the enemy.
	 */
	UPROPERTY(EditAnywhere, Category = "DF|Warp")
	bool bRotationOnly = false;

	/** If false (default), keep owner's Z so we don't warp into the floor / ceiling. */
	UPROPERTY(EditAnywhere, Category = "DF|Warp")
	bool bMatchTargetZ = false;

	/** Re-evaluate warp every tick (good for moving targets, e.g. strafing players). */
	UPROPERTY(EditAnywhere, Category = "DF|Warp")
	bool bUpdateEveryTick = true;

	/**
	 * If true, also snap actor yaw on @c NotifyBegin (no easing). This is a safety net for
	 * characters whose AnimGraph doesn't have a Motion Warping node yet. Set false once the
	 * AnimGraph is properly wired (motion warping handles rotation more smoothly).
	 */
	UPROPERTY(EditAnywhere, Category = "DF|Warp")
	bool bSnapYawOnBegin = false;

	/** Debug draw warp target + line to it. */
	UPROPERTY(EditAnywhere, Category = "DF|Warp|Debug")
	bool bDrawDebug = false;

	//~ UAnimNotifyState
	virtual FString GetNotifyName_Implementation() const override;
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

private:
	void UpdateWarpTarget(USkeletalMeshComponent* MeshComp) const;
};
