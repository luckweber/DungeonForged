// Source/DungeonForged/Private/Camera/UDFLockOnComponent.cpp

#include "Camera/UDFLockOnComponent.h"

#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Camera/UDFCameraComponent.h"
#include "Characters/ADFEnemyBase.h"
#include "Combat/DFLockOnDebug.h"
#include "DFAssetManager.h"
#include "Data/UDFCombatTuningData.h"
#include "DungeonForgedModule.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "UI/UDFLockOnWidget.h"
#include "AI/DFAIKeys.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

namespace
{
static void ApplyTargetingTag(AActor* const Owner, const bool bAdd)
{
	if (!Owner || !FDFGameplayTags::State_Targeting.IsValid())
	{
		return;
	}
	UAbilitySystemComponent* const ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!ASC)
	{
		return;
	}
	if (bAdd)
	{
		ASC->AddLooseGameplayTag(FDFGameplayTags::State_Targeting);
	}
	else
	{
		ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Targeting, 1);
	}
}
} // namespace

UDFLockOnComponent::UDFLockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	LockTargetClass = ADFEnemyBase::StaticClass();
}

void UDFLockOnComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AActor* const O = GetOwner())
	{
		Camera = O->FindComponentByClass<UDFCameraComponent>();
	}
	if (!Camera)
	{
		UE_LOG(LogTemp, Warning, TEXT("UDFLockOnComponent: no UDFCameraComponent on %s — lock-on will not move the camera boom."),
			*GetNameSafe(GetOwner()));
	}

	if (const UDFCombatTuningData* const Tuning = UDFAssetManager::GetCombatTuningDataSafe())
	{
		LockOnRange = Tuning->LockOnRange;
		LockOnAngle = Tuning->LockOnConeAngle;
		AutoBreakGraceDelay = Tuning->LockOnAutoBreakGraceDelay;
		ScoreCameraWeight = Tuning->LockOnScoreCameraWeight;
		ScoreDistanceWeight = Tuning->LockOnScoreDistanceWeight;
		ScoreThreatWeight = Tuning->LockOnScoreThreatWeight;
		ScoreElevationWeight = Tuning->LockOnScoreElevationWeight;
		ElevationTolerance = Tuning->LockOnElevationTolerance;
		bRetargetOnHit = Tuning->bLockOnRetargetOnHit;
		bSoftAimWhenUnlocked = Tuning->bLockOnSoftAimWhenUnlocked;
		if (Camera)
		{
			Camera->SetRotationInterpSpeed(Tuning->LockOnCameraInterpSpeed);
		}
	}
}

void UDFLockOnComponent::GetViewPoint(FVector& OutOrigin, FVector& OutForward) const
{
	OutOrigin = FVector::ZeroVector;
	OutForward = FVector::ForwardVector;
	AActor* const O = GetOwner();
	if (!IsValid(O))
	{
		return;
	}
	if (const APawn* const Pawn = Cast<APawn>(O))
	{
		if (const APlayerController* const PC = Cast<APlayerController>(Pawn->GetController()))
		{
			FRotator ViewRot;
			PC->GetPlayerViewPoint(OutOrigin, ViewRot);
			OutForward = ViewRot.Vector();
			return;
		}
	}
	OutOrigin = O->GetActorLocation();
	OutForward = O->GetActorForwardVector();
}

void UDFLockOnComponent::TickComponent(
	float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsLockedOn)
	{
#if !UE_BUILD_SHIPPING
		DFLockOnDebug::DrawLockOnDebug(this, GetWorld(), GetOwner());
#endif
		return;
	}

	if (!IsTargetValidForMaintain(CurrentTarget.Get()))
	{
		if (!IsOwnerDodging())
		{
			TimeTargetInvalid += DeltaTime;
			if (TimeTargetInvalid >= AutoBreakGraceDelay)
			{
				DFLockOnDebug::Logf(TEXT("Auto-break target=%s invalidFor=%.2fs"),
					*GetNameSafe(CurrentTarget.Get()), TimeTargetInvalid);
				ReleaseLockOn();
			}
		}
		else
		{
			TimeTargetInvalid = 0.f;
		}
		UpdateIndicator(DeltaTime);
		return;
	}

	TimeTargetInvalid = 0.f;
	UpdateIndicator(DeltaTime);

#if !UE_BUILD_SHIPPING
	DFLockOnDebug::DrawLockOnDebug(this, GetWorld(), GetOwner());
#endif
}

bool UDFLockOnComponent::IsActorValidEnemyType(AActor* const Actor) const
{
	if (!IsValid(Actor) || Actor == GetOwner())
	{
		return false;
	}
	if (LockTargetClass)
	{
		return Actor->IsA(LockTargetClass);
	}
	return true;
}

float UDFLockOnComponent::AngleFromView(AActor* const Target) const
{
	if (!IsValid(Target))
	{
		return 180.f;
	}
	FVector Origin;
	FVector ViewForward;
	GetViewPoint(Origin, ViewForward);
	const FVector ToTarget = (Target->GetActorLocation() - Origin).GetSafeNormal();
	if (ToTarget.IsNearlyZero() || ViewForward.IsNearlyZero())
	{
		return 0.f;
	}
	return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(ViewForward, ToTarget), -1.f, 1.f)));
}

float UDFLockOnComponent::SignedViewAngle(AActor* const Target) const
{
	if (!IsValid(Target))
	{
		return 0.f;
	}
	FVector Origin;
	FVector ViewForward;
	GetViewPoint(Origin, ViewForward);
	FVector ToTarget = Target->GetActorLocation() - Origin;
	ToTarget.Z = 0.f;
	ViewForward.Z = 0.f;
	if (ToTarget.IsNearlyZero() || ViewForward.IsNearlyZero())
	{
		return 0.f;
	}
	ToTarget.Normalize();
	ViewForward.Normalize();
	const float CrossZ = FVector::CrossProduct(ViewForward, ToTarget).Z;
	const float Dot = FVector::DotProduct(ViewForward, ToTarget);
	return FMath::RadiansToDegrees(FMath::Atan2(CrossZ, Dot));
}

bool UDFLockOnComponent::HasLineOfSight(AActor* const Target) const
{
	AActor* const O = GetOwner();
	if (!IsValid(O) || !IsValid(Target) || !GetWorld())
	{
		return false;
	}
	const FVector Start = O->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector End = Target->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LockOnLOS), true, O);
	Params.AddIgnoredActor(O);
	Params.bReturnPhysicalMaterial = false;
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		return true;
	}
	return Hit.GetActor() == Target;
}

bool UDFLockOnComponent::IsOwnerDodging() const
{
	UAbilitySystemComponent* const ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	return ASC && ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Dodging);
}

bool UDFLockOnComponent::IsTargetValidForMaintain(AActor* const Target) const
{
	if (!IsValid(Target) || !IsActorValidEnemyType(Target) || !GetOwner())
	{
		return false;
	}
	const float Dist = FVector::Dist(GetOwner()->GetActorLocation(), Target->GetActorLocation());
	if (Dist > LockOnRange + 1.f)
	{
		return false;
	}
	if (!HasLineOfSight(Target))
	{
		return false;
	}
	if (const ADFEnemyBase* const E = Cast<ADFEnemyBase>(Target))
	{
		if (UAbilitySystemComponent* const ASC = E->GetAbilitySystemComponent())
		{
			if (ASC->GetNumericAttribute(UDFAttributeSet::GetHealthAttribute()) <= 0.01f)
			{
				return false;
			}
		}
	}
	return true;
}

bool UDFLockOnComponent::IsTargetValidForAcquire(AActor* const Target) const
{
	if (!IsTargetValidForMaintain(Target))
	{
		return false;
	}
	return AngleFromView(Target) <= LockOnAngle * 0.5f + 0.5f;
}

bool UDFLockOnComponent::IsTargetValid(AActor* const Target) const
{
	return IsTargetValidForAcquire(Target);
}

float UDFLockOnComponent::GetThreatScore(AActor* const Target) const
{
	if (!IsValid(Target))
	{
		return 0.f;
	}
	float Threat = 0.f;
	AActor* const O = GetOwner();
	if (UAbilitySystemComponent* const ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
	{
		if (FDFGameplayTags::State_Attacking.IsValid() && ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Attacking))
		{
			Threat += 0.55f;
		}
	}
	if (const APawn* const EnemyPawn = Cast<APawn>(Target))
	{
		if (const AAIController* const AIC = Cast<AAIController>(EnemyPawn->GetController()))
		{
			if (const UBlackboardComponent* const BB = AIC->GetBlackboardComponent())
			{
				if (BB->GetValueAsObject(DFAIKeys::TargetActor) == O)
				{
					Threat += 0.45f;
				}
			}
		}
	}
	return FMath::Clamp(Threat, 0.f, 1.f);
}

float UDFLockOnComponent::ScoreTarget(AActor* const Target) const
{
	if (!IsValid(Target) || !GetOwner())
	{
		return -1.f;
	}
	FVector ViewOrigin;
	FVector ViewForward;
	GetViewPoint(ViewOrigin, ViewForward);
	const FVector ToTarget = Target->GetActorLocation() - ViewOrigin;
	const float Dist = ToTarget.Size();
	if (Dist <= KINDA_SMALL_NUMBER || Dist > LockOnRange)
	{
		return -1.f;
	}
	const FVector Dir = ToTarget / Dist;
	const float CameraDot = FVector::DotProduct(ViewForward, Dir);
	const float CameraScore = FMath::Clamp((CameraDot + 1.f) * 0.5f, 0.f, 1.f);
	const float DistScore = 1.f - FMath::Clamp(Dist / LockOnRange, 0.f, 1.f);
	const float ElevScore = 1.f - FMath::Clamp(FMath::Abs(ToTarget.Z) / FMath::Max(50.f, ElevationTolerance), 0.f, 1.f);
	const float ThreatScore = GetThreatScore(Target);

	const float WeightSum = FMath::Max(
		KINDA_SMALL_NUMBER,
		ScoreCameraWeight + ScoreDistanceWeight + ScoreThreatWeight + ScoreElevationWeight);
	return (ScoreCameraWeight * CameraScore
		+ ScoreDistanceWeight * DistScore
		+ ScoreThreatWeight * ThreatScore
		+ ScoreElevationWeight * ElevScore) / WeightSum;
}

bool UDFLockOnComponent::BuildCandidates(TArray<AActor*>& OutSorted, const ELockOnCandidateSort SortMode) const
{
	OutSorted.Reset();
	AActor* const O = GetOwner();
	UWorld* const W = GetWorld();
	if (!IsValid(O) || !W)
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LockOnOverlap), false, O);
	TArray<FOverlapResult> Overlaps;
	const bool bAny = W->OverlapMultiByChannel(
		Overlaps,
		O->GetActorLocation(),
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(LockOnRange),
		QueryParams);

	if (!bAny)
	{
		return false;
	}

	TArray<TPair<float, AActor*>> Ranked;
	for (const FOverlapResult& R : Overlaps)
	{
		AActor* const A = R.GetActor();
		if (!IsTargetValidForAcquire(A))
		{
			continue;
		}
		const float Key = SortMode == ELockOnCandidateSort::ViewAngle
			? SignedViewAngle(A)
			: ScoreTarget(A);
		if (SortMode == ELockOnCandidateSort::Score && Key < 0.f)
		{
			continue;
		}
		Ranked.Add(TPair<float, AActor*>(Key, A));
	}
	if (SortMode == ELockOnCandidateSort::Score)
	{
		Ranked.Sort([](const TPair<float, AActor*>& L, const TPair<float, AActor*>& R) {
			return L.Key > R.Key;
		});
	}
	else
	{
		Ranked.Sort([](const TPair<float, AActor*>& L, const TPair<float, AActor*>& R) {
			return L.Key < R.Key;
		});
	}
	for (const TPair<float, AActor*>& P : Ranked)
	{
		OutSorted.Add(P.Value);
	}
	return OutSorted.Num() > 0;
}

AActor* UDFLockOnComponent::GetSoftTarget() const
{
	if (!bSoftAimWhenUnlocked || bIsLockedOn)
	{
		return nullptr;
	}
	TArray<AActor*> Sorted;
	if (BuildCandidates(Sorted, ELockOnCandidateSort::Score) && Sorted.Num() > 0)
	{
		return Sorted[0];
	}
	return nullptr;
}

void UDFLockOnComponent::SetCurrentTarget(AActor* const NewTarget)
{
	if (!IsValid(NewTarget))
	{
		return;
	}
	CurrentTarget = NewTarget;
	if (Camera)
	{
		Camera->EnableLockOn(NewTarget);
	}
}

bool UDFLockOnComponent::TryLockOn()
{
	AActor* const O = GetOwner();
	APawn* const OwnerPawn = Cast<APawn>(O);
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
	{
		return false;
	}

	TArray<AActor*> Sorted;
	if (!BuildCandidates(Sorted, ELockOnCandidateSort::Score) || Sorted.Num() == 0)
	{
		DFLockOnDebug::Log(TEXT("TryLockOn FAIL — no valid target in range"));
		return false;
	}
	AActor* const Pick = Sorted[0];
	SetCurrentTarget(Pick);
	bIsLockedOn = true;
	TimeTargetInvalid = 0.f;
	LockCycleIndex = 0;
	ApplyTargetingTag(O, true);
	CandidateBuffer.Reset();
	for (AActor* A : Sorted)
	{
		CandidateBuffer.Add(A);
	}
	EnsureLockOnWidget();
	OnLockOnChanged.Broadcast(true);
	DFLockOnDebug::Logf(TEXT("TryLockOn OK target=%s score=%.2f candidates=%d"),
		*GetNameSafe(Pick), ScoreTarget(Pick), Sorted.Num());
	return true;
}

void UDFLockOnComponent::CycleLockOnTarget(const float Direction)
{
	if (FMath::IsNearlyZero(Direction) || !bIsLockedOn)
	{
		return;
	}
	AActor* const O = GetOwner();
	APawn* const OwnerPawn = Cast<APawn>(O);
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
	{
		return;
	}
	TArray<AActor*> Sorted;
	if (!BuildCandidates(Sorted, ELockOnCandidateSort::ViewAngle) || Sorted.Num() == 0)
	{
		DFLockOnDebug::Log(TEXT("Cycle — no candidates, keeping current lock"));
		return;
	}
	int32 Index = 0;
	if (CurrentTarget.IsValid())
	{
		Index = Sorted.Find(CurrentTarget.Get());
		if (Index == INDEX_NONE)
		{
			Index = 0;
		}
	}
	const int32 Len = Sorted.Num();
	const int32 Next = (Index + (Direction > 0.f ? 1 : -1) + Len * 2) % Len;
	SetCurrentTarget(Sorted[Next]);
	LockCycleIndex = Next;
	CandidateBuffer.Reset();
	for (AActor* A : Sorted)
	{
		CandidateBuffer.Add(A);
	}
	DFLockOnDebug::Logf(TEXT("Cycle dir=%.0f -> %s"), Direction, *GetNameSafe(Sorted[Next]));
}

void UDFLockOnComponent::NotifyCombatHitConfirmed(AActor* const HitVictim)
{
	if (!bRetargetOnHit || !IsValid(HitVictim) || !IsActorValidEnemyType(HitVictim))
	{
		return;
	}
	APawn* const OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
	{
		return;
	}
	if (!IsTargetValidForMaintain(HitVictim))
	{
		return;
	}
	if (CurrentTarget.Get() == HitVictim)
	{
		return;
	}
	if (!bIsLockedOn)
	{
		if (!bSoftAimWhenUnlocked)
		{
			return;
		}
	}
	SetCurrentTarget(HitVictim);
	if (!bIsLockedOn)
	{
		DFLockOnDebug::Logf(TEXT("Soft retarget on hit -> %s"), *GetNameSafe(HitVictim));
		return;
	}
	TimeTargetInvalid = 0.f;
	TArray<AActor*> Sorted;
	if (BuildCandidates(Sorted, ELockOnCandidateSort::Score))
	{
		LockCycleIndex = Sorted.Find(HitVictim);
		if (LockCycleIndex == INDEX_NONE)
		{
			LockCycleIndex = 0;
		}
		CandidateBuffer.Reset();
		for (AActor* A : Sorted)
		{
			CandidateBuffer.Add(A);
		}
	}
	DFLockOnDebug::Logf(TEXT("Retarget on hit -> %s"), *GetNameSafe(HitVictim));
}

void UDFLockOnComponent::ReleaseLockOn()
{
	const bool bWasLocked = bIsLockedOn;
	TimeTargetInvalid = 0.f;

	if (bWasLocked)
	{
		ApplyTargetingTag(GetOwner(), false);
	}

	CurrentTarget = nullptr;
	bIsLockedOn = false;
	CandidateBuffer.Reset();
	if (Camera)
	{
		Camera->DisableLockOn();
	}
	if (LockOnWidget)
	{
		LockOnWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (bWasLocked)
	{
		OnLockOnChanged.Broadcast(false);
		DFLockOnDebug::Log(TEXT("ReleaseLockOn"));
	}
}

void UDFLockOnComponent::UpdateIndicator(const float /*DeltaTime*/)
{
	AActor* const T = CurrentTarget.Get();
	APawn* const OwnerPawn = Cast<APawn>(GetOwner());
	if (!bIsLockedOn || !IsValid(T) || !OwnerPawn)
	{
		if (LockOnWidget)
		{
			LockOnWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}
	if (!OwnerPawn->IsLocallyControlled() || !GetWorld())
	{
		return;
	}
	APlayerController* const PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC || !LockOnWidget)
	{
		return;
	}
	const FVector WorldPos = T->GetActorLocation() + FVector(0.f, 0.f, 80.f);
	LockOnWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	LockOnWidget->UpdateScreenPosition(PC, WorldPos);
}

void UDFLockOnComponent::EnsureLockOnWidget()
{
	if (bWidgetCreated || !LockOnWidgetClass)
	{
		return;
	}
	APawn* const P = Cast<APawn>(GetOwner());
	if (!P || !P->IsLocallyControlled())
	{
		return;
	}
	APlayerController* const PC = Cast<APlayerController>(P->GetController());
	if (!PC)
	{
		return;
	}
	LockOnWidget = CreateWidget<UDFLockOnWidget>(PC, LockOnWidgetClass);
	if (LockOnWidget)
	{
		LockOnWidget->AddToViewport(100);
		LockOnWidget->SetVisibility(ESlateVisibility::Collapsed);
		bWidgetCreated = true;
	}
}
