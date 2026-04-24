// Source/DungeonForged/Private/Dungeon/Traps/ADFTrap_DartWall.cpp
#include "Dungeon/Traps/ADFTrap_DartWall.h"
#include "Dungeon/Traps/ADFDartProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ADFTrap_DartWall::ADFTrap_DartWall()
{
	TriggerDelay = 0.2f;
	WallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallMesh"));
	RootComponent = WallMesh;
	WallMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WallMesh->SetCollisionObjectType(ECC_WorldStatic);

	TripwireBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TripwireBox"));
	TripwireBox->SetupAttachment(RootComponent);
	TripwireBox->SetBoxExtent(FVector(12.f, 400.f, 80.f));
	TripwireBox->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	TripwireBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TripwireBox->SetGenerateOverlapEvents(true);
	TripwireBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TripwireBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ADFTrap_DartWall::BeginPlay()
{
	Super::BeginPlay();
	if (bIsTripwire)
	{
		TripwireBox->OnComponentBeginOverlap.AddDynamic(
			this,
			&ADFTrap_DartWall::OnTripwireBeginOverlap);
	}
	else if (HasAuthority() && FireInterval > 0.1f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			TimedVolleyHandle,
			this,
			&ADFTrap_DartWall::TimedFire,
			FireInterval,
			true);
	}
}

void ADFTrap_DartWall::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(TimedVolleyHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ADFTrap_DartWall::OnTripwireBeginOverlap(
	UPrimitiveComponent* const /*Overlapped*/,
	AActor* const Other,
	UPrimitiveComponent* const /*OtherComp*/,
	int32 const /*OtherBodyIndex*/,
	bool const /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	if (!bIsTripwire)
	{
		return;
	}
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

void ADFTrap_DartWall::TelegraphActivation_Implementation(AActor* const /*InstigatorActor*/)
{
	if (WallTelegraphClickSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, WallTelegraphClickSound, GetActorLocation());
	}
	// "Light flash" in Blueprint: flash dynamic material, etc.
}

void ADFTrap_DartWall::OnTrapTriggered_Implementation(AActor* const InstigatorActor)
{
	if (!bIsTripwire)
	{
		return;
	}
	// VFX: CompleteTrap -> Multicast_PlayTriggerVFX; avoid double burst here.
	FireAllDarts(InstigatorActor, false);
	CompleteTrap(InstigatorActor);
}

void ADFTrap_DartWall::FireAllDarts(AActor* const InstigatorActor, const bool bPlayVFX)
{
	if (!HasAuthority() || !GetWorld() || !DartClass)
	{
		return;
	}
	if (bPlayVFX)
	{
		Multicast_PlayTriggerVFX();
	}
	ADFTrap_DartWall* const Trap = this;
	const FVector Fwd = GetActorRightVector();
	const FVector AimEnd = Fwd * DartTravelDistance;
	const FTransform T = WallMesh ? WallMesh->GetComponentTransform() : GetActorTransform();
	TArray<FVector> Offs = DartSpawnOffsets;
	if (Offs.IsEmpty())
	{
		Offs.Add(FVector(20.f, 0.f, 100.f));
		Offs.Add(FVector(20.f, 0.f, 150.f));
		Offs.Add(FVector(20.f, 0.f, 200.f));
	}
	FActorSpawnParameters Sp;
	Sp.Owner = this;
	Sp.Instigator = Cast<APawn>(InstigatorActor);
	Sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (FVector o : Offs)
	{
		const FVector W = T.TransformPosition(o);
		ADFDartProjectile* P = GetWorld()->SpawnActor<ADFDartProjectile>(DartClass, W, T.GetRotation().Rotator(), Sp);
		if (P)
		{
			P->OwningTrap = Trap;
			P->EffectInstigator = InstigatorActor;
			P->HitDamage = DamageAmount;
			if (P->ProjectileMove)
			{
				P->ProjectileMove->InitialSpeed = DartSpeed;
			}
			P->FireInDirection(Fwd);
		}
	}
}

void ADFTrap_DartWall::TimedFire()
{
	if (bIsTripwire || !HasAuthority())
	{
		return;
	}
	FireAllDarts(nullptr, true);
}
