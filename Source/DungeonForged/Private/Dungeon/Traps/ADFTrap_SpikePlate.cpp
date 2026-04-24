// Source/DungeonForged/Private/Dungeon/Traps/ADFTrap_SpikePlate.cpp
#include "Dungeon/Traps/ADFTrap_SpikePlate.h"
#include "AbilitySystemInterface.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TimelineComponent.h"
#include "GAS/Effects/UGE_Damage_Physical.h"
#include "GAS/Effects/UGE_DoT_Bleed.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
bool ActorHasGAS(AActor* const A)
{
	if (const IAbilitySystemInterface* const I = Cast<IAbilitySystemInterface>(A))
	{
		return I->GetAbilitySystemComponent() != nullptr;
	}
	return false;
}
} // namespace

ADFTrap_SpikePlate::ADFTrap_SpikePlate()
{
	bIsHidden = true;
	TriggerDelay = 0.3f;
	RearmDelay = 1.5f;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(RootComponent);
	TriggerBox->SetBoxExtent(FVector(50.f, 50.f, 32.f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);

	PlateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlateMesh"));
	PlateMesh->SetupAttachment(RootComponent);
	PlateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SpikesMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpikesMesh"));
	SpikesMesh->SetupAttachment(PlateMesh);
	SpikesMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SpikeTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("SpikeTimeline"));
}

void ADFTrap_SpikePlate::BeginPlay()
{
	Super::BeginPlay();
	if (SpikesMesh)
	{
		SpikesStartRelative = SpikesMesh->GetRelativeLocation();
		bSpikesStateCached = true;
		AnimateSpikesToAlpha(0.f);
	}
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ADFTrap_SpikePlate::OnTriggerBeginOverlap);
}

void ADFTrap_SpikePlate::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* W = GetWorld())
	{
		FTimerManager& Tm = W->GetTimerManager();
		Tm.ClearTimer(SpikesEmergeEndTimer);
		Tm.ClearTimer(SpikesAfterHoldTimer);
		Tm.ClearTimer(SpikesRetractEndTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void ADFTrap_SpikePlate::OnTriggerBeginOverlap(
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

void ADFTrap_SpikePlate::TelegraphActivation_Implementation(AActor* const /*InstigatorActor*/)
{
	if (TelegraphClickSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, TelegraphClickSound, GetActorLocation());
	}
}

void ADFTrap_SpikePlate::AnimateSpikesToAlpha(const float Alpha) const
{
	if (!SpikesMesh)
	{
		return;
	}
	const float t = FMath::Clamp(Alpha, 0.f, 1.f);
	FVector const L = SpikesStartRelative;
	SpikesMesh->SetRelativeLocation(
		FVector(L.X, L.Y, L.Z + t * SpikesEmergeOffset));
}

void ADFTrap_SpikePlate::ApplySpikeDamage(AActor* const InstigatorActor) const
{
	if (!GetWorld() || !HasAuthority())
	{
		return;
	}
	TArray<AActor*> Ign;
	Ign.Add(const_cast<ADFTrap_SpikePlate*>(this));
	TArray<AActor*> Out;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjTypes;
	ObjTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	UKismetSystemLibrary::BoxOverlapActors(
		this,
		GetActorLocation() + FVector(0.f, 0.f, HitBoxExtent.Z * 0.5f),
		HitBoxExtent,
		ObjTypes,
		APawn::StaticClass(),
		Ign,
		Out);
	for (AActor* A : Out)
	{
		if (!ActorHasGAS(A))
		{
			continue;
		}
		ApplyEffectWithMagnitude(A, InstigatorActor, UGE_Damage_Physical::StaticClass(), DamageAmount, -1.f);
		ApplyEffectWithMagnitude(
			A, InstigatorActor, UGE_DoT_Bleed::StaticClass(), 0.2f * DamageAmount, BleedDurationSeconds);
	}
}

void ADFTrap_SpikePlate::StartRetractAfterHold(AActor* const InstigatorActor)
{
	if (UWorld* W = GetWorld())
	{
		const float t = FMath::Max(0.01f, SpikesEmergeTime);
		W->GetTimerManager().SetTimer(
			SpikesRetractEndTimer,
			FTimerDelegate::CreateUObject(this, &ADFTrap_SpikePlate::OnRetractTimerFinished, InstigatorActor),
			t,
			false);
	}
}

void ADFTrap_SpikePlate::OnRetractTimerFinished(AActor* const InstigatorActor)
{
	AnimateSpikesToAlpha(0.f);
	CompleteSpikeSequence(InstigatorActor);
}

void ADFTrap_SpikePlate::CompleteSpikeSequence(AActor* const InstigatorActor)
{
	CompleteTrap(InstigatorActor);
}

void ADFTrap_SpikePlate::OnTrapTriggered_Implementation(AActor* const InstigatorActor)
{
	if (!bSpikesStateCached)
	{
		CompleteSpikeSequence(InstigatorActor);
		return;
	}
	if (SpikesEmergeSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SpikesEmergeSound, GetActorLocation());
	}
	(void)SpikeTimeline;
	(void)SpikeHeightCurve;
	const float RiseT = FMath::Max(0.01f, SpikesEmergeTime);
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			SpikesEmergeEndTimer,
			FTimerDelegate::CreateWeakLambda(
				this,
				[this, InstigatorActor]()
				{
					AnimateSpikesToAlpha(1.f);
					ApplySpikeDamage(InstigatorActor);
					if (UWorld* W2 = GetWorld())
					{
						W2->GetTimerManager().SetTimer(
							SpikesAfterHoldTimer,
							FTimerDelegate::CreateUObject(
								this,
								&ADFTrap_SpikePlate::StartRetractAfterHold,
								InstigatorActor),
							SpikesHoldTime,
							false);
					}
				}),
			RiseT,
			false);
	}
}

bool ADFTrap_SpikePlate::CanBeSeen_Implementation() const
{
	if (!bIsHidden)
	{
		return true;
	}
	const UWorld* W = GetWorld();
	if (!W)
	{
		return false;
	}
	APawn* P = nullptr;
	if (APlayerController* PC = W->GetFirstPlayerController())
	{
		P = PC->GetPawn();
	}
	if (!P)
	{
		return false;
	}
	return FVector::Dist(P->GetActorLocation(), GetActorLocation()) < HiddenViewDistance;
}

TArray<UPrimitiveComponent*> ADFTrap_SpikePlate::GetHighlightPrimitives_Implementation() const
{
	TArray<UPrimitiveComponent*> P;
	if (PlateMesh)
	{
		P.Add(PlateMesh);
	}
	if (SpikesMesh)
	{
		P.Add(SpikesMesh);
	}
	return P;
}
