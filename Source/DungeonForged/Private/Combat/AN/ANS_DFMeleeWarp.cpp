// Source/DungeonForged/Private/Combat/AN/ANS_DFMeleeWarp.cpp
#include "Combat/AN/ANS_DFMeleeWarp.h"

#include "Animation/AnimSequenceBase.h"
#include "Combat/DFCombatDebug.h"
#include "Combat/UDFMeleeAimComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "MotionWarpingComponent.h"

namespace
{
UMotionWarpingComponent* GetWarpComp(USkeletalMeshComponent* const MeshComp)
{
	AActor* const Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	return IsValid(Owner) ? Owner->FindComponentByClass<UMotionWarpingComponent>() : nullptr;
}

UDFMeleeAimComponent* GetAimComp(USkeletalMeshComponent* const MeshComp)
{
	AActor* const Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	return IsValid(Owner) ? Owner->FindComponentByClass<UDFMeleeAimComponent>() : nullptr;
}
} // namespace

UANS_DFMeleeWarp::UANS_DFMeleeWarp()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(255, 140, 0);
#endif
}

FString UANS_DFMeleeWarp::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("DF Warp [%s]%s"),
		*WarpTargetName.ToString(),
		bRotationOnly ? TEXT(" (Rot)") : TEXT(""));
}

void UANS_DFMeleeWarp::NotifyBegin(USkeletalMeshComponent* const MeshComp,
	UAnimSequenceBase* const Animation, const float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (!MeshComp)
	{
		return;
	}
	if (bSnapYawOnBegin)
	{
		if (UDFMeleeAimComponent* const Aim = GetAimComp(MeshComp))
		{
			if (AActor* const T = Aim->ResolveCurrentTarget())
			{
				Aim->SnapYawTowardTarget(T);
			}
		}
	}
	UpdateWarpTarget(MeshComp);
}

void UANS_DFMeleeWarp::NotifyTick(USkeletalMeshComponent* const MeshComp,
	UAnimSequenceBase* const Animation, const float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	if (bUpdateEveryTick)
	{
		UpdateWarpTarget(MeshComp);
	}
}

void UANS_DFMeleeWarp::NotifyEnd(USkeletalMeshComponent* const MeshComp,
	UAnimSequenceBase* const Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (UMotionWarpingComponent* const Warp = GetWarpComp(MeshComp))
	{
		Warp->RemoveWarpTarget(WarpTargetName);
	}
}

void UANS_DFMeleeWarp::UpdateWarpTarget(USkeletalMeshComponent* const MeshComp) const
{
	if (!MeshComp)
	{
		return;
	}
	AActor* const Owner = MeshComp->GetOwner();
	UMotionWarpingComponent* const Warp = GetWarpComp(MeshComp);
	UDFMeleeAimComponent* const Aim = GetAimComp(MeshComp);
	if (!Owner || !Warp || !Aim)
	{
		return;
	}
	AActor* const Target = Aim->ResolveCurrentTarget();
	if (!IsValid(Target))
	{
		Warp->RemoveWarpTarget(WarpTargetName);
		return;
	}
	const FVector OwnerLoc = Owner->GetActorLocation();
	const FVector TargetLoc = Target->GetActorLocation();
	FVector Flat = TargetLoc - OwnerLoc;
	Flat.Z = 0.f;
	const float Dist = Flat.Size();
	if (Dist <= KINDA_SMALL_NUMBER)
	{
		Warp->RemoveWarpTarget(WarpTargetName);
		return;
	}
	if (MaxWarpDistance > 0.f && Dist > MaxWarpDistance)
	{
		Warp->RemoveWarpTarget(WarpTargetName);
		return;
	}
	const FVector Dir = Flat / Dist;

	// Stop short of target so the swing reaches without intersecting capsules.
	const float StopDist = FMath::Min(Dist, DesiredStopDistance);
	const FVector WarpLoc = bRotationOnly
		? OwnerLoc
		: (TargetLoc - Dir * StopDist);
	FVector FinalLoc = WarpLoc;
	if (!bMatchTargetZ)
	{
		FinalLoc.Z = OwnerLoc.Z;
	}
	const FRotator FinalRot = Dir.Rotation();
	const FTransform Xform(FinalRot, FinalLoc, FVector::OneVector);
	Warp->AddOrUpdateWarpTargetFromTransform(WarpTargetName, Xform);

#if ENABLE_DRAW_DEBUG
	if (bDrawDebug || DFCombatDebug::IsChannelEnabled(DFCombatDebug::EChannel::Warp))
	{
		if (UWorld* const W = Owner->GetWorld())
		{
			DrawDebugSphere(W, FinalLoc, 16.f, 12, FColor::Yellow, false, 0.05f);
			DrawDebugLine(W, OwnerLoc, FinalLoc, FColor::Orange, false, 0.05f, 0, 2.f);
		}
	}
#endif
}
