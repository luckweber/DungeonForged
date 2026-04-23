// Source/DungeonForged/Private/Combat/DFBlizzardZone.cpp
#include "Combat/DFBlizzardZone.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Characters/ADFEnemyBase.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GAS/Effects/UGE_Damage_Magic.h"
#include "GAS/Effects/UGE_DoT_Frost.h"
#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "NiagaraFunctionLibrary.h"

ADFBlizzardZone::ADFBlizzardZone()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	USceneComponent* const Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	GroundDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("GroundDecal"));
	GroundDecal->SetupAttachment(RootComponent);
	GroundDecal->DecalSize = FVector(800.f, 800.f, 800.f);

	MagicDamageEffect = UGE_Damage_Magic::StaticClass();
	DoTFrostEffect = UGE_DoT_Frost::StaticClass();
}

void ADFBlizzardZone::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority())
	{
		return;
	}
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	W->GetTimerManager().SetTimer(BlizzardTimer, this, &ADFBlizzardZone::OnBlizzardTick, TickPeriod, true, 0.f);
	W->GetTimerManager().SetTimer(EndTimer, this, &ADFBlizzardZone::OnBlizzardZoneExpire, ZoneDuration, false);
}

void ADFBlizzardZone::OnBlizzardZoneExpire()
{
	Destroy();
}

void ADFBlizzardZone::OnBlizzardTick()
{
	if (!HasAuthority())
	{
		return;
	}
	APawn* const Inst = GetInstigator();
	if (!Inst)
	{
		return;
	}
	UAbilitySystemComponent* const SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Inst);
	if (!SourceASC || !MagicDamageEffect)
	{
		return;
	}
	const float Intel = SourceASC->GetNumericAttribute(UDFAttributeSet::GetIntelligenceAttribute());
	const float Sbc = Intel * (-0.1f); // 0.4*I total
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	if (TickNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, TickNiagara, GetActorLocation(), FRotator::ZeroRotator, FVector(1.f), true, true, ENCPoolMethod::None, true);
	}
	FCollisionQueryParams P(SCENE_QUERY_STAT(BlizzardOverlap), false, this);
	TArray<FOverlapResult> Overlaps;
	W->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(OverlapRadius), P);
	for (const FOverlapResult& O : Overlaps)
	{
		AActor* const A = O.GetActor();
		if (!IsValid(A) || !A->IsA<ADFEnemyBase>())
		{
			continue;
		}
		UAbilitySystemComponent* const TASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(A);
		if (!TASC)
		{
			continue;
		}
		FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
		Ctx.AddInstigator(Inst, Inst);
		Ctx.AddSourceObject(this);
		const FGameplayEffectSpecHandle Dmg = SourceASC->MakeOutgoingSpec(MagicDamageEffect, 1.f, Ctx);
		if (Dmg.IsValid() && Dmg.Data && FDFGameplayTags::Data_Damage.IsValid())
		{
			Dmg.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, Sbc);
			SourceASC->ApplyGameplayEffectSpecToTarget(*Dmg.Data, TASC);
		}
		if (DoTFrostEffect)
		{
			const FGameplayEffectSpecHandle Dot = SourceASC->MakeOutgoingSpec(DoTFrostEffect, 1.f, Ctx);
			if (Dot.IsValid() && Dot.Data)
			{
				SourceASC->ApplyGameplayEffectSpecToTarget(*Dot.Data, TASC);
			}
		}
	}
}
