// Source/DungeonForged/Private/Combat/DFFireballProjectile.cpp
#include "Combat/DFFireballProjectile.h"
#include "Combat/UDFProjectileHitTrackerComponent.h"
#include "Combat/UDFProjectilePoolLibrary.h"
#include "Combat/UDFProjectileSweepComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "FX/UDFCombatFeedbackLibrary.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"

ADFFireballProjectile::ADFFireballProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetSphereRadius(20.f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionSphere->SetCollisionProfileName(TEXT("BlockAll"));
	CollisionSphere->SetCanEverAffectNavigation(false);
	CollisionSphere->SetGenerateOverlapEvents(false);
	RootComponent = CollisionSphere;
	CollisionSphere->OnComponentHit.AddDynamic(this, &ADFFireballProjectile::OnHit);
	HitTracker = CreateDefaultSubobject<UDFProjectileHitTrackerComponent>(TEXT("HitTracker"));

	ProjectileMove = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMove"));
	ProjectileMove->bRotationFollowsVelocity = true;
	ProjectileMove->InitialSpeed = 2000.f;
	ProjectileMove->MaxSpeed = 2000.f;
	ProjectileMove->ProjectileGravityScale = 0.f;
	ProjectileMove->bShouldBounce = false;

	TrailVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailVFX"));
	TrailVFX->SetupAttachment(RootComponent);
	TrailVFX->bAutoActivate = false;
	ProjectileSweep = CreateDefaultSubobject<UDFProjectileSweepComponent>(TEXT("ProjectileSweep"));
}

FName ADFFireballProjectile::GetPoolName() const
{
	return FName(TEXT("FireballProjectile"));
}

void ADFFireballProjectile::OnAcquiredFromPool()
{
	if (HitTracker)
	{
		HitTracker->ClearHitHistory();
	}
	if (ProjectileMove)
	{
		ProjectileMove->Velocity = GetActorForwardVector() * ProjectileMove->InitialSpeed;
		ProjectileMove->UpdateComponentVelocity();
	}
	if (ProjectileSweep)
	{
		ProjectileSweep->ResetTraceSegment();
	}
	SetLifeSpan(0.f);
}

void ADFFireballProjectile::OnReleasedToPool()
{
	if (TrailVFX)
	{
		TrailVFX->Deactivate();
	}
	if (ProjectileMove)
	{
		ProjectileMove->StopMovementImmediately();
	}
}

void ADFFireballProjectile::FinishProjectile()
{
	UDFProjectilePoolLibrary::FinishProjectile(this, this);
}

void ADFFireballProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* Other, UPrimitiveComponent* OtherComp,
	FVector Impulse, const FHitResult& Hit)
{
	(void)HitComponent;
	(void)OtherComp;
	(void)Impulse;
	if (!HasAuthority() || !IsValid(Other) || Other == this || Other == GetInstigator() || Other == GetOwner())
	{
		if (HasAuthority())
		{
			FinishProjectile();
		}
		return;
	}
	if (HitTracker && !HitTracker->TryRegisterHit(Other))
	{
		FinishProjectile();
		return;
	}
	ApplyFireDamageTo(Other, Hit);
	FinishProjectile();
}

void ADFFireballProjectile::OnSweepHit(const FHitResult& Hit, UPrimitiveComponent* const SweptComponent)
{
	OnHit(SweptComponent, Hit.GetActor(), Hit.GetComponent(), FVector::ZeroVector, Hit);
}

void ADFFireballProjectile::ApplyFireDamageTo(AActor* Target, const FHitResult& Hit)
{
	if (!IsValid(FireDamageEffect) || !IsValid(Target))
	{
		return;
	}
	APawn* const Inst = GetInstigator();
	if (!IsValid(Inst))
	{
		return;
	}
	const IAbilitySystemInterface* const IASI = Cast<IAbilitySystemInterface>(Inst);
	UAbilitySystemComponent* const SourceASC =
		IASI ? IASI->GetAbilitySystemComponent() : UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Inst);
	if (!SourceASC)
	{
		return;
	}
	UAbilitySystemComponent* const TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC)
	{
		return;
	}
	// DFDamageCalculation uses: (SetByCaller + Intelligence * 0.5) * mitigation * crit; SetByCaller is Strength.
	const float SetByCallerMagnitude = SourceASC->GetNumericAttribute(UDFAttributeSet::GetStrengthAttribute());

	FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
	Ctx.AddInstigator(Inst, Inst);
	Ctx.AddHitResult(Hit);
	Ctx.AddSourceObject(this);
	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(FireDamageEffect, 1.f, Ctx);
	if (Spec.IsValid() && Spec.Data)
	{
		const FGameplayTag DataTag = FDFGameplayTags::ResolveDataDamageTag();
		if (DataTag.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(DataTag, SetByCallerMagnitude);
		}
		if (FDFGameplayTags::Effect_Element_Fire.IsValid())
		{
			Spec.Data->AddDynamicAssetTag(FDFGameplayTags::Effect_Element_Fire);
		}
		UDFCombatFeedbackLibrary::MarkSpecCombatFeedbackCentralized(*Spec.Data.Get());
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
		UDFCombatFeedbackLibrary::DispatchProjectileHitConfirmed(
			this, Inst, Target, Hit, SetByCallerMagnitude, 0.f, FDFGameplayTags::Effect_Element_Fire);
	}
	if (FireDoTEffect)
	{
		const FGameplayEffectSpecHandle DoT = SourceASC->MakeOutgoingSpec(FireDoTEffect, 1.f, Ctx);
		if (DoT.IsValid() && DoT.Data)
		{
			SourceASC->ApplyGameplayEffectSpecToTarget(*DoT.Data, TargetASC);
		}
	}
}

void ADFFireballProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (IsValid(TrailNiagara) && IsValid(TrailVFX))
	{
		TrailVFX->SetAsset(TrailNiagara);
		TrailVFX->Activate(true);
	}
	if (ProjectileSweep)
	{
		ProjectileSweep->OnSweepHit.AddDynamic(this, &ADFFireballProjectile::OnSweepHit);
		ProjectileSweep->ResetTraceSegment();
	}
}
