// Source/DungeonForged/Private/Dungeon/Traps/ADFTrap_PoisonVent.cpp
#include "Dungeon/Traps/ADFTrap_PoisonVent.h"
#include "Dungeon/Traps/ADFPoisonCloudActor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

ADFTrap_PoisonVent::ADFTrap_PoisonVent()
{
	RearmDelay = 9.f; // user may prefer Cloud+4 in defaults
	TriggerDelay = 0.f;
	bIsRepeating = true;
	VentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VentMesh"));
	RootComponent = VentMesh;
	DetectionRange = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionRange"));
	DetectionRange->SetupAttachment(RootComponent);
	DetectionRange->SetSphereRadius(PlayerDetectionRadius);
	DetectionRange->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionRange->SetGenerateOverlapEvents(true);
	DetectionRange->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionRange->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ADFTrap_PoisonVent::BeginPlay()
{
	Super::BeginPlay();
	if (UWorld* W = GetWorld())
	{
		// Recompute editor radius into runtime overlap.
		DetectionRange->SetSphereRadius(PlayerDetectionRadius);
	}
	DetectionRange->OnComponentBeginOverlap.AddDynamic(
		this,
		&ADFTrap_PoisonVent::OnDetectBegin);
	RearmDelay = FMath::Max(0.1f, CloudDuration + PostCloudRearmBuffer);
}

void ADFTrap_PoisonVent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(PlayerDetectHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ADFTrap_PoisonVent::OnDetectBegin(
	UPrimitiveComponent* const /*Overlapped*/,
	AActor* const Other,
	UPrimitiveComponent* const /*OtherComp*/,
	int32 const /*OtherBodyIndex*/,
	bool const /*bFromSweep*/,
	const FHitResult& /*Hit*/)
{
	if (!HasAuthority() || !bIsArmed)
	{
		return;
	}
	// "Player" focus: your Character / controller pawn.
	APawn* const P = Cast<APawn>(Other);
	if (!P)
	{
		return;
	}
	if (!P->GetController() || !P->GetController()->IsA<APlayerController>())
	{
		return;
	}
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			PlayerDetectHandle,
			FTimerDelegate::CreateUObject(
				this,
				&ADFTrap_PoisonVent::OnDetectDelayFired,
				P),
			PlayerDetectDelay,
			false);
	}
}

void ADFTrap_PoisonVent::OnDetectDelayFired(APawn* P)
{
	if (!HasAuthority() || !bIsArmed)
	{
		return;
	}
	if (P == nullptr || !IsValid(P))
	{
		return;
	}
	// If they left, abort.
	if (!DetectionRange->IsOverlappingActor(P))
	{
		return;
	}
	// Begin sequence: this trap uses custom completion (rearm) after cloud+wait.
	bIsArmed = false;
	if (VentActuateSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, VentActuateSound, GetActorLocation());
	}
	FActorSpawnParameters Sp;
	Sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Sp.Owner = this;
	ADFPoisonCloudActor* C = GetWorld()
		? GetWorld()->SpawnActor<ADFPoisonCloudActor>(CloudClass, GetActorLocation(), GetActorRotation(), Sp)
		: nullptr;
	if (C)
	{
		ActiveCloud = C;
		C->SourceTrap = this;
		C->CloudVolume->SetSphereRadius(CloudRadius);
	}
	if (C)
	{
		C->SetLifeSpan(CloudDuration);
	}
	// Re-arm after the cloud lifetime + buffer; mirrors RearmDelay.
	ScheduleRearm();
}
