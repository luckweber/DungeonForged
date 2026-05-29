// Source/DungeonForged/Private/Combat/UDFLauncherComponent.cpp
#include "Combat/UDFLauncherComponent.h"
#include "Combat/UDFCombatCrowdControlComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

void UDFLauncherComponent::ApplyLaunch(AActor* Target, FVector LaunchVel, float TargetGravity, float Hangtime)
{
	if (!Target)
	{
		return;
	}
	AActor* const Owner = GetOwner();
	if (UDFCombatCrowdControlComponent* const VictimCC = Target->FindComponentByClass<UDFCombatCrowdControlComponent>())
	{
		if (!VictimCC->TryReceiveLaunch(LaunchVel, TargetGravity, Hangtime, Owner))
		{
			return;
		}
		return;
	}

	ACharacter* const TargetChar = Cast<ACharacter>(Target);
	if (!TargetChar)
	{
		return;
	}
	if (MaxJuggleHitsPerTarget > 0)
	{
		const TWeakObjectPtr<AActor> Key(Target);
		int32& Count = JuggleHitCounts.FindOrAdd(Key);
		if (Count >= MaxJuggleHitsPerTarget)
		{
			return;
		}
		++Count;
		ScheduleJuggleCountReset(Target);
	}
	UCharacterMovementComponent* const CMC = TargetChar->GetCharacterMovement();
	if (!CMC)
	{
		return;
	}

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

void UDFLauncherComponent::ScheduleJuggleCountReset(AActor* const Target)
{
	if (!Target || JuggleCountResetSeconds <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	const TWeakObjectPtr<AActor> Key(Target);
	FTimerHandle TH;
	W->GetTimerManager().SetTimer(
		TH,
		FTimerDelegate::CreateWeakLambda(this, [this, Key]()
		{
			JuggleHitCounts.Remove(Key);
		}),
		JuggleCountResetSeconds,
		false);
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

void UDFLauncherComponent::ApplySelfLaunch(FVector SelfVel)
{
	ACharacter* const Char = Cast<ACharacter>(GetOwner());
	if (!Char)
	{
		return;
	}
	const FVector WorldVel = Char->GetActorTransform().TransformVectorNoScale(SelfVel);
	Char->LaunchCharacter(WorldVel, true, true);
}
