// Source/DungeonForged/Private/Dungeon/Traps/ADFTrap_FireJet.cpp
#include "Dungeon/Traps/ADFTrap_FireJet.h"
#include "GAS/Effects/UGE_Damage_True.h"
#include "GAS/Effects/UGE_DoT_Fire.h"
#include "Components/CapsuleComponent.h"
#include "NiagaraComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

ADFTrap_FireJet::ADFTrap_FireJet()
{
	PrimaryActorTick.bCanEverTick = true;
	bIsRepeating = true;
	TriggerDelay = 0.f;
	RearmDelay = 0.f; // not used: internal cycle
	DamageAmount = 12.f; // per tick, tune in editor

	DamageVolume = CreateDefaultSubobject<UCapsuleComponent>(TEXT("DamageVolume"));
	DamageVolume->SetupAttachment(RootComponent);
	DamageVolume->SetCapsuleHalfHeight(120.f);
	DamageVolume->SetCapsuleRadius(60.f);
	DamageVolume->SetCollisionObjectType(ECC_WorldDynamic);
	DamageVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	DamageVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DamageVolume->SetGenerateOverlapEvents(true);
	DamageVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FireJetNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FireJetNiagara"));
	FireJetNiagara->SetupAttachment(RootComponent);
}

void ADFTrap_FireJet::Disarm()
{
	// Intentional no-op: always-on hazard.
}

void ADFTrap_FireJet::ResetCycleTimers()
{
	if (UWorld* W = GetWorld())
	{
		FTimerManager& Tm = W->GetTimerManager();
		Tm.ClearTimer(ActivePhaseTimer);
		Tm.ClearTimer(InactivePhaseTimer);
		Tm.ClearTimer(PreIgniteTimer);
		Tm.ClearTimer(DamageTickTimer);
	}
}

void ADFTrap_FireJet::BeginPlay()
{
	bIsArmed = true;
	Super::BeginPlay();
	ResetCycleTimers();
	if (HasAuthority())
	{
		OnCycleTurnInactive();
	}
}

void ADFTrap_FireJet::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ResetCycleTimers();
	Super::EndPlay(EndPlayReason);
}

void ADFTrap_FireJet::Tick(const float /*DeltaSeconds*/)
{
	UpdateVignetteForLocalPawn();
}

void ADFTrap_FireJet::OnCycleTurnInactive()
{
	if (!HasAuthority())
	{
		return;
	}
	bFireActive = false;
	if (DamageVolume)
	{
		DamageVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (FireJetNiagara)
	{
		FireJetNiagara->Deactivate();
	}
	// Pre-ignite 0.5s before the next active window.
	const float t = FMath::Max(0.01f, FMath::Max(0.f, InactiveDuration - 0.5f));
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			PreIgniteTimer,
			this,
			&ADFTrap_FireJet::OnCyclePreIgnite,
			t,
			false);
	}
}

void ADFTrap_FireJet::OnCyclePreIgnite()
{
	if (!HasAuthority())
	{
		return;
	}
	if (PreIgniteSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PreIgniteSound, GetActorLocation());
	}
	// "Brief flame flicker" in BP: reactivate Niagra briefly, etc.
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			ActivePhaseTimer,
			this,
			&ADFTrap_FireJet::OnCycleTurnActive,
			0.5f,
			false);
	}
}

void ADFTrap_FireJet::OnCycleTurnActive()
{
	if (!HasAuthority())
	{
		return;
	}
	bFireActive = true;
	if (DamageVolume)
	{
		DamageVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	if (FireJetNiagara)
	{
		FireJetNiagara->Activate(true);
	}
	ApplyFireDamageTick();
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			DamageTickTimer,
			this,
			&ADFTrap_FireJet::ApplyFireDamageTick,
			DamageTickInterval,
			true);
	}
	if (UWorld* W2 = GetWorld())
	{
		W2->GetTimerManager().SetTimer(
			InactivePhaseTimer,
			FTimerDelegate::CreateWeakLambda(
				this,
				[this]()
				{
					if (UWorld* W3 = GetWorld())
					{
						W3->GetTimerManager().ClearTimer(DamageTickTimer);
					}
					OnCycleTurnInactive();
				}),
			ActiveDuration,
			false);
	}
}

void ADFTrap_FireJet::ApplyFireDamageTick()
{
	if (!bFireActive || !HasAuthority() || !DamageVolume)
	{
		return;
	}
	TSet<AActor*> Overlap;
	DamageVolume->GetOverlappingActors(Overlap, APawn::StaticClass());
	for (AActor* A : Overlap)
	{
		ApplyEffectWithMagnitude(
			A, A, UGE_Damage_True::StaticClass(), DamageAmount, -1.f);
		ApplyEffectWithMagnitude(
			A, A, UGE_DoT_Fire::StaticClass(), DamageAmount * 0.1f, 3.f);
	}
}

void ADFTrap_FireJet::UpdateVignetteForLocalPawn() const
{
	float Str = 0.f;
	if (APlayerController* const PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (APawn* P = PC->GetPawn())
		{
			if (bFireActive && DamageVolume->IsOverlappingActor(P))
			{
				Str = 0.35f;
			}
		}
	}
	// Cast away const: delegate broadcast from a const tick path.
	ADFTrap_FireJet* MutableThis = const_cast<ADFTrap_FireJet*>(this);
	if (OnLocalVignetteStrength.IsBound())
	{
		MutableThis->OnLocalVignetteStrength.Broadcast(Str);
	}
}
