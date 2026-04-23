// Source/DungeonForged/Public/Animation/UDFAnimInstance.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/UDFLocomotionTypes.h"
#include "UDFAnimInstance.generated.h"

class ACharacter;
class UAbilitySystemComponent;
class UDFCharacterMovementComponent;
struct FGameplayTag;

UCLASS(Blueprintable)
class DUNGEONFORGED_API UUDFAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	/** Root-motion montage: stash current movement and enter MOVE_Custom (for PhysCustom in project CMC). */
	UFUNCTION(BlueprintCallable, Category = "DF|RootMotion")
	void PushAnimNotifiedCustomMovement();

	/** Restore stashed mode after UDFAnimNotify_DisableRootMotion. */
	UFUNCTION(BlueprintCallable, Category = "DF|RootMotion")
	void PopAnimNotifiedCustomMovement();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DF|Locomotion")
	bool HasTag(const FGameplayTag& Tag) const;

protected:
	/** Yaw change rate of the actor, used to bank lean into turns. */
	UFUNCTION(BlueprintCallable, Category = "DF|Locomotion")
	void CalculateLean(float DeltaTime);

	/** Control rotation relative to mesh / actor: drives AimOffset node. */
	UFUNCTION(BlueprintCallable, Category = "DF|Locomotion")
	void CalculateAimOffsets();

	/** Fills MovementDirection; use bUseEightWay for 8-wedge vs 4-cardinal. */
	UFUNCTION(BlueprintCallable, Category = "DF|Locomotion")
	void DetermineMovementDirection(bool bUseEightWay = true);

	/** Line trace from foot sockets and pelvis: GroundDistance, foot Z offsets. */
	UFUNCTION(BlueprintCallable, Category = "DF|Locomotion")
	void UpdateFootIK(float DeltaTime);

	//~ Cached locomotion
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	TObjectPtr<ACharacter> OwningCharacter;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	TObjectPtr<UDFCharacterMovementComponent> DFCharacterMovement;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	TObjectPtr<UAbilitySystemComponent> OwningAbilitySystem;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	float Speed = 0.f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	float Direction = 0.f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	bool bIsInAir = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	bool bIsDodging = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	bool bIsDead = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	bool bIsInCombat = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	bool bIsLockedOn = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	float LeanAngle = 0.f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Aim")
	float AimPitch = 0.f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Aim")
	float AimYaw = 0.f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|State")
	bool bIsAttacking = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|State")
	bool bIsCasting = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|State")
	bool bIsStunned = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	bool bShouldStrafe = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
	EDFMovementDirection MovementDirection = EDFMovementDirection::Forward;

	/** Traced distance from actor to walkable hit below; useful for landing / air blend. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|IK")
	float GroundDistance = 0.f;

	/** Additive world-space style vertical correction to feed into Two-Bone IK (per foot). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|IK")
	float LeftFootHeightOffsetZ = 0.f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|IK")
	float RightFootHeightOffsetZ = 0.f;

	/** 0-1: trace hit valid. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|IK")
	float LeftFootIKAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|IK")
	float RightFootIKAlpha = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|IK")
	float FootIK_TraceUp = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|IK")
	float FootIK_TraceDown = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|IK")
	float FootIK_SmoothSpeed = 12.f;

	/** Skeletal names for Mannequin-style rigs; change per character if needed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|IK")
	FName LeftFootSocketName = FName(TEXT("foot_l"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|IK")
	FName RightFootSocketName = FName(TEXT("foot_r"));

	/** Yaw change rate to lean mapping scale (deg lean per world yaw rate). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Lean")
	float LeanFromYawRateScale = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Lean")
	float MaxLeanAngleDeg = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Lean")
	float LeanInterpSpeed = 6.f;

private:
	float LastActorYaw = 0.f;
	bool bLastYawInit = false;

	// Root motion notify stash
	bool bStashedForAnimRoot = false;
	TEnumAsByte<EMovementMode> StashedMovementMode;
	uint8 StashedCustomSubMode = 0;
};
