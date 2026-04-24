// Source/DungeonForged/Private/Dungeon/Traps/ADFTrap_CollapsingFloor.cpp
#include "Dungeon/Traps/ADFTrap_CollapsingFloor.h"
#include "GAS/Effects/UGE_Damage_True.h"
#include "GAS/Effects/UGE_Debuff_Stun.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "AbilitySystemInterface.h"

ADFTrap_CollapsingFloor::ADFTrap_CollapsingFloor()
{
	bIsRepeating = false;
	TriggerDelay = 0.8f;
	WalkTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("WalkTrigger"));
	WalkTrigger->SetupAttachment(RootComponent);
	WalkTrigger->SetBoxExtent(FVector(100.f, 100.f, 32.f));
	WalkTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WalkTrigger->SetGenerateOverlapEvents(true);
	WalkTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	WalkTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ADFTrap_CollapsingFloor::BeginPlay()
{
	Super::BeginPlay();
	WalkTrigger->OnComponentBeginOverlap.AddDynamic(
		this,
		&ADFTrap_CollapsingFloor::OnWalkOverlap);
}

void ADFTrap_CollapsingFloor::OnWalkOverlap(
	UPrimitiveComponent* const /*Overlapped*/,
	AActor* const Other,
	UPrimitiveComponent* const /*OtherComp*/,
	int32 const /*OtherBodyIndex*/,
	bool const /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	if (!HasAuthority() || !bIsArmed)
	{
		return;
	}
	if (!Other->IsA<APawn>())
	{
		return;
	}
	StartTriggerSequence(Other);
}

void ADFTrap_CollapsingFloor::TelegraphActivation_Implementation(AActor* const /*InstigatorActor*/)
{
	if (CrackRumbleSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			CrackRumbleSound,
			WalkTrigger ? WalkTrigger->GetComponentLocation() : GetActorLocation());
	}
}

void ADFTrap_CollapsingFloor::OnTrapTriggered_Implementation(AActor* const InstigatorActor)
{
	RunCollapse(InstigatorActor);
}

void ADFTrap_CollapsingFloor::RunCollapse(AActor* const InstigatorActor)
{
	if (!HasAuthority())
	{
		return;
	}
	auto const AddIfGAS = [](TSet<AActor*>& S, AActor* const A)
	{
		if (IAbilitySystemInterface* const I = Cast<IAbilitySystemInterface>(A))
		{
			if (I->GetAbilitySystemComponent())
			{
				S.Add(A);
			}
		}
	};

	TSet<AActor*> Damaged;
	if (InstigatorActor)
	{
		AddIfGAS(Damaged, InstigatorActor);
	}
	for (TObjectPtr<UStaticMeshComponent> const Tm : FloorTiles)
	{
		if (!Tm)
		{
			continue;
		}
		if (InstigatorActor && Tm->IsOverlappingActor(InstigatorActor))
		{
			AddIfGAS(Damaged, InstigatorActor);
		}
		TArray<AActor*> Over;
		Tm->GetOverlappingActors(Over, APawn::StaticClass());
		for (AActor* A : Over)
		{
			AddIfGAS(Damaged, A);
		}
		Tm->SetSimulatePhysics(true);
		Tm->SetNotifyRigidBodyCollision(true);
		Tm->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Tm->SetHiddenInGame(true);
	}
	for (AActor* A : Damaged)
	{
		ApplyEffectWithMagnitude(
			A, A, UGE_Damage_True::StaticClass(), FallDamage, -1.f);
		ApplyEffectWithMagnitude(
			A, A, UGE_Debuff_Stun::StaticClass(), 0.f, 0.5f);
	}
	CompleteTrap(InstigatorActor);
}

TArray<UPrimitiveComponent*> ADFTrap_CollapsingFloor::GetHighlightPrimitives_Implementation() const
{
	TArray<UPrimitiveComponent*> P;
	for (TObjectPtr<UStaticMeshComponent> T : FloorTiles)
	{
		if (T)
		{
			P.Add(T);
		}
	}
	return P;
}
