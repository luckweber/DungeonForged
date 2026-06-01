// Source/DungeonForged/Private/Characters/UDFCharacterMovementComponent.cpp
#include "Characters/UDFCharacterMovementComponent.h"

#include "Characters/ADFPlayerCharacter.h"
#include "Combat/DFAirDashDebug.h"
#include "Combat/DFDodgeDebug.h"
#include "Combat/DFJumpDebug.h"
#include "Combat/UDFComboComponent.h"
#include "Combat/UDFCombatCrowdControlComponent.h"
#include "DFAssetManager.h"
#include "Data/UDFCombatTuningData.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameFramework/RootMotionSource.h"
#include "GameplayEffect.h"
#include "TimerManager.h"

UDFCharacterMovementComponent::UDFCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	MaxWalkSpeed = RunSpeed;
	MaxWalkSpeedCrouched = CrouchSpeed;
	DefaultBrakingFrictionFactor = BrakingFrictionFactor;
	// Snappy but smooth turn toward movement (hack-and-slash / action third-person).
	RotationRate = FRotator(0.f, 720.f, 0.f);
	JumpZVelocity = DFJumpZVelocity;
	AirControl = DFAirControl;
	GravityScale = DFGravityScale;

	// ── Locomotion stop glide ────────────────────────────────────────────
	// UE's default braking (GroundFriction 8 × BrakingFrictionFactor 2 = friction 16) stops the
	// capsule in ~6 cm — far short of the ~202 cm the Stop animations (root-motion, distance-matched)
	// are authored to cover. The result is a hard, animation-less "brusco" stop. Use a pure, separate
	// braking deceleration so the capsule glides the authored distance and the Stop clip actually
	// plays with planted feet. Tune via WalkStopBrakingDeceleration.
	bUseSeparateBrakingFriction = true;
	BrakingFriction = 0.f;
	BrakingDecelerationWalking = WalkStopBrakingDeceleration;
}

void UDFCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyJumpTuningFromDataAsset();

	// Re-assert the locomotion stop-glide braking after Blueprint/DataAsset class defaults are
	// applied (the constructor runs before them, so an edited WalkStopBrakingDeceleration would
	// otherwise be ignored). This is also the value the landing brake restores back to.
	bUseSeparateBrakingFriction = true;
	BrakingFriction = 0.f;
	BrakingDecelerationWalking = WalkStopBrakingDeceleration;
	NormalBrakingDecelerationWalking = WalkStopBrakingDeceleration;
}

void UDFCharacterMovementComponent::ClearLooseGameplayTagAll(UAbilitySystemComponent* const ASC, const FGameplayTag& Tag) const
{
	if (!ASC || !Tag.IsValid())
	{
		return;
	}
	ASC->SetLooseGameplayTagCount(Tag, 0);
}

void UDFCharacterMovementComponent::ClearJumpAirborneLooseTags(UAbilitySystemComponent* const ASC) const
{
	ClearLooseGameplayTagAll(ASC, FDFGameplayTags::State_Jumping);
	ClearLooseGameplayTagAll(ASC, FDFGameplayTags::State_Falling);
}

void UDFCharacterMovementComponent::ClearJumpLandingLooseTag(UAbilitySystemComponent* const ASC) const
{
	ClearLooseGameplayTagAll(ASC, FDFGameplayTags::State_Landing);
}

void UDFCharacterMovementComponent::SyncJumpLooseTagsWhileGrounded(UAbilitySystemComponent* const ASC)
{
	if (!ASC)
	{
		return;
	}
	ClearJumpAirborneLooseTags(ASC);
	bJumpFallingTagActive = false;

	const UWorld* const W = GetWorld();
	if (!W || TimeLastLanded < 0.f)
	{
		ClearJumpLandingLooseTag(ASC);
		return;
	}
	if ((W->GetTimeSeconds() - TimeLastLanded) >= DFLandingRecoveryWindow)
	{
		ClearJumpLandingLooseTag(ASC);
	}
}

void UDFCharacterMovementComponent::AddJumpLooseTagOnce(UAbilitySystemComponent* const ASC, const FGameplayTag& Tag) const
{
	if (!ASC || !Tag.IsValid() || ASC->HasMatchingGameplayTag(Tag))
	{
		return;
	}
	ASC->AddLooseGameplayTag(Tag);
}

void UDFCharacterMovementComponent::ApplyJumpTuningFromDataAsset()
{
	if (const UDFCombatTuningData* const Tuning = UDFAssetManager::GetCombatTuningDataSafe())
	{
		DFJumpZVelocity = Tuning->JumpZVelocity;
		DFAirControl = Tuning->JumpAirControl;
		DFGravityScale = Tuning->JumpGravityScale;
		DFFallGravityMultiplier = Tuning->JumpFallGravityMultiplier;
		DFJumpStaminaCost = Tuning->JumpStaminaCost;
		DFJumpCooldown = Tuning->JumpCooldown;
		DFLandingRecoveryWindow = Tuning->JumpLandingRecoveryWindow;
		CoyoteTime = Tuning->JumpCoyoteTime;
		JumpApexCutScale = Tuning->JumpApexCutScale;
		SprintJumpHorizontalBoost = Tuning->SprintJumpHorizontalBoost;
		JumpBufferGroundDistance = Tuning->JumpBufferGroundDistance;
		DFDoubleJumpStaminaCost = Tuning->DoubleJumpStaminaCost;
		DFDoubleJumpZScale = Tuning->DoubleJumpZScale;
		AirDashDistance = Tuning->AirDashDistance;
		AirDashDuration = Tuning->AirDashDuration;
		AirDashCooldown = Tuning->AirDashCooldown;
		AirDashLandingRecoverySkipWindow = Tuning->AirDashLandingRecoverySkipWindow;
		if (Tuning->DodgeCooldown > 0.f)
		{
			DodgeCooldown = Tuning->DodgeCooldown;
		}
		if (Tuning->DodgeIFrameDuration > 0.f)
		{
			IFrameDuration = Tuning->DodgeIFrameDuration;
		}
	}
	JumpZVelocity = DFJumpZVelocity;
	AirControl = DFAirControl;
	GravityScale = DFGravityScale;
}

bool UDFCharacterMovementComponent::IsWithinCoyoteWindow() const
{
	if (!bCoyoteFromLedgeDrop || TimeLastLeftGround < 0.f)
	{
		return false;
	}
	const UWorld* const W = GetWorld();
	if (!W || MovementMode != MOVE_Falling)
	{
		return false;
	}
	return (W->GetTimeSeconds() - TimeLastLeftGround) <= CoyoteTime;
}

bool UDFCharacterMovementComponent::IsFallingNearGround(const float MaxGroundDistance) const
{
	if (!IsFalling() || !CharacterOwner)
	{
		return false;
	}
	const float Threshold = MaxGroundDistance > 0.f ? MaxGroundDistance : JumpBufferGroundDistance;
	const FVector Start = CharacterOwner->GetActorLocation() + FVector(0.f, 0.f, 40.f);
	const FVector End = Start - FVector(0.f, 0.f, 5000.f);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(JumpBufferGround), false, CharacterOwner.Get());
	if (CharacterOwner->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		const float Dist = FMath::Max(0.f, Start.Z - Hit.ImpactPoint.Z);
		return Dist <= Threshold;
	}
	return false;
}

void UDFCharacterMovementComponent::NotifyAirDashPerformed()
{
	if (UWorld* const W = GetWorld())
	{
		TimeLastAirDash = W->GetTimeSeconds();
	}
	bAirDodgeUsedThisJump = true;
}

float UDFCharacterMovementComponent::GetAirDashCooldownRemaining() const
{
	const UWorld* const W = GetWorld();
	if (!W || TimeLastAirDash < 0.f || AirDashCooldown <= 0.f)
	{
		return 0.f;
	}
	return FMath::Max(0.f, AirDashCooldown - (W->GetTimeSeconds() - TimeLastAirDash));
}

void UDFCharacterMovementComponent::BeginAirDashAltitudeLock(const float LockedWorldZ)
{
	bAirDashAltitudeLockActive = true;
	AirDashLockedWorldZ = LockedWorldZ;
	GravityScale = 0.f;
	Velocity.Z = 0.f;
}

void UDFCharacterMovementComponent::ClearAirDashAltitudeLock()
{
	bAirDashAltitudeLockActive = false;
	AirDashLockedWorldZ = 0.f;
}

void UDFCharacterMovementComponent::BeginAirDashDrive(const FVector& DirWorld, const float Distance,
	const float Duration, const float LockedWorldZ)
{
	BeginAirDashAltitudeLock(LockedWorldZ);
	bAirDashDriveActive = true;
	AirDashDriveDir = DirWorld.GetSafeNormal2D();
	AirDashDriveSpeed = Distance / FMath::Max(Duration, 0.01f);
	if (UWorld* const W = GetWorld())
	{
		AirDashDriveEndTime = W->GetTimeSeconds() + Duration;
	}
	SavedAirDashMaxAcceleration = MaxAcceleration;
	MaxAcceleration = 0.f;
	Velocity = AirDashDriveDir * AirDashDriveSpeed;
	Velocity.Z = 0.f;
	DFAirDashDebug::Logf(TEXT("Drive start dir=%s speed=%.0f dur=%.2fs"),
		*AirDashDriveDir.ToCompactString(), AirDashDriveSpeed, Duration);
}

void UDFCharacterMovementComponent::EndAirDashDriveImpulse()
{
	if (!bAirDashDriveActive)
	{
		return;
	}

	bAirDashDriveActive = false;
	AirDashDriveDir = FVector::ZeroVector;
	AirDashDriveSpeed = 0.f;
	AirDashDriveEndTime = -1.f;
	Velocity.X = 0.f;
	Velocity.Y = 0.f;
	Velocity.Z = 0.f;
	DFAirDashDebug::Log(TEXT("Drive impulse end — hang (altitude lock until ability ends)"));
}

void UDFCharacterMovementComponent::EndAirDashDrive(const float ExitVelocityRetain)
{
	if (!bAirDashDriveActive && !bAirDashAltitudeLockActive)
	{
		return;
	}

	const float Retain = FMath::Clamp(ExitVelocityRetain, 0.f, 1.f);
	FVector ExitVel = Velocity;
	ExitVel.Z = 0.f;
	if (Retain <= KINDA_SMALL_NUMBER)
	{
		ExitVel = FVector::ZeroVector;
	}
	else if (!FMath::IsNearlyEqual(Retain, 1.f))
	{
		ExitVel *= Retain;
	}
	Velocity.X = ExitVel.X;
	Velocity.Y = ExitVel.Y;

	if (IsFalling() && FMath::Abs(Velocity.Z) <= 1.f)
	{
		Velocity.Z = -150.f;
	}

	bAirDashDriveActive = false;
	AirDashDriveDir = FVector::ZeroVector;
	AirDashDriveSpeed = 0.f;
	AirDashDriveEndTime = -1.f;
	MaxAcceleration = SavedAirDashMaxAcceleration > 0.f ? SavedAirDashMaxAcceleration : MaxAcceleration;
	ClearAirDashAltitudeLock();
	DFAirDashDebug::Logf(TEXT("Drive end retain=%.2f vel=%s"), Retain, *Velocity.ToCompactString());
}

bool UDFCharacterMovementComponent::TryConsumeStaminaForJumpCost(const float Cost) const
{
	if (Cost <= 0.f || !CharacterOwner)
	{
		return true;
	}
	if (IAbilitySystemInterface* const IAS = Cast<IAbilitySystemInterface>(CharacterOwner))
	{
		if (UAbilitySystemComponent* const ASC = IAS->GetAbilitySystemComponent())
		{
			if (UDFAttributeSet* const Attrs = const_cast<UDFAttributeSet*>(ASC->GetSet<UDFAttributeSet>()))
			{
				if (Attrs->GetStamina() < Cost)
				{
					DFJumpDebug::Logf(TEXT("DoJump SKIP stamina=%.1f need=%.1f"), Attrs->GetStamina(), Cost);
					return false;
				}
				Attrs->SetStamina(Attrs->GetStamina() - Cost);
			}
		}
	}
	return true;
}

void UDFCharacterMovementComponent::ApplySprintJumpMomentumBoost()
{
	if (bIsSprinting && SprintJumpHorizontalBoost > 1.f)
	{
		Velocity.X *= SprintJumpHorizontalBoost;
		Velocity.Y *= SprintJumpHorizontalBoost;
	}
}

float UDFCharacterMovementComponent::GetJumpCooldownRemaining() const
{
	const UWorld* const W = GetWorld();
	if (!W || TimeLastJump < 0.f)
	{
		return 0.f;
	}
	return FMath::Max(0.f, DFJumpCooldown - (W->GetTimeSeconds() - TimeLastJump));
}

bool UDFCharacterMovementComponent::RequestJump(const bool bReplayingMoves)
{
	return DoJump(bReplayingMoves);
}

bool UDFCharacterMovementComponent::DoJump(const bool bReplayingMoves)
{
	if (GetJumpCooldownRemaining() > 0.f)
	{
		DFJumpDebug::Log(TEXT("DoJump SKIP cooldown"));
		return false;
	}

	ACharacter* const Char = Cast<ACharacter>(CharacterOwner);
	if (!Char)
	{
		return false;
	}

	const bool bCoyote = IsWithinCoyoteWindow();
	const bool bDoubleJump = IsFalling() && Char->JumpCurrentCount >= 1 && !bCoyote;
	const bool bGroundedJump = IsMovingOnGround() || MovementMode == MOVE_Walking;

	if (!bGroundedJump && !bCoyote && !bDoubleJump)
	{
		DFJumpDebug::Log(TEXT("DoJump SKIP not grounded/coyote/double"));
		return false;
	}

	const float StaminaCost = bDoubleJump ? DFDoubleJumpStaminaCost : DFJumpStaminaCost;
	if (!TryConsumeStaminaForJumpCost(StaminaCost))
	{
		return false;
	}

	auto FinishJumpImpulse = [this, Char]() -> bool
	{
		ApplySprintJumpMomentumBoost();
		if (UWorld* const W = GetWorld())
		{
			TimeLastJump = W->GetTimeSeconds();
		}
		if (!IsFalling())
		{
			bAirDodgeUsedThisJump = false;
		}
		bCoyoteFromLedgeDrop = false;
		DFJumpDebug::Logf(TEXT("DoJump OK JumpZ=%.0f Vz=%.0f"), JumpZVelocity, Velocity.Z);
		return true;
	};

	if (bDoubleJump)
	{
		Velocity.Z = 0.f;
		const float ImpulseZ = JumpZVelocity * DFDoubleJumpZScale;
		Velocity.Z = ImpulseZ;
		Char->JumpCurrentCount++;
		if (IAbilitySystemInterface* const IAS = Cast<IAbilitySystemInterface>(Char))
		{
			if (UAbilitySystemComponent* const ASC = IAS->GetAbilitySystemComponent())
			{
				ClearLooseGameplayTagAll(ASC, FDFGameplayTags::State_DoubleJumping);
				AddJumpLooseTagOnce(ASC, FDFGameplayTags::State_DoubleJumping);
				ClearJumpAirborneLooseTags(ASC);
				AddJumpLooseTagOnce(ASC, FDFGameplayTags::State_Jumping);
				bJumpFallingTagActive = false;
			}
		}
		return FinishJumpImpulse();
	}

	if (bCoyote)
	{
		Velocity.Z = FMath::Max(Velocity.Z, JumpZVelocity);
		Char->JumpCurrentCount = FMath::Max(1, Char->JumpCurrentCount + 1);
		if (IAbilitySystemInterface* const IAS = Cast<IAbilitySystemInterface>(Char))
		{
			if (UAbilitySystemComponent* const ASC = IAS->GetAbilitySystemComponent())
			{
				ClearJumpAirborneLooseTags(ASC);
				AddJumpLooseTagOnce(ASC, FDFGameplayTags::State_Jumping);
				bJumpFallingTagActive = false;
			}
		}
		return FinishJumpImpulse();
	}

	const bool bOk = Super::DoJump(bReplayingMoves);
	if (bOk)
	{
		ApplySprintJumpMomentumBoost();
		if (UWorld* const W = GetWorld())
		{
			TimeLastJump = W->GetTimeSeconds();
		}
		bAirDodgeUsedThisJump = false;
		bCoyoteFromLedgeDrop = false;
		DFJumpDebug::Logf(TEXT("DoJump OK JumpZ=%.0f"), JumpZVelocity);
	}
	return bOk;
}

void UDFCharacterMovementComponent::SetStrafeMode(const bool bStrafe)
{
	if (bIsStrafing == bStrafe)
	{
		return;
	}
	bIsStrafing = bStrafe;
	bOrientRotationToMovement = !bStrafe;
	bUseControllerDesiredRotation = bStrafe;
	BrakingFrictionFactor = bStrafe ? 2.f : DefaultBrakingFrictionFactor;
}

FNetworkPredictionData_Client* UDFCharacterMovementComponent::GetPredictionData_Client() const
{
	if (ClientPredictionData == nullptr)
	{
		UDFCharacterMovementComponent* const MutableThis = const_cast<UDFCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_DF(*this);
	}
	return ClientPredictionData;
}

void UDFCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	const bool bSprintWanted = (Flags & static_cast<uint8>(FSavedMove_Character::FLAG_Custom_0)) != 0;
	SetSprinting(bSprintWanted, true);
}

void UDFCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	const EMovementMode NewMode = MovementMode;
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	OnDFMovementModeChanged.Broadcast(NewMode, PreviousMovementMode, PreviousCustomMode);

	if (!CharacterOwner)
	{
		return;
	}

	IAbilitySystemInterface* const IAS = Cast<IAbilitySystemInterface>(CharacterOwner);
	UAbilitySystemComponent* const ASC = IAS ? IAS->GetAbilitySystemComponent() : nullptr;

	if (PreviousMovementMode == MOVE_Walking && NewMode == MOVE_Falling)
	{
		bJumpFallingTagActive = false;
		UWorld* const W = GetWorld();
		if (W)
		{
			if (Velocity.Z <= 1.f)
			{
				TimeLastLeftGround = W->GetTimeSeconds();
				bCoyoteFromLedgeDrop = true;
			}
			else
			{
				bCoyoteFromLedgeDrop = false;
			}
		}
		if (ASC)
		{
			// Chain-jumps via buffer can leave State.Landing from the previous arc — clear on any takeoff/leave-ground.
			ClearJumpLandingLooseTag(ASC);
			if (Velocity.Z > 1.f)
			{
				if (W)
				{
					W->GetTimerManager().ClearTimer(TimerHandle_EndLanding);
				}
				if (bIsApplyingLandingBrake && NormalBrakingDecelerationWalking >= 0.f)
				{
					BrakingDecelerationWalking = NormalBrakingDecelerationWalking;
					bIsApplyingLandingBrake = false;
				}
			}

			// Going up after jump → Jumping; ledge drop / step-off → Falling directly.
			if (Velocity.Z > 1.f)
			{
				AddJumpLooseTagOnce(ASC, FDFGameplayTags::State_Jumping);
			}
			else
			{
				ClearJumpAirborneLooseTags(ASC);
				AddJumpLooseTagOnce(ASC, FDFGameplayTags::State_Falling);
				bJumpFallingTagActive = true;
			}
		}
		if (ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(CharacterOwner))
		{
			if (UDFComboComponent* const Combo = PC->Combo)
			{
				const bool bPreserve = Combo->IsInCancelWindow() || Combo->HasAerialContinuation();
				if (!bPreserve)
				{
					Combo->RequestDeferredReset(0.35f);
				}
			}
		}
	}
	else if (PreviousMovementMode == MOVE_Falling && NewMode == MOVE_Walking)
	{
		bAirDodgeUsedThisJump = false;
		EndAirDashDrive(AirDashExitVelocityRetain);
		bJumpFallingTagActive = false;
		bCoyoteFromLedgeDrop = false;
		if (ASC)
		{
			ClearLooseGameplayTagAll(ASC, FDFGameplayTags::State_DoubleJumping);
			ClearLooseGameplayTagAll(ASC, FDFGameplayTags::State_AirDashing);
		}
		if (UWorld* const W = GetWorld())
		{
			TimeLastLanded = W->GetTimeSeconds();
		}
		if (UDFCombatCrowdControlComponent* const CC =
			CharacterOwner->FindComponentByClass<UDFCombatCrowdControlComponent>())
		{
			CC->OnOwnerLanded();
		}
		if (ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(CharacterOwner))
		{
			if (UDFComboComponent* const Combo = PC->Combo)
			{
				Combo->OnOwnerLanded();
			}
		}

		float LandingRecoveryDuration = DFLandingRecoveryWindow;
		if (UWorld* const W = GetWorld())
		{
			if (TimeLastAirDash >= 0.f
				&& (W->GetTimeSeconds() - TimeLastAirDash) <= AirDashLandingRecoverySkipWindow)
			{
				LandingRecoveryDuration = 0.f;
				DFJumpDebug::Log(TEXT("Landed — skip recovery (recent air dash)"));
			}
		}

		// ── Landing impulse damping ──────────────────────────────────────────
		// Shed momentum on touch-down to prevent the "slide" after a forward jump.
		// Without this, Velocity.XY stays at runtime speed (e.g. 540 cm/s) and decays
		// only via BrakingDecelerationWalking — visually the character slides ~0.26s.
		if (LandingHorizontalVelocityRetain < 1.f)
		{
			Velocity.X *= LandingHorizontalVelocityRetain;
			Velocity.Y *= LandingHorizontalVelocityRetain;
		}
		// Bump braking deceleration during the recovery window so even if input is held,
		// the player feels the "stick the landing" snap. Reverted in the EndLanding timer.
		if (NormalBrakingDecelerationWalking < 0.f)
		{
			NormalBrakingDecelerationWalking = BrakingDecelerationWalking;
		}
		BrakingDecelerationWalking = LandingBrakingDeceleration;
		bIsApplyingLandingBrake = true;

		if (ASC)
		{
			ClearJumpAirborneLooseTags(ASC);
			ClearJumpLandingLooseTag(ASC);
			if (LandingRecoveryDuration > 0.f)
			{
				AddJumpLooseTagOnce(ASC, FDFGameplayTags::State_Landing);
			}
		}
		if (UWorld* const W = GetWorld())
		{
			W->GetTimerManager().ClearTimer(TimerHandle_EndLanding);
			if (LandingRecoveryDuration > 0.f)
			{
				W->GetTimerManager().SetTimer(
					TimerHandle_EndLanding,
					[this]()
					{
						if (!CharacterOwner)
						{
							return;
						}
						if (bIsApplyingLandingBrake && NormalBrakingDecelerationWalking >= 0.f)
						{
							BrakingDecelerationWalking = NormalBrakingDecelerationWalking;
							bIsApplyingLandingBrake = false;
						}
						if (IAbilitySystemInterface* const I = Cast<IAbilitySystemInterface>(CharacterOwner))
						{
							if (UAbilitySystemComponent* const A = I->GetAbilitySystemComponent())
							{
								ClearJumpLandingLooseTag(A);
							}
						}
					},
					LandingRecoveryDuration,
					false);
				DFJumpDebug::Log(TEXT("Landed — State.Landing window started"));
			}
			else if (bIsApplyingLandingBrake && NormalBrakingDecelerationWalking >= 0.f)
			{
				BrakingDecelerationWalking = NormalBrakingDecelerationWalking;
				bIsApplyingLandingBrake = false;
			}
		}
	}
}

void UDFCharacterMovementComponent::TickComponent(
	float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bAirDashDriveActive && GetWorld())
	{
		if (GetWorld()->GetTimeSeconds() >= AirDashDriveEndTime)
		{
			EndAirDashDriveImpulse();
		}
		else
		{
			Velocity = AirDashDriveDir * AirDashDriveSpeed;
			Velocity.Z = 0.f;
			GravityScale = 0.f;
		}
	}

	if (bAirDashAltitudeLockActive && UpdatedComponent)
	{
		FVector Loc = UpdatedComponent->GetComponentLocation();
		if (!FMath::IsNearlyEqual(Loc.Z, AirDashLockedWorldZ, 0.5f))
		{
			Loc.Z = AirDashLockedWorldZ;
			UpdatedComponent->SetWorldLocation(Loc, false, nullptr, ETeleportType::None);
		}
		Velocity.Z = 0.f;
		GravityScale = 0.f;
		DFAirDashDebug::DrawPathPoint(GetWorld(), Loc, FColor::Orange);
	}

	RefreshMaxWalkSpeed();
	if (bIsSprinting)
	{
		TickSprintStamina(DeltaTime);
	}

	if (MovementMode == MOVE_Falling)
	{
		const float ZVel = Velocity.Z;
		if (!bAirDashAltitudeLockActive)
		{
			GravityScale = (ZVel < 0.f) ? (DFGravityScale * DFFallGravityMultiplier) : DFGravityScale;
		}

		if (!bJumpFallingTagActive && ZVel <= 1.f && CharacterOwner)
		{
			if (IAbilitySystemInterface* const AirIAS = Cast<IAbilitySystemInterface>(CharacterOwner))
			{
				if (UAbilitySystemComponent* const AirASC = AirIAS->GetAbilitySystemComponent())
				{
					ClearJumpAirborneLooseTags(AirASC);
					AddJumpLooseTagOnce(AirASC, FDFGameplayTags::State_Falling);
					bJumpFallingTagActive = true;
				}
			}
		}
	}
	else if (MovementMode == MOVE_Walking)
	{
		GravityScale = DFGravityScale;
		if (CharacterOwner)
		{
			if (IAbilitySystemInterface* const GroundIAS = Cast<IAbilitySystemInterface>(CharacterOwner))
			{
				if (UAbilitySystemComponent* const GroundASC = GroundIAS->GetAbilitySystemComponent())
				{
					SyncJumpLooseTagsWhileGrounded(GroundASC);
				}
			}
		}
	}
}

void UDFCharacterMovementComponent::SetSprintStaminaFromGameplayEffect(bool bFromEffect)
{
	bSprintStaminaFromGameplayEffect = bFromEffect;
}

void UDFCharacterMovementComponent::SetSprinting(const bool bSprint, const bool /*bFromNetworkPrediction*/)
{
	bIsSprinting = bSprint;
	RefreshMaxWalkSpeed();
}

void UDFCharacterMovementComponent::RefreshMaxWalkSpeed()
{
	if (IsCrouching())
	{
		MaxWalkSpeedCrouched = CrouchSpeed;
		return;
	}
	MaxWalkSpeed = bIsSprinting ? SprintSpeed : RunSpeed;
}

void UDFCharacterMovementComponent::TickSprintStamina(const float DeltaTime)
{
	if (bSprintStaminaFromGameplayEffect || !bIsSprinting)
	{
		if (IAbilitySystemInterface* IAS = Cast<IAbilitySystemInterface>(CharacterOwner))
		{
			if (UAbilitySystemComponent* ASC = IAS->GetAbilitySystemComponent())
			{
				const FGameplayAttribute S = UDFAttributeSet::GetStaminaAttribute();
				if (S.IsValid() && ASC->GetNumericAttribute(S) <= 0.01f)
				{
					SetSprinting(false, false);
					ApplySprintExhaustionIfAny();
				}
			}
		}
		return;
	}

	IAbilitySystemInterface* const IAS = Cast<IAbilitySystemInterface>(CharacterOwner);
	if (!IAS)
	{
		return;
	}
	UAbilitySystemComponent* const ASC = IAS->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}
	UDFAttributeSet* const Attrs = const_cast<UDFAttributeSet*>(ASC->GetSet<UDFAttributeSet>());
	if (!Attrs)
	{
		return;
	}
	const float Current = Attrs->GetStamina();
	const float Next = FMath::Max(0.f, Current - SprintStaminaDrain * DeltaTime);
	Attrs->SetStamina(Next);
	if (Next <= 0.01f)
	{
		SetSprinting(false, false);
		ApplySprintExhaustionIfAny();
	}
}

void UDFCharacterMovementComponent::ApplySprintExhaustionIfAny()
{
	if (!SprintExhaustionEffect)
	{
		return;
	}
	IAbilitySystemInterface* const IAS = Cast<IAbilitySystemInterface>(CharacterOwner);
	if (!IAS)
	{
		return;
	}
	UAbilitySystemComponent* const ASC = IAS->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}
	if (!ASC->GetOwner() || !ASC->GetOwner()->HasAuthority())
	{
		return;
	}
	const FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(SprintExhaustionEffect, 1.f, Ctx);
	if (Spec.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

float UDFCharacterMovementComponent::GetDodgeCooldownRemaining() const
{
	const UWorld* const W = GetWorld();
	if (!W || TimeLastDodge < 0.f)
	{
		return 0.f;
	}
	return FMath::Max(0.f, DodgeCooldown - (W->GetTimeSeconds() - TimeLastDodge));
}

FVector UDFCharacterMovementComponent::GetDodgeDirection() const
{
	FVector D = GetLastInputVector();
	if (D.SizeSquared() > 0.01f)
	{
		return D.GetSafeNormal();
	}
	if (CharacterOwner)
	{
		return -CharacterOwner->GetActorForwardVector();
	}
	return FVector::ForwardVector;
}

void UDFCharacterMovementComponent::PerformDodge(const FVector& DirectionWorld, const bool bApplyProgrammaticDisplacement)
{
	if (!CharacterOwner)
	{
		return;
	}
	if (IAbilitySystemInterface* const ASI = Cast<IAbilitySystemInterface>(CharacterOwner))
	{
		if (UAbilitySystemComponent* const ASC = ASI->GetAbilitySystemComponent())
		{
			if (FDFGameplayTags::State_Exhausted.IsValid() && ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Exhausted))
			{
				DFDodgeDebug::Log(TEXT("PerformDodge SKIP State.Exhausted"));
				return;
			}
		}
	}
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	const float Now = W->GetTimeSeconds();
	if (TimeLastDodge >= 0.f && (Now - TimeLastDodge) < DodgeCooldown)
	{
		DFDodgeDebug::Logf(TEXT("PerformDodge SKIP cooldown rem=%.2f"), DodgeCooldown - (Now - TimeLastDodge));
		return;
	}
	TimeLastDodge = Now;
	DFDodgeDebug::Logf(TEXT("PerformDodge OK dirWorld=%s programmaticRM=%d dist=%.0f"),
		*DirectionWorld.GetSafeNormal().ToCompactString(), bApplyProgrammaticDisplacement ? 1 : 0, DodgeDistance);
	FVector Dir = DirectionWorld;
	if (Dir.IsNearlyZero())
	{
		Dir = GetDodgeDirection();
	}
	else
	{
		Dir = Dir.GetSafeNormal();
	}

	IAbilitySystemInterface* const IAS = Cast<IAbilitySystemInterface>(CharacterOwner);
	if (UAbilitySystemComponent* ASC = IAS ? IAS->GetAbilitySystemComponent() : nullptr)
	{
		ASC->AddLooseGameplayTag(FDFGameplayTags::State_Dodging);
		ASC->AddLooseGameplayTag(FDFGameplayTags::State_Invulnerable);
	}

	bIsDodging = true;

	if (IFrameDuration > 0.f)
	{
		W->GetTimerManager().SetTimer(
			TimerHandle_EndIFrame, this, &UDFCharacterMovementComponent::EndIFrameState, IFrameDuration, false);
	}
	if (DodgeDuration > 0.f)
	{
		W->GetTimerManager().SetTimer(
			TimerHandle_EndDodging, this, &UDFCharacterMovementComponent::EndDodgingState, DodgeDuration, false);
	}

	if (bApplyProgrammaticDisplacement && UpdatedComponent)
	{
		const FVector Start = UpdatedComponent->GetComponentLocation();
		const FVector End = Start + Dir * DodgeDistance;
		const TSharedPtr<FRootMotionSource_MoveToForce> Source = MakeShared<FRootMotionSource_MoveToForce>();
		Source->InstanceName = FName(TEXT("DFDodge"));
		Source->Priority = 600;
		Source->Duration = DodgeDuration;
		Source->AccumulateMode = ERootMotionAccumulateMode::Override;
		Source->bInLocalSpace = false;
		Source->StartLocation = Start;
		Source->TargetLocation = End;
		Source->bRestrictSpeedToExpected = true;
		ApplyRootMotionSource(Source);
	}
}

void UDFCharacterMovementComponent::EndIFrameState()
{
	if (!CharacterOwner)
	{
		return;
	}
	if (IAbilitySystemInterface* const IAS = Cast<IAbilitySystemInterface>(CharacterOwner))
	{
		if (UAbilitySystemComponent* const ASC = IAS->GetAbilitySystemComponent())
		{
			ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Invulnerable, 1);
		}
	}
}

void UDFCharacterMovementComponent::EndDodgingState()
{
	bIsDodging = false;
	if (!CharacterOwner)
	{
		return;
	}
	if (IAbilitySystemInterface* const IAS = Cast<IAbilitySystemInterface>(CharacterOwner))
	{
		if (UAbilitySystemComponent* const ASC = IAS->GetAbilitySystemComponent())
		{
			ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Dodging, 1);
		}
	}
}

void FSavedMove_DF::Clear()
{
	FSavedMove_Character::Clear();
	bWantsSprint = false;
	bIsDodging = false;
	bAirDashActive = false;
}

uint8 FSavedMove_DF::GetCompressedFlags() const
{
	uint8 F = FSavedMove_Character::GetCompressedFlags();
	if (bWantsSprint)
	{
		F |= static_cast<uint8>(FSavedMove_Character::FLAG_Custom_0);
	}
	if (bIsDodging)
	{
		F |= static_cast<uint8>(FSavedMove_Character::FLAG_Custom_1);
	}
	if (bAirDashActive)
	{
		F |= static_cast<uint8>(FSavedMove_Character::FLAG_Custom_2);
	}
	return F;
}

void FSavedMove_DF::SetMoveFor(
	ACharacter* C,
	const float InDeltaTime,
	const FVector& NewAccel,
	FNetworkPredictionData_Client_Character& ClientData)
{
	FSavedMove_Character::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);
	if (C)
	{
		if (const UDFCharacterMovementComponent* const DF = Cast<UDFCharacterMovementComponent>(C->GetCharacterMovement()))
		{
			bWantsSprint = DF->bIsSprinting;
			bIsDodging = DF->bIsDodging;
			bAirDashActive = DF->IsAirDashDriveActive() || DF->IsAirDashAltitudeLocked();
		}
	}
}

void FSavedMove_DF::PrepMoveFor(ACharacter* C)
{
	FSavedMove_Character::PrepMoveFor(C);
	if (C)
	{
		if (UDFCharacterMovementComponent* const DF = Cast<UDFCharacterMovementComponent>(C->GetCharacterMovement()))
		{
			DF->SetSprinting(bWantsSprint, true);
		}
	}
}

bool FSavedMove_DF::CanCombineWith(
	const FSavedMovePtr& NewMove, ACharacter* InCharacter, const float MaxDelta) const
{
	if (!FSavedMove_Character::CanCombineWith(NewMove, InCharacter, MaxDelta))
	{
		return false;
	}
	if (const FSavedMove_DF* const B = static_cast<FSavedMove_DF*>(NewMove.Get()))
	{
		return bWantsSprint == B->bWantsSprint
			&& bIsDodging == B->bIsDodging
			&& bAirDashActive == B->bAirDashActive;
	}
	return true;
}

FNetworkPredictionData_DF::FNetworkPredictionData_DF(const UCharacterMovementComponent& ClientMovement)
	: FNetworkPredictionData_Client_Character(ClientMovement)
{
}

FSavedMovePtr FNetworkPredictionData_DF::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_DF());
}
