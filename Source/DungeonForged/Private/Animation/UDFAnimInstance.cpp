// Source/DungeonForged/Private/Animation/UDFAnimInstance.cpp
#include "Animation/UDFAnimInstance.h"

#include "Combat/DFJumpDebug.h"
#include "Animation/UDFLocomotionTypes.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"
#include "Characters/ADFEnemyBase.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Characters/UDFCharacterMovementComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimEnums.h"
#include "Animation/AnimClassInterface.h"
#include "Animation/AnimNode_StateMachine.h"
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
	const bool bStrafeForDir = !bIsDead && (bIsLockedOn || bIsInCombat);

	if (!IsPrimaryMeshAnimInstance())
	{
		CalculateLean(DeltaSeconds);
		CalculateAimOffsets();
		UpdateFootIK(DeltaSeconds);
		SyncEquippedWeaponAnimLayerFromOwner();
		return;
	}

	const bool bNowInAir = bIsInAir;
	VerticalVelocity = Velocity.Z;

	const float GroundedTimeBeforeThisFrame = GroundedTime;
	if (!bNowInAir)
	{
		GroundedTime += DeltaSeconds;
	}
	else if (!bWasInAirPreviousFrame)
	{
		GroundedTime = 0.f;
	}

	const bool bNewAirborneFrame = bNowInAir && !bWasInAirPreviousFrame;
	const bool bStableGroundBeforeTakeoff = GroundedTimeBeforeThisFrame >= MinGroundedTimeBeforeJump;
	const bool bValidTakeoff = bNewAirborneFrame && bStableGroundBeforeTakeoff;

	if (bValidTakeoff)
	{
		bJumpArcActive = true;
		bHasPassedJumpApex = false;
		bIsLongFallLanding = false;
		bIsLanding = false;
		LandingRecoveryTimer = 0.f;
		JumpLoopPhaseTime = 0.f;
		CachedJumpLoopPhaseTimeAtLand = 0.f;
		PredictedLandingDistance = LandPredictionTraceMax;
		CaptureTakeoffJumpDirection(bStrafeForDir);
		AirTime = 0.f;
		if (UAnimSequenceBase* const StartAnim = GetJumpStartAnim())
		{
			CachedJumpStartPlayTime = StartAnim->GetPlayLength();
		}
		else
		{
			CachedJumpStartPlayTime = JumpStartMinPlayTime;
		}
		DFJumpDebug::Logf(TEXT("Takeoff dir=%d speed=%.0f StartLen=%.3fs StartToLoopAt=%.3fs"),
			static_cast<int32>(LastJumpDirection), Speed, CachedJumpStartPlayTime, ComputeStartToLoopTime());
	}
	else if (bNewAirborneFrame && !bJumpArcActive)
	{
		// Ledge drop / step-off with brief ground contact — still drive jump SM transitions.
		bJumpArcActive = true;
		bIsLongFallLanding = false;
		bIsLanding = false;
		LandingRecoveryTimer = 0.f;
		CachedJumpLoopPhaseTimeAtLand = 0.f;
		PredictedLandingDistance = LandPredictionTraceMax;
		CaptureTakeoffJumpDirection(bStrafeForDir);
		AirTime = 0.f;
		CachedJumpStartPlayTime = JumpStartMinPlayTime;
		bHasPassedJumpApex = VerticalVelocity <= JumpApexVelocityThreshold;
		JumpLoopPhaseTime = bHasPassedJumpApex ? JumpLoopLandMinPhaseTime : 0.f;
		DFJumpDebug::Logf(TEXT("Airborne (quick) dir=%d vz=%.0f loopPhase=%.3fs"),
			static_cast<int32>(LastJumpDirection), VerticalVelocity, JumpLoopPhaseTime);
	}
	else if (bNowInAir && bJumpArcActive)
	{
		AirTime += DeltaSeconds;
		const float StartToLoopAt = ComputeStartToLoopTime();
		const bool bPastApexExitable = bHasPassedJumpApex
			&& VerticalVelocity < JumpApexVelocityThreshold
			&& AirTime >= JumpStartMinPlayTime;
		if (AirTime >= StartToLoopAt || bPastApexExitable)
		{
			JumpLoopPhaseTime += DeltaSeconds;
		}
	}
	else if (bWasInAirPreviousFrame && !bNowInAir && bJumpArcActive && !bIsLanding)
	{
		bIsLongFallLanding = AirTime >= LongFallAirTimeThreshold;
		CachedJumpLoopPhaseTimeAtLand = JumpLoopPhaseTime;
		bIsLanding = true;
		AirTime = 0.f;
		JumpLoopPhaseTime = 0.f;
		float Recovery = JumpLandRecoveryMinTime;
		if (DFCharacterMovement)
		{
			Recovery = FMath::Max(Recovery, DFCharacterMovement->DFLandingRecoveryWindow);
		}
		if (UAnimSequenceBase* const LandAnim = GetJumpLandAnim())
		{
			Recovery = FMath::Max(Recovery, LandAnim->GetPlayLength() * JumpLandRecoveryAnimFraction);
		}
		// Short hops / double jump: cap recovery so Loop does not linger on the ground.
		if (CachedJumpLoopPhaseTimeAtLand < JumpLoopLandMinPhaseTime)
		{
			Recovery = FMath::Min(Recovery, JumpLandRecoveryMinTime * 0.5f);
		}
		LandingRecoveryTimer = Recovery;
		DFJumpDebug::Logf(TEXT("Landed dir=%d loopPhase=%.3fs recovery=%.3fs"),
			static_cast<int32>(LastJumpDirection), CachedJumpLoopPhaseTimeAtLand, LandingRecoveryTimer);
	}

	if (!bNowInAir && bJumpArcActive && !bIsLanding && GroundedTime >= JumpArcGroundedExitTime)
	{
		bJumpArcActive = false;
		bJumpArcEndLatch = false;
	}
	else if (!bNowInAir && (!bJumpArcActive || GroundedTime >= MinGroundedTimeBeforeJump))
	{
		bHasPassedJumpApex = false;
	}
	else if (bNowInAir && bJumpArcActive && !bHasPassedJumpApex
		&& AirTime >= JumpApexMinRisingTime && VerticalVelocity <= JumpApexVelocityThreshold)
	{
		bHasPassedJumpApex = true;
	}

	bIsJumping = bNowInAir && !bHasPassedJumpApex;
	bIsFalling = bNowInAir && bHasPassedJumpApex;

	bIsDoubleJumping = false;
	if (OwningCharacter)
	{
		if (IAbilitySystemInterface* const IAS = Cast<IAbilitySystemInterface>(OwningCharacter))
		{
			if (UAbilitySystemComponent* const ASC = IAS->GetAbilitySystemComponent())
			{
				bIsDoubleJumping = ASC->HasMatchingGameplayTag(FDFGameplayTags::State_DoubleJumping);
			}
		}
	}

	if (bIsDoubleJumping && !bWasDoubleJumpingPreviousFrame && bJumpArcActive && bNowInAir)
	{
		AirTime = 0.f;
		JumpLoopPhaseTime = 0.f;
		bHasPassedJumpApex = false;
		if (UAnimSequenceBase* const DJStart = GetJumpDoubleStartAnim())
		{
			CachedJumpStartPlayTime = DJStart->GetPlayLength();
		}
		DFJumpDebug::Logf(TEXT("DoubleJump reset StartToLoopAt=%.3fs"), ComputeStartToLoopTime());
	}
	bWasDoubleJumpingPreviousFrame = bIsDoubleJumping;

	if (bIsInAir && bHasPassedJumpApex && VerticalVelocity <= JumpApexVelocityThreshold && OwningCharacter)
	{
		PredictedLandingDistance = LandPredictionTraceMax;
		if (UWorld* const W = OwningCharacter->GetWorld())
		{
			const FVector Origin = OwningCharacter->GetActorLocation();
			const FVector Down = Origin - FVector(0.f, 0.f, LandPredictionTraceMax);
			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(JumpLandTrace), false, OwningCharacter.Get());
			if (W->LineTraceSingleByChannel(Hit, Origin, Down, ECC_Visibility, Params))
			{
				PredictedLandingDistance = FMath::Max(0.f,
					(Origin - Hit.ImpactPoint).Z - OwningCharacter->GetSimpleCollisionHalfHeight());
			}
		}
	}
	else
	{
		PredictedLandingDistance = LandPredictionTraceMax;
	}

	if (bIsLanding && !bNowInAir)
	{
		LandingRecoveryTimer -= DeltaSeconds;
		if (LandingRecoveryTimer <= 0.f)
		{
			bIsLanding = false;
		}
	}

	UpdateJumpTransitionHints();
	bWasInAirPreviousFrame = bNowInAir;

#if !UE_BUILD_SHIPPING
	if (DFJumpDebug::IsLogEnabled())
	{
		LogJumpTransitionEdges();
	}
	if (DFJumpDebug::IsDeepLogEnabled() && (bJumpArcActive || bIsInAir || bIsLanding))
	{
		JumpDeepLogTimer += DeltaSeconds;
		if (JumpDeepLogTimer >= 0.25f)
		{
			JumpDeepLogTimer = 0.f;
			LogJumpDeepSnapshot(TEXT("tick"));
		}
	}
	else
	{
		JumpDeepLogTimer = 0.f;
	}
	if (GEngine && OwningCharacter && OwningCharacter->IsLocallyControlled())
	{
		if (DFJumpDebug::IsDeepLogEnabled())
		{
			const FString Deep = BuildJumpDeepDebugString();
			TArray<FString> Lines;
			Deep.ParseIntoArrayLines(Lines);
			for (int32 i = 0; i < Lines.Num() && i < 18; ++i)
			{
				GEngine->AddOnScreenDebugMessage(0x100 + i, 0.f, FColor::Cyan, Lines[i]);
			}
		}
		else if (DFJumpDebug::IsTransitionHudEnabled())
		{
			GEngine->AddOnScreenDebugMessage(0x100, 0.f, FColor::Cyan, BuildJumpTransitionDebugString());
		}
		else if (DFJumpDebug::IsHudEnabled())
		{
			GEngine->AddOnScreenDebugMessage(0x100, 0.f, FColor::Cyan,
				FString::Printf(TEXT("Jump: J=%d F=%d L=%d Air=%.2fs Vz=%.0f Land=%.0f Dir=%d"),
					bIsJumping ? 1 : 0, bIsFalling ? 1 : 0, bIsLanding ? 1 : 0,
					AirTime, VerticalVelocity, PredictedLandingDistance,
					static_cast<int32>(LastJumpDirection)));
		}
	}
#endif
	if (bIsDead)
	{
		Speed = 0.f;
		Velocity = FVector::ZeroVector;
		bShouldStrafe = false;
		return;
	}
	bShouldStrafe = !bIsDead && (bIsLockedOn || bIsInCombat);
	if (!bNowInAir)
	{
		DetermineMovementDirection(bShouldStrafe);
	}
	CalculateLean(DeltaSeconds);
	CalculateAimOffsets();
	UpdateFootIK(DeltaSeconds);
	SyncEquippedWeaponAnimLayerFromOwner();
}

void UUDFAnimInstance::SyncEquippedWeaponAnimLayerFromOwner()
{
	bHasWeaponEquipped = false;
	EquippedWeaponItemRow = NAME_None;
	TSubclassOf<UAnimInstance> DesiredLayer;
	const FUDAnimSet* DesiredAnimSet = nullptr;
	FName DesiredItemRow = NAME_None;

	if (ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(OwningCharacter.Get()))
	{
		if (UDFEquipmentComponent* Eq = PC->Equipment; Eq && !Eq->IsSlotEmpty(EEquipmentSlot::Weapon))
		{
			bHasWeaponEquipped = true;
			EquippedWeaponItemRow = Eq->EquippedItems.FindRef(EEquipmentSlot::Weapon);
			DesiredItemRow = EquippedWeaponItemRow;

			FDFItemTableRow Row;
			if (Eq->TryGetEquippedItemData(EEquipmentSlot::Weapon, Row))
			{
				DesiredLayer = Row.WeaponLinkedAnimLayerClass;
				// Cache the WeaponAnimSet so we apply it below (jump/locomotion/idle anims).
				if (Row.WeaponAnimSet.IsValid())
				{
					DesiredAnimSet = &Row.WeaponAnimSet;
				}
			}
		}
	}

	// ── Apply per-weapon AnimSet (jump start/loop/land, blend spaces) ───────
	// Without this, GetJumpStartAnim() always reads DefaultAnimSet (unarmed) even when
	// holding a sword. We track the source by item row to avoid re-applying every frame.
	if (DesiredItemRow != CachedAnimSetItemRow)
	{
		if (DesiredAnimSet)
		{
			ActiveAnimSet = *DesiredAnimSet;
		}
		else
		{
			ActiveAnimSet = DefaultAnimSet;
		}
		CachedAnimSetItemRow = DesiredItemRow;
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

UAnimSequenceBase* UUDFAnimInstance::GetJumpStartAnim() const
{
	return ActiveAnimSet.ResolveJumpStart(LastJumpDirection);
}

UAnimSequenceBase* UUDFAnimInstance::GetJumpLoopAnim() const
{
	return ActiveAnimSet.ResolveJumpLoop();
}

UAnimSequenceBase* UUDFAnimInstance::GetJumpLandAnim() const
{
	return ActiveAnimSet.ResolveJumpLand(LastJumpDirection);
}

UAnimSequenceBase* UUDFAnimInstance::GetJumpDoubleStartAnim() const
{
	return ActiveAnimSet.ResolveJumpDoubleStart();
}

UAnimSequenceBase* UUDFAnimInstance::GetJumpDoubleLoopAnim() const
{
	return ActiveAnimSet.ResolveJumpDoubleLoop();
}

float UUDFAnimInstance::GetLandPreparationAlpha() const
{
	if (!bIsInAir || !bHasPassedJumpApex || VerticalVelocity > JumpApexVelocityThreshold
		|| LandPreparationThreshold <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}
	return FMath::Clamp(1.f - (PredictedLandingDistance / LandPreparationThreshold), 0.f, 1.f);
}

void UUDFAnimInstance::NotifyLandingRecoveryBegin(const float Duration)
{
	bIsLanding = true;
	LandingRecoveryTimer = FMath::Max(LandingRecoveryTimer, Duration);
}

void UUDFAnimInstance::NotifyLandingRecoveryEnd()
{
	bIsLanding = false;
	LandingRecoveryTimer = 0.f;
}

float UUDFAnimInstance::ComputeStartToLoopTime() const
{
	return FMath::Max(JumpStartMinPlayTime, CachedJumpStartPlayTime);
}

void UUDFAnimInstance::CaptureTakeoffJumpDirection(const bool bStrafeForDir)
{
	if (Speed > 50.f)
	{
		DetermineMovementDirection(bStrafeForDir);
		LastJumpDirection = MovementDirection;
		return;
	}

	if (DFCharacterMovement)
	{
		const FVector WorldInput = DFCharacterMovement->GetLastInputVector();
		if (WorldInput.SizeSquared2D() > 100.f)
		{
			const FRotator YawRot(0.f, OwningCharacter->GetActorRotation().Yaw, 0.f);
			const float SavedDirection = Direction;
			Direction = UKismetAnimationLibrary::CalculateDirection(WorldInput, YawRot);
			DetermineMovementDirection(bStrafeForDir);
			LastJumpDirection = MovementDirection;
			Direction = SavedDirection;
			return;
		}
	}

	LastJumpDirection = EDFMovementDirection::None;
}

void UUDFAnimInstance::UpdateJumpTransitionHints()
{
	const float LandPrepAlpha = GetLandPreparationAlpha();
	const float StartToLoopAt = ComputeStartToLoopTime();
	const bool bInJumpStartWindow = AirTime < StartToLoopAt;
	const bool bGroundedStable = !bIsInAir && GroundedTime >= JumpArcGroundedExitTime;

	// Hold Loco→Start for the full start clip; Start→Loop when the clip would finish (not physics apex).
	bTransition_LocomotionToJumpStart = bJumpArcActive && bIsInAir && bInJumpStartWindow;

	// Allow early exit from Start when physics says we're past apex and clearly descending — even if the
	// asset is still playing. This prevents the SM from getting stuck in JumpStart on short forward jumps.
	const bool bPastApexExitable = bHasPassedJumpApex
		&& VerticalVelocity < JumpApexVelocityThreshold
		&& AirTime >= JumpStartMinPlayTime;

	bTransition_JumpStartToLoop = bJumpArcActive && bIsInAir && (
		AirTime >= StartToLoopAt || AirTime >= JumpStartMaxPlayTime || bPastApexExitable);

	// Loop phase must run past Start->Loop crossfade before prep / land (JumpLoopPhaseTime tracks post-start air time).
	bTransition_JumpLoopToLandPrep = bJumpArcActive && bIsFalling
		&& JumpLoopPhaseTime >= JumpLoopPrepMinPhaseTime
		&& LandPrepAlpha >= JumpLandPrepAlphaThreshold;

	// Sustained through landing recovery. No min loop phase — if SM is in Jump Loop, it must exit on touch-down
	// even when loopPhase < LandMin (short hop / late Start->Loop crossfade). JumpStart->Land covers Start state.
	bTransition_JumpLoopToLand = !bIsInAir && bIsLanding;

	// Emergency direct path: landed while SM is still in JumpStart (short-airtime / long-fall step-off).
	bTransition_JumpStartToLand = !bIsInAir && bIsLanding
		&& CachedJumpLoopPhaseTimeAtLand < JumpLoopLandMinPhaseTime;

	// Exit Land once recovery ends; keep true after arc timeout so a missed Land->Loco still fires.
	bTransition_LandToLocomotion = !bIsInAir && !bIsLanding && (bJumpArcActive || bGroundedStable);

	// Escape Jump Start/Loop when grounded — persists even after bJumpArcActive clears (prevents permanent SM stick).
	bTransition_JumpGroundedExit = !bIsInAir && !bIsLanding && bGroundedStable;

	if (bTransition_LandToLocomotion)
	{
		if (bJumpArcEndLatch)
		{
			bJumpArcActive = false;
			bJumpArcEndLatch = false;
		}
		else
		{
			bJumpArcEndLatch = true;
		}
	}
	else
	{
		bJumpArcEndLatch = false;
	}
}

void UUDFAnimInstance::LogJumpTransitionEdges()
{
	auto LogEdge = [this](const TCHAR* Name, const bool bNow, bool& bPrev)
	{
		if (bNow && !bPrev)
		{
			DFJumpDebug::Logf(TEXT("Transition READY: %s"), Name);
			if (DFJumpDebug::IsDeepLogEnabled())
			{
				LogJumpDeepSnapshot(Name);
			}
		}
		else if (!bNow && bPrev)
		{
			DFJumpDebug::Logf(TEXT("Transition OFF: %s"), Name);
		}
		bPrev = bNow;
	};

	LogEdge(TEXT("Loco->JumpStart"), bTransition_LocomotionToJumpStart, bPrevTransition_LocoToStart);
	LogEdge(TEXT("JumpStart->Loop"), bTransition_JumpStartToLoop, bPrevTransition_StartToLoop);
	LogEdge(TEXT("JumpStart->Land"), bTransition_JumpStartToLand, bPrevTransition_StartToLand);
	LogEdge(TEXT("Loop->LandPrep"), bTransition_JumpLoopToLandPrep, bPrevTransition_LoopToLandPrep);
	LogEdge(TEXT("Loop->Land"), bTransition_JumpLoopToLand, bPrevTransition_LoopToLand);
	LogEdge(TEXT("Land->Loco"), bTransition_LandToLocomotion, bPrevTransition_LandToLoco);
	LogEdge(TEXT("GroundedExit"), bTransition_JumpGroundedExit, bPrevTransition_JumpGroundedExit);
}

bool UUDFAnimInstance::IsPrimaryMeshAnimInstance() const
{
	const USkeletalMeshComponent* Skel = GetSkelMeshComponent();
	return !Skel || Skel->GetAnimInstance() == this;
}

#if !UE_BUILD_SHIPPING
namespace
{
const TCHAR* OnOff(const bool b)
{
	return b ? TEXT("ON ") : TEXT("off");
}
} // namespace

FString UUDFAnimInstance::BuildJumpTransitionDebugString() const
{
	const float LandPrepAlpha = GetLandPreparationAlpha();
	return FString::Printf(
		TEXT("[Jump|SM] Loco>Start=%s Start>Loop=%s Start>Land=%s Loop>Prep=%s Loop>Land=%s Land>Loco=%s GndExit=%s\n")
		TEXT("  J=%d F=%d InAir=%d Apex=%d Arc=%d L=%d | Vz=%.0f Air=%.2fs Gnd=%.2fs PredLand=%.0f Alpha=%.2f\n")
		TEXT("  StartMin=%.2fs StartMax=%.2fs ApexRise=%.2fs ApexVz=%.0f | LoopPhase=%.2fs PrepMin=%.2fs LandMin=%.2fs\n")
		TEXT("  Blends: Loco>Start=%.2fs Start>Loop=%.2fs Loop>Land=%.2fs Land>Loco=%.2fs | Dir=%d"),
		OnOff(bTransition_LocomotionToJumpStart),
		OnOff(bTransition_JumpStartToLoop),
		OnOff(bTransition_JumpStartToLand),
		OnOff(bTransition_JumpLoopToLandPrep),
		OnOff(bTransition_JumpLoopToLand),
		OnOff(bTransition_LandToLocomotion),
		OnOff(bTransition_JumpGroundedExit),
		bIsJumping ? 1 : 0,
		bIsFalling ? 1 : 0,
		bIsInAir ? 1 : 0,
		bHasPassedJumpApex ? 1 : 0,
		bJumpArcActive ? 1 : 0,
		bIsLanding ? 1 : 0,
		VerticalVelocity,
		AirTime,
		GroundedTime,
		PredictedLandingDistance,
		LandPrepAlpha,
		JumpStartMinPlayTime,
		JumpStartMaxPlayTime,
		JumpApexMinRisingTime,
		JumpApexVelocityThreshold,
		JumpLoopPhaseTime,
		JumpLoopPrepMinPhaseTime,
		JumpLoopLandMinPhaseTime,
		JumpBlend_LocoToStart,
		JumpBlend_StartToLoop,
		JumpBlend_LoopToLand,
		JumpBlend_LandToLoco,
		static_cast<int32>(LastJumpDirection));
}

namespace
{
const TCHAR* RootMotionRootLockName(const ERootMotionRootLock::Type Lock)
{
	switch (Lock)
	{
	case ERootMotionRootLock::RefPose:
		return TEXT("RefPose");
	case ERootMotionRootLock::AnimFirstFrame:
		return TEXT("AnimFirstFrame");
	case ERootMotionRootLock::Zero:
		return TEXT("Zero");
	default:
		return TEXT("Unknown");
	}
}

const TCHAR* AdditiveAnimTypeName(const EAdditiveAnimationType Type)
{
	switch (Type)
	{
	case AAT_None:
		return TEXT("None");
	case AAT_LocalSpaceBase:
		return TEXT("LocalSpace");
	case AAT_RotationOffsetMeshSpace:
		return TEXT("MeshSpace");
	default:
		return TEXT("Unknown");
	}
}

const TCHAR* AnimInstanceRootMotionModeName(const ERootMotionMode::Type Mode)
{
	switch (Mode)
	{
	case ERootMotionMode::NoRootMotionExtraction:
		return TEXT("NoExtraction");
	case ERootMotionMode::IgnoreRootMotion:
		return TEXT("Ignore");
	case ERootMotionMode::RootMotionFromEverything:
		return TEXT("FromEverything");
	case ERootMotionMode::RootMotionFromMontagesOnly:
		return TEXT("MontagesOnly");
	default:
		return TEXT("Unknown");
	}
}

FString FormatJumpAnimAssetFlags(UAnimSequenceBase* Asset)
{
	if (!Asset)
	{
		return TEXT("RM=? Lock=? ForceLock=? NormRM=? Add=?");
	}

	if (const UAnimSequence* Seq = Cast<UAnimSequence>(Asset))
	{
		return FString::Printf(TEXT("RM=%d Lock=%s ForceLock=%d NormRM=%d Add=%s"),
			Seq->bEnableRootMotion ? 1 : 0,
			RootMotionRootLockName(Seq->RootMotionRootLock),
			Seq->bForceRootLock ? 1 : 0,
			Seq->bUseNormalizedRootMotionScale ? 1 : 0,
			AdditiveAnimTypeName(Seq->AdditiveAnimType));
	}

	return FString::Printf(TEXT("RM=%d Add=%s"),
		Asset->HasRootMotion() ? 1 : 0,
		AdditiveAnimTypeName(Asset->GetAdditiveAnimType()));
}

FString FormatJumpAnimAsset(const TCHAR* Label, UAnimSequenceBase* Asset, const float CompareMinPlayTime = -1.f)
{
	if (!Asset)
	{
		return FString::Printf(TEXT("Anim|%s: (null)"), Label);
	}
	const float Len = Asset->GetPlayLength();
	FString FrameInfo;
	if (const UAnimSequence* Seq = Cast<UAnimSequence>(Asset))
	{
		FrameInfo = FString::Printf(TEXT(" keys=%d"), Seq->GetNumberOfSampledKeys());
	}
	FString TuneHint;
	if (CompareMinPlayTime >= 0.f)
	{
		TuneHint = FString::Printf(TEXT(" | vs StartMin=%.3fs (delta=%+.3fs)"), CompareMinPlayTime, Len - CompareMinPlayTime);
	}
	return FString::Printf(TEXT("Anim|%s: %s len=%.3fs%s%s\n  %s"),
		Label, *Asset->GetName(), Len, *FrameInfo, *TuneHint, *FormatJumpAnimAssetFlags(Asset));
}

FString BuildAnimInstanceRootMotionLine(const UAnimInstance* Anim)
{
	if (!Anim)
	{
		return TEXT("AnimInst| RootMotionMode=(null)");
	}
	return FString::Printf(TEXT("AnimInst| RootMotionMode=%s"),
		AnimInstanceRootMotionModeName(Anim->RootMotionMode));
}

FString BuildJumpStateMachineDebugLines(UAnimInstance* Anim)
{
	if (!Anim || !IsValid(Anim))
	{
		return TEXT("StateMachines: (invalid AnimInstance)");
	}

	FString Out = TEXT("StateMachines:");
	bool bAny = false;

	IAnimClassInterface* const AnimBlueprintClass = IAnimClassInterface::GetFromClass(Anim->GetClass());
	if (!AnimBlueprintClass)
	{
		return TEXT("StateMachines: (not an AnimBlueprint class)");
	}

	const TArray<FStructProperty*>& AnimNodeProperties = AnimBlueprintClass->GetAnimNodeProperties();
	for (int32 MachineIndex = 0; MachineIndex < AnimNodeProperties.Num(); ++MachineIndex)
	{
		const int32 InstancePropertyIndex = AnimNodeProperties.Num() - 1 - MachineIndex;
		FStructProperty* const NodeProperty = AnimNodeProperties[InstancePropertyIndex];
		if (!NodeProperty || !NodeProperty->Struct->IsChildOf(FAnimNode_StateMachine::StaticStruct()))
		{
			continue;
		}

		FAnimNode_StateMachine* const MachineInstance =
			NodeProperty->ContainerPtrToValuePtr<FAnimNode_StateMachine>(Anim);
		if (!MachineInstance)
		{
			continue;
		}

		const FName StateName = MachineInstance->GetCurrentStateName();
		if (StateName.IsNone())
		{
			continue;
		}

		bAny = true;
		const float StateElapsed = Anim->GetInstanceCurrentStateElapsedTime(MachineIndex);
		const float MachineWeight = Anim->GetInstanceMachineWeight(MachineIndex);
		Out += FString::Printf(TEXT("\n  SM[%d] State=%s elapsed=%.3fs weight=%.2f"),
			MachineIndex, *StateName.ToString(), StateElapsed, MachineWeight);

		for (int32 TransIdx = 0; TransIdx < 32; ++TransIdx)
		{
			const float TransFrac = Anim->GetInstanceTransitionTimeElapsedFraction(MachineIndex, TransIdx);
			if (TransFrac <= KINDA_SMALL_NUMBER)
			{
				continue;
			}
			const float TransElapsed = Anim->GetInstanceTransitionTimeElapsed(MachineIndex, TransIdx);
			const float TransDuration = Anim->GetInstanceTransitionCrossfadeDuration(MachineIndex, TransIdx);
			Out += FString::Printf(TEXT("\n    -> Transition[%d] elapsed=%.3fs dur=%.3fs (%.0f%%)"),
				TransIdx, TransElapsed, TransDuration, TransFrac * 100.f);
		}
	}

	if (!bAny)
	{
		Out += TEXT("\n  (no active state machine states)");
	}
	return Out;
}

FString BuildActiveMontageDebugLine(UAnimInstance* Anim)
{
	if (!Anim)
	{
		return TEXT("Montage: (no anim instance)");
	}
	if (UAnimMontage* const Montage = Anim->GetCurrentActiveMontage())
	{
		const float Pos = Anim->Montage_GetPosition(Montage);
		const float Len = Montage->GetPlayLength();
		return FString::Printf(TEXT("Montage: %s pos=%.3fs/%.3fs (%.0f%%)"),
			*Montage->GetName(), Pos, Len, Len > KINDA_SMALL_NUMBER ? (Pos / Len) * 100.f : 0.f);
	}
	return TEXT("Montage: (none)");
}
} // namespace

FString UUDFAnimInstance::BuildJumpDeepDebugString()
{
	const float LandPrepAlpha = GetLandPreparationAlpha();
	const float StartToLoopAt = ComputeStartToLoopTime();
	const bool bInJumpStartWindow = AirTime < StartToLoopAt;
	const float StartToLoopIn = FMath::Max(0.f, StartToLoopAt - AirTime);

	UAnimSequenceBase* const StartAnim = GetJumpStartAnim();
	UAnimSequenceBase* const LoopAnim = GetJumpLoopAnim();
	UAnimSequenceBase* const LandAnim = GetJumpLandAnim();

	FString Out = BuildJumpTransitionDebugString();
	Out += FString::Printf(
		TEXT("\nTiming| StartWindow=%d StartToLoopAt=%.3fs StartToLoopIn=%.3fs LoopPhase=%.3fs PrepMin=%.3fs LandMin=%.3fs LandRecov=%.3fs PrepTh=%.2f\n")
		TEXT("Blends| Loco>Start=%.3fs Start>Loop=%.3fs Loop>Land=%.3fs Land>Loco=%.3fs (wire these in AnimBP crossfade)\n")
		TEXT("%s\n%s\n%s\n%s\n%s\n%s"),
		bInJumpStartWindow ? 1 : 0,
		StartToLoopAt,
		StartToLoopIn,
		JumpLoopPhaseTime,
		JumpLoopPrepMinPhaseTime,
		JumpLoopLandMinPhaseTime,
		LandingRecoveryTimer,
		JumpLandPrepAlphaThreshold,
		JumpBlend_LocoToStart,
		JumpBlend_StartToLoop,
		JumpBlend_LoopToLand,
		JumpBlend_LandToLoco,
		*FormatJumpAnimAsset(TEXT("Start"), StartAnim, JumpStartMinPlayTime),
		*FormatJumpAnimAsset(TEXT("Loop"), LoopAnim),
		*FormatJumpAnimAsset(TEXT("Land"), LandAnim),
		*BuildAnimInstanceRootMotionLine(this),
		*BuildActiveMontageDebugLine(this),
		*BuildJumpStateMachineDebugLines(this));

	if (StartAnim && JumpStartMinPlayTime > 0.f)
	{
		const float AnimLen = StartAnim->GetPlayLength();
		if (FMath::Abs(AnimLen - JumpStartMinPlayTime) > 0.05f)
		{
			Out += FString::Printf(
				TEXT("\nTune| JumpStartMinPlayTime (%.3fs) differs from Start anim (%.3fs) — using max at runtime (%.3fs)"),
				JumpStartMinPlayTime, AnimLen, FMath::Max(JumpStartMinPlayTime, AnimLen));
		}
	}
	if (!LandAnim)
	{
		Out += TEXT("\nWARN| Anim|Land is NULL — assign JumpSet.Land_Idle (or legacy JumpLandAnim) on DefaultAnimSet");
	}
	return Out;
}

void UUDFAnimInstance::LogJumpDeepSnapshot(const TCHAR* const Reason)
{
	if (!DFJumpDebug::IsDeepLogEnabled())
	{
		return;
	}
	const FString Body = BuildJumpDeepDebugString();
	DFJumpDebug::LogBlock(Reason, Body);
}
#endif

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
		TEXT("locked=%d strafe=%d inCombat=%d | BS=%s (%s) | MoveBS=%s StrafeBS=%s | Spd=%.0f Dir=%.0f wedge=%s | CMC strafe=%d | Jump J=%d F=%d L=%d air=%.2fs vz=%.0f predLand=%.0f takeoff=%s"),
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
		bCMCStrafe ? 1 : 0,
		bIsJumping ? 1 : 0,
		bIsFalling ? 1 : 0,
		bIsLanding ? 1 : 0,
		AirTime,
		VerticalVelocity,
		PredictedLandingDistance,
		LocomotionDirName(LastJumpDirection));
}
#endif
