// Source/DungeonForged/Private/Boss/ADFMeteorImpactActor.cpp
#include "Boss/ADFMeteorImpactActor.h"
#include "Boss/ADFBossBase.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UDFGameplayEffectLibrary.h"
#include "GAS/Effects/UGE_Debuff_Stun.h"
#include "Components/SphereComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"

ADFMeteorImpactActor::ADFMeteorImpactActor()
{
	PrimaryActorTick.bCanEverTick = false;
	HitSphere = CreateDefaultSubobject<USphereComponent>(TEXT("HitSphere"));
	RootComponent = HitSphere;
	HitSphere->InitSphereRadius(400.f);
	HitSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitSphere->SetCollisionObjectType(ECC_WorldDynamic);
	HitSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ADFMeteorImpactActor::InitializeImpact(
	ADFBossBase* const InBoss, const FVector& WorldLocation, const float InOuterDamage, const float InInnerDamage)
{
	BossSource = InBoss;
	OuterDamage = InOuterDamage;
	InnerDamage = InInnerDamage;
	ZoneRadius = 400.f;
	InnerRadius = 150.f;
	SetActorLocation(WorldLocation);
	if (HitSphere)
	{
		HitSphere->SetSphereRadius(ZoneRadius, false);
	}
}

void ADFMeteorImpactActor::BeginPlay()
{
	Super::BeginPlay();
	// In case no InitializeImpact (designer test), one-frame delay so overlap queries world state
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ADFMeteorImpactActor::OnDelayElapsed));
	}
}

void ADFMeteorImpactActor::OnDelayElapsed()
{
	RunImpactOnAuthority();
	Destroy();
}

void ADFMeteorImpactActor::RunImpactOnAuthority()
{
	if (!GetWorld() || !HasAuthority())
	{
		return;
	}
	ADFBossBase* const Boss = BossSource.Get();
	AActor* const Inst = Boss ? static_cast<AActor*>(Boss) : static_cast<AActor*>(this);
	const FVector L = GetActorLocation();
	FCollisionObjectQueryParams Obj(ECC_Pawn);
	FCollisionQueryParams Q(SCENE_QUERY_STAT(DF_MeteorImpact), false, this);
	if (Inst)
	{
		Q.AddIgnoredActor(Inst);
	}
	UAbilitySystemComponent* const Src = Inst
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Inst)
		: nullptr;
	if (!Src)
	{
		return;
	}
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, L, FQuat::Identity, Obj, FCollisionShape::MakeSphere(ZoneRadius), Q);

	const FGameplayEffectSpecHandle DmgOuter = UDFGameplayEffectLibrary::MakeDamageEffect(
		OuterDamage, FDFGameplayTags::Effect_Damage_True, Inst);
	const FGameplayEffectSpecHandle DmgInner = UDFGameplayEffectLibrary::MakeDamageEffect(
		InnerDamage, FDFGameplayTags::Effect_Damage_True, Inst);

	FGameplayEffectSpecHandle StunH = Src->MakeOutgoingSpec(UGE_Debuff_Stun::StaticClass(), 1.f, Src->MakeEffectContext());
	if (StunH.IsValid() && StunH.Data.IsValid() && FDFGameplayTags::Data_Duration.IsValid())
	{
		StunH.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, StunDuration);
	}

	for (const FOverlapResult& O : Overlaps)
	{
		AActor* const HitA = O.GetActor();
		if (!HitA)
		{
			continue;
		}
		if (UAbilitySystemComponent* Tgt = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitA))
		{
			const float D2D = FVector::Dist2D(HitA->GetActorLocation(), L);
			if (D2D <= InnerRadius)
			{
				if (DmgInner.IsValid() && DmgInner.Data)
				{
					Src->ApplyGameplayEffectSpecToTarget(*DmgInner.Data, Tgt);
				}
			}
			else
			{
				if (DmgOuter.IsValid() && DmgOuter.Data)
				{
					Src->ApplyGameplayEffectSpecToTarget(*DmgOuter.Data, Tgt);
				}
			}
			if (StunH.IsValid() && StunH.Data)
			{
				Src->ApplyGameplayEffectSpecToTarget(*StunH.Data, Tgt);
			}
		}
	}

	if (BossSource)
	{
		BossSource->Multicast_BossLocalAttackFX(
			L, CameraShake, ExplosionNiagara, CameraShakeInner, CameraShakeOuter);
	}
}
