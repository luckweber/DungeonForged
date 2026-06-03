// Source/DungeonForged/Private/Boss/UDFBossMinionComponent.cpp
#include "Boss/UDFBossMinionComponent.h"
#include "AI/UDFAILibrary.h"
#include "Boss/ADFBossBase.h"
#include "Characters/ADFEnemyBase.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Damage_Magic.h"
#include "GAS/UDFAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/OverlapResult.h"
#include "GameplayEffectTypes.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"

UDFBossMinionComponent::UDFBossMinionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UDFBossMinionComponent::InitializeMinion(ADFBossBase* const InBoss, const EDFBossMinionRole InRole)
{
	OwningBoss = InBoss;
	MinionRole = InRole;
	bDetonated = false;
	ApplyRoleTags();
}

void UDFBossMinionComponent::ApplyRoleTags()
{
	ADFEnemyBase* const OwnerEnemy = Cast<ADFEnemyBase>(GetOwner());
	if (!OwnerEnemy || !OwnerEnemy->GetAbilitySystemComponent())
	{
		return;
	}
	UAbilitySystemComponent* const ASC = OwnerEnemy->GetAbilitySystemComponent();
	if (FDFGameplayTags::State_Spawned_Boss_Guard.IsValid() && MinionRole == EDFBossMinionRole::Guard)
	{
		ASC->AddLooseGameplayTag(FDFGameplayTags::State_Spawned_Boss_Guard, 1);
	}
	if (FDFGameplayTags::State_Spawned_Boss_Exploder.IsValid() && MinionRole == EDFBossMinionRole::Exploder)
	{
		ASC->AddLooseGameplayTag(FDFGameplayTags::State_Spawned_Boss_Exploder, 1);
	}
}

bool UDFBossMinionComponent::ShouldGuardBoss() const
{
	if (MinionRole != EDFBossMinionRole::Guard)
	{
		return false;
	}
	ADFBossBase* const Boss = OwningBoss.Get();
	ADFEnemyBase* const Self = Cast<ADFEnemyBase>(GetOwner());
	if (!IsValid(Boss) || !IsValid(Self) || Boss->HasDied())
	{
		return false;
	}
	const FVector BossLoc = Boss->GetActorLocation();
	AActor* const Threat = UDFAILibrary::FindNearestHostilePlayerTarget(
		GetWorld(), BossLoc, BossThreatRadiusCm, nullptr, false, Self);
	return IsValid(Threat);
}

bool UDFBossMinionComponent::ComputeGuardLocation(FVector& OutLocation) const
{
	ADFBossBase* const Boss = OwningBoss.Get();
	ADFEnemyBase* const Self = Cast<ADFEnemyBase>(GetOwner());
	if (!ShouldGuardBoss() || !IsValid(Boss) || !IsValid(Self))
	{
		return false;
	}
	const FVector BossLoc = Boss->GetActorLocation();
	AActor* const Threat = UDFAILibrary::FindNearestHostilePlayerTarget(
		GetWorld(), BossLoc, BossThreatRadiusCm, nullptr, false, Self);
	if (!IsValid(Threat))
	{
		return false;
	}
	const FVector ToThreat = (Threat->GetActorLocation() - BossLoc).GetSafeNormal2D();
	OutLocation = BossLoc + ToThreat * GuardStandOffDistanceCm;
	OutLocation.Z = BossLoc.Z;
	return true;
}

bool UDFBossMinionComponent::TryDetonateFromProximity()
{
	if (MinionRole != EDFBossMinionRole::Exploder || bDetonated)
	{
		return false;
	}
	ADFEnemyBase* const Self = Cast<ADFEnemyBase>(GetOwner());
	if (!IsValid(Self) || !Self->HasAuthority() || Self->HasDied())
	{
		return false;
	}
	AActor* const Target = UDFAILibrary::FindNearestHostilePlayerTarget(
		GetWorld(), Self->GetActorLocation(), DetonationRangeCm * 2.f, nullptr, false, Self);
	if (!IsValid(Target))
	{
		return false;
	}
	if (FVector::Dist(Self->GetActorLocation(), Target->GetActorLocation()) > DetonationRangeCm)
	{
		return false;
	}
	ApplyExplosionDamage();
	Multicast_PlayExplosionFX();
	bDetonated = true;
	ForceOwnerDeath();
	return true;
}

void UDFBossMinionComponent::HandleOwnerDeath(AActor* const Killer)
{
	(void)Killer;
	if (MinionRole != EDFBossMinionRole::Exploder || bDetonated)
	{
		return;
	}
	ADFEnemyBase* const Self = Cast<ADFEnemyBase>(GetOwner());
	if (!IsValid(Self) || !Self->HasAuthority())
	{
		return;
	}
	bDetonated = true;
	ApplyExplosionDamage();
	Multicast_PlayExplosionFX();
}

void UDFBossMinionComponent::ApplyExplosionDamage()
{
	ADFEnemyBase* const Self = Cast<ADFEnemyBase>(GetOwner());
	if (!IsValid(Self) || !Self->HasAuthority())
	{
		return;
	}
	const FVector Origin = Self->GetActorLocation();
	FCollisionObjectQueryParams Obj(ECC_Pawn);
	FCollisionQueryParams Q(SCENE_QUERY_STAT(DF_BossMinionExplode), false, Self);
	Q.AddIgnoredActor(Self);
	if (ADFBossBase* const Boss = OwningBoss.Get())
	{
		Q.AddIgnoredActor(Boss);
	}
	TArray<FOverlapResult> Overlaps;
	if (UWorld* const World = GetWorld())
	{
		World->OverlapMultiByObjectType(
			Overlaps, Origin, FQuat::Identity, Obj, FCollisionShape::MakeSphere(ExplosionRadiusCm), Q);
	}
	UAbilitySystemComponent* const Src = Self->GetAbilitySystemComponent();
	if (!Src)
	{
		return;
	}
	for (const FOverlapResult& Hit : Overlaps)
	{
		AActor* const HitActor = Hit.GetActor();
		if (!IsValid(HitActor))
		{
			continue;
		}
		if (UAbilitySystemComponent* const TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor))
		{
			const FGameplayEffectSpecHandle SpecHandle = Src->MakeOutgoingSpec(UGE_Damage_Magic::StaticClass(), 1.f, Src->MakeEffectContext());
			if (SpecHandle.IsValid() && SpecHandle.Data.IsValid() && FDFGameplayTags::Data_Damage.IsValid())
			{
				SpecHandle.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, ExplosionDamage);
				Src->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data, TargetASC);
			}
		}
	}
}

void UDFBossMinionComponent::ForceOwnerDeath()
{
	ADFEnemyBase* const Self = Cast<ADFEnemyBase>(GetOwner());
	if (!IsValid(Self) || !Self->HasAuthority() || Self->HasDied())
	{
		return;
	}
	if (UAbilitySystemComponent* const ASC = Self->GetAbilitySystemComponent())
	{
		ASC->ApplyModToAttribute(UDFAttributeSet::GetHealthAttribute(), EGameplayModOp::Override, 0.f);
	}
}

void UDFBossMinionComponent::Multicast_PlayExplosionFX_Implementation()
{
	AActor* const OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return;
	}
	const FVector Loc = OwnerActor->GetActorLocation();
	if (ExplosionVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			OwnerActor, ExplosionVFX, Loc, FRotator::ZeroRotator, FVector(1.f), true, true, ENCPoolMethod::None, true);
	}
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(OwnerActor, ExplosionSound, Loc);
	}
}
