// Source/DungeonForged/Private/UI/ClassSelection/UDFClassPreviewRotatorComponent.cpp
#include "UI/ClassSelection/UDFClassPreviewRotatorComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Actor.h"

UDFClassPreviewRotatorComponent::UDFClassPreviewRotatorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UDFClassPreviewRotatorComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AActor* const O = GetOwner())
	{
		CurrentYaw = TargetYaw = O->GetActorRotation().Yaw;
	}
}

void UDFClassPreviewRotatorComponent::AddYawInput(const float Delta)
{
	bUserInteracting = true;
	IdleTimer = 0.f;
	TargetYaw = FMath::Clamp(TargetYaw + Delta * 0.5f, -180.f, 180.f);
}

void UDFClassPreviewRotatorComponent::AddZoomInput(const float Delta)
{
	if (!SpringArm)
	{
		return;
	}
	const float NewLen = FMath::Clamp(
		SpringArm->TargetArmLength + Delta * 40.f, MinArmLength, MaxArmLength);
	SpringArm->TargetArmLength = NewLen;
}

void UDFClassPreviewRotatorComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AActor* const Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	if (bUserInteracting)
	{
		CurrentYaw = FMath::FInterpTo(CurrentYaw, TargetYaw, DeltaTime, YawLerpSpeed);
		if (FMath::IsNearlyEqual(CurrentYaw, TargetYaw, 0.1f))
		{
			bUserInteracting = false;
		}
		IdleTimer = 0.f;
	}
	else
	{
		IdleTimer += DeltaTime;
		if (IdleTimer > IdleThreshold)
		{
			TargetYaw = FMath::UnwindDegrees(TargetYaw + AutoRotateSpeed * DeltaTime);
		}
		CurrentYaw = FMath::FInterpTo(CurrentYaw, TargetYaw, DeltaTime, YawLerpSpeed);
	}
	CurrentYaw = FMath::UnwindDegrees(CurrentYaw);
	const FRotator R = Owner->GetActorRotation();
	Owner->SetActorRotation(FRotator(R.Pitch, CurrentYaw, R.Roll));
}
