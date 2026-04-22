// Source/DungeonForged/Private/Combat/DFFireballProjectile.cpp
#include "Combat/DFFireballProjectile.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GAS/DFNativeGameplayTags.h"
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

	ProjectileMove = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMove"));
	ProjectileMove->bRotationFollowsVelocity = true;
	ProjectileMove->InitialSpeed = 2000.f;
	ProjectileMove->MaxSpeed = 2000.f;
	ProjectileMove->ProjectileGravityScale = 0.f;
	ProjectileMove->bShouldBounce = false;

	TrailVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailVFX"));
	TrailVFX->SetupAttachment(RootComponent);
	TrailVFX->bAutoActivate = false;
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
			Destroy();
		}
		return;
	}
	ApplyFireDamageTo(Other, Hit);
	Destroy();
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
		Spec.Data->SetSetByCallerMagnitude(TAG_DF_Data_Damage, SetByCallerMagnitude);
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
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
}
