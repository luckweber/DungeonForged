// Source/DungeonForged/Private/Combat/ADFKnifeProjectile.cpp
#include "Combat/ADFKnifeProjectile.h"
#include "Combat/UDFProjectileHitTrackerComponent.h"
#include "Combat/UDFProjectilePoolLibrary.h"
#include "Combat/UDFProjectileSweepComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/DFRogueGAS.h"
#include "GAS/Effects/UGE_Damage_Physical.h"
#include "GAS/Effects/UGE_DoT_Poison.h"
#include "FX/UDFCombatFeedbackLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "NiagaraSystem.h"

ADFKnifeProjectile::ADFKnifeProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->SetSphereRadius(12.f);
	Collision->SetCollisionProfileName(TEXT("BlockAll"));
	Collision->SetCanEverAffectNavigation(false);
	RootComponent = Collision;
	Move = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Move"));
	Move->bRotationFollowsVelocity = true;
	Move->InitialSpeed = FlightSpeed;
	Move->MaxSpeed = FlightSpeed;
	Move->bIsHomingProjectile = false;
	Move->ProjectileGravityScale = 0.f;
	Collision->OnComponentHit.AddDynamic(this, &ADFKnifeProjectile::OnHit);
	HitTracker = CreateDefaultSubobject<UDFProjectileHitTrackerComponent>(TEXT("HitTracker"));
	ProjectileSweep = CreateDefaultSubobject<UDFProjectileSweepComponent>(TEXT("ProjectileSweep"));
	PhysicalDamageEffect = UGE_Damage_Physical::StaticClass();
	PoisonEffect = UGE_DoT_Poison::StaticClass();
}

FName ADFKnifeProjectile::GetPoolName() const
{
	return FName(TEXT("KnifeProjectile"));
}

void ADFKnifeProjectile::OnAcquiredFromPool()
{
	if (HitTracker)
	{
		HitTracker->ClearHitHistory();
	}
	if (Move)
	{
		Move->MaxSpeed = FlightSpeed;
		Move->InitialSpeed = FlightSpeed;
		Move->Velocity = GetActorForwardVector() * FlightSpeed;
		Move->UpdateComponentVelocity();
	}
	if (ProjectileSweep)
	{
		ProjectileSweep->ResetTraceSegment();
	}
	SetLifeSpan(0.f);
}

void ADFKnifeProjectile::OnReleasedToPool()
{
	PhysicalHitDamage = 0.f;
	PoisonMagnitude = 0.f;
	if (Move)
	{
		Move->StopMovementImmediately();
	}
}

void ADFKnifeProjectile::FinishProjectile()
{
	UDFProjectilePoolLibrary::FinishProjectile(this, this);
}

void ADFKnifeProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (Move)
	{
		Move->MaxSpeed = FlightSpeed;
		Move->InitialSpeed = FlightSpeed;
		Move->Velocity = GetActorForwardVector() * FlightSpeed;
	}
	if (ProjectileSweep)
	{
		ProjectileSweep->OnSweepHit.AddDynamic(this, &ADFKnifeProjectile::OnSweepHit);
		ProjectileSweep->ResetTraceSegment();
	}
}

void ADFKnifeProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* Other, UPrimitiveComponent* OtherComp, FVector N, const FHitResult& Hit)
{
	(void)HitComp;
	(void)N;
	(void)OtherComp;
	APawn* const Inst = GetInstigator();
	if (!HasAuthority() || !IsValid(Other) || !Inst || Other == Inst)
	{
		if (bDestroyOnHit)
		{
			FinishProjectile();
		}
		return;
	}
	UAbilitySystemComponent* const TgtPrecheck = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Other);
	if (!TgtPrecheck)
	{
		if (bDestroyOnHit)
		{
			FinishProjectile();
		}
		return;
	}
	if (HitTracker && !HitTracker->TryRegisterHit(Other))
	{
		if (bDestroyOnHit)
		{
			FinishProjectile();
		}
		return;
	}
	UAbilitySystemComponent* const Src = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Inst);
	UAbilitySystemComponent* const Tgt = TgtPrecheck;
	float AppliedDamage = 0.f;
	if (Src && Tgt && PhysicalDamageEffect)
	{
		const float Pre = DF_Rogue_CompensatePhysicalSetBy(Src, PhysicalHitDamage);
		const FGameplayEffectContextHandle Ctx = DF_Rogue_EffectContext(Src, Inst, &Hit);
		const FGameplayEffectSpecHandle S = Src->MakeOutgoingSpec(PhysicalDamageEffect, 1.f, Ctx);
		if (S.IsValid() && S.Data && FDFGameplayTags::Data_Damage.IsValid())
		{
			S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, Pre);
			if (FDFGameplayTags::Damage_Source_Pierce.IsValid())
			{
				S.Data->AddDynamicAssetTag(FDFGameplayTags::Damage_Source_Pierce);
			}
			UDFCombatFeedbackLibrary::MarkSpecCombatFeedbackCentralized(*S.Data.Get());
			Src->ApplyGameplayEffectSpecToTarget(*S.Data, Tgt);
			AppliedDamage = Pre;
		}
	}
	if (Src && Tgt && PoisonEffect && FDFGameplayTags::Data_Duration.IsValid() && FDFGameplayTags::Data_Damage.IsValid())
	{
		const FGameplayEffectContextHandle Ctx = DF_Rogue_EffectContext(Src, Inst, &Hit);
		const FGameplayEffectSpecHandle S = Src->MakeOutgoingSpec(PoisonEffect, 1.f, Ctx);
		if (S.IsValid() && S.Data)
		{
			S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, 3.f);
			S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, FMath::Max(1.f, PoisonMagnitude));
			Src->ApplyGameplayEffectSpecToTarget(*S.Data, Tgt);
		}
	}
	if (AppliedDamage > KINDA_SMALL_NUMBER)
	{
		UDFCombatFeedbackLibrary::DispatchProjectileHitConfirmed(
			this,
			Inst,
			Other,
			Hit,
			AppliedDamage,
			0.f,
			FDFGameplayTags::Damage_Source_Pierce);
	}
	if (UWorld* const W = GetWorld())
	{
		if (ImpactBladeGlintVFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				this, ImpactBladeGlintVFX, Hit.ImpactPoint, FRotator::ZeroRotator, FVector(1.f), true, true, ENCPoolMethod::None, true);
		}
	}
	if (bDestroyOnHit)
	{
		FinishProjectile();
	}
}

void ADFKnifeProjectile::OnSweepHit(const FHitResult& Hit, UPrimitiveComponent* const SweptComponent)
{
	OnHit(SweptComponent, Hit.GetActor(), Hit.GetComponent(), FVector::ZeroVector, Hit);
}
