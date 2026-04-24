// Source/DungeonForged/Private/Dungeon/Traps/ADFPoisonCloudActor.cpp
#include "Dungeon/Traps/ADFPoisonCloudActor.h"
#include "Dungeon/Traps/ADFTrapBase.h"
#include "GAS/Effects/UGE_DoT_Poison.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "AbilitySystemInterface.h"

ADFPoisonCloudActor::ADFPoisonCloudActor()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	CloudVolume = CreateDefaultSubobject<USphereComponent>(TEXT("CloudVolume"));
	RootComponent = CloudVolume;
	CloudVolume->SetCollisionObjectType(ECC_WorldDynamic);
	CloudVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	CloudVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CloudVolume->SetGenerateOverlapEvents(true);

	CloudNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("CloudNiagara"));
	CloudNiagara->SetupAttachment(RootComponent);
}

void ADFPoisonCloudActor::BeginPlay()
{
	Super::BeginPlay();
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			PoisonTickHandle,
			this,
			&ADFPoisonCloudActor::OnPoisonTick,
			TickInterval,
			true);
	}
}

void ADFPoisonCloudActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(PoisonTickHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ADFPoisonCloudActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TickClientCloudUi();
}

void ADFPoisonCloudActor::TickClientCloudUi() const
{
	bool b = false;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (APawn* P = PC->GetPawn())
		{
			if (CloudVolume->IsOverlappingActor(P))
			{
				b = true;
			}
		}
	}
	ADFPoisonCloudActor* Mutable = const_cast<ADFPoisonCloudActor*>(this);
	if (b != bClientIsInCloud)
	{
		Mutable->bClientIsInCloud = b;
		Mutable->OnClientCloudVisualChanged(b);
	}
}

void ADFPoisonCloudActor::OnPoisonTick()
{
	ADFTrapBase* const T = SourceTrap.Get();
	if (!T || !HasAuthority() || !CloudVolume)
	{
		return;
	}
	TSet<AActor*> Over;
	CloudVolume->GetOverlappingActors(Over, APawn::StaticClass());
	for (AActor* A : Over)
	{
		if (Cast<IAbilitySystemInterface>(A) && T)
		{
			T->ApplyEffectWithMagnitude(
				A, A, UGE_DoT_Poison::StaticClass(), DoTTickStrength, DoTDurationSeconds);
		}
	}
}
