// Source/DungeonForged/Private/Interaction/ADFInteractableBase.cpp
#include "Interaction/ADFInteractableBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Interaction/UDFInteractionComponent.h"
#include "Interaction/UDFInteractionPromptWidget.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"

ADFInteractableBase::ADFInteractableBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionRange"));
	RootComponent = InteractionRange;
	InteractionRange->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	InteractionRange->SetGenerateOverlapEvents(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	Mesh->SetCanEverAffectNavigation(false);

	InteractionPromptWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionPromptWidget"));
	InteractionPromptWidget->SetupAttachment(Mesh);
	InteractionPromptWidget->SetWidgetSpace(EWidgetSpace::Screen);
	InteractionPromptWidget->SetDrawAtDesiredSize(true);
	InteractionPromptWidget->SetTwoSided(true);
	InteractionPromptWidget->SetVisibility(false);
	InteractionPromptWidget->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
}

void ADFInteractableBase::BeginPlay()
{
	Super::BeginPlay();
	SyncInteractionSphereRadius();
	InteractionRange->OnComponentBeginOverlap.AddDynamic(this, &ADFInteractableBase::OnRangeBeginOverlap);
	InteractionRange->OnComponentEndOverlap.AddDynamic(this, &ADFInteractableBase::OnRangeEndOverlap);
	if (InteractionPromptClass)
	{
		InteractionPromptWidget->SetWidgetClass(InteractionPromptClass);
		InteractionPromptWidget->InitWidget();
	}
	RefreshPrompt();
}

void ADFInteractableBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADFInteractableBase, bIsInteractable);
	DOREPLIFETIME(ADFInteractableBase, bSingleUse);
}

FText ADFInteractableBase::GetInteractionText_Implementation() const
{
	return NSLOCTEXT("DF", "Interact", "Interact");
}

bool ADFInteractableBase::CanInteract_Implementation(ACharacter* /*Interactor*/) const
{
	return bIsInteractable;
}

float ADFInteractableBase::GetInteractionRange_Implementation() const
{
	if (InteractionRange)
	{
		return InteractionRange->GetScaledSphereRadius();
	}
	return 200.f;
}

void ADFInteractableBase::SyncInteractionSphereRadius() const
{
	// No-op: radius is set in editor; subclass may override in BeginPlay
}

void ADFInteractableBase::OnRangeBeginOverlap(
	UPrimitiveComponent* const /*OverlappedComponent*/,
	AActor* const OtherActor,
	UPrimitiveComponent* const /*OtherComp*/,
	const int32 /*OtherBodyIndex*/,
	const bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	ACharacter* const C = Cast<ACharacter>(OtherActor);
	if (!C)
	{
		return;
	}
	if (UDFInteractionComponent* const IC = C->FindComponentByClass<UDFInteractionComponent>())
	{
		IC->RegisterInteractable(this);
	}
	SetPromptWidgetVisible(true);
	if (UDFInteractionPromptWidget* const W = Cast<UDFInteractionPromptWidget>(InteractionPromptWidget->GetWidget()))
	{
		RefreshPrompt();
		W->PlayEnterAnimation();
	}
}

void ADFInteractableBase::OnRangeEndOverlap(
	UPrimitiveComponent* const /*OverlappedComponent*/,
	AActor* const OtherActor,
	UPrimitiveComponent* const /*OtherComp*/,
	const int32 /*OtherBodyIndex*/)
{
	ACharacter* const C = Cast<ACharacter>(OtherActor);
	if (!C)
	{
		return;
	}
	if (UDFInteractionComponent* const IC = C->FindComponentByClass<UDFInteractionComponent>())
	{
		IC->UnregisterInteractable(this);
	}
	SetPromptWidgetVisible(false);
}

void ADFInteractableBase::SetPromptWidgetVisible(const bool bVisible) const
{
	if (InteractionPromptWidget)
	{
		InteractionPromptWidget->SetVisibility(bVisible);
	}
}

void ADFInteractableBase::SetPromptPrimaryFocus(const bool bIsPrimary) const
{
	if (UDFInteractionPromptWidget* const W = Cast<UDFInteractionPromptWidget>(InteractionPromptWidget->GetWidget()))
	{
		W->SetPrimaryFocus(bIsPrimary);
	}
}

void ADFInteractableBase::SetPromptData(const FText& Action, UTexture2D* KeyIcon) const
{
	if (UDFInteractionPromptWidget* const W = Cast<UDFInteractionPromptWidget>(InteractionPromptWidget->GetWidget()))
	{
		UTexture2D* const Tex = KeyIcon ? KeyIcon : DefaultKeyIcon.Get();
		W->UpdatePrompt(Action, Tex, FText::GetEmpty());
	}
}

void ADFInteractableBase::RefreshPrompt() const
{
	SetPromptData(GetInteractionText(), DefaultKeyIcon);
}

void ADFInteractableBase::Interact_Implementation(ACharacter* const Interactor)
{
	if (!HasAuthority() || !Interactor)
	{
		return;
	}
	if (!bIsInteractable)
	{
		return;
	}
	if (!IDFInteractable::Execute_CanInteract(this, Interactor))
	{
		return;
	}
	PlayInteractEffects(Interactor);
	if (bSingleUse)
	{
		bIsInteractable = false;
	}
}

void ADFInteractableBase::PlayInteractEffects_Implementation(ACharacter* /*Interactor*/)
{
}
