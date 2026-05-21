// Source/DungeonForged/Private/Combat/UDFLauncherComponent.cpp
#include "Combat/UDFLauncherComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

void UDFLauncherComponent::ApplyLaunch(AActor* const Target, const FVector& LaunchVel,
	const float TargetGravity, const float Hangtime)
{
	ACharacter* const TargetChar = Cast<ACharacter>(Target);
	if (!TargetChar)
	{
		return;
	}
	UCharacterMovementComponent* const CMC = TargetChar->GetCharacterMovement();
	if (!CMC)
	{
		return;
	}

	const AActor* const Owner = GetOwner();
	const FVector WorldVel = Owner
		? Owner->GetActorTransform().TransformVectorNoScale(LaunchVel)
		: LaunchVel;
	TargetChar->LaunchCharacter(WorldVel, true, true);

	if (TargetGravity > 0.f && TargetGravity < 1.f && Hangtime > 0.f)
	{
		const float Prior = CMC->GravityScale;
		CMC->GravityScale = TargetGravity;
		FTimerHandle TH;
		if (UWorld* const W = GetWorld())
		{
			W->GetTimerManager().SetTimer(
				TH,
				FTimerDelegate::CreateUObject(this, &UDFLauncherComponent::RestoreTargetGravity,
					TWeakObjectPtr<ACharacter>(TargetChar), Prior),
				Hangtime,
				false);
		}
	}
}

void UDFLauncherComponent::RestoreTargetGravity(const TWeakObjectPtr<ACharacter> TargetChar, const float PriorGravity)
{
	if (TargetChar.IsValid())
	{
		if (UCharacterMovementComponent* const CMC = TargetChar->GetCharacterMovement())
		{
			CMC->GravityScale = PriorGravity;
		}
	}
}

void UDFLauncherComponent::ApplySelfLaunch(const FVector& SelfVel)
{
	ACharacter* const Char = Cast<ACharacter>(GetOwner());
	if (!Char)
	{
		return;
	}
	const FVector WorldVel = Char->GetActorTransform().TransformVectorNoScale(SelfVel);
	Char->LaunchCharacter(WorldVel, true, true);
}
