// Source/DungeonForged/Private/Animation/UDFAnimInstance.cpp
#include "Animation/UDFAnimInstance.h"

#include "Animation/UDFLocomotionTypes.h"
#include "Characters/ADFEnemyBase.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Characters/UDFCharacterMovementComponent.h"
#include "Data/DFDataTableStructs.h"
#include "Equipment/DFEquipmentTypes.h"
#include "Equipment/UDFEquipmentComponent.h"
#include "GAS/DFGameplayTags.h"
#include "Animation/AnimInstance.h"
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

	ActiveAnimSet = DefaultAnimSet;
}

void UUDFAnimInstance::ApplyAnimSet(const FUDAnimSet& NewAnimSet)
{
	if (NewAnimSet.IsValid())
	{
		ActiveAnimSet = NewAnimSet;
	}
}

void UUDFAnimInstance::RevertToDefaultAnimSet()
{
	ActiveAnimSet = DefaultAnimSet;
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
		LastDodgeDirection = DFCharacterMovement->LastDodgeDirection;
	}
	else
	{
		bIsInAir = OwningCharacter->GetCharacterMovement() ? OwningCharacter->GetCharacterMovement()->IsFalling() : false;
		bIsSprinting = false;
		bIsDodging = false;
		LastDodgeDirection = EDFDodgeDirection::Backward;
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
	if (bIsDead)
	{
		Speed = 0.f;
		Velocity = FVector::ZeroVector;
		bShouldStrafe = false;
		return;
	}
	bShouldStrafe = !bIsDead && (bIsLockedOn || bIsInCombat);
	CalculateLean(DeltaSeconds);
	CalculateAimOffsets();
	DetermineMovementDirection(bShouldStrafe);
	UpdateFootIK(DeltaSeconds);
	SyncEquippedWeaponAnimLayerFromOwner();
}

void UUDFAnimInstance::SyncEquippedWeaponAnimLayerFromOwner()
{
	bHasWeaponEquipped = false;
	EquippedWeaponItemRow = NAME_None;
	TSubclassOf<UAnimInstance> DesiredLayer;

	if (ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(OwningCharacter.Get()))
	{
		if (UDFEquipmentComponent* Eq = PC->Equipment; Eq && !Eq->IsSlotEmpty(EEquipmentSlot::Weapon))
		{
			bHasWeaponEquipped = true;
			EquippedWeaponItemRow = Eq->EquippedItems.FindRef(EEquipmentSlot::Weapon);

			FDFItemTableRow Row;
			if (Eq->TryGetEquippedItemData(EEquipmentSlot::Weapon, Row))
			{
				DesiredLayer = Row.WeaponLinkedAnimLayerClass;
			}
		}
	}

	if (DesiredLayer == CachedLinkedWeaponLayerClass)
	{
		return;
	}

	if (CachedLinkedWeaponLayerClass)
	{
		UnlinkAnimClassLayers(CachedLinkedWeaponLayerClass);
		CachedLinkedWeaponLayerClass = nullptr;
	}

	if (DesiredLayer)
	{
		LinkAnimClassLayers(DesiredLayer);
		CachedLinkedWeaponLayerClass = DesiredLayer;
	}
}

void UUDFAnimInstance::LinkWeaponAnimLayerClass(TSubclassOf<UAnimInstance> AnimLayerClass)
{
	if (!AnimLayerClass)
	{
		UnlinkWeaponAnimLayerClass();
		return;
	}
	if (AnimLayerClass == CachedLinkedWeaponLayerClass)
	{
		return;
	}
	if (CachedLinkedWeaponLayerClass)
	{
		UnlinkAnimClassLayers(CachedLinkedWeaponLayerClass);
		CachedLinkedWeaponLayerClass = nullptr;
	}
	LinkAnimClassLayers(AnimLayerClass);
	CachedLinkedWeaponLayerClass = AnimLayerClass;
}

void UUDFAnimInstance::UnlinkWeaponAnimLayerClass()
{
	if (!CachedLinkedWeaponLayerClass)
	{
		return;
	}
	UnlinkAnimClassLayers(CachedLinkedWeaponLayerClass);
	CachedLinkedWeaponLayerClass = nullptr;
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

#if !UE_BUILD_SHIPPING
namespace
{
const TCHAR* LocomotionDirName(const EDFMovementDirection Dir)
{
	switch (Dir)
	{
	case EDFMovementDirection::Forward:
		return TEXT("Forward");
	case EDFMovementDirection::ForwardRight:
		return TEXT("ForwardRight");
	case EDFMovementDirection::Right:
		return TEXT("Right");
	case EDFMovementDirection::BackwardRight:
		return TEXT("BackwardRight");
	case EDFMovementDirection::Backward:
		return TEXT("Backward");
	case EDFMovementDirection::BackwardLeft:
		return TEXT("BackwardLeft");
	case EDFMovementDirection::Left:
		return TEXT("Left");
	case EDFMovementDirection::ForwardLeft:
		return TEXT("ForwardLeft");
	default:
		return TEXT("None");
	}
}

static FString BlendSpaceDebugName(const UBlendSpace* const BS)
{
	return BS ? BS->GetName() : FString(TEXT("(null)"));
}
} // namespace

FString UUDFAnimInstance::BuildLocomotionDebugString() const
{
	const UBlendSpace* const ActiveBS = ActiveAnimSet.ResolveLocomotionBS(bShouldStrafe);
	const bool bUsingStrafeBS = bShouldStrafe && ActiveAnimSet.StrafeBlendSpace != nullptr;
	const bool bCMCStrafe = DFCharacterMovement && DFCharacterMovement->bIsStrafing;
	return FString::Printf(
		TEXT("locked=%d strafe=%d inCombat=%d | BS=%s (%s) | MoveBS=%s StrafeBS=%s | Spd=%.0f Dir=%.0f wedge=%s | CMC strafe=%d"),
		bIsLockedOn ? 1 : 0,
		bShouldStrafe ? 1 : 0,
		bIsInCombat ? 1 : 0,
		*BlendSpaceDebugName(ActiveBS),
		bUsingStrafeBS ? TEXT("8-way strafe") : TEXT("1D movement"),
		*BlendSpaceDebugName(ActiveAnimSet.MovementBlendSpace.Get()),
		*BlendSpaceDebugName(ActiveAnimSet.StrafeBlendSpace.Get()),
		Speed,
		Direction,
		LocomotionDirName(MovementDirection),
		bCMCStrafe ? 1 : 0);
}
#endif
