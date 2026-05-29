// Source/DungeonForged/Private/Combat/UDFMeleeAimComponent.cpp
#include "Combat/UDFMeleeAimComponent.h"

#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Camera/UDFLockOnComponent.h"
#include "Characters/ADFPlayerCharacter.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "AI/DFAIKeys.h"
#include "Combat/DFCombatDebug.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "WorldCollision.h"
#include "Engine/OverlapResult.h"

namespace
{
bool IsAlive_AimComp(AActor* const Actor)
{
	if (!IsValid(Actor))
	{
		return false;
	}
	UAbilitySystemComponent* const ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
	if (!ASC)
	{
		return true; // non-GAS targets are considered "alive" by default
	}
	if (FDFGameplayTags::State_Dead.IsValid() && ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Dead))
	{
		return false;
	}
	const FGameplayAttribute HealthAttr = UDFAttributeSet::GetHealthAttribute();
	return !HealthAttr.IsValid() || ASC->GetNumericAttribute(HealthAttr) > 0.f;
}

float ScoreCandidate(const FVector& Origin, const FVector& Forward, AActor* const Candidate,
	const float DistanceWeight, const float MaxDistance)
{
	const FVector ToTarget = Candidate->GetActorLocation() - Origin;
	const float Dist = ToTarget.Size();
	if (Dist <= KINDA_SMALL_NUMBER || Dist > MaxDistance)
	{
		return -1.f;
	}
	const FVector Dir = ToTarget / Dist;
	const float Dot = FVector::DotProduct(Forward, Dir); // [-1, 1]
	const float AngleScore = (Dot + 1.f) * 0.5f; // [0, 1]
	const float DistScore = 1.f - FMath::Clamp(Dist / MaxDistance, 0.f, 1.f); // closer = higher
	return FMath::Lerp(AngleScore, DistScore, DistanceWeight);
}
} // namespace

UDFMeleeAimComponent::UDFMeleeAimComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDFMeleeAimComponent::BeginPlay()
{
	Super::BeginPlay();
}

APawn* UDFMeleeAimComponent::GetOwningPawn() const
{
	return Cast<APawn>(GetOwner());
}

bool UDFMeleeAimComponent::IsValidMeleeTarget(AActor* const Candidate) const
{
	if (!IsValid(Candidate) || Candidate == GetOwner())
	{
		return false;
	}
	if (TargetClassFilter && !Candidate->IsA(TargetClassFilter))
	{
		return false;
	}
	return IsAlive_AimComp(Candidate);
}

bool UDFMeleeAimComponent::HasLineOfSight(AActor* const Candidate) const
{
	const AActor* const Owner = GetOwner();
	UWorld* const World = GetWorld();
	if (!Owner || !World || !IsValid(Candidate))
	{
		return false;
	}
	const FVector Start = Owner->GetActorLocation();
	const FVector End = Candidate->GetActorLocation();
	FCollisionQueryParams Q(SCENE_QUERY_STAT(DFMeleeAimLOS), false);
	Q.AddIgnoredActor(Owner);
	Q.AddIgnoredActor(Candidate);
	FHitResult Hit;
	const bool bBlocked = World->LineTraceSingleByChannel(Hit, Start, End, SoftSearchVisChannel, Q);
	return !bBlocked;
}

AActor* UDFMeleeAimComponent::TryGetPlayerLockOnTarget() const
{
	if (!bConsiderPlayerLockOn)
	{
		return nullptr;
	}
	if (const ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(GetOwner()))
	{
		if (const UDFLockOnComponent* const Lock = PC->LockOnComponent)
		{
			if (Lock->IsLockedOn())
			{
				AActor* const T = Lock->GetCurrentTarget();
				if (IsValidMeleeTarget(T))
				{
					return T;
				}
			}
			if (AActor* const Soft = Lock->GetSoftTarget())
			{
				if (IsValidMeleeTarget(Soft))
				{
					return Soft;
				}
			}
		}
	}
	return nullptr;
}

AActor* UDFMeleeAimComponent::TryGetAIBlackboardTarget() const
{
	if (!bConsiderAIBlackboard)
	{
		return nullptr;
	}
	const APawn* const Pawn = GetOwningPawn();
	if (!Pawn)
	{
		return nullptr;
	}
	if (const AAIController* const AI = Cast<AAIController>(Pawn->GetController()))
	{
		if (const UBlackboardComponent* const BB = AI->GetBlackboardComponent())
		{
			AActor* const T = Cast<AActor>(BB->GetValueAsObject(DFAIKeys::TargetActor));
			if (IsValidMeleeTarget(T))
			{
				return T;
			}
		}
	}
	return nullptr;
}

AActor* UDFMeleeAimComponent::FindSoftConeTarget() const
{
	if (!bConsiderSoftCone)
	{
		return nullptr;
	}
	const AActor* const Owner = GetOwner();
	UWorld* const World = GetWorld();
	if (!Owner || !World || SoftSearchDistance <= 0.f)
	{
		return nullptr;
	}
	const FVector Origin = Owner->GetActorLocation();
	const FVector Forward = Owner->GetActorForwardVector();
	const float Sweep = SoftSearchSphereRadius;
	const FVector ConeCenter = Origin + Forward * (SoftSearchDistance * 0.5f);

	FCollisionObjectQueryParams Obj(ECC_Pawn);
	FCollisionQueryParams Q(SCENE_QUERY_STAT(DFMeleeAimSoftCone), false, Owner);
	Q.AddIgnoredActor(Owner);
	TArray<FOverlapResult> Overlaps;
	if (!World->OverlapMultiByObjectType(Overlaps, ConeCenter,
			FQuat::Identity, Obj, FCollisionShape::MakeSphere(Sweep + SoftSearchDistance * 0.5f), Q))
	{
		return nullptr;
	}

	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(SoftSearchHalfAngle));
	AActor* Best = nullptr;
	float BestScore = -1.f;
	for (const FOverlapResult& Hit : Overlaps)
	{
		AActor* const A = Hit.GetActor();
		if (!IsValidMeleeTarget(A))
		{
			continue;
		}
		const FVector ToA = A->GetActorLocation() - Origin;
		const float Dist = ToA.Size();
		if (Dist <= KINDA_SMALL_NUMBER || Dist > SoftSearchDistance)
		{
			continue;
		}
		const FVector Dir = ToA / Dist;
		if (FVector::DotProduct(Forward, Dir) < CosHalfAngle)
		{
			continue;
		}
		if (!HasLineOfSight(A))
		{
			continue;
		}
		const float Score = ScoreCandidate(Origin, Forward, A, SoftSearchDistanceWeight, SoftSearchDistance);
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = A;
		}
	}

#if ENABLE_DRAW_DEBUG
	if ((bDrawDebug || DFCombatDebug::IsChannelEnabled(DFCombatDebug::EChannel::Aim)) && World)
	{
		DrawDebugCone(World, Origin, Forward, SoftSearchDistance,
			FMath::DegreesToRadians(SoftSearchHalfAngle), FMath::DegreesToRadians(SoftSearchHalfAngle),
			12, FColor(50, 180, 255), false, 0.25f);
		if (Best)
		{
			DrawDebugLine(World, Origin, Best->GetActorLocation(), FColor::Green, false, 0.25f, 0, 2.f);
		}
	}
#endif
	return Best;
}

AActor* UDFMeleeAimComponent::ResolveCurrentTarget() const
{
	if (ManualTarget.IsValid() && IsValidMeleeTarget(ManualTarget.Get()))
	{
		return ManualTarget.Get();
	}
	if (AActor* const Lock = TryGetPlayerLockOnTarget())
	{
		return Lock;
	}
	if (AActor* const BBT = TryGetAIBlackboardTarget())
	{
		return BBT;
	}
	return FindSoftConeTarget();
}

float UDFMeleeAimComponent::SnapYawTowardTarget(AActor* const Target)
{
	AActor* const Owner = GetOwner();
	if (!Owner || !IsValid(Target))
	{
		return 0.f;
	}
	FVector To = Target->GetActorLocation() - Owner->GetActorLocation();
	To.Z = 0.f;
	if (To.IsNearlyZero())
	{
		return 0.f;
	}
	const FRotator Desired = To.GetSafeNormal().Rotation();
	const FRotator Current = Owner->GetActorRotation();
	const float DeltaYaw = FRotator::NormalizeAxis(Desired.Yaw - Current.Yaw);
	if (FMath::Abs(DeltaYaw) <= SnapYawTolerance)
	{
		return 0.f;
	}
	const float ClampedDelta = FMath::Clamp(DeltaYaw, -SnapMaxYawPerActivation, SnapMaxYawPerActivation);
	const FRotator NewRot(Current.Pitch, FRotator::NormalizeAxis(Current.Yaw + ClampedDelta), Current.Roll);
	Owner->SetActorRotation(NewRot);
	return ClampedDelta;
}

AActor* UDFMeleeAimComponent::AcquireAndCommitTarget()
{
	AActor* const T = ResolveCurrentTarget();
	if (T)
	{
		ManualTarget = T;
		SnapYawTowardTarget(T);
	}
	return T;
}

void UDFMeleeAimComponent::ReleaseAttackTarget()
{
	ManualTarget.Reset();
}
