// Source/DungeonForged/Private/Combat/ADFKnifeProjectile.cpp
#include "Combat/ADFKnifeProjectile.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/DFRogueGAS.h"
#include "GAS/Effects/UGE_Damage_Physical.h"
#include "GAS/Effects/UGE_DoT_Poison.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "NiagaraSystem.h"
#include "Characters/ADFEnemyBase.h"

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
	PhysicalDamageEffect = UGE_Damage_Physical::StaticClass();
	PoisonEffect = UGE_DoT_Poison::StaticClass();
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
			Destroy();
		}
		return;
	}
	// PvE: only hurt enemies; tune for PvP later.
	if (!Cast<ADFEnemyBase>(Other))
	{
		if (bDestroyOnHit)
		{
			Destroy();
		}
		return;
	}
	UAbilitySystemComponent* const Src = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Inst);
	UAbilitySystemComponent* const Tgt = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Other);
	if (Src && Tgt && PhysicalDamageEffect)
	{
		const float Pre = DF_Rogue_CompensatePhysicalSetBy(Src, PhysicalHitDamage);
		const FGameplayEffectContextHandle Ctx = DF_Rogue_EffectContext(Src, Inst, &Hit);
		const FGameplayEffectSpecHandle S = Src->MakeOutgoingSpec(PhysicalDamageEffect, 1.f, Ctx);
		if (S.IsValid() && S.Data && FDFGameplayTags::Data_Damage.IsValid())
		{
			S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, Pre);
			Src->ApplyGameplayEffectSpecToTarget(*S.Data, Tgt);
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
		Destroy();
	}
}
