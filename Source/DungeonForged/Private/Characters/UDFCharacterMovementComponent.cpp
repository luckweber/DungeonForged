// Source/DungeonForged/Private/Characters/UDFCharacterMovementComponent.cpp
#include "Characters/UDFCharacterMovementComponent.h"

#include "Characters/ADFPlayerCharacter.h"
#include "Combat/DFDodgeDebug.h"
#include "Combat/DFJumpDebug.h"
#include "Combat/UDFComboComponent.h"
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
}

void UDFCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyJumpTuningFromDataAsset();
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
	}
	JumpZVelocity = DFJumpZVelocity;
	AirControl = DFAirControl;
	GravityScale = DFGravityScale;
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

bool UDFCharacterMovementComponent::DoJump(const bool bReplayingMoves)
{
	if (GetJumpCooldownRemaining() > 0.f)
	{
		DFJumpDebug::Log(TEXT("DoJump SKIP cooldown"));
		return false;
	}

	if (DFJumpStaminaCost > 0.f && CharacterOwner)
	{
		if (IAbilitySystemInterface* const IAS = Cast<IAbilitySystemInterface>(CharacterOwner))
		{
			if (UAbilitySystemComponent* const ASC = IAS->GetAbilitySystemComponent())
			{
				if (UDFAttributeSet* const Attrs = const_cast<UDFAttributeSet*>(ASC->GetSet<UDFAttributeSet>()))
				{
					if (Attrs->GetStamina() < DFJumpStaminaCost)
					{
						DFJumpDebug::Logf(TEXT("DoJump SKIP stamina=%.1f need=%.1f"),
							Attrs->GetStamina(), DFJumpStaminaCost);
						return false;
					}
					Attrs->SetStamina(Attrs->GetStamina() - DFJumpStaminaCost);
				}
			}
		}
	}

	const bool bOk = Super::DoJump(bReplayingMoves);
	if (bOk)
	{
		if (UWorld* const W = GetWorld())
		{
			TimeLastJump = W->GetTimeSeconds();
		}
		bAirDodgeUsedThisJump = false;
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
		if (ASC)
		{
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
		bJumpFallingTagActive = false;
		if (UWorld* const W = GetWorld())
		{
			TimeLastLanded = W->GetTimeSeconds();
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
			AddJumpLooseTagOnce(ASC, FDFGameplayTags::State_Landing);
		}
		if (UWorld* const W = GetWorld())
		{
			W->GetTimerManager().ClearTimer(TimerHandle_EndLanding);
			W->GetTimerManager().SetTimer(
				TimerHandle_EndLanding,
				[this]()
				{
					if (!CharacterOwner)
					{
						return;
					}
					// Restore normal braking when landing window expires.
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
				DFLandingRecoveryWindow,
				false);
		}
		DFJumpDebug::Log(TEXT("Landed — State.Landing window started"));
	}
}

void UDFCharacterMovementComponent::TickComponent(
	float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshMaxWalkSpeed();
	if (bIsSprinting)
	{
		TickSprintStamina(DeltaTime);
	}

	if (MovementMode == MOVE_Falling)
	{
		const float ZVel = Velocity.Z;
		GravityScale = (ZVel < 0.f) ? (DFGravityScale * DFFallGravityMultiplier) : DFGravityScale;

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
}

uint8 FSavedMove_DF::GetCompressedFlags() const
{
	uint8 F = FSavedMove_Character::GetCompressedFlags();
	if (bWantsSprint)
	{
		F |= static_cast<uint8>(FSavedMove_Character::FLAG_Custom_0);
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
		return bWantsSprint == B->bWantsSprint;
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
