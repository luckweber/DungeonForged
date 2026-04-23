// Source/DungeonForged/Private/Animation/UDFAnimInstance_Enemy.cpp
#include "Animation/UDFAnimInstance_Enemy.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"

UAnimMontage* UUDFAnimInstance_Enemy::SelectHitMontage(const FVector& WorldHitDirection) const
{
	if (!OwningCharacter)
	{
		return HitReact_Fallback;
	}
	const FVector2D Fwd(FVector2D(OwningCharacter->GetActorForwardVector().GetSafeNormal2D()));
	FVector2D ToHit(0.f, 0.f);
	if (!WorldHitDirection.IsNearlyZero(1.f))
	{
		ToHit = FVector2D(WorldHitDirection).GetSafeNormal();
	}
	else if (!HitReactionDirection.IsNearlyZero(1.f))
	{
		ToHit = FVector2D(HitReactionDirection).GetSafeNormal();
	}
	else if (!HitFromWorldLocation.IsNearlyZero(1.f))
	{
		ToHit = FVector2D((OwningCharacter->GetActorLocation() - HitFromWorldLocation).GetSafeNormal2D());
	}
	if (ToHit.IsNearlyZero(0.01f))
	{
		return HitReact_Fallback ? HitReact_Fallback : HitReact_Front;
	}
	ToHit = ToHit.GetSafeNormal();
	// Clockwise from forward: +90 is right, -90 is left, 180 is back
	const float CrossZ = Fwd.X * ToHit.Y - Fwd.Y * ToHit.X;
	const float Dot = FMath::Clamp(FVector2D::DotProduct(Fwd, ToHit), -1.f, 1.f);
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(CrossZ, Dot));
	const float A = FMath::Abs(Yaw);
	if (A < 45.f)
	{
		return HitReact_Front ? HitReact_Front : HitReact_Fallback;
	}
	if (A > 135.f)
	{
		return HitReact_Back ? HitReact_Back : HitReact_Fallback;
	}
	if (Yaw > 0.f)
	{
		return HitReact_Right ? HitReact_Right : HitReact_Fallback;
	}
	return HitReact_Left ? HitReact_Left : HitReact_Fallback;
}
