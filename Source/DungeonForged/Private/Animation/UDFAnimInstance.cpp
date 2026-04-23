// Source/DungeonForged/Private/Animation/UDFAnimInstance.cpp
#include "Animation/UDFAnimInstance.h"

#include "Animation/UDFLocomotionTypes.h"
#include "Characters/UDFCharacterMovementComponent.h"
#include "GAS/DFGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "KismetAnimationLibrary.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"

void UUDFAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	OwningCharacter = Cast<ACharacter>(GetOwningActor());
	DFCharacterMovement = OwningCharacter ? Cast<UDFCharacterMovementComponent>(OwningCharacter->GetCharacterMovement()) : nullptr;
	if (IAbilitySystemInterface* const IAS = Cast<IAbilitySystemInterface>(OwningCharacter))
	{
		OwningAbilitySystem = IAS->GetAbilitySystemComponent();
	}
	bLastYawInit = false;
}

void UUDFAnimInstance::NativeUpdateAnimation(const float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (!OwningCharacter)
	{
		OwningCharacter = Cast<ACharacter>(GetOwningActor());
	}
	if (!OwningCharacter)
	{
		return;
	}
	if (!DFCharacterMovement)
	{
		DFCharacterMovement = Cast<UDFCharacterMovementComponent>(OwningCharacter->GetCharacterMovement());
	}
	if (DFCharacterMovement)
	{
		Velocity = DFCharacterMovement->Velocity;
	}
	else
	{
		Velocity = OwningCharacter->GetVelocity();
	}
	Speed = Velocity.Size2D();
	if (IAbilitySystemInterface* const IAS = Cast<IAbilitySystemInterface>(OwningCharacter))
	{
		OwningAbilitySystem = IAS->GetAbilitySystemComponent();
	}
	if (Speed > 1.f)
	{
		const FRotator BaseRot(0.f, OwningCharacter->GetActorRotation().Yaw, 0.f);
		Direction = UKismetAnimationLibrary::CalculateDirection(Velocity, BaseRot);
	}
	else
	{
		Direction = 0.f;
	}
	if (DFCharacterMovement)
	{
		bIsInAir = DFCharacterMovement->IsFalling();
		bIsSprinting = DFCharacterMovement->bIsSprinting;
		bIsDodging = DFCharacterMovement->bIsDodging;
	}
	else
	{
		bIsInAir = OwningCharacter->GetCharacterMovement() ? OwningCharacter->GetCharacterMovement()->IsFalling() : false;
		bIsSprinting = false;
		bIsDodging = false;
	}
	if (UAbilitySystemComponent* const ASC = OwningAbilitySystem.Get())
	{
		bIsDead = ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Dead);
		bIsInCombat = ASC->HasMatchingGameplayTag(FDFGameplayTags::State_InCombat);
		bIsLockedOn = ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Targeting);
		bIsAttacking = ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Attacking);
		bIsCasting = ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Casting);
		bIsStunned = ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Stunned);
	}
	else
	{
		bIsDead = false;
		bIsInCombat = false;
		bIsLockedOn = false;
		bIsAttacking = false;
		bIsCasting = false;
		bIsStunned = false;
	}
	bShouldStrafe = bIsLockedOn || bIsInCombat;
	CalculateLean(DeltaSeconds);
	CalculateAimOffsets();
	DetermineMovementDirection(bShouldStrafe);
	UpdateFootIK(DeltaSeconds);
}

void UUDFAnimInstance::NativeThreadSafeUpdateAnimation(const float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
	(void)DeltaSeconds;
}

void UUDFAnimInstance::PushAnimNotifiedCustomMovement()
{
	if (ACharacter* const Ch = OwningCharacter.Get())
	{
		if (UCharacterMovementComponent* CMC = Ch->GetCharacterMovement())
		{
			if (!bStashedForAnimRoot)
			{
				StashedMovementMode = CMC->MovementMode;
				StashedCustomSubMode = CMC->CustomMovementMode;
				bStashedForAnimRoot = true;
			}
			CMC->SetMovementMode(MOVE_Custom, 0);
		}
	}
}

void UUDFAnimInstance::PopAnimNotifiedCustomMovement()
{
	if (ACharacter* const Ch = OwningCharacter.Get())
	{
		if (UCharacterMovementComponent* CMC = Ch->GetCharacterMovement())
		{
			if (bStashedForAnimRoot)
			{
				CMC->SetMovementMode(StashedMovementMode, StashedCustomSubMode);
				bStashedForAnimRoot = false;
			}
			else
			{
				if (CMC->IsFalling() == false)
				{
					CMC->SetMovementMode(MOVE_Walking, 0);
				}
			}
		}
	}
}

bool UUDFAnimInstance::HasTag(const FGameplayTag& Tag) const
{
	if (!Tag.IsValid() || !OwningAbilitySystem)
	{
		return false;
	}
	return OwningAbilitySystem->HasMatchingGameplayTag(Tag);
}

void UUDFAnimInstance::CalculateLean(const float DeltaTime)
{
	if (!OwningCharacter)
	{
		return;
	}
	const float Yaw = OwningCharacter->GetActorRotation().Yaw;
	if (!bLastYawInit)
	{
		LastActorYaw = Yaw;
		bLastYawInit = true;
		return;
	}
	const float YawRate = FMath::FindDeltaAngleDegrees(LastActorYaw, Yaw) / FMath::Max(DeltaTime, 1e-4f);
	LastActorYaw = Yaw;
	const float Target = FMath::Clamp(-YawRate * LeanFromYawRateScale, -MaxLeanAngleDeg, MaxLeanAngleDeg);
	LeanAngle = FMath::FInterpTo(LeanAngle, Target, DeltaTime, LeanInterpSpeed);
}

void UUDFAnimInstance::CalculateAimOffsets()
{
	if (!OwningCharacter)
	{
		AimPitch = 0.f;
		AimYaw = 0.f;
		return;
	}
	const AController* PC = OwningCharacter->GetController();
	const FRotator ControlRot = PC ? PC->GetControlRotation() : OwningCharacter->GetBaseAimRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(ControlRot, OwningCharacter->GetActorRotation());
	AimPitch = FMath::Clamp(Delta.Pitch, -90.f, 90.f);
	AimYaw = FMath::Clamp(Delta.Yaw, -180.f, 180.f);
}

void UUDFAnimInstance::DetermineMovementDirection(const bool bUseEightWay)
{
	if (!bUseEightWay)
	{
		const float A = FMath::Abs(Direction);
		if (A < 45.f)
		{
			MovementDirection = EDFMovementDirection::Forward;
		}
		else if (A > 135.f)
		{
			MovementDirection = EDFMovementDirection::Backward;
		}
		else if (Direction > 0.f)
		{
			MovementDirection = EDFMovementDirection::Right;
		}
		else
		{
			MovementDirection = EDFMovementDirection::Left;
		}
		return;
	}
	const float D = FMath::UnwindDegrees(Direction);
	if (D >= -22.5f && D < 22.5f)
	{
		MovementDirection = EDFMovementDirection::Forward;
	}
	else if (D >= 22.5f && D < 67.5f)
	{
		MovementDirection = EDFMovementDirection::ForwardRight;
	}
	else if (D >= 67.5f && D < 112.5f)
	{
		MovementDirection = EDFMovementDirection::Right;
	}
	else if (D >= 112.5f && D < 157.5f)
	{
		MovementDirection = EDFMovementDirection::BackwardRight;
	}
	else if (D >= 157.5f || D < -157.5f)
	{
		MovementDirection = EDFMovementDirection::Backward;
	}
	else if (D >= -157.5f && D < -112.5f)
	{
		MovementDirection = EDFMovementDirection::BackwardLeft;
	}
	else if (D >= -112.5f && D < -67.5f)
	{
		MovementDirection = EDFMovementDirection::Left;
	}
	else
	{
		MovementDirection = EDFMovementDirection::ForwardLeft;
	}
}

void UUDFAnimInstance::UpdateFootIK(const float DeltaTime)
{
	USkeletalMeshComponent* const Skel = GetSkelMeshComponent();
	ACharacter* const Ch = OwningCharacter.Get();
	if (!Skel || !Ch || !Ch->GetWorld())
	{
		LeftFootIKAlpha = 0.f;
		RightFootIKAlpha = 0.f;
		GroundDistance = 0.f;
		return;
	}
	// Ground line for landing prediction when airborne
	{
		const FVector Start = Ch->GetActorLocation() + FVector(0.f, 0.f, 40.f);
		const FVector End = Start - FVector(0.f, 0.f, 5000.f);
		FHitResult GHit;
		FCollisionQueryParams GParams(SCENE_QUERY_STAT(DF_Foot_IK_Ground), true, Ch);
		if (bIsInAir && Ch->GetWorld()->LineTraceSingleByChannel(GHit, Start, End, ECC_Visibility, GParams) && GHit.bBlockingHit)
		{
			GroundDistance = FMath::Max(0.f, Start.Z - GHit.ImpactPoint.Z);
		}
		else
		{
			GroundDistance = 0.f;
		}
	}
	const auto TraceDownOne = [&](FName SocketName, float& OutZTarget, float& OutAlpha) -> void
	{
		if (SocketName.IsNone() || !Skel->DoesSocketExist(SocketName))
		{
			OutZTarget = 0.f;
			OutAlpha = 0.f;
			return;
		}
		const FVector Foot = Skel->GetSocketLocation(SocketName);
		const FVector Start = Foot + FVector(0.f, 0.f, FootIK_TraceUp);
		const FVector End = Foot - FVector(0.f, 0.f, FootIK_TraceUp + FootIK_TraceDown);
		FHitResult Hit;
		FCollisionQueryParams P(SCENE_QUERY_STAT(DF_Foot_IK), true, Ch);
		P.AddIgnoredActor(Ch);
		if (Ch->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, P) && Hit.bBlockingHit)
		{
			const float DeltaZ = (Hit.ImpactPoint.Z - Foot.Z);
			OutZTarget = FMath::Clamp(DeltaZ, -40.f, 40.f);
			OutAlpha = 1.f;
		}
		else
		{
			OutZTarget = 0.f;
			OutAlpha = 0.f;
		}
	};
	float LTarget = 0.f;
	float RTarget = 0.f;
	float LA = 0.f;
	float RA = 0.f;
	TraceDownOne(LeftFootSocketName, LTarget, LA);
	TraceDownOne(RightFootSocketName, RTarget, RA);
	LeftFootHeightOffsetZ = FMath::FInterpTo(LeftFootHeightOffsetZ, LTarget, DeltaTime, FootIK_SmoothSpeed);
	RightFootHeightOffsetZ = FMath::FInterpTo(RightFootHeightOffsetZ, RTarget, DeltaTime, FootIK_SmoothSpeed);
	LeftFootIKAlpha = FMath::FInterpTo(LeftFootIKAlpha, LA, DeltaTime, FootIK_SmoothSpeed);
	RightFootIKAlpha = FMath::FInterpTo(RightFootIKAlpha, RA, DeltaTime, FootIK_SmoothSpeed);
}
