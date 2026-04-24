// Source/DungeonForged/Private/Dungeon/Traps/ADFTrapBase.cpp
#include "Dungeon/Traps/ADFTrapBase.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffect.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"

namespace
{
FGameplayTag ResolveDurationTag()
{
	return FDFGameplayTags::Data_Duration.IsValid()
		? FDFGameplayTags::Data_Duration
		: FGameplayTag::RequestGameplayTag(FName("Data.Duration"), false);
}
} // namespace

ADFTrapBase::ADFTrapBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	TrapAbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("TrapAbilitySystem"));
	TrapAbilitySystem->SetIsReplicated(false);

	USceneComponent* R = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = R;

	StateNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("StateNiagara"));
	StateNiagaraComponent->SetupAttachment(RootComponent);
	StateNiagaraComponent->SetAutoDestroy(false);
	StateNiagaraComponent->SetCanEverAffectNavigation(false);
}

void ADFTrapBase::BeginPlay()
{
	Super::BeginPlay();
	if (TrapAbilitySystem)
	{
		TrapAbilitySystem->InitAbilityActorInfo(this, this);
	}
	UpdateStateNiagaraFromArmed();
}

void ADFTrapBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* const W = GetWorld())
	{
		FTimerManager& T = W->GetTimerManager();
		T.ClearTimer(RearmTimer);
		T.ClearTimer(TriggerDelayTimer);
		T.ClearTimer(TelegraphTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void ADFTrapBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADFTrapBase, bIsArmed);
}

void ADFTrapBase::CreateTrapContext(FGameplayEffectContextHandle& Ctx, AActor* InstigatorPawn) const
{
	if (TrapAbilitySystem)
	{
		Ctx = TrapAbilitySystem->MakeEffectContext();
	}
	Ctx.AddSourceObject(const_cast<ADFTrapBase*>(this));
	if (APawn* const P = Cast<APawn>(InstigatorPawn))
	{
		Ctx.AddInstigator(P, P);
	}
}

void ADFTrapBase::Disarm()
{
	if (!HasAuthority())
	{
		return;
	}
	bIsArmed = false;
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(RearmTimer);
	}
	if (StateNiagaraComponent)
	{
		if (DisabledVFX)
		{
			StateNiagaraComponent->SetAsset(DisabledVFX);
			StateNiagaraComponent->Activate(true);
		}
		else
		{
			StateNiagaraComponent->Deactivate();
		}
	}
}

void ADFTrapBase::Rearm()
{
	if (HasAuthority())
	{
		bIsArmed = true;
	}
	UpdateStateNiagaraFromArmed();
}

void ADFTrapBase::UpdateStateNiagaraFromArmed() const
{
	if (!StateNiagaraComponent)
	{
		return;
	}
	if (bIsArmed && ActiveVFX)
	{
		StateNiagaraComponent->SetAsset(ActiveVFX);
		StateNiagaraComponent->Activate(true);
	}
	else if (!bIsArmed && DisabledVFX)
	{
		StateNiagaraComponent->SetAsset(DisabledVFX);
		StateNiagaraComponent->Activate(true);
	}
}

void ADFTrapBase::OnTelegraphForTrigger(AActor* InstigatorActor)
{
	// bIsArmed is cleared at StartTrigger start; this runs mid–fire cycle.
	TelegraphActivation(InstigatorActor);
}

void ADFTrapBase::OnTriggerDelayElapsed(AActor* InstigatorActor)
{
	// Subclasses: damage / behaviour; end with CompleteTrap(Inst) or use default
	// OnTrapTriggered_Implementation (calls CompleteTrap).
	OnTrapTriggered(InstigatorActor);
}

void ADFTrapBase::CompleteTrap(AActor* InstigatorActor)
{
	if (HasAuthority())
	{
		Multicast_PlayTriggerVFX();
	}
	if (InstigatorActor && TrapEffect && HasAuthority())
	{
		TryApplyTrapEffect(InstigatorActor, InstigatorActor);
	}
	if (!HasAuthority())
	{
		return;
	}
	if (!bIsRepeating)
	{
		Disarm();
	}
	else
	{
		ScheduleRearm();
	}
}

void ADFTrapBase::Multicast_PlayTriggerVFX_Implementation()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	SpawnNiagaraAtTrap(TriggerVFX);
}

void ADFTrapBase::StartTriggerSequence(AActor* InstigatorActor)
{
	if (!bIsArmed)
	{
		return;
	}
	if (!HasAuthority())
	{
		return;
	}
	// While in a fire cycle, avoid re-entrancy until CompleteTrap / Disarm.
	bIsArmed = false;
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	FTimerManager& T = W->GetTimerManager();
	T.ClearTimer(TriggerDelayTimer);
	if (FMath::IsNearlyZero(TriggerDelay))
	{
		OnTelegraphForTrigger(InstigatorActor);
		OnTriggerDelayElapsed(InstigatorActor);
	}
	else
	{
		OnTelegraphForTrigger(InstigatorActor);
		T.SetTimer(
			TriggerDelayTimer,
			FTimerDelegate::CreateUObject(this, &ADFTrapBase::OnTriggerDelayElapsed, InstigatorActor),
			TriggerDelay,
			false);
	}
}

void ADFTrapBase::ScheduleRearm()
{
	UWorld* const W = GetWorld();
	if (!W || !HasAuthority() || FMath::IsNearlyZero(RearmDelay))
	{
		Rearm();
		return;
	}
	W->GetTimerManager().SetTimer(
		RearmTimer, this, &ADFTrapBase::OnRearmTimerFired, RearmDelay, false);
}

void ADFTrapBase::OnRearmTimerFired()
{
	Rearm();
}

void ADFTrapBase::TryApplyTrapEffect(AActor* Target, AActor* InstigatorPawn) const
{
	if (!Target || !TrapEffect)
	{
		return;
	}
	UAbilitySystemComponent* const TASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TASC || !TrapAbilitySystem)
	{
		return;
	}
	FGameplayEffectContextHandle Cx;
	CreateTrapContext(Cx, InstigatorPawn);
	const FGameplayEffectSpecHandle S = TrapAbilitySystem->MakeOutgoingSpec(TrapEffect, 1.f, Cx);
	if (S.IsValid() && S.Data)
	{
		TrapAbilitySystem->ApplyGameplayEffectSpecToTarget(*S.Data, TASC);
	}
}

void ADFTrapBase::ApplyDamageGE(
	AActor* Target,
	AActor* InstigatorPawn,
	TSubclassOf<UGameplayEffect> const EffectClass) const
{
	if (!Target || !EffectClass)
	{
		return;
	}
	UAbilitySystemComponent* const TASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TASC || !TrapAbilitySystem)
	{
		return;
	}
	FGameplayEffectContextHandle Cx;
	CreateTrapContext(Cx, InstigatorPawn);
	const FGameplayEffectSpecHandle S = TrapAbilitySystem->MakeOutgoingSpec(EffectClass, 1.f, Cx);
	if (!S.IsValid() || !S.Data)
	{
		return;
	}
	if (FDFGameplayTags::Data_Damage.IsValid() && DamageAmount > 0.f)
	{
		S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, DamageAmount);
	}
	TrapAbilitySystem->ApplyGameplayEffectSpecToTarget(*S.Data, TASC);
}

void ADFTrapBase::ApplyEffectWithMagnitude(
	AActor* const Target,
	AActor* const InstigatorPawn,
	const TSubclassOf<UGameplayEffect> EffectClass,
	const float DamageMagnitude,
	const float OptionalDuration) const
{
	if (!Target || !EffectClass)
	{
		return;
	}
	UAbilitySystemComponent* const TASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TASC || !TrapAbilitySystem)
	{
		return;
	}
	FGameplayEffectContextHandle Cx;
	CreateTrapContext(Cx, InstigatorPawn);
	const FGameplayEffectSpecHandle S = TrapAbilitySystem->MakeOutgoingSpec(EffectClass, 1.f, Cx);
	if (!S.IsValid() || !S.Data)
	{
		return;
	}
	if (FDFGameplayTags::Data_Damage.IsValid() && DamageMagnitude > 0.f)
	{
		S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, DamageMagnitude);
	}
	if (OptionalDuration >= 0.f)
	{
		const FGameplayTag Du = ResolveDurationTag();
		if (Du.IsValid())
		{
			S.Data->SetSetByCallerMagnitude(Du, FMath::Max(0.f, OptionalDuration));
		}
	}
	TrapAbilitySystem->ApplyGameplayEffectSpecToTarget(*S.Data, TASC);
}

void ADFTrapBase::TelegraphActivation_Implementation(AActor* /*InstigatorActor*/)
{
	// Subclasses: audio / flash / mesh tint.
}

void ADFTrapBase::OnTrapTriggered_Implementation(AActor* InstigatorActor)
{
	// No subclass logic: VFX + optional TrapEffect + disarm or rearm.
	CompleteTrap(InstigatorActor);
}

void ADFTrapBase::SpawnNiagaraAtTrap(UNiagaraSystem* const FX) const
{
	if (!FX)
	{
		return;
	}
	const FVector L = GetActorLocation();
	const FRotator R = GetActorRotation();
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(), FX, L, R, FVector(1.f), true, true, ENCPoolMethod::None, true);
}

TArray<UPrimitiveComponent*> ADFTrapBase::GetHighlightPrimitives_Implementation() const
{
	return {};
}

void ADFTrapBase::SetTrapHighlight(const bool bEnabled)
{
	for (UPrimitiveComponent* P : GetHighlightPrimitives())
	{
		if (P)
		{
			P->SetRenderCustomDepth(bEnabled);
			P->SetCustomDepthStencilValue(bEnabled ? CustomDepthStencilValue : 0);
		}
	}
}

bool ADFTrapBase::CanBeSeen_Implementation() const
{
	return !bIsHidden;
}
