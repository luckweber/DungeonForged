// Source/DungeonForged/Private/Dungeon/Traps/ADFDartProjectile.cpp
#include "Dungeon/Traps/ADFDartProjectile.h"
#include "Dungeon/Traps/ADFTrapBase.h"
#include "GAS/Effects/UGE_Damage_Physical.h"
#include "GAS/Effects/UGE_Debuff_Slow.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "AbilitySystemInterface.h"

ADFDartProjectile::ADFDartProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);

	DartMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DartMesh"));
	RootComponent = DartMesh;
	DartMesh->SetCollisionObjectType(ECC_WorldDynamic);
	DartMesh->SetCollisionResponseToAllChannels(ECR_Block);
	DartMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	DartMesh->SetSimulatePhysics(false);
	DartMesh->SetNotifyRigidBodyCollision(true);
	DartMesh->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	DartMesh->BodyInstance.SetObjectType(ECC_WorldDynamic);
	DartMesh->BodyInstance.SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	DartMesh->OnComponentHit.AddDynamic(this, &ADFDartProjectile::OnImpact);

	ProjectileMove = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMove"));
	ProjectileMove->bRotationFollowsVelocity = true;
	ProjectileMove->bShouldBounce = false;
	ProjectileMove->ProjectileGravityScale = 0.f;
	ProjectileMove->SetUpdatedComponent(DartMesh);
}

void ADFDartProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority() && OwningTrap.IsValid())
	{
		if (const ADFTrapBase* T = OwningTrap.Get())
		{
			// Mirror trap designer speed; trap sets InitialSpeed in FireInDirection.
			ProjectileMove->MaxSpeed = FMath::Max(ProjectileMove->MaxSpeed, ProjectileMove->InitialSpeed);
		}
	}
}

void ADFDartProjectile::FireInDirection(const FVector& Dir)
{
	if (!ProjectileMove)
	{
		return;
	}
	const FVector N = Dir.GetSafeNormal();
	if (N.IsZero())
	{
		return;
	}
	ProjectileMove->Velocity = N * ProjectileMove->InitialSpeed;
	ProjectileMove->SetUpdatedComponent(DartMesh);
	ProjectileMove->UpdateComponentVelocity();
}

void ADFDartProjectile::OnImpact(
	UPrimitiveComponent* const /*HitComp*/,
	AActor* const OtherActor,
	UPrimitiveComponent* const OtherComp,
	FVector const /*Impulse*/,
	const FHitResult& Hit)
{
	if (bDealt || !HasAuthority())
	{
		return;
	}
	bDealt = true;
	APawn* const InstP = EffectInstigator.IsValid() ? Cast<APawn>(EffectInstigator.Get()) : nullptr;
	if (ADFTrapBase* const T = OwningTrap.Get())
	{
		if (IAbilitySystemInterface* const I = Cast<IAbilitySystemInterface>(OtherActor))
		{
			if (I->GetAbilitySystemComponent())
			{
				T->ApplyEffectWithMagnitude(OtherActor, InstP, UGE_Damage_Physical::StaticClass(), HitDamage, -1.f);
				T->ApplyEffectWithMagnitude(OtherActor, InstP, UGE_Debuff_Slow::StaticClass(), 0.f, SlowDurationSeconds);
			}
		}
	}
	if (ProjectileMove)
	{
		ProjectileMove->SetUpdatedComponent(nullptr);
		ProjectileMove->StopMovementImmediately();
		ProjectileMove->SetActive(false);
	}
	if (DartMesh)
	{
		DartMesh->SetSimulatePhysics(false);
		DartMesh->SetNotifyRigidBodyCollision(false);
		if (OtherComp)
		{
			const FName Bone = Hit.BoneName;
			DartMesh->AttachToComponent(OtherComp, FAttachmentTransformRules::KeepWorldTransform, Bone);
		}
	}
	SetLifeSpan(60.f);
}
