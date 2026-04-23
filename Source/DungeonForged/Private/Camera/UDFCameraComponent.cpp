// Source/DungeonForged/Private/Camera/UDFCameraComponent.cpp

#include "Camera/UDFCameraComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UDFCameraComponent::UDFCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	bUsePawnControlRotation = true;
	bInheritPitch = true;
	bInheritRoll = true;
	bInheritYaw = true;

	TargetArmLength = DefaultArmLength;
	BlendedStateArm = DefaultArmLength;
	StateGoalArmLength = FMath::Clamp(BlendedStateArm + UserArmOffset, MinZoom, MaxZoom);
	TargetOffset = DefaultSocketOffset;
	SmoothedSocketOffset = DefaultSocketOffset;

	// Camera lag: map user "lag" time feel to engine speed (higher = snappier)
	bEnableCameraLag = true;
	if (CameraLag > KINDA_SMALL_NUMBER)
	{
		CameraLagSpeed = 1.f / FMath::Max(CameraLag, 0.01f);
	}
	else
	{
		CameraLagSpeed = 0.f;
	}
	HandleCameraOcclusion();
}

void UDFCameraComponent::HandleCameraOcclusion()
{
	bDoCollisionTest = true;
	// Camera collision probe radius (keeps the boom from clipping through world geometry)
	ProbeSize = 12.f;
}

void UDFCameraComponent::TickComponent(
	float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TickCamera(DeltaTime);
}

void UDFCameraComponent::TickCamera(float DeltaTime)
{
	// 1) Blend combat enter/exit over CombatModeBlendDuration
	if (bCombatBlendActive)
	{
		CombatBlendElapsed += DeltaTime;
		const float Alpha = FMath::Clamp(CombatBlendElapsed / FMath::Max(CombatModeBlendDuration, 0.01f), 0.f, 1.f);
		BlendedStateArm = FMath::Lerp(CombatBlendFromArm, CombatBlendToArm, Alpha);
		if (Alpha >= 1.f - KINDA_SMALL_NUMBER)
		{
			bCombatBlendActive = false;
		}
	}
	else
	{
		switch (CurrentState)
		{
		case ECameraState::Default:
			BlendedStateArm = DefaultArmLength;
			break;
		case ECameraState::Combat:
			BlendedStateArm = CombatArmLength;
			break;
		case ECameraState::LockOn:
			BlendedStateArm = LockOnArmLength;
			break;
		}
	}

	// 2) User zoom and clamp
	ClampUserArmOffset();
	const float TargetArm = FMath::Clamp(BlendedStateArm + UserArmOffset, MinZoom, MaxZoom);
	StateGoalArmLength = TargetArm;
	const float NewArm = FMath::FInterpTo(TargetArmLength, StateGoalArmLength, DeltaTime, InterpSpeed);
	TargetArmLength = NewArm;

	// 3) Shoulder / lock-on offset
	const FVector TargetSocket = GetTargetSocketOffsetForState();
	SmoothedSocketOffset = FMath::VInterpTo(SmoothedSocketOffset, TargetSocket, DeltaTime, InterpSpeed);
	TargetOffset = SmoothedSocketOffset;

	// 4) Lock-on rotation
	if (CurrentState == ECameraState::LockOn && LockOnTarget.IsValid())
	{
		UpdateLockOnRotation(DeltaTime);
	}
}

void UDFCameraComponent::ClampUserArmOffset()
{
	UserArmOffset = FMath::Clamp(
		UserArmOffset,
		MinZoom - BlendedStateArm,
		MaxZoom - BlendedStateArm);
}

FVector UDFCameraComponent::GetTargetSocketOffsetForState() const
{
	if (CurrentState == ECameraState::LockOn)
	{
		return LockOnSocketOffset;
	}
	return DefaultSocketOffset;
}

void UDFCameraComponent::OnZoomInput(float AxisValue)
{
	if (FMath::IsNearlyZero(AxisValue))
	{
		return;
	}
	UserArmOffset += AxisValue * ZoomSpeed;
	ClampUserArmOffset();
}

void UDFCameraComponent::EnterCombatMode()
{
	if (CurrentState == ECameraState::LockOn)
	{
		return;
	}
	CurrentState = ECameraState::Combat;
	CombatBlendFromArm = TargetArmLength;
	CombatBlendToArm = CombatArmLength;
	CombatBlendElapsed = 0.f;
	bCombatBlendActive = true;
}

void UDFCameraComponent::ExitCombatMode()
{
	if (CurrentState == ECameraState::LockOn)
	{
		return;
	}
	CurrentState = ECameraState::Default;
	CombatBlendFromArm = TargetArmLength;
	CombatBlendToArm = DefaultArmLength;
	CombatBlendElapsed = 0.f;
	bCombatBlendActive = true;
}

void UDFCameraComponent::EnableLockOn(AActor* Target)
{
	if (!IsValid(Target))
	{
		return;
	}
	if (CurrentState != ECameraState::LockOn)
	{
		StateBeforeLockOn = CurrentState;
	}
	CurrentState = ECameraState::LockOn;
	LockOnTarget = Target;
	bCombatBlendActive = false;
	BlendedStateArm = LockOnArmLength;
}

void UDFCameraComponent::DisableLockOn()
{
	LockOnTarget = nullptr;
	CurrentState = StateBeforeLockOn;
	const float Goal = (StateBeforeLockOn == ECameraState::Combat) ? CombatArmLength : DefaultArmLength;
	CombatBlendFromArm = TargetArmLength;
	CombatBlendToArm = Goal;
	CombatBlendElapsed = 0.f;
	bCombatBlendActive = true;
}

void UDFCameraComponent::UpdateLockOnRotation(float DeltaTime)
{
	AActor* const Target = LockOnTarget.Get();
	APawn* const OwnerPawn = Cast<APawn>(GetOwner());
	if (!IsValid(Target) || !IsValid(OwnerPawn))
	{
		return;
	}
	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC)
	{
		return;
	}
	const FVector From = OwnerPawn->GetPawnViewLocation();
	const FVector To = Target->GetActorLocation() + FVector(0.f, 0.f, OwnerPawn->BaseEyeHeight * 0.5f);
	const FRotator LookAt = (To - From).Rotation();

	FRotator Control = PC->GetControlRotation();
	const FQuat QCur = Control.Quaternion();
	const FQuat QGoal = LookAt.Quaternion();
	const float T = 1.f - FMath::Exp(-InterpSpeed * DeltaTime);
	const FQuat QNew = FQuat::Slerp(QCur, QGoal, T);
	Control = QNew.Rotator();
	PC->SetControlRotation(Control);
}
