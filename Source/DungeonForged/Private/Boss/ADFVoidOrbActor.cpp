// Source/DungeonForged/Private/Boss/ADFVoidOrbActor.cpp
#include "Boss/ADFVoidOrbActor.h"
#include "Boss/ADFBossBase.h"
#include "GAS/Abilities/Boss/UDFBossAbility_VoidBarrier.h"
#include "GAS/DFGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GAS/Effects/UDFGameplayEffectLibrary.h"
#include "GAS/Effects/UGE_Debuff_Slow.h"
#include "Components/SphereComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Controller.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"

ADFVoidOrbActor::ADFVoidOrbActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetCanBeDamaged(true);
	Body = CreateDefaultSubobject<USphereComponent>(TEXT("Body"));
	RootComponent = Body;
	Body->InitSphereRadius(28.f);
	Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Body->SetCollisionObjectType(ECC_WorldDynamic);
	Body->SetCollisionResponseToAllChannels(ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Body->OnComponentBeginOverlap.AddDynamic(this, &ADFVoidOrbActor::OnSphereOverlap);
}

void ADFVoidOrbActor::Init(UDFBossAbility_VoidBarrier* const Owning, ADFBossBase* const InBoss, const FVector& OrbitOffsetWorld)
{
	OwnerAbility = Owning;
	Boss = InBoss;
	const float R = OrbitOffsetWorld.Size2D();
	OrbitAngle = R > KINDA_SMALL_NUMBER ? FMath::Atan2(OrbitOffsetWorld.Y, OrbitOffsetWorld.X) : 0.f;
	if (Boss)
	{
		const FVector B = Boss->GetActorLocation();
		SetActorLocation(B + FVector(OrbitOffsetWorld.X, OrbitOffsetWorld.Y, 80.f));
	}
}

void ADFVoidOrbActor::UpdateOrbit(const float Dt)
{
	if (!Boss)
	{
		return;
	}
	OrbitAngle += OrbitSpeed * Dt;
	const float R = 300.f;
	const FVector B = Boss->GetActorLocation();
	SetActorLocation(B + FVector(FMath::Cos(OrbitAngle) * R, FMath::Sin(OrbitAngle) * R, 80.f));
}

void ADFVoidOrbActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateOrbit(DeltaSeconds);
}

float ADFVoidOrbActor::TakeDamage(
	const float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	(void)DamageEvent;
	(void)EventInstigator;
	(void)DamageCauser;
	if (DamageAmount <= 0.f)
	{
		return 0.f;
	}
	Health -= DamageAmount;
	if (Health <= 0.f)
	{
		Destroy();
	}
	return DamageAmount;
}

void ADFVoidOrbActor::OnSphereOverlap(
	UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor, UPrimitiveComponent* /*OtherComp*/, const int32 /*OtherBodyIndex*/, const bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	static const float RehitCd = 0.35f;
	if (!GetWorld() || !Boss || !OtherActor || OtherActor == Boss)
	{
		return;
	}
	if (UAbilitySystemComponent* Tgt = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
	{
		if (Tgt->HasMatchingGameplayTag(FDFGameplayTags::State_CCIgnore))
		{
			return;
		}
		const float T = GetWorld()->GetTimeSeconds();
		if (T - LastTickOverlap < RehitCd)
		{
			return;
		}
		LastTickOverlap = T;
		if (UAbilitySystemComponent* Src = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Boss))
		{
			const FGameplayEffectSpecHandle Dmg = UDFGameplayEffectLibrary::MakeDamageEffect(
				80.f, FDFGameplayTags::Effect_Damage_True, Boss);
			if (Dmg.IsValid() && Dmg.Data)
			{
				Src->ApplyGameplayEffectSpecToTarget(*Dmg.Data, Tgt);
			}
			const FGameplayEffectSpecHandle Sl = Src->MakeOutgoingSpec(UGE_Debuff_Slow::StaticClass(), 1.f, Src->MakeEffectContext());
			if (Sl.IsValid() && Sl.Data.IsValid() && FDFGameplayTags::Data_Duration.IsValid())
			{
				Sl.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, 2.f);
				Src->ApplyGameplayEffectSpecToTarget(*Sl.Data, Tgt);
			}
		}
	}
}

void ADFVoidOrbActor::Orphan()
{
	OwnerAbility = nullptr;
}

void ADFVoidOrbActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (OwnerAbility)
	{
		OwnerAbility->NotifyVoidOrbDestroyed(this);
		OwnerAbility = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}
