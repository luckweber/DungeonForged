// Source/DungeonForged/Private/Combat/DFFrostBoltProjectile.cpp
#include "Combat/DFFrostBoltProjectile.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GAS/Effects/UGE_CrowdControl_Freeze.h"
#include "GAS/Effects/UGE_DoT_Frost.h"
#include "GAS/Effects/UGE_Debuff_FrostSlow.h"
#include "GAS/Effects/UGE_Damage_Magic.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GameplayEffect.h"

ADFFrostBoltProjectile::ADFFrostBoltProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetSphereRadius(18.f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionSphere->SetCollisionProfileName(TEXT("BlockAll"));
	CollisionSphere->SetCanEverAffectNavigation(false);
	RootComponent = CollisionSphere;
	CollisionSphere->OnComponentHit.AddDynamic(this, &ADFFrostBoltProjectile::OnHit);

	ProjectileMove = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMove"));
	ProjectileMove->bRotationFollowsVelocity = true;
	ProjectileMove->InitialSpeed = 2400.f;
	ProjectileMove->MaxSpeed = 2400.f;
	ProjectileMove->ProjectileGravityScale = 0.f;
	ProjectileMove->bIsHomingProjectile = true;
	ProjectileMove->HomingAccelerationMagnitude = 2400.f;

	MagicDamageEffect = UGE_Damage_Magic::StaticClass();
	FrostSlowEffect = UGE_Debuff_FrostSlow::StaticClass();
	DoTFrostEffect = UGE_DoT_Frost::StaticClass();
	FreezeEffect = UGE_CrowdControl_Freeze::StaticClass();
}

void ADFFrostBoltProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (IsValid(HomingTarget) && IsValid(HomingTarget->GetRootComponent()))
	{
		ProjectileMove->HomingTargetComponent = HomingTarget->GetRootComponent();
	}
}

void ADFFrostBoltProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* Other, UPrimitiveComponent* OtherComp, FVector Impulse, const FHitResult& Hit)
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
	ApplyFrostTo(Other, Hit);
	Destroy();
}

void ADFFrostBoltProjectile::ApplyFrostTo(AActor* Target, const FHitResult& InHit)
{
	APawn* const Inst = GetInstigator();
	if (!IsValid(Inst) || !IsValid(Target) || !MagicDamageEffect)
	{
		return;
	}
	UAbilitySystemComponent* const SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Inst);
	UAbilitySystemComponent* const TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!SourceASC || !TargetASC)
	{
		return;
	}
	const float Intel = SourceASC->GetNumericAttribute(UDFAttributeSet::GetIntelligenceAttribute());
	const float Sbc = Intel * 0.3f; // 0.5*I (exec) + 0.3*I = 0.8*I
	FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
	Ctx.AddInstigator(Inst, Inst);
	Ctx.AddHitResult(InHit);
	Ctx.AddSourceObject(this);
	const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(MagicDamageEffect, 1.f, Ctx);
	if (Spec.IsValid() && Spec.Data)
	{
		if (FDFGameplayTags::Data_Damage.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, Sbc);
		}
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
	}
	const bool bHasFrostDoT = FDFGameplayTags::Effect_DoT_Frost.IsValid() && TargetASC->HasMatchingGameplayTag(FDFGameplayTags::Effect_DoT_Frost);
	if (bHasFrostDoT && FreezeEffect)
	{
		const FGameplayEffectSpecHandle Fr = SourceASC->MakeOutgoingSpec(FreezeEffect, 1.f, Ctx);
		if (Fr.IsValid() && Fr.Data && FDFGameplayTags::Data_Duration.IsValid())
		{
			Fr.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, 1.5f);
			SourceASC->ApplyGameplayEffectSpecToTarget(*Fr.Data, TargetASC);
		}
	}
	else if (FrostSlowEffect)
	{
		const FGameplayEffectSpecHandle Ss = SourceASC->MakeOutgoingSpec(FrostSlowEffect, 1.f, Ctx);
		if (Ss.IsValid() && Ss.Data && FDFGameplayTags::Data_Duration.IsValid())
		{
			Ss.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, 3.f);
			SourceASC->ApplyGameplayEffectSpecToTarget(*Ss.Data, TargetASC);
		}
	}
	if (HitNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, HitNiagara, InHit.ImpactPoint, FRotator::ZeroRotator, FVector(1.f), true, true, ENCPoolMethod::None, true);
	}
}
