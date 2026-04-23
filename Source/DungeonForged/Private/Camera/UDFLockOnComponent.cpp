// Source/DungeonForged/Private/Camera/UDFLockOnComponent.cpp

#include "Camera/UDFLockOnComponent.h"
#include "Camera/UDFCameraComponent.h"
#include "Characters/ADFEnemyBase.h"
#include "UI/UDFLockOnWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "AbilitySystemComponent.h"
#include "GAS/UDFAttributeSet.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

UDFLockOnComponent::UDFLockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	LockTargetClass = ADFEnemyBase::StaticClass();
}

void UDFLockOnComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AActor* O = GetOwner())
	{
		Camera = O->FindComponentByClass<UDFCameraComponent>();
	}
	if (!Camera)
	{
		UE_LOG(LogTemp, Warning, TEXT("UDFLockOnComponent: no UDFCameraComponent on %s — lock-on will not move the camera boom."),
			*GetNameSafe(GetOwner()));
	}
}

void UDFLockOnComponent::TickComponent(
	float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bIsLockedOn)
	{
		if (!IsTargetValid(CurrentTarget.Get()))
		{
			ReleaseLockOn();
			return;
		}
		UpdateIndicator(DeltaTime);
	}
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

float UDFLockOnComponent::AngleFromForward(AActor* const Target) const
{
	AActor* const O = GetOwner();
	if (!IsValid(O) || !IsValid(Target))
	{
		return 180.f;
	}
	const FVector Forward = O->GetActorForwardVector();
	const FVector ToTarget = (Target->GetActorLocation() - O->GetActorLocation()).GetSafeNormal();
	if (ToTarget.IsNearlyZero())
	{
		return 0.f;
	}
	return FMath::RadiansToDegrees(acosf(FMath::Clamp(FVector::DotProduct(Forward, ToTarget), -1.f, 1.f)));
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

bool UDFLockOnComponent::IsTargetValid(AActor* const Target) const
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
	if (AngleFromForward(Target) > LockOnAngle * 0.5f + 0.5f)
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

bool UDFLockOnComponent::BuildCandidatesInView(TArray<AActor*>& OutSorted) const
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

	TArray<TPair<float, AActor*>> Scored;
	for (const FOverlapResult& R : Overlaps)
	{
		AActor* const A = R.GetActor();
		if (!IsTargetValid(A))
		{
			continue;
		}
		const float D = FVector::DistSquared(O->GetActorLocation(), A->GetActorLocation());
		Scored.Add(TPair<float, AActor*>(D, A));
	}
	Scored.Sort([](const TPair<float, AActor*>& L, const TPair<float, AActor*>& R) {
		return L.Key < R.Key;
	});
	for (const TPair<float, AActor*>& P : Scored)
	{
		OutSorted.Add(P.Value);
	}
	return OutSorted.Num() > 0;
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
	if (!BuildCandidatesInView(Sorted) || Sorted.Num() == 0)
	{
		return false;
	}
	AActor* const Pick = Sorted[0];
	CurrentTarget = Pick;
	bIsLockedOn = true;
	LockCycleIndex = 0;
	if (Camera)
	{
		Camera->EnableLockOn(Pick);
	}
	CandidateBuffer.Reset();
	for (AActor* A : Sorted)
	{
		CandidateBuffer.Add(A);
	}
	EnsureLockOnWidget();
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
	if (!BuildCandidatesInView(Sorted) || Sorted.Num() == 0)
	{
		ReleaseLockOn();
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
	CurrentTarget = Sorted[Next];
	LockCycleIndex = Next;
	if (Camera)
	{
		Camera->EnableLockOn(Sorted[Next]);
	}
}

void UDFLockOnComponent::ReleaseLockOn()
{
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
