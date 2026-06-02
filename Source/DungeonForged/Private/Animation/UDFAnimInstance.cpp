// Source/DungeonForged/Private/Animation/UDFAnimInstance.cpp
#include "Animation/UDFAnimInstance.h"

#include "Combat/DFJumpDebug.h"
#include "Animation/DFLocomotionDebug.h"
#include "Animation/DFTurnInPlaceDebug.h"
#include "Animation/UDFLocomotionTypes.h"
#include "DungeonForgedModule.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"
#include "Characters/ADFEnemyBase.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Characters/UDFCharacterMovementComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceBase.h"
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

namespace DFLocomotionDistanceCurve
{
	static const FName DistanceCurveName(TEXT("Distance"));

	float FindStopSequenceTimeForRemainingDistance(const UAnimSequence* Seq, const float RemainingDistanceCm)
	{
		if (!Seq)
		{
			return 0.f;
		}
		const float PlayLength = Seq->GetPlayLength();
		if (PlayLength <= KINDA_SMALL_NUMBER || RemainingDistanceCm <= KINDA_SMALL_NUMBER)
		{
			return PlayLength;
		}

		const float TargetCurveValue = -FMath::Abs(RemainingDistanceCm);
		constexpr int32 Samples = 48;
		float BestTime = 0.f;
		float BestError = TNumericLimits<float>::Max();
		for (int32 Index = 0; Index <= Samples; ++Index)
		{
			const float T = PlayLength * static_cast<float>(Index) / static_cast<float>(Samples);
			const float CurveValue = Seq->EvaluateCurveData(DistanceCurveName, T);
			const float Error = FMath::Abs(CurveValue - TargetCurveValue);
			if (Error < BestError)
			{
				BestError = Error;
				BestTime = T;
			}
		}
		return BestTime;
	}

	/** Last time (s) the Distance curve still has meaningful decel; after this the clip is mostly hold. */
	float FindStopMotionEndTime(const UAnimSequence* Seq, const float NearZeroCm = 8.f)
	{
		if (!Seq)
		{
			return 0.f;
		}
		const float PlayLength = Seq->GetPlayLength();
		if (PlayLength <= KINDA_SMALL_NUMBER)
		{
			return PlayLength;
		}

		constexpr int32 Samples = 64;
		for (int32 Index = Samples; Index >= 0; --Index)
		{
			const float T = PlayLength * static_cast<float>(Index) / static_cast<float>(Samples);
			if (FMath::Abs(Seq->EvaluateCurveData(DistanceCurveName, T)) > NearZeroCm)
			{
				return FMath::Min(PlayLength, T + (PlayLength / static_cast<float>(Samples)));
			}
		}
		return PlayLength * 0.85f;
	}
}

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
	EnsureActiveAnimSetTurnSet();
	TryAutoTuneAuthoredLoopSpeedFromDefaultRunLoop();
	TryAutoTuneAuthoredStopDistanceFromDefaultRunStop();
}

void UUDFAnimInstance::ApplyAnimSet(const FUDAnimSet& NewAnimSet)
{
	if (NewAnimSet.IsValid())
	{
		ActiveAnimSet = NewAnimSet;
		EnsureActiveAnimSetTurnSet();
		TryAutoTuneAuthoredLoopSpeedFromDefaultRunLoop();
		TryAutoTuneAuthoredStopDistanceFromDefaultRunStop();
	}
}

void UUDFAnimInstance::RevertToDefaultAnimSet()
{
	ActiveAnimSet = DefaultAnimSet;
	EnsureActiveAnimSetTurnSet();
	TryAutoTuneAuthoredLoopSpeedFromDefaultRunLoop();
	TryAutoTuneAuthoredStopDistanceFromDefaultRunStop();
}

void UUDFAnimInstance::EnsureActiveAnimSetTurnSet()
{
	if (!ActiveAnimSet.TurnSet.IsValid())
	{
		ActiveAnimSet.MergeTurnSetFrom(DefaultAnimSet.TurnSet);
	}
	if (!ActiveAnimSet.TurnSet.IsValid())
	{
		DefaultAnimSet.TryAutoFillTurnSetFromIdlePackagePaths();
		ActiveAnimSet.MergeTurnSetFrom(DefaultAnimSet.TurnSet);
	}
	if (!ActiveAnimSet.TurnSet.IsValid())
	{
		DefaultAnimSet.TryAutoFillTurnSetFromKnownContentPaths();
		ActiveAnimSet.MergeTurnSetFrom(DefaultAnimSet.TurnSet);
	}
	if (!ActiveAnimSet.TurnSet.IsValid())
	{
		ActiveAnimSet.TryAutoFillTurnSetFromIdlePackagePaths();
	}
	if (!ActiveAnimSet.TurnSet.IsValid())
	{
		ActiveAnimSet.TryAutoFillTurnSetFromKnownContentPaths();
	}
	if (IsPrimaryMeshAnimInstance())
	{
		MergeTurnSetFromLinkedLayersIfEmpty();
	}
}

void UUDFAnimInstance::MergeTurnSetFromLinkedLayersIfEmpty()
{
	if (!IsPrimaryMeshAnimInstance() || ActiveAnimSet.TurnSet.IsValid())
	{
		return;
	}

	auto TryMergeFromLayer = [this](UAnimInstance* const Inst)
	{
		if (!Inst || Inst == this)
		{
			return false;
		}
		UUDFAnimInstance* const LayerDF = Cast<UUDFAnimInstance>(Inst);
		if (!LayerDF)
		{
			return false;
		}
		if (!LayerDF->DefaultAnimSet.TurnSet.IsValid() && !LayerDF->ActiveAnimSet.TurnSet.IsValid())
		{
			LayerDF->DefaultAnimSet.TryAutoFillTurnSetFromKnownContentPaths();
		}
		if (LayerDF->DefaultAnimSet.TurnSet.IsValid())
		{
			DefaultAnimSet.MergeTurnSetFrom(LayerDF->DefaultAnimSet.TurnSet);
			ActiveAnimSet.MergeTurnSetFrom(LayerDF->DefaultAnimSet.TurnSet);
		}
		else if (LayerDF->ActiveAnimSet.TurnSet.IsValid())
		{
			ActiveAnimSet.MergeTurnSetFrom(LayerDF->ActiveAnimSet.TurnSet);
		}
		return ActiveAnimSet.TurnSet.IsValid();
	};

	static const FName DefaultSharedGroup(TEXT("DefaultSharedGroup"));
	TArray<UAnimInstance*> LinkedByGroup;
	GetLinkedAnimLayerInstancesByGroup(DefaultSharedGroup, LinkedByGroup);
	for (UAnimInstance* const Inst : LinkedByGroup)
	{
		if (TryMergeFromLayer(Inst))
		{
			return;
		}
	}
	if (CachedLinkedWeaponLayerClass)
	{
		TryMergeFromLayer(GetLinkedAnimLayerInstanceByClass(CachedLinkedWeaponLayerClass, true));
	}
}

void UUDFAnimInstance::SyncTurnSetFromPrimaryMeshIfEmpty()
{
	if (IsPrimaryMeshAnimInstance() || ActiveAnimSet.TurnSet.IsValid())
	{
		return;
	}
	const USkeletalMeshComponent* const Skel = GetSkelMeshComponent();
	if (!Skel)
	{
		return;
	}
	const UUDFAnimInstance* const Primary = Cast<UUDFAnimInstance>(Skel->GetAnimInstance());
	if (!Primary || Primary == this)
	{
		return;
	}
	ActiveAnimSet.MergeTurnSetFrom(Primary->ActiveAnimSet.TurnSet);
	if (!ActiveAnimSet.TurnSet.IsValid())
	{
		ActiveAnimSet.MergeTurnSetFrom(Primary->DefaultAnimSet.TurnSet);
	}
	if (!ActiveAnimSet.TurnSet.IsValid())
	{
		DefaultAnimSet.MergeTurnSetFrom(Primary->DefaultAnimSet.TurnSet);
		ActiveAnimSet.MergeTurnSetFrom(DefaultAnimSet.TurnSet);
	}
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
		bIsAirDashing = ASC->HasMatchingGameplayTag(FDFGameplayTags::State_AirDashing)
			|| (DFCharacterMovement && DFCharacterMovement->IsAirDashDriveActive());
	}
	else
	{
		bIsDead = false;
		bIsInCombat = false;
		bIsLockedOn = false;
		bIsAttacking = false;
		bIsCasting = false;
		bIsStunned = false;
		bIsAirDashing = false;
	}

	const bool bSuppressLocomotionBlend = bIsAirDashing
		|| (DFCharacterMovement && DFCharacterMovement->IsAirDashDriveActive());
	if (bSuppressLocomotionBlend)
	{
		Speed = 0.f;
		Direction = 0.f;
	}

	const bool bStrafeForDir = !bIsDead && (bIsLockedOn || bIsInCombat);

	if (!IsPrimaryMeshAnimInstance())
	{
		SyncDirectionalLocomotionFromPrimaryMesh();
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
		StopFallLoopSlotOverlay();
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
		if (AirTime >= StartToLoopAt || IsPastApexExitableForJumpLoop())
		{
			JumpLoopPhaseTime += DeltaSeconds;
		}
	}
	else if (bWasInAirPreviousFrame && !bNowInAir && bJumpArcActive && !bIsLanding)
	{
		StopFallLoopSlotOverlay();
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

	if (AirDashResumeFallLoopLatchTime > 0.f)
	{
		AirDashResumeFallLoopLatchTime = FMath::Max(0.f, AirDashResumeFallLoopLatchTime - DeltaSeconds);
	}

	if (bFallLoopOverlayActive && !bNowInAir)
	{
		StopFallLoopSlotOverlay();
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
	UpdateDirectionalLocomotion(DeltaSeconds);
	UpdateDistanceMatching(DeltaSeconds);
	UpdateStrideWarping(DeltaSeconds);
	CalculateLean(DeltaSeconds);
	CalculateAimOffsets();          // populates AimYaw / AimPitch — TIP depends on this
	EnsureActiveAnimSetTurnSet();
	UpdateTurnInPlace(DeltaSeconds); // must run after CalculateAimOffsets
	UpdateAimOffsetBlend(DeltaSeconds);
	UpdateFootIK(DeltaSeconds);
	UpdateFootPlantCurves();
	SyncEquippedWeaponAnimLayerFromOwner();
	PropagateDirectionalLocomotionToLinkedAnimLayers();

#if !UE_BUILD_SHIPPING
	if (GEngine && OwningCharacter && OwningCharacter->IsLocallyControlled())
	{
		const bool bPrimary = IsPrimaryMeshAnimInstance();
		const float SpeedDelta = Speed - LocomotionDebugPrevSpeed;
		LocomotionDebugPrevSpeed = Speed;

		if (DFLocomotionDebug::IsLogEnabled())
		{
			if (DFLocomotionDebug::IsVerboseEnabled() && bPrimary)
			{
				const float MaxWS = DFCharacterMovement ? DFCharacterMovement->MaxWalkSpeed : 0.f;
				UE_LOG(LogDungeonForged, Log,
					TEXT("[Loco|Main] Spd=%.0f(d%+.0f) MaxWS=%.0f Gait=%d Dir=%d Stride=%.2f | I>S=%d S>L=%d L>P=%d S>M=%d P>I=%d TIP=%d T=%.0f° off=%.0f t=%.2f"),
					Speed, SpeedDelta, MaxWS, static_cast<int32>(Gait), static_cast<int32>(MovementDirection),
					StrideScale, bTransition_IdleToStart ? 1 : 0, bTransition_StartToLoop ? 1 : 0,
					bTransition_LoopToStop ? 1 : 0, bTransition_StopToMove ? 1 : 0, bTransition_StopToIdle ? 1 : 0,
					bInTurnInPlacePhase ? 1 : 0, TurnInPlaceAnimDegrees, RootYawOffset, TurnInPlaceExplicitTime);
			}
			else
			{
				UE_LOG(LogDungeonForged, Log,
					TEXT("[Loco|%s] Spd=%.0f Gait=%d Dir=%d Accel=%d | I>S=%d S>L=%d L>P=%d S>M=%d P>I=%d TIP=%d off=%.0f"),
					bPrimary ? TEXT("Main") : TEXT("Layer"),
					Speed, static_cast<int32>(Gait), static_cast<int32>(MovementDirection),
					bIsAccelerating ? 1 : 0, bTransition_IdleToStart ? 1 : 0, bTransition_StartToLoop ? 1 : 0,
					bTransition_LoopToStop ? 1 : 0, bTransition_StopToMove ? 1 : 0, bTransition_StopToIdle ? 1 : 0,
					bInTurnInPlacePhase ? 1 : 0, RootYawOffset);
			}
		}

		// Mirror on-screen HUD lines to Output Log (same content, throttled).
		if (DFLocomotionDebug::IsHudEnabled() && DFLocomotionDebug::IsLogEnabled() && bPrimary)
		{
			const float HudLogInterval = DFLocomotionDebug::IsVerboseEnabled() ? 0.25f : 0.5f;
			LocomotionVerboseLogTimer += DeltaSeconds;
			if (LocomotionVerboseLogTimer >= HudLogInterval)
			{
				LocomotionVerboseLogTimer = 0.f;
				const FString HudBody = DFLocomotionDebug::IsVerboseEnabled()
					? (BuildDirectionalLocomotionDebugString() + BuildDirectionalLocomotionDeepDebugString())
					: BuildDirectionalLocomotionDebugString();
				TArray<FString> HudLines;
				HudBody.ParseIntoArrayLines(HudLines);
				UE_LOG(LogDungeonForged, Log, TEXT("[Loco|HUD] ----------"));
				for (const FString& Line : HudLines)
				{
					if (!Line.IsEmpty())
					{
						UE_LOG(LogDungeonForged, Log, TEXT("[Loco|HUD] %s"), *Line);
					}
				}
			}
		}
		else if (!DFLocomotionDebug::IsHudEnabled())
		{
			LocomotionVerboseLogTimer = 0.f;
		}

		if (DFLocomotionDebug::IsHudEnabled())
		{
			const int32 KeyBase = bPrimary ? 0x200 : 0x220;
			const FColor Col = bPrimary ? FColor::Green : FColor::Yellow;
			const int32 MaxHudLines = DFLocomotionDebug::IsVerboseEnabled() ? 24 : 14;
			TArray<FString> Lines;
			const FString HudBody = DFLocomotionDebug::IsVerboseEnabled()
				? (BuildDirectionalLocomotionDebugString() + BuildDirectionalLocomotionDeepDebugString())
				: BuildDirectionalLocomotionDebugString();
			HudBody.ParseIntoArrayLines(Lines);
			for (int32 i = 0; i < Lines.Num() && i < MaxHudLines; ++i)
			{
				GEngine->AddOnScreenDebugMessage(KeyBase + i, 0.f, Col, Lines[i]);
			}
		}
		if (DFLocomotionDebug::IsDrawEnabled() && bPrimary)
		{
			if (const UWorld* const World = GetWorld())
			{
				const FVector Origin = OwningCharacter->GetActorLocation();
				const FVector Facing = OwningCharacter->GetActorForwardVector();
				DrawDebugDirectionalArrow(World, Origin, Origin + Facing * 120.f, 30.f, FColor::Blue, false, -1.f, 0, 2.f);
				if (Velocity.SizeSquared2D() > 1.f)
				{
					const FVector VelDir = Velocity.GetSafeNormal2D();
					DrawDebugDirectionalArrow(World, Origin, Origin + VelDir * 150.f, 36.f, FColor::Green, false, -1.f, 0, 3.f);
				}
			}
		}

		if (bPrimary && (DFTurnInPlaceDebug::IsLogEnabled() || DFTurnInPlaceDebug::IsHudEnabled()
			|| DFTurnInPlaceDebug::IsDrawEnabled()))
		{
			const UWorld* const World = GetWorld();
			if (DFTurnInPlaceDebug::IsLogEnabled())
			{
				const float Interval = DFTurnInPlaceDebug::IsHudEnabled() ? 0.35f : 0.2f;
				TurnInPlaceDebugLogTimer += DeltaSeconds;
				if (TurnInPlaceDebugLogTimer >= Interval
					|| bTransition_TurnInPlace || (bInTurnInPlacePhase && !bWasInTurnInPlacePhasePreviousFrame))
				{
					TurnInPlaceDebugLogTimer = 0.f;
					UE_LOG(LogDungeonForged, Log, TEXT("[TIP] %s"), *BuildTurnInPlaceDebugString());
					UE_LOG(LogDungeonForged, Log, TEXT("%s"), *BuildTurnInPlaceDebugOneLiner());
				}
			}
			if (DFTurnInPlaceDebug::IsHudEnabled())
			{
				const FColor TipCol(200, 120, 255);
				TArray<FString> Lines;
				BuildTurnInPlaceDebugString().ParseIntoArrayLines(Lines);
				for (int32 i = 0; i < Lines.Num() && i < 10; ++i)
				{
					GEngine->AddOnScreenDebugMessage(0x200 + i, 0.f, TipCol, Lines[i]);
				}
			}
			if (DFTurnInPlaceDebug::IsDrawEnabled() && World)
			{
				DrawTurnInPlaceDebug(World);
			}
		}
		else
		{
			TurnInPlaceDebugLogTimer = 0.f;
		}
	}
#endif
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

void UUDFAnimInstance::CopyDirectionalLocomotionStateFrom(const UUDFAnimInstance& Source)
{
	MovementDirection = Source.MovementDirection;
	LocomotionStartDirection = Source.LocomotionStartDirection;
	LocomotionStopDirection = Source.LocomotionStopDirection;
	LocomotionStartGait = Source.LocomotionStartGait;
	LocomotionStopGait = Source.LocomotionStopGait;
	Gait = Source.Gait;
	bIsAccelerating = Source.bIsAccelerating;
	bTransition_IdleToStart = Source.bTransition_IdleToStart;
	bTransition_StartToLoop = Source.bTransition_StartToLoop;
	bTransition_LoopToStop = Source.bTransition_LoopToStop;
	bTransition_StopToIdle = Source.bTransition_StopToIdle;
	bTransition_StopToMove = Source.bTransition_StopToMove;
	LocomotionStartElapsed = Source.LocomotionStartElapsed;
	bWasAcceleratingPreviousFrame = Source.bWasAcceleratingPreviousFrame;
	bWasInLocomotionStopPhasePreviousFrame = Source.bWasInLocomotionStopPhasePreviousFrame;
	bStopToMoveLatch = Source.bStopToMoveLatch;
	bInLocomotionStopPhase = Source.bInLocomotionStopPhase;
	LocomotionPeakSpeedThisBurst = Source.LocomotionPeakSpeedThisBurst;
	LocomotionStopInitialTarget = Source.LocomotionStopInitialTarget;
	LocomotionStopDistanceConsumed = Source.LocomotionStopDistanceConsumed;
	CachedStopMotionEndTime = Source.CachedStopMotionEndTime;
	DistanceMatchingDistance = Source.DistanceMatchingDistance;
	DistanceMatchingStartSpeed = Source.DistanceMatchingStartSpeed;
	DistanceMatchingDelta = Source.DistanceMatchingDelta;
	DistanceMatchingStopToTarget = Source.DistanceMatchingStopToTarget;
	DistanceMatchingStopExplicitTime = Source.DistanceMatchingStopExplicitTime;
	bActiveStopAnimHasDistanceCurve = Source.bActiveStopAnimHasDistanceCurve;
	RootYawOffset = Source.RootYawOffset;
	YawDeltaThisFrame = Source.YawDeltaThisFrame;
	bTransition_TurnInPlace = Source.bTransition_TurnInPlace;
	TurnInPlaceDirection = Source.TurnInPlaceDirection;
	bInTurnInPlacePhase = Source.bInTurnInPlacePhase;
	TurnInPlaceExplicitTime = Source.TurnInPlaceExplicitTime;
	TurnInPlaceAnimDegrees = Source.TurnInPlaceAnimDegrees;
	CachedTurnAnimPlayLength = Source.CachedTurnAnimPlayLength;
	bWasInTurnInPlacePhasePreviousFrame = Source.bWasInTurnInPlacePhasePreviousFrame;
	StrideWarpingAlpha = Source.StrideWarpingAlpha;
	StrideScale = Source.StrideScale;
	LocomotionPlayRate = Source.LocomotionPlayRate;
	StrideWarpingDirection = Source.StrideWarpingDirection;
}

void UUDFAnimInstance::SyncDirectionalLocomotionFromPrimaryMesh()
{
	const USkeletalMeshComponent* const Skel = GetSkelMeshComponent();
	if (!Skel)
	{
		return;
	}
	const UUDFAnimInstance* const Primary = Cast<UUDFAnimInstance>(Skel->GetAnimInstance());
	if (!Primary || Primary == this)
	{
		return;
	}
	SyncTurnSetFromPrimaryMeshIfEmpty();
	CopyDirectionalLocomotionStateFrom(*Primary);
}

void UUDFAnimInstance::PropagateDirectionalLocomotionToLinkedAnimLayers()
{
	if (!IsPrimaryMeshAnimInstance())
	{
		return;
	}

	auto PropagateTo = [this](UAnimInstance* const Inst)
	{
		if (!Inst || Inst == this)
		{
			return;
		}
		if (UUDFAnimInstance* const LayerDF = Cast<UUDFAnimInstance>(Inst))
		{
			LayerDF->SyncTurnSetFromPrimaryMeshIfEmpty();
			LayerDF->CopyDirectionalLocomotionStateFrom(*this);
		}
	};

	// Matches "Default Shared Group" in Anim Layer Interface (editor label).
	static const FName DefaultSharedGroup(TEXT("DefaultSharedGroup"));
	TArray<UAnimInstance*> LinkedByGroup;
	GetLinkedAnimLayerInstancesByGroup(DefaultSharedGroup, LinkedByGroup);
	for (UAnimInstance* const Inst : LinkedByGroup)
	{
		PropagateTo(Inst);
	}

	if (CachedLinkedWeaponLayerClass)
	{
		PropagateTo(GetLinkedAnimLayerInstanceByClass(CachedLinkedWeaponLayerClass, true));
	}
}

void UUDFAnimInstance::UpdateDirectionalLocomotion(const float DeltaSeconds)
{
	if (bIsInAir)
	{
		Gait = EDFGait::Idle;
		bIsAccelerating = false;
		bTransition_IdleToStart = false;
		bTransition_StartToLoop = false;
		bTransition_LoopToStop = false;
		bTransition_StopToIdle = false;
		bInLocomotionStopPhase = false;
		return;
	}

	const float CurrentSpeed = Speed;
	if (CurrentSpeed >= RunSpeedThreshold)
	{
		Gait = bIsSprinting ? EDFGait::Sprint : EDFGait::Run;
	}
	else if (CurrentSpeed >= WalkSpeedThreshold)
	{
		Gait = EDFGait::Walk;
	}
	else
	{
		Gait = EDFGait::Idle;
	}

	FVector InputVec = FVector::ZeroVector;
	if (DFCharacterMovement)
	{
		InputVec = DFCharacterMovement->GetLastInputVector();
	}
	const bool bHasInput = InputVec.SizeSquared2D() > 0.01f;
	const bool bMoving = CurrentSpeed > IdleSpeedDeadband;
	bIsAccelerating = bHasInput && bMoving;

	// Reset edge flags each frame; recompute below.
	bTransition_IdleToStart = false;
	bTransition_StartToLoop = false;
	bTransition_LoopToStop = false;
	bTransition_StopToIdle = false;

	// Idle → Start: true rising edge of acceleration, independent of the start-phase timer.
	// Capturing the snapshot here (and ONLY here) guarantees it fires once per movement,
	// which also drives the Distance Matching reset (see UpdateDistanceMatching).
	const bool bStartRisingEdge = bIsAccelerating && !bWasAcceleratingPreviousFrame;
	if (bStartRisingEdge)
	{
		LocomotionStartDirection = MovementDirection;
		LocomotionStartGait = (Gait > EDFGait::Idle) ? Gait : EDFGait::Walk;
		LocomotionStartElapsed = 0.f;
		LocomotionPeakSpeedThisBurst = CurrentSpeed;
		bInLocomotionStopPhase = false;
		bTransition_IdleToStart = true;
	}

	if (bIsAccelerating)
	{
		LocomotionPeakSpeedThisBurst = FMath::Max(LocomotionPeakSpeedThisBurst, CurrentSpeed);
	}

	// Start phase: still accelerating and the start timer has not reached the cap.
	const bool bInStartPhase = bIsAccelerating && LocomotionStartElapsed < StartMaxPlayTime;
	if (bInStartPhase && !bStartRisingEdge)
	{
		// Follow the settling direction and (only-upward) gait so the chosen Start clip matches
		// where the character is actually going — WITHOUT resetting the timer. Resetting here is
		// what caused the Start clip to restart mid-acceleration and never reach Loop.
		LocomotionStartDirection = MovementDirection;
		if (Gait > LocomotionStartGait)
		{
			LocomotionStartGait = Gait;
		}

		LocomotionStartElapsed += DeltaSeconds;
		if (LocomotionStartElapsed >= StartMaxPlayTime)
		{
			LocomotionStartElapsed = StartMaxPlayTime;
		}
	}

	// Start → Loop: level-triggered so the AnimBP can leave Start whenever the cap is reached
	// (covers the case where the rising-edge frame already advanced past the cap).
	bTransition_StartToLoop = bIsAccelerating && LocomotionStartElapsed >= StartMaxPlayTime;

	// Loop → Stop: latch on input release (stay true until stop distance consumed or re-accelerate).
	const bool bJustReleasedInput = !bHasInput && bWasAcceleratingPreviousFrame;
	if (bJustReleasedInput && (bMoving || LocomotionPeakSpeedThisBurst > IdleSpeedDeadband))
	{
		LocomotionStopDirection = MovementDirection;
		const float ReleaseSpeed = FMath::Max(CurrentSpeed, LocomotionPeakSpeedThisBurst);
		if (ReleaseSpeed >= RunSpeedThreshold)
		{
			LocomotionStopGait = bIsSprinting ? EDFGait::Sprint : EDFGait::Run;
		}
		else if (ReleaseSpeed >= WalkSpeedThreshold)
		{
			LocomotionStopGait = EDFGait::Walk;
		}
		else
		{
			LocomotionStopGait = EDFGait::Idle;
		}

		float StopDistanceSpan = AuthoredStopDistance;
		if (const UAnimSequenceBase* const PendingStopAnim = ActiveAnimSet.ResolveLocomotionStop(
			LocomotionStopGait, LocomotionStopDirection))
		{
			if (const UAnimSequence* const StopSeq = Cast<UAnimSequence>(PendingStopAnim))
			{
				static const FName DistanceCurveName(TEXT("Distance"));
				const float PlayLength = StopSeq->GetPlayLength();
				const float DistStart = StopSeq->EvaluateCurveData(DistanceCurveName, 0.f);
				const float DistEnd = StopSeq->EvaluateCurveData(DistanceCurveName, PlayLength);
				const float Span = FMath::Abs(DistEnd - DistStart);
				if (Span > 0.5f)
				{
					StopDistanceSpan = Span;
				}
			}
		}

		const float RefSpeed = FMath::Max(RunSpeedThreshold, 1.f);
		const float SpeedRatio = FMath::Clamp(ReleaseSpeed / RefSpeed, 0.f, 1.25f);
		DistanceMatchingStopToTarget = FMath::Clamp(StopDistanceSpan * SpeedRatio, 40.f, StopDistanceSpan);
		LocomotionStopInitialTarget = DistanceMatchingStopToTarget;
		LocomotionStopDistanceConsumed = 0.f;
		DistanceMatchingStopExplicitTime = 0.f;
		bInLocomotionStopPhase = true;
	}

	if (!bIsAccelerating)
	{
		bStopToMoveLatch = false;
	}
	else if (bWasInLocomotionStopPhasePreviousFrame)
	{
		bStopToMoveLatch = true;
	}
	bTransition_StopToMove = bStopToMoveLatch;
	if (bIsAccelerating)
	{
		bInLocomotionStopPhase = false;
		DistanceMatchingStopToTarget = 0.f;
		DistanceMatchingStopExplicitTime = 0.f;
		LocomotionStopInitialTarget = 0.f;
		LocomotionStopDistanceConsumed = 0.f;
	}

	bTransition_LoopToStop = bInLocomotionStopPhase;
	bWasInLocomotionStopPhasePreviousFrame = bInLocomotionStopPhase;

	// Fully stopped: end the Stop phase and reset Start state so the next Idle→Start
	// rising edge fires cleanly.
	//
	// The Stop phase MUST end unconditionally here. The distance-matching catch-up
	// (UpdateDistanceMatching / StopTailCatchUpSeconds) only smooths the visual stop tail
	// WHILE the capsule is still sliding above IdleSpeedDeadband. Once Speed is inside the
	// deadband with no input, the character is physically idle: keeping bInLocomotionStopPhase
	// alive just because the authored Stop distance has not fully drained (which happens
	// whenever BrakingDecelerationWalking decelerates faster than the Stop clip's Distance
	// curve — e.g. the hard 4096 brake left over from a landing) holds bTransition_LoopToStop
	// true at the SAME time as bTransition_StopToIdle. The AnimBP then receives two conflicting
	// transition edges and deadlocks on the Stop state (df.LocomotionDebug: L>P=1 & P>I=1, Stop
	// anim frozen at Time=0.77/1.50). Force-clearing makes the two edges mutually exclusive.
	if (!bMoving && !bHasInput)
	{
		bTransition_StopToIdle = true;
		bTransition_LoopToStop = false;
		bTransition_StopToMove = false;
		bInLocomotionStopPhase = false;
		bStopToMoveLatch = false;
		bWasInLocomotionStopPhasePreviousFrame = false;
		// Distance-match target/time are zeroed authoritatively by the
		// `else if (!bInLocomotionStopPhase)` branch in UpdateDistanceMatching (runs right
		// after this), so no need to touch the settled Stop pose here.
		LocomotionStartElapsed = 0.f;
		LocomotionStartGait = EDFGait::Idle;
		LocomotionStartDirection = EDFMovementDirection::Forward;
		LocomotionStopGait = EDFGait::Idle;
		LocomotionPeakSpeedThisBurst = 0.f;
		LocomotionStopInitialTarget = 0.f;
		LocomotionStopDistanceConsumed = 0.f;
	}

	bWasAcceleratingPreviousFrame = bIsAccelerating;
}

void UUDFAnimInstance::UpdateDistanceMatching(const float DeltaSeconds)
{
	// Reset on idle / takeoff. Capture takeoff speed when entering Start.
	if (bTransition_IdleToStart)
	{
		DistanceMatchingAccum = 0.f;
		DistanceMatchingStartSpeed = Speed;
		DistanceMatchingStopToTarget = 0.f;
		DistanceMatchingStopExplicitTime = 0.f;
	}

	const bool bMoving = Speed > IdleSpeedDeadband;
	if (bMoving && !bIsInAir)
	{
		DistanceMatchingDelta = Speed * DeltaSeconds;
		DistanceMatchingAccum += DistanceMatchingDelta;
	}
	else
	{
		DistanceMatchingDelta = 0.f;
		if (!bMoving)
		{
			DistanceMatchingAccum = 0.f;
		}
	}
	DistanceMatchingDistance = DistanceMatchingAccum;

	// Deceleration: shrink remaining stop distance (feed Distance Match to Target / Explicit Time).
	if (bInLocomotionStopPhase && !bIsInAir)
	{
		if (!bWasInLocomotionStopPhasePreviousFrame)
		{
			LocomotionStopDistanceConsumed = 0.f;
			if (LocomotionStopInitialTarget <= KINDA_SMALL_NUMBER && DistanceMatchingStopToTarget > KINDA_SMALL_NUMBER)
			{
				LocomotionStopInitialTarget = DistanceMatchingStopToTarget;
			}
		}

		const UAnimSequence* StopSeq = Cast<UAnimSequence>(GetLocomotionStopAnim());
		if (StopSeq)
		{
			const float PlayLength = StopSeq->GetPlayLength();
			const float DistStart = StopSeq->EvaluateCurveData(DFLocomotionDistanceCurve::DistanceCurveName, 0.f);
			const float DistEnd = StopSeq->EvaluateCurveData(DFLocomotionDistanceCurve::DistanceCurveName, PlayLength);
			bActiveStopAnimHasDistanceCurve = FMath::Abs(DistEnd - DistStart) > 0.5f;
			if (PlayLength > KINDA_SMALL_NUMBER)
			{
				CachedAuthoredStopPlayLength = PlayLength;
			}
			if (bActiveStopAnimHasDistanceCurve)
			{
				CachedStopMotionEndTime = DFLocomotionDistanceCurve::FindStopMotionEndTime(StopSeq, StopCurveNearZeroCm);
			}
		}
		else
		{
			bActiveStopAnimHasDistanceCurve = false;
		}

		float Decay = DistanceMatchingDelta;
		if (Speed < StopTailCatchUpSpeedThreshold && DistanceMatchingStopToTarget > KINDA_SMALL_NUMBER)
		{
			const float CatchUpDecay = (DistanceMatchingStopToTarget / FMath::Max(StopTailCatchUpSeconds, 0.05f)) * DeltaSeconds;
			Decay = FMath::Max(Decay, CatchUpDecay);
		}

		LocomotionStopDistanceConsumed += Decay;
		if (LocomotionStopInitialTarget > KINDA_SMALL_NUMBER)
		{
			DistanceMatchingStopToTarget = FMath::Max(
				0.f, LocomotionStopInitialTarget - LocomotionStopDistanceConsumed);
		}
		else
		{
			DistanceMatchingStopToTarget = FMath::Max(0.f, DistanceMatchingStopToTarget - Decay);
		}

		if (DistanceMatchingStopToTarget <= KINDA_SMALL_NUMBER)
		{
			bInLocomotionStopPhase = false;
		}

		const float MotionEndTime = (CachedStopMotionEndTime > KINDA_SMALL_NUMBER)
			? CachedStopMotionEndTime
			: CachedAuthoredStopPlayLength;

		if (StopSeq && bActiveStopAnimHasDistanceCurve)
		{
			const float RawTime = DFLocomotionDistanceCurve::FindStopSequenceTimeForRemainingDistance(
				StopSeq, DistanceMatchingStopToTarget);
			DistanceMatchingStopExplicitTime = FMath::Min(RawTime, MotionEndTime);

			if (Speed < StopTailCatchUpSpeedThreshold && DistanceMatchingStopExplicitTime < MotionEndTime)
			{
				const float TimeGap = MotionEndTime - DistanceMatchingStopExplicitTime;
				const float TimeAdvance = (TimeGap / FMath::Max(StopTailCatchUpSeconds, 0.05f)) * DeltaSeconds;
				DistanceMatchingStopExplicitTime = FMath::Min(
					MotionEndTime, DistanceMatchingStopExplicitTime + TimeAdvance);
				const float CurveValue = StopSeq->EvaluateCurveData(
					DFLocomotionDistanceCurve::DistanceCurveName, DistanceMatchingStopExplicitTime);
				DistanceMatchingStopToTarget = FMath::Max(0.f, -CurveValue);
				if (LocomotionStopInitialTarget > KINDA_SMALL_NUMBER)
				{
					LocomotionStopDistanceConsumed = FMath::Max(
						0.f, LocomotionStopInitialTarget - DistanceMatchingStopToTarget);
				}
			}
		}
		else if (LocomotionStopInitialTarget > KINDA_SMALL_NUMBER && CachedAuthoredStopPlayLength > KINDA_SMALL_NUMBER)
		{
			const float Alpha = 1.f - FMath::Clamp(
				DistanceMatchingStopToTarget / LocomotionStopInitialTarget, 0.f, 1.f);
			DistanceMatchingStopExplicitTime = FMath::Min(Alpha * CachedAuthoredStopPlayLength, MotionEndTime);
		}

		// Release to Idle the moment the Stop clip reaches its authored motion end — do NOT wait for
		// the capsule speed to fall under IdleSpeedDeadband. The catch-up drives the explicit time to
		// the motion end while the capsule still has a small residual glide (Speed ~50-70 cm/s); if we
		// kept the Stop state until Speed≈0, the last Stop frame would freeze for ~0.1s, a visible
		// hitch right before Idle. Once the authored settle is done (and the player is not feeding new
		// movement input), end the Stop phase and raise Stop→Idle now. The residual glide is tiny
		// (a few cm) so the Idle pose blends in without noticeable foot sliding.
		const bool bStopClipFinished = MotionEndTime > KINDA_SMALL_NUMBER
			&& DistanceMatchingStopExplicitTime >= (MotionEndTime - KINDA_SMALL_NUMBER);
		const bool bReaccelerating = DFCharacterMovement
			&& DFCharacterMovement->GetLastInputVector().SizeSquared2D() > 0.01f;
		if (bStopClipFinished && !bReaccelerating)
		{
			bInLocomotionStopPhase = false;
			bTransition_LoopToStop = false;
			bTransition_StopToIdle = true;
		}
	}
	else if (!bInLocomotionStopPhase)
	{
		DistanceMatchingStopToTarget = 0.f;
		DistanceMatchingStopExplicitTime = 0.f;
		bActiveStopAnimHasDistanceCurve = false;
	}
}

void UUDFAnimInstance::UpdateStrideWarping(const float DeltaSeconds)
{
	// How much the loop must stretch each step so the feet match the capsule:
	// capsule speed / speed the loop was authored at. 1.0 = native (no warp needed).
	const float TargetScale = (AuthoredLoopSpeed > KINDA_SMALL_NUMBER) ? (Speed / AuthoredLoopSpeed) : 1.f;

	// Stride Direction (Manual mode): velocity expressed in mesh component space. While not
	// strafing the character faces velocity so this stays ~(1,0,0); when strafing it points
	// to the true travel direction. Falls back to forward when nearly stopped.
	if (const USkeletalMeshComponent* const Skel = GetSkelMeshComponent())
	{
		const FVector CompVel = Skel->GetComponentTransform().InverseTransformVectorNoScale(Velocity);
		const FVector FlatVel(CompVel.X, CompVel.Y, 0.f);
		if (FlatVel.SizeSquared() > 1.f)
		{
			StrideWarpingDirection = FlatVel.GetSafeNormal();
		}
	}

	// No stride warp during Stop — avoids half-scale feet (slow-motion look) on the settle.
	if (bInLocomotionStopPhase)
	{
		StrideWarpingAlpha = FMath::FInterpTo(StrideWarpingAlpha, 0.f, DeltaSeconds, 12.f);
		StrideScale = FMath::FInterpTo(StrideScale, 1.f, DeltaSeconds, 12.f);
		LocomotionPlayRate = 1.f;
		return;
	}

	// Engage stride warping only when moving above the min threshold and grounded.
	if (bIsInAir || Speed < StrideWarpingMinSpeed || AuthoredLoopSpeed <= KINDA_SMALL_NUMBER)
	{
		StrideWarpingAlpha = FMath::FInterpTo(StrideWarpingAlpha, 0.f, DeltaSeconds, 10.f);
		StrideScale = FMath::FInterpTo(StrideScale, 1.f, DeltaSeconds, 10.f);
		LocomotionPlayRate = 1.f;
		return;
	}
	StrideWarpingAlpha = FMath::FInterpTo(StrideWarpingAlpha, 1.f, DeltaSeconds, 8.f);
	StrideScale = FMath::FInterpTo(StrideScale, TargetScale, DeltaSeconds, 8.f);
	// Alternative to stride warping: drive the loop sequence player's Play Rate instead.
	// Use ONE of StrideScale (warp) OR LocomotionPlayRate (play rate), never both.
	LocomotionPlayRate = FMath::Clamp(TargetScale, 0.2f, 3.f);
}

void UUDFAnimInstance::UpdateTurnInPlace(const float DeltaSeconds)
{
	bTransition_TurnInPlace = false;

	ACharacter* const Owner = OwningCharacter.Get();
	if (!Owner)
	{
		YawDeltaThisFrame = 0.f;
		RootYawOffset = 0.f;
		bInTurnInPlacePhase = false;
		TurnInPlaceExplicitTime = 0.f;
		TurnInPlaceAnimDegrees = 0.f;
		TurnInPlaceDirection = 0.f;
		TurnInPlaceYawAppliedTotal = 0.f;
		return;
	}

	const float CurrentActorYaw = Owner->GetActorRotation().Yaw;
	YawDeltaThisFrame = FMath::FindDeltaAngleDegrees(PreviousActorYaw, CurrentActorYaw);
	PreviousActorYaw = CurrentActorYaw;

	const bool bGrounded = !bIsInAir && !bInLocomotionStopPhase;
	const bool bIdleEnoughToStartTurn = bGrounded && Speed <= IdleSpeedDeadband && !bIsAccelerating;
	// Do not abort a turn for Speed alone — SetActorRotation at clip end can spike Velocity.
	const bool bAbortTurnPhase = !bGrounded
		|| (!bInTurnInPlacePhase && !bIdleEnoughToStartTurn);

	auto StopHorizontalVelocityAfterTurnSnap = [this]()
	{
		if (UDFCharacterMovement)
		{
			FVector Vel = DFCharacterMovement->Velocity;
			Vel.X = 0.f;
			Vel.Y = 0.f;
			DFCharacterMovement->Velocity = Vel;
		}
	};

	auto FinishTurnPhase = [this](const TCHAR* EndReason, const bool bAllowImmediateRetrigger)
	{
		bInTurnInPlacePhase = false;
		bTransition_TurnInPlace = false;
		TurnInPlaceExplicitTime = 0.f;
		TurnInPlaceAnimDegrees = 0.f;
		TurnInPlaceDirection = 0.f;
		TurnInPlaceYawAppliedTotal = 0.f;
		bTurnInPlaceRetriggerArmed = bAllowImmediateRetrigger;
		TurnInPlaceLastEndReason = EndReason ? EndReason : TEXT("?");
	};

	if (bAbortTurnPhase)
	{
		if (bInTurnInPlacePhase)
		{
			FinishTurnPhase(TEXT("NotGrounded"), true);
		}
		else
		{
			TurnInPlaceExplicitTime = 0.f;
			TurnInPlaceAnimDegrees = 0.f;
			TurnInPlaceDirection = 0.f;
			TurnInPlaceYawAppliedTotal = 0.f;
		}
		if (!bInTurnInPlacePhase)
		{
			RootYawOffset = FMath::FInterpTo(RootYawOffset, 0.f, DeltaSeconds, TurnInPlaceYawInterpSpeed * 0.25f);
		}
		bWasInTurnInPlacePhasePreviousFrame = bInTurnInPlacePhase;
		if (!bInTurnInPlacePhase)
		{
			return;
		}
	}

	// While a turn plays, keep the latched offset; refreshing from AimYaw every frame fights consumption.
	if (!bInTurnInPlacePhase)
	{
		RootYawOffset = AimYaw;
		if (FMath::Abs(RootYawOffset) < TurnInPlaceRetriggerYaw)
		{
			bTurnInPlaceRetriggerArmed = true;
		}
	}

	if (bInTurnInPlacePhase)
	{
		const float LatchedDir = FMath::IsNearlyZero(TurnInPlaceDirection)
			? FMath::Sign(RootYawOffset)
			: TurnInPlaceDirection;
		const UAnimSequence* TurnSeq = Cast<UAnimSequence>(GetLocomotionTurnAnim());
		if (TurnSeq)
		{
			const float PlayLength = TurnSeq->GetPlayLength();
			if (PlayLength > KINDA_SMALL_NUMBER)
			{
				CachedTurnAnimPlayLength = PlayLength;
			}
			const float AbsDeg = TurnInPlaceAnimDegrees > KINDA_SMALL_NUMBER
				? TurnInPlaceAnimDegrees
				: 90.f;
			const float DegPerSec = AbsDeg / FMath::Max(CachedTurnAnimPlayLength, 0.01f);
			const float ConsumedYaw = LatchedDir * DegPerSec * DeltaSeconds;
			TurnInPlaceExplicitTime += DeltaSeconds;

			if (bTurnInPlaceApplyActorYawFromCode && Owner && !FMath::IsNearlyZero(ConsumedYaw))
			{
				FRotator R = Owner->GetActorRotation();
				R.Yaw += ConsumedYaw;
				Owner->SetActorRotation(R);
				ConsumeRootYawOffset(ConsumedYaw);
			}

			const bool bClipDone = TurnInPlaceExplicitTime >= CachedTurnAnimPlayLength - 0.03f;
			const bool bPastMinTime = TurnInPlaceExplicitTime >= TurnInPlaceMinPhaseTime;
			const bool bOffsetDone = bTurnInPlaceApplyActorYawFromCode && bPastMinTime
				&& FMath::Abs(RootYawOffset) <= TurnInPlaceCompleteYaw;
			if (bClipDone || bOffsetDone)
			{
				// Anim-only path (Fab clips, no RM): rotate the pawn once when the clip finishes.
				if (bClipDone && !bTurnInPlaceApplyActorYawFromCode && Owner)
				{
					const float RemainingBefore = RootYawOffset;
					const float MaxApply = FMath::Min(AbsDeg, FMath::Abs(RemainingBefore));
					const float YawToApply = LatchedDir * MaxApply;
					if (!FMath::IsNearlyZero(YawToApply))
					{
						FRotator R = Owner->GetActorRotation();
						R.Yaw += YawToApply;
						Owner->SetActorRotation(R);
						ConsumeRootYawOffset(YawToApply);
						StopHorizontalVelocityAfterTurnSnap();
					}
				}

				const float Remaining = RootYawOffset;
				if (Owner && TurnInPlacePostTurnSnapMaxYaw > KINDA_SMALL_NUMBER
					&& FMath::Abs(Remaining) > TurnInPlaceCompleteYaw
					&& FMath::Abs(Remaining) <= TurnInPlacePostTurnSnapMaxYaw)
				{
					FRotator R = Owner->GetActorRotation();
					R.Yaw += Remaining;
					Owner->SetActorRotation(R);
					ConsumeRootYawOffset(Remaining);
					StopHorizontalVelocityAfterTurnSnap();
				}
				FinishTurnPhase(bOffsetDone ? TEXT("OffsetDone") : TEXT("ClipDone"), false);
				RootYawOffset = AimYaw;
				bTurnInPlaceRetriggerArmed = FMath::Abs(RootYawOffset) < TurnInPlaceRetriggerYaw;
			}
		}
		else
		{
			FinishTurnPhase(TEXT("NoClip"), true);
		}
		bWasInTurnInPlacePhasePreviousFrame = bInTurnInPlacePhase;
		return;
	}

	if (bIdleEnoughToStartTurn && bTurnInPlaceRetriggerArmed
		&& FMath::Abs(RootYawOffset) > TurnInPlaceTriggerYaw)
	{
		EnsureActiveAnimSetTurnSet();
		const float DirSign = bInvertTurnInPlaceDirection ? -1.f : 1.f;
		const float AbsOff = FMath::Abs(RootYawOffset);
		TurnInPlaceDirection = FMath::Sign(RootYawOffset) * DirSign;
		const bool bUse180 = AbsOff >= TurnInPlace180Threshold;
		TurnInPlaceAnimDegrees = bUse180 ? 180.f : 90.f;
		const bool bTurnRight = TurnInPlaceDirection > 0.f;
		if (!ActiveAnimSet.ResolveLocomotionTurn(bUse180, bTurnRight))
		{
#if !UE_BUILD_SHIPPING
			if (!bWarnedMissingTurnSet)
			{
				bWarnedMissingTurnSet = true;
				UE_LOG(LogDungeonForged, Warning,
					TEXT("[TIP] %s: |RootYawOff|=%.0f but Turn Set is empty on main — fill Turn Set on ABP_Test_UnArmed_Layer (Default Anim Set / Overrides) or Sword_and_Shield/09_Turn; recompile C++ so main inherits layer Turn Set."),
					*GetClass()->GetName(), RootYawOffset);
			}
#endif
			return;
		}
		bTransition_TurnInPlace = true;
		bInTurnInPlacePhase = true;
		TurnInPlaceExplicitTime = 0.f;
		TurnInPlaceYawAppliedTotal = 0.f;
	}

	bWasInTurnInPlacePhasePreviousFrame = bInTurnInPlacePhase;
}

void UUDFAnimInstance::ConsumeRootYawOffset(const float ConsumedYaw)
{
	RootYawOffset -= ConsumedYaw;
	RootYawOffset = FMath::UnwindDegrees(RootYawOffset);
}

#if !UE_BUILD_SHIPPING
namespace DFLocoDebugDraw
{
static void DrawGroundCircle(const UWorld* World, const FVector& Center, const float Radius, const FColor& Color,
	const float ZOffset, const int32 Segments, const float Thickness)
{
	if (!World || Radius <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	const FVector Base(Center.X, Center.Y, Center.Z + ZOffset);
	FVector Prev = Base + FVector(Radius, 0.f, 0.f);
	for (int32 i = 1; i <= Segments; ++i)
	{
		const float Angle = (2.f * UE_PI) * static_cast<float>(i) / static_cast<float>(Segments);
		const FVector Next = Base + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
		DrawDebugLine(World, Prev, Next, Color, false, -1.f, 0, Thickness);
		Prev = Next;
	}
}

static void DrawGroundYawArc(const UWorld* World, const FVector& Center, const float Radius,
	const float CenterYawDeg, const float SweepDeg, const FColor& Color, const float Thickness)
{
	if (!World || FMath::IsNearlyZero(SweepDeg) || Radius <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	const FVector Base(Center.X, Center.Y, Center.Z + 2.f);
	const int32 Segments = FMath::Clamp(FMath::RoundToInt(FMath::Abs(SweepDeg) / 8.f), 4, 48);
	const float StartRad = FMath::DegreesToRadians(CenterYawDeg);
	const float StepRad = FMath::DegreesToRadians(SweepDeg) / static_cast<float>(Segments);
	FVector Prev = Base;
	for (int32 i = 0; i <= Segments; ++i)
	{
		const float Angle = StartRad + StepRad * static_cast<float>(i);
		const FVector Offset(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
		const FVector Point = Base + Offset;
		if (i > 0)
		{
			DrawDebugLine(World, Prev, Point, Color, false, -1.f, 0, Thickness);
		}
		Prev = Point;
	}
}

static FVector YawToFlatVector(const float YawDeg)
{
	const float Rad = FMath::DegreesToRadians(YawDeg);
	return FVector(FMath::Cos(Rad), FMath::Sin(Rad), 0.f);
}
} // namespace DFLocoDebugDraw
#endif

void UUDFAnimInstance::DrawTurnInPlaceDebug(const UWorld* World) const
{
#if !UE_BUILD_SHIPPING
	if (!World || !OwningCharacter)
	{
		return;
	}

	const FVector Origin = OwningCharacter->GetActorLocation();
	const float ActorYaw = OwningCharacter->GetActorRotation().Yaw;
	const float Radius = DFTurnInPlaceDebug::GetCircleRadiusCm();
	const float Z = 3.f;

	DFLocoDebugDraw::DrawGroundCircle(World, Origin, Radius, FColor(180, 180, 180, 140), Z, 32, 1.5f);

	const FVector BodyFwd = OwningCharacter->GetActorForwardVector().GetSafeNormal2D();
	const FVector DesiredFwd = DFLocoDebugDraw::YawToFlatVector(ActorYaw + RootYawOffset);
	const float Sweep = FMath::Clamp(RootYawOffset, -180.f, 180.f);

	const FColor WedgeColor = FMath::Abs(RootYawOffset) >= TurnInPlaceTriggerYaw
		? FColor::Cyan
		: FColor(255, 200, 80, 200);
	DFLocoDebugDraw::DrawGroundYawArc(World, Origin, Radius * 0.92f, ActorYaw, Sweep, WedgeColor, 3.f);

	DrawDebugDirectionalArrow(World, Origin + FVector(0, 0, 12.f), Origin + FVector(0, 0, 12.f) + BodyFwd * 95.f,
		22.f, FColor::Blue, false, -1.f, 0, 2.5f);
	DrawDebugDirectionalArrow(World, Origin + FVector(0, 0, 14.f), Origin + FVector(0, 0, 14.f) + DesiredFwd * 110.f,
		26.f, FColor::Orange, false, -1.f, 0, 3.f);

	const float TriggerSign = RootYawOffset >= 0.f ? 1.f : -1.f;
	const float TriggerEdgeYaw = ActorYaw + TriggerSign * TurnInPlaceTriggerYaw;
	const FVector TriggerEdge = DFLocoDebugDraw::YawToFlatVector(TriggerEdgeYaw);
	DrawDebugLine(World, Origin + FVector(0, 0, Z), Origin + FVector(0, 0, Z) + TriggerEdge * (Radius + 15.f),
		FColor(255, 80, 80, 180), false, -1.f, 0, 2.f);

	if (bInTurnInPlacePhase || bTransition_TurnInPlace)
	{
		DFLocoDebugDraw::DrawGroundCircle(World, Origin, Radius * 1.05f, FColor::Magenta, Z + 1.f, 40, 4.f);
		const float TurnSweep = TurnInPlaceDirection * FMath::Min(TurnInPlaceAnimDegrees, FMath::Abs(RootYawOffset) + 5.f);
		DFLocoDebugDraw::DrawGroundYawArc(World, Origin, Radius * 0.75f, ActorYaw, TurnSweep, FColor::Green, 5.f);
		const FVector TurnEnd = DFLocoDebugDraw::YawToFlatVector(ActorYaw + TurnSweep);
		DrawDebugDirectionalArrow(World, Origin + FVector(0, 0, 18.f), Origin + FVector(0, 0, 18.f) + TurnEnd * 70.f,
			20.f, FColor::Green, false, -1.f, 0, 3.5f);
	}

	const FString Label = FString::Printf(TEXT("TIP off=%.0f trig=%.0f %s"),
		RootYawOffset, TurnInPlaceTriggerYaw,
		bInTurnInPlacePhase ? TEXT("TURN") : (FMath::Abs(RootYawOffset) >= TurnInPlaceTriggerYaw ? TEXT("READY") : TEXT("idle")));
	DrawDebugString(World, Origin + FVector(0, 0, 110.f), Label, nullptr, FColor::White, 0.f, true, 1.1f);
#endif
}

void UUDFAnimInstance::UpdateFootPlantCurves()
{
	// Anim notify curves named FootPlantCurveLeft/Right should be authored on Walk/Run cycles.
	// Threshold 0.5: above = planted, below = free. Falls back to false if curves are missing.
	const float L = GetCurveValue(FootPlantCurveLeft);
	const float R = GetCurveValue(FootPlantCurveRight);
	bLeftFootPlanted = L > 0.5f;
	bRightFootPlanted = R > 0.5f;
}

void UUDFAnimInstance::UpdateAimOffsetBlend(const float DeltaSeconds)
{
	const bool bShouldAim = bAimOffsetRequested || bShouldStrafe;
	const float Target = bShouldAim ? 1.f : 0.f;
	AimOffsetAlpha = FMath::FInterpTo(AimOffsetAlpha, Target, DeltaSeconds, AimOffsetInterpSpeed);
}

UAnimSequenceBase* UUDFAnimInstance::GetLocomotionStartAnim() const
{
	const EDFGait StartGait = (LocomotionStartGait != EDFGait::Idle) ? LocomotionStartGait : Gait;
	return ActiveAnimSet.ResolveLocomotionStart(StartGait, LocomotionStartDirection);
}

UAnimSequenceBase* UUDFAnimInstance::GetLocomotionLoopAnim() const
{
	return ActiveAnimSet.ResolveLocomotionLoop(Gait, MovementDirection);
}

UAnimSequenceBase* UUDFAnimInstance::GetLocomotionStopAnim() const
{
	const EDFGait StopGait = (LocomotionStopGait != EDFGait::Idle) ? LocomotionStopGait : Gait;
	return ActiveAnimSet.ResolveLocomotionStop(StopGait, LocomotionStopDirection);
}

UAnimSequenceBase* UUDFAnimInstance::GetLocomotionIdleAnim() const
{
	return ActiveAnimSet.ResolveLocomotionIdle();
}

UAnimSequenceBase* UUDFAnimInstance::GetLocomotionTurnAnim() const
{
	bool bUse180 = false;
	bool bTurnRight = false;
	if (bInTurnInPlacePhase || bTransition_TurnInPlace)
	{
		bUse180 = TurnInPlaceAnimDegrees >= 135.f;
		const float Dir = FMath::IsNearlyZero(TurnInPlaceDirection)
			? FMath::Sign(RootYawOffset)
			: TurnInPlaceDirection;
		bTurnRight = Dir > 0.f;
	}
	else
	{
		const float AbsOff = FMath::Abs(RootYawOffset);
		bUse180 = AbsOff >= TurnInPlace180Threshold;
		const float DirSign = bInvertTurnInPlaceDirection ? -1.f : 1.f;
		bTurnRight = (FMath::Sign(RootYawOffset) * DirSign) > 0.f;
	}
	return ActiveAnimSet.ResolveLocomotionTurn(bUse180, bTurnRight);
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

void UUDFAnimInstance::NotifyAirDashEndedWhileAirborne()
{
	if (!bIsInAir)
	{
		return;
	}

	bJumpArcActive = true;
	bHasPassedJumpApex = true;
	bIsLanding = false;
	LandingRecoveryTimer = 0.f;

	const float StartToLoopAt = ComputeStartToLoopTime();
	AirTime = FMath::Max(AirTime, StartToLoopAt);
	JumpLoopPhaseTime = FMath::Max(JumpLoopPhaseTime, JumpLoopLandMinPhaseTime);
	AirDashResumeFallLoopLatchTime = 0.25f;
	PlayFallLoopSlotAfterAirDash();

	DFJumpDebug::Logf(TEXT("AirDash end -> fall loop sync loopPhase=%.3fs air=%.3fs"),
		JumpLoopPhaseTime, AirTime);
}

void UUDFAnimInstance::PlayFallLoopSlotAfterAirDash()
{
	if (!bIsInAir || FallLoopOverlaySlotName == NAME_None)
	{
		return;
	}

	UAnimSequenceBase* const LoopAnim = GetJumpLoopAnim();
	if (!LoopAnim)
	{
		return;
	}

	StopFallLoopSlotOverlay();
	FallLoopOverlayMontage = PlaySlotAnimationAsDynamicMontage(
		LoopAnim, FallLoopOverlaySlotName, 0.08f, 0.12f, 1.f, 100, -1.f, 0.f);
	bFallLoopOverlayActive = FallLoopOverlayMontage != nullptr;
	if (bFallLoopOverlayActive)
	{
		DFJumpDebug::Logf(TEXT("AirDash fall loop overlay slot='%s' seq='%s'"),
			*FallLoopOverlaySlotName.ToString(), *LoopAnim->GetName());
	}
}

void UUDFAnimInstance::StopFallLoopSlotOverlay()
{
	if (bFallLoopOverlayActive && FallLoopOverlaySlotName != NAME_None)
	{
		StopSlotAnimation(0.1f, FallLoopOverlaySlotName);
	}
	bFallLoopOverlayActive = false;
	FallLoopOverlayMontage = nullptr;
}

bool UUDFAnimInstance::IsPastApexExitableForJumpLoop() const
{
	if (!bHasPassedJumpApex || AirTime < JumpStartMinPlayTime)
	{
		return false;
	}

	if (VerticalVelocity < JumpApexVelocityThreshold)
	{
		return true;
	}

	return DFCharacterMovement && DFCharacterMovement->IsAirDashAltitudeLocked();
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
	const bool bPastApexExitable = IsPastApexExitableForJumpLoop();
	const bool bPastStartWindow = AirTime >= StartToLoopAt;

	bTransition_JumpStartToLoop = bJumpArcActive && bIsInAir && (
		bPastStartWindow || AirTime >= JumpStartMaxPlayTime || bPastApexExitable);

	bTransition_LocomotionToJumpLoop = bJumpArcActive && bIsInAir && bIsFalling
		&& AirDashResumeFallLoopLatchTime > 0.f;

	const bool bAirDashHangActive = bIsAirDashing
		|| (DFCharacterMovement && DFCharacterMovement->IsAirDashAltitudeLocked());
	bKeepJumpLoopWhileAirborne = bIsInAir
		&& (bAirDashHangActive || AirDashResumeFallLoopLatchTime > 0.f || bFallLoopOverlayActive);

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
	bTransition_JumpLoopToLocomotion = bTransition_JumpGroundedExit && !bKeepJumpLoopWhileAirborne;

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
	LogEdge(TEXT("Loco->JumpLoop"), bTransition_LocomotionToJumpLoop, bPrevTransition_LocoToLoop);
	LogEdge(TEXT("JumpLoop->Loco"), bTransition_JumpLoopToLocomotion, bPrevTransition_LoopToLoco);
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
		TEXT("[Jump|SM] Loco>Start=%s Loco>Loop=%s Loop>Loco=%s Start>Loop=%s Start>Land=%s Loop>Prep=%s Loop>Land=%s Land>Loco=%s GndExit=%s KeepLoop=%s\n")
		TEXT("  J=%d F=%d InAir=%d Apex=%d Arc=%d L=%d | Vz=%.0f Air=%.2fs Gnd=%.2fs PredLand=%.0f Alpha=%.2f\n")
		TEXT("  StartMin=%.2fs StartMax=%.2fs ApexRise=%.2fs ApexVz=%.0f | LoopPhase=%.2fs PrepMin=%.2fs LandMin=%.2fs\n")
		TEXT("  Blends: Loco>Start=%.2fs Loco>Loop=%.2fs Start>Loop=%.2fs Loop>Land=%.2fs Land>Loco=%.2fs | Dir=%d"),
		OnOff(bTransition_LocomotionToJumpStart),
		OnOff(bTransition_LocomotionToJumpLoop),
		OnOff(bTransition_JumpLoopToLocomotion),
		OnOff(bTransition_JumpStartToLoop),
		OnOff(bTransition_JumpStartToLand),
		OnOff(bTransition_JumpLoopToLandPrep),
		OnOff(bTransition_JumpLoopToLand),
		OnOff(bTransition_LandToLocomotion),
		OnOff(bTransition_JumpGroundedExit),
		OnOff(bKeepJumpLoopWhileAirborne),
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
		JumpBlend_LocoToLoop,
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

const TCHAR* LocomotionGaitName(const EDFGait Gait)
{
	switch (Gait)
	{
	case EDFGait::Idle:
		return TEXT("Idle");
	case EDFGait::Walk:
		return TEXT("Walk");
	case EDFGait::Run:
		return TEXT("Run");
	case EDFGait::Sprint:
		return TEXT("Sprint");
	default:
		return TEXT("?");
	}
}

static FString BlendSpaceDebugName(const UBlendSpace* const BS)
{
	return BS ? BS->GetName() : FString(TEXT("(null)"));
}

struct FDFLocoAnimSample
{
	float PlayLength = 0.f;
	bool bRootMotion = false;
	float DistStart = 0.f;
	float DistEnd = 0.f;
	float AvgCurveSpeed = 0.f;
	bool bHasDistanceCurve = false;
};

static FDFLocoAnimSample SampleLocomotionAnim(const UAnimSequenceBase* const Anim)
{
	FDFLocoAnimSample S;
	if (!Anim)
	{
		return S;
	}
	S.PlayLength = Anim->GetPlayLength();
	if (const UAnimSequence* const Seq = Cast<UAnimSequence>(Anim))
	{
		S.bRootMotion = Seq->bEnableRootMotion;
		static const FName DistanceCurveName(TEXT("Distance"));
		S.DistStart = Seq->EvaluateCurveData(DistanceCurveName, 0.f);
		S.DistEnd = Seq->EvaluateCurveData(DistanceCurveName, S.PlayLength);
		const float DistSpan = FMath::Abs(S.DistEnd - S.DistStart);
		S.bHasDistanceCurve = DistSpan > 0.5f;
		if (S.bHasDistanceCurve && S.PlayLength > KINDA_SMALL_NUMBER)
		{
			S.AvgCurveSpeed = DistSpan / S.PlayLength;
		}
	}
	return S;
}

static FString FormatAnimSampleLine(const TCHAR* const Label, const UAnimSequenceBase* const Anim)
{
	const FDFLocoAnimSample S = SampleLocomotionAnim(Anim);
	if (!Anim)
	{
		return FString::Printf(TEXT("%s: (null)"), Label);
	}
	if (S.bHasDistanceCurve)
	{
		return FString::Printf(TEXT("%s: %s len=%.2fs RM=%d Dist %.0f->%.0f avgSpd~%.0f"),
			Label, *Anim->GetName(), S.PlayLength, S.bRootMotion ? 1 : 0,
			S.DistStart, S.DistEnd, S.AvgCurveSpeed);
	}
	return FString::Printf(TEXT("%s: %s len=%.2fs RM=%d (no Distance curve)"),
		Label, *Anim->GetName(), S.PlayLength, S.bRootMotion ? 1 : 0);
}
} // namespace

void UUDFAnimInstance::TryAutoTuneAuthoredLoopSpeedFromDefaultRunLoop()
{
	if (!bAutoTuneAuthoredLoopSpeedFromRunLoop)
	{
		return;
	}
	UAnimSequenceBase* const LoopAnim = ActiveAnimSet.ResolveLocomotionLoop(EDFGait::Run, EDFMovementDirection::Forward);
	if (!LoopAnim)
	{
		return;
	}
	if (const UAnimSequence* const Seq = Cast<UAnimSequence>(LoopAnim))
	{
		static const FName DistanceCurveName(TEXT("Distance"));
		const float PlayLength = Seq->GetPlayLength();
		const float DistStart = Seq->EvaluateCurveData(DistanceCurveName, 0.f);
		const float DistEnd = Seq->EvaluateCurveData(DistanceCurveName, PlayLength);
		if (PlayLength > KINDA_SMALL_NUMBER && FMath::Abs(DistEnd - DistStart) > 0.5f)
		{
			AuthoredLoopSpeed = FMath::Abs(DistEnd - DistStart) / PlayLength;
		}
	}
}

void UUDFAnimInstance::TryAutoTuneAuthoredStopDistanceFromDefaultRunStop()
{
	if (!bAutoTuneAuthoredStopDistanceFromRunStop)
	{
		return;
	}
	UAnimSequenceBase* const StopAnim = ActiveAnimSet.ResolveLocomotionStop(EDFGait::Run, EDFMovementDirection::Forward);
	if (!StopAnim)
	{
		return;
	}
	if (const UAnimSequence* const Seq = Cast<UAnimSequence>(StopAnim))
	{
		static const FName DistanceCurveName(TEXT("Distance"));
		const float PlayLength = Seq->GetPlayLength();
		const float DistStart = Seq->EvaluateCurveData(DistanceCurveName, 0.f);
		const float DistEnd = Seq->EvaluateCurveData(DistanceCurveName, PlayLength);
		const float DistSpan = FMath::Abs(DistEnd - DistStart);
		if (PlayLength > KINDA_SMALL_NUMBER)
		{
			CachedAuthoredStopPlayLength = PlayLength;
		}
		if (DistSpan > 0.5f)
		{
			AuthoredStopDistance = DistSpan;
			CachedStopMotionEndTime = DFLocomotionDistanceCurve::FindStopMotionEndTime(Seq, StopCurveNearZeroCm);
		}
	}
}

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

FString UUDFAnimInstance::BuildTurnInPlaceDebugString() const
{
	const UAnimSequence* const TurnSeq = Cast<UAnimSequence>(GetLocomotionTurnAnim());
	const bool bRm = TurnSeq && TurnSeq->HasRootMotion();
	const float AbsOff = FMath::Abs(RootYawOffset);
	float ActorYaw = 0.f;
	float CtrlYaw = 0.f;
	if (const ACharacter* const Ch = OwningCharacter.Get())
	{
		ActorYaw = Ch->GetActorRotation().Yaw;
		if (const AController* const C = Ch->GetController())
		{
			CtrlYaw = C->GetControlRotation().Yaw;
		}
	}
	const bool bGrounded = !bIsInAir && !bInLocomotionStopPhase;
	const bool bIdleEnoughToStartTurn = bGrounded && Speed <= IdleSpeedDeadband && !bIsAccelerating;
	const bool bReady = bIdleEnoughToStartTurn && bTurnInPlaceRetriggerArmed
		&& AbsOff > TurnInPlaceTriggerYaw;

	FString Out;
	Out += FString::Printf(TEXT("== TurnInPlace %s =="), *GetClass()->GetName());
	Out += FString::Printf(TEXT("\noff=%.1f aim=%.1f |off|=%.0f trig=%.0f rearm=%.0f 180=%.0f"),
		RootYawOffset, AimYaw, AbsOff, TurnInPlaceTriggerYaw, TurnInPlaceRetriggerYaw, TurnInPlace180Threshold);
	Out += FString::Printf(TEXT("\nTrans=%d Phase=%d armed=%d ready=%d idleOk=%d"),
		bTransition_TurnInPlace ? 1 : 0, bInTurnInPlacePhase ? 1 : 0,
		bTurnInPlaceRetriggerArmed ? 1 : 0, bReady ? 1 : 0, bIdleEnoughToStartTurn ? 1 : 0);
	Out += FString::Printf(TEXT("\nspd=%.0f accel=%d abortSpd=%.0f"),
		Speed, bIsAccelerating ? 1 : 0, TurnInPlaceAbortSpeed);
	Out += FString::Printf(TEXT("\nlatched dir=%+.0f deg=%.0f time=%.2f/%.2fs codeYaw=%d end=%s"),
		TurnInPlaceDirection, TurnInPlaceAnimDegrees, TurnInPlaceExplicitTime, CachedTurnAnimPlayLength,
		bTurnInPlaceApplyActorYawFromCode ? 1 : 0, *TurnInPlaceLastEndReason);
	Out += FString::Printf(TEXT("\nactorYaw=%.0f ctrlYaw=%.0f yawDeltaFrame=%.1f"),
		ActorYaw, CtrlYaw, YawDeltaThisFrame);
	Out += FString::Printf(TEXT("\nclip=%s RM=%d turnSet=%d idle=%s"),
		*GetNameSafe(GetLocomotionTurnAnim()), bRm ? 1 : 0, ActiveAnimSet.TurnSet.IsValid() ? 1 : 0,
		*GetNameSafe(GetLocomotionIdleAnim()));
	if (IsPrimaryMeshAnimInstance())
	{
		static const FName DefaultSharedGroup(TEXT("DefaultSharedGroup"));
		TArray<UAnimInstance*> LinkedByGroup;
		GetLinkedAnimLayerInstancesByGroup(DefaultSharedGroup, LinkedByGroup);
		for (UAnimInstance* const Inst : LinkedByGroup)
		{
			if (const UUDFAnimInstance* const LayerDF = Cast<UUDFAnimInstance>(Inst))
			{
				Out += FString::Printf(TEXT("\nlayer=%s layerPhase=%d layerDir=%+.0f layerTime=%.2f"),
					*LayerDF->GetClass()->GetName(),
					LayerDF->bInTurnInPlacePhase ? 1 : 0,
					LayerDF->TurnInPlaceDirection,
					LayerDF->TurnInPlaceExplicitTime);
				break;
			}
		}
	}
	return Out;
}

FString UUDFAnimInstance::BuildTurnInPlaceDebugOneLiner() const
{
	const float AbsOff = FMath::Abs(RootYawOffset);
	const bool bIdleEnoughToStartTurn = !bIsInAir && !bInLocomotionStopPhase
		&& Speed <= IdleSpeedDeadband && !bIsAccelerating;
	const bool bReady = bIdleEnoughToStartTurn && bTurnInPlaceRetriggerArmed
		&& AbsOff > TurnInPlaceTriggerYaw;
	return FString::Printf(
		TEXT("[TIP|1] %s off=%.1f aim=%.1f |off|=%.0f trans=%d phase=%d armed=%d ready=%d idle=%d spd=%.0f accel=%d dir=%+.0f deg=%.0f t=%.2f/%.2f clip=%s end=%s codeYaw=%d"),
		*GetClass()->GetName(),
		RootYawOffset, AimYaw, AbsOff,
		bTransition_TurnInPlace ? 1 : 0, bInTurnInPlacePhase ? 1 : 0,
		bTurnInPlaceRetriggerArmed ? 1 : 0, bReady ? 1 : 0, bIdleEnoughToStartTurn ? 1 : 0,
		Speed, bIsAccelerating ? 1 : 0,
		TurnInPlaceDirection, TurnInPlaceAnimDegrees,
		TurnInPlaceExplicitTime, CachedTurnAnimPlayLength,
		*GetNameSafe(GetLocomotionTurnAnim()), *TurnInPlaceLastEndReason,
		bTurnInPlaceApplyActorYawFromCode ? 1 : 0);
}

FString UUDFAnimInstance::BuildDirectionalLocomotionDebugString() const
{
	const UAnimSequenceBase* const StartA = GetLocomotionStartAnim();
	const UAnimSequenceBase* const LoopA = GetLocomotionLoopAnim();
	const UAnimSequenceBase* const StopA = GetLocomotionStopAnim();
	const bool bPrimary = IsPrimaryMeshAnimInstance();

	FString Out;
	Out += FString::Printf(TEXT("== Locomotion [%s] %s =="),
		bPrimary ? TEXT("MAIN") : TEXT("LAYER"), *GetClass()->GetName());
	Out += FString::Printf(TEXT("\nSpeed=%.0f Gait=%s Dir=%s Accel=%d Strafe=%d"),
		Speed, LocomotionGaitName(Gait), LocomotionDirName(MovementDirection),
		bIsAccelerating ? 1 : 0, bShouldStrafe ? 1 : 0);
	Out += FString::Printf(TEXT("\nStartDir=%s StartGait=%s StopDir=%s StopGait=%s StartElapsed=%.2f/%.2fs"),
		LocomotionDirName(LocomotionStartDirection), LocomotionGaitName(LocomotionStartGait),
		LocomotionDirName(LocomotionStopDirection), LocomotionGaitName(LocomotionStopGait),
		LocomotionStartElapsed, StartMaxPlayTime);
	Out += FString::Printf(TEXT("\nTrans: Idle>Start=%d Start>Loop=%d Loop>Stop=%d Stop>Move=%d Stop>Idle=%d TIP=%d Turn=%d"),
		bTransition_IdleToStart ? 1 : 0, bTransition_StartToLoop ? 1 : 0,
		bTransition_LoopToStop ? 1 : 0, bTransition_StopToMove ? 1 : 0, bTransition_StopToIdle ? 1 : 0,
		bTransition_TurnInPlace ? 1 : 0, bInTurnInPlacePhase ? 1 : 0);
	Out += FString::Printf(TEXT("\nAnim Idle=%s"), *GetNameSafe(GetLocomotionIdleAnim()));
	Out += FString::Printf(TEXT("\nAnim Turn =%s (%.0f° L/R=%+.0f) Time=%.2f/%.2fs"),
		*GetNameSafe(GetLocomotionTurnAnim()), TurnInPlaceAnimDegrees, TurnInPlaceDirection,
		TurnInPlaceExplicitTime, CachedTurnAnimPlayLength);
	Out += FString::Printf(TEXT("\nAnim Start=%s"), *GetNameSafe(StartA));
	Out += FString::Printf(TEXT("\nAnim Loop =%s"), *GetNameSafe(LoopA));
	Out += FString::Printf(TEXT("\nAnim Stop =%s"), *GetNameSafe(StopA));
	Out += FString::Printf(TEXT("\nTIP RootYawOff=%.1f AimYaw=%.1f Trig>=%.0f 180>=%.0f"),
		RootYawOffset, AimYaw, TurnInPlaceTriggerYaw, TurnInPlace180Threshold);
	const float StopInitDisplay = (LocomotionStopInitialTarget > KINDA_SMALL_NUMBER)
		? LocomotionStopInitialTarget
		: AuthoredStopDistance;
	const float MotionEndDisplay = (CachedStopMotionEndTime > KINDA_SMALL_NUMBER)
		? CachedStopMotionEndTime
		: CachedAuthoredStopPlayLength;
	Out += FString::Printf(
		TEXT("\nStrideAlpha=%.2f DistMatch=%.0f Delta=%.1f Stop=%.0f/%.0f(consumed %.0f) Time=%.2f/%.2f(end %.2f) DistCurve=%d"),
		StrideWarpingAlpha, DistanceMatchingDistance, DistanceMatchingDelta, DistanceMatchingStopToTarget,
		StopInitDisplay, LocomotionStopDistanceConsumed, DistanceMatchingStopExplicitTime,
		CachedAuthoredStopPlayLength, MotionEndDisplay, bActiveStopAnimHasDistanceCurve ? 1 : 0);
	return Out;
}

FString UUDFAnimInstance::BuildDirectionalLocomotionDeepDebugString() const
{
	const UAnimSequenceBase* const LoopA = GetLocomotionLoopAnim();
	const FDFLocoAnimSample LoopS = SampleLocomotionAnim(LoopA);
	const float CurveStrideScale = (LoopS.AvgCurveSpeed > KINDA_SMALL_NUMBER) ? (Speed / LoopS.AvgCurveSpeed) : 0.f;

	float MaxWS = 0.f;
	float CmcRun = 0.f;
	float CmcSprint = 0.f;
	float InputMag = 0.f;
	if (DFCharacterMovement)
	{
		MaxWS = DFCharacterMovement->MaxWalkSpeed;
		CmcRun = DFCharacterMovement->RunSpeed;
		CmcSprint = DFCharacterMovement->SprintSpeed;
		InputMag = DFCharacterMovement->GetLastInputVector().Size2D();
	}

	FString Out;
	Out += FString::Printf(TEXT("\n--- Deep (capsule vs anim) ---"));
	Out += FString::Printf(TEXT("\nVelXY=%.0f Dir=%.1f Input=%.2f MaxWS=%.0f RunCfg=%.0f SprintCfg=%.0f Sprint=%d"),
		Speed, Direction, InputMag, MaxWS, CmcRun, CmcSprint, bIsSprinting ? 1 : 0);
	Out += FString::Printf(TEXT("\nGaitThr Walk>=%.0f Run>=%.0f | AuthoredLoop=%.0f StrideScale=%.2f (vs curve %.2f)"),
		WalkSpeedThreshold, RunSpeedThreshold, AuthoredLoopSpeed, StrideScale, CurveStrideScale);
	Out += FString::Printf(TEXT("\nDistMatchStartSpd=%.0f YawDelta=%.1f RootYawOff=%.1f TurnPhase=%d"),
		DistanceMatchingStartSpeed, YawDeltaThisFrame, RootYawOffset, bInTurnInPlacePhase ? 1 : 0);
	Out += FString::Printf(TEXT("\n%s"), *FormatAnimSampleLine(TEXT("Loop"), LoopA));
	Out += FString::Printf(TEXT("\n%s"), *FormatAnimSampleLine(TEXT("Start"), GetLocomotionStartAnim()));
	Out += FString::Printf(TEXT("\n%s"), *FormatAnimSampleLine(TEXT("Stop"), GetLocomotionStopAnim()));

	if (LoopS.bHasDistanceCurve && AuthoredLoopSpeed > KINDA_SMALL_NUMBER
		&& FMath::Abs(LoopS.AvgCurveSpeed - AuthoredLoopSpeed) > 25.f)
	{
		Out += FString::Printf(TEXT("\nWARN AuthoredLoopSpeed(%.0f) != loop Distance avg(%.0f) — tune Class Defaults"),
			AuthoredLoopSpeed, LoopS.AvgCurveSpeed);
	}
	if (MaxWS > KINDA_SMALL_NUMBER && Speed > WalkSpeedThreshold
		&& FMath::Abs(Speed - MaxWS) < 60.f && LoopS.bHasDistanceCurve
		&& FMath::Abs(LoopS.AvgCurveSpeed - MaxWS) > 50.f)
	{
		Out += FString::Printf(TEXT("\nWARN Capsule~MaxWS(%.0f) but loop curve~%.0f — stride/RM mismatch"),
			MaxWS, LoopS.AvgCurveSpeed);
	}
	return Out;
}
#endif
