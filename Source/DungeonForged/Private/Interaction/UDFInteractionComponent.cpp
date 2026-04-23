// Source/DungeonForged/Private/Interaction/UDFInteractionComponent.cpp
#include "Interaction/UDFInteractionComponent.h"
#include "Interaction/ADFInteractableBase.h"
#include "Interaction/DFInteractable.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

UDFInteractionComponent::UDFInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);
}

void UDFInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UDFInteractionComponent::RegisterInteractable(AActor* const Actor)
{
	if (!IsValid(Actor) || !Actor->GetClass()->ImplementsInterface(UDFInteractable::StaticClass()))
	{
		return;
	}
	NearbyInteractables.AddUnique(Actor);
}

void UDFInteractionComponent::UnregisterInteractable(AActor* const Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}
	NearbyInteractables.RemoveAll(
		[Actor](const TWeakObjectPtr<AActor>& P) { return P.Get() == Actor; });
	if (CurrentFocusedActor.Get() == Actor)
	{
		CurrentFocusedActor = nullptr;
	}
}

void UDFInteractionComponent::TickComponent(
	const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RecomputeCurrentFocus();
}

void UDFInteractionComponent::RecomputeCurrentFocus()
{
	ACharacter* const OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar || !GetWorld())
	{
		return;
	}
	AActor* Traced = nullptr;
	if (TraceInteractedActor(Traced) && Traced)
	{
		if (IDFInteractable::Execute_CanInteract(Traced, OwnerChar))
		{
			if (Traced != CurrentFocusedActor)
			{
				if (ADFInteractableBase* const Prev = Cast<ADFInteractableBase>(CurrentFocusedActor.Get()))
				{
					Prev->SetPromptPrimaryFocus(false);
				}
				CurrentFocusedActor = Traced;
			}
			if (ADFInteractableBase* const Now = Cast<ADFInteractableBase>(CurrentFocusedActor.Get()))
			{
				Now->SetPromptPrimaryFocus(true);
			}
			// dim other nearby
			for (TWeakObjectPtr<AActor>& W : NearbyInteractables)
			{
				if (W.Get() && W.Get() != Traced)
				{
					if (ADFInteractableBase* O = Cast<ADFInteractableBase>(W.Get()))
					{
						O->SetPromptPrimaryFocus(false);
					}
				}
			}
			return;
		}
	}
	// Fallback: best in nearby
	AActor* Picked = PickBestFromNearby();
	if (Picked != CurrentFocusedActor)
	{
		if (ADFInteractableBase* const Prev = Cast<ADFInteractableBase>(CurrentFocusedActor.Get()))
		{
			Prev->SetPromptPrimaryFocus(false);
		}
		CurrentFocusedActor = Picked;
	}
	if (ADFInteractableBase* const N = Cast<ADFInteractableBase>(CurrentFocusedActor.Get()))
	{
		N->SetPromptPrimaryFocus(true);
	}
	for (TWeakObjectPtr<AActor>& W : NearbyInteractables)
	{
		if (W.Get() && W.Get() != CurrentFocusedActor.Get())
		{
			if (ADFInteractableBase* O = Cast<ADFInteractableBase>(W.Get()))
			{
				O->SetPromptPrimaryFocus(false);
			}
		}
	}
}

bool UDFInteractionComponent::TraceInteractedActor(AActor*& OutActor) const
{
	OutActor = nullptr;
	ACharacter* const C = Cast<ACharacter>(GetOwner());
	if (!C)
	{
		return false;
	}
	const FVector Eye = C->GetPawnViewLocation();
	const FRotator Rot = C->GetViewRotation();
	const FVector Fwd = Rot.Vector();
	const FVector End = Eye + Fwd * InteractTraceRange;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(DFInteractionTrace), false, C);
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Eye, End, ECC_Visibility, Q))
	{
		AActor* A = Hit.GetActor();
		if (A)
		{
			if (A->GetClass()->ImplementsInterface(UDFInteractable::StaticClass()) && IsValid(A))
			{
				OutActor = A;
				return true;
			}
			AActor* Next = A->GetParentActor();
			while (IsValid(Next))
			{
				if (Next->GetClass()->ImplementsInterface(UDFInteractable::StaticClass()))
				{
					OutActor = Next;
					return true;
				}
				Next = Next->GetParentActor();
			}
		}
	}
	return false;
}

AActor* UDFInteractionComponent::PickBestFromNearby() const
{
	const ACharacter* const C = Cast<ACharacter>(GetOwner());
	if (!C)
	{
		return nullptr;
	}
	const FVector PLoc = C->GetActorLocation();
	const FVector Fwd = C->GetControlRotation().Vector();
	float Best = -2.f;
	AActor* BestA = nullptr;
	for (TWeakObjectPtr<AActor> W : NearbyInteractables)
	{
		AActor* A = W.Get();
		if (!IsValid(A) || !A->GetClass()->ImplementsInterface(UDFInteractable::StaticClass()))
		{
			continue;
		}
		if (!IDFInteractable::Execute_CanInteract(A, const_cast<ACharacter*>(C)))
		{
			continue;
		}
		const FVector ToA = (A->GetActorLocation() - PLoc);
		if (ToA.IsNearlyZero(1.f))
		{
			return A;
		}
		const float D = FVector::DotProduct(Fwd, ToA.GetSafeNormal());
		if (D >= FocusDirectionDot && D > Best)
		{
			Best = D;
			BestA = A;
		}
	}
	return BestA;
}

void UDFInteractionComponent::TryInteract()
{
	ACharacter* const C = Cast<ACharacter>(GetOwner());
	if (!C || !C->IsLocallyControlled())
	{
		return;
	}
	RecomputeCurrentFocus();
	AActor* T = CurrentFocusedActor.Get();
	if (!T)
	{
		TraceInteractedActor(T);
	}
	if (!T || !T->GetClass()->ImplementsInterface(UDFInteractable::StaticClass()))
	{
		return;
	}
	Server_Interact(T);
}

void UDFInteractionComponent::Server_Interact_Implementation(AActor* Target)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValid(Target))
	{
		return;
	}
	ACharacter* C = Cast<ACharacter>(GetOwner());
	if (!C)
	{
		return;
	}
	const float MaxDist = FMath::Max(InteractTraceRange, IDFInteractable::Execute_GetInteractionRange(Target) * 1.5f);
	if (FVector::Dist(C->GetActorLocation(), Target->GetActorLocation()) > MaxDist * 1.2f)
	{
		// not in valid range; still allow if in Registered nearby (server trust overlap)
		bool bInRange = false;
		for (const TWeakObjectPtr<AActor>& W : NearbyInteractables)
		{
			if (W.Get() == Target)
			{
				bInRange = true;
				break;
			}
		}
		if (!bInRange)
		{
			return;
		}
	}
	IDFInteractable::Execute_Interact(Target, C);
}
