// Source/DungeonForged/Private/Combat/DFArcaneMissileProjectile.cpp
#include "Combat/DFArcaneMissileProjectile.h"
#include "GAS/Abilities/Mage/UDFAbility_Mage_ArcaneBarrage.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GAS/Effects/UGE_Damage_Magic.h"
#include "GAS/Effects/UGE_Debuff_Silence.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GameplayEffect.h"

ADFArcaneMissileProjectile::ADFArcaneMissileProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetSphereRadius(14.f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionSphere->SetCollisionProfileName(TEXT("BlockAll"));
	RootComponent = CollisionSphere;
	CollisionSphere->OnComponentHit.AddDynamic(this, &ADFArcaneMissileProjectile::OnHit);

	ProjectileMove = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMove"));
	ProjectileMove->bRotationFollowsVelocity = true;
	ProjectileMove->InitialSpeed = 3000.f;
	ProjectileMove->MaxSpeed = 3000.f;
	ProjectileMove->ProjectileGravityScale = 0.f;
	ProjectileMove->bIsHomingProjectile = true;
	ProjectileMove->HomingAccelerationMagnitude = 1800.f;

	MagicDamageEffect = UGE_Damage_Magic::StaticClass();
	OverloadDamageEffect = UGE_Damage_Magic::StaticClass();
	SilenceEffect = UGE_Debuff_Silence::StaticClass();
}

void ADFArcaneMissileProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (IsValid(HomingTarget) && IsValid(HomingTarget->GetRootComponent()))
	{
		ProjectileMove->HomingTargetComponent = HomingTarget->GetRootComponent();
	}
}

void ADFArcaneMissileProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* Other, UPrimitiveComponent* OtherComp, FVector Impulse, const FHitResult& Hit)
{
	(void)HitComponent;
	(void)OtherComp;
	(void)Impulse;
	if (!HasAuthority() || !IsValid(Other) || Other == this || Other == GetInstigator() || Other == GetOwner())
	{
		if (HasAuthority())
		{
			Destroy();
		}
		return;
	}
	APawn* const Inst = GetInstigator();
	if (!IsValid(Inst) || !MagicDamageEffect)
	{
		Destroy();
		return;
	}
	UAbilitySystemComponent* const SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Inst);
	UAbilitySystemComponent* const TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Other);
	if (!SourceASC || !TargetASC)
	{
		Destroy();
		return;
	}
	const float Intel = SourceASC->GetNumericAttribute(UDFAttributeSet::GetIntelligenceAttribute());
	const float Sbc = Intel * (-0.1f); // 0.5 + (-0.1) = 0.4 * I
	FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
	Ctx.AddInstigator(Inst, Inst);
	Ctx.AddHitResult(Hit);
	Ctx.AddSourceObject(this);
	const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(MagicDamageEffect, 1.f, Ctx);
	if (Spec.IsValid() && Spec.Data && FDFGameplayTags::Data_Damage.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, Sbc);
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
	}
	if (HitNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, HitNiagara, Hit.ImpactPoint, FRotator::ZeroRotator, FVector(1.f), true, true, ENCPoolMethod::None, true);
	}
	if (UDFAbility_Mage_ArcaneBarrage* const Ab = Cast<UDFAbility_Mage_ArcaneBarrage>(SourceAbility.Get()))
	{
		Ab->NotifyArcaneMissileHit(Other, TargetASC, Ctx);
	}
	Destroy();
}
