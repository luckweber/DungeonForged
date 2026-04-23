// Source/DungeonForged/Private/Boss/UDFBossAbility_GroundSlam.cpp
#include "Boss/UDFBossAbility_GroundSlam.h"
#include "Boss/ADFBossBase.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UDFGameplayEffectLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

UDFBossAbility_GroundSlam::UDFBossAbility_GroundSlam()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UDFBossAbility_GroundSlam::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Attack_Melee);
	}
}

void UDFBossAbility_GroundSlam::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo, nullptr))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (AbilityMontage)
	{
		UAbilityTask_PlayMontageAndWait* const M = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, AbilityMontage, 1.f, NAME_None, true, 1.f, 0.f, false);
		if (M)
		{
			M->OnCompleted.AddDynamic(this, &UDFBossAbility_GroundSlam::OnMontageEnd);
			M->OnInterrupted.AddDynamic(this, &UDFBossAbility_GroundSlam::OnMontageEnd);
			M->OnCancelled.AddDynamic(this, &UDFBossAbility_GroundSlam::OnMontageEnd);
			M->ReadyForActivation();
		}
	}

	if (UAbilityTask_WaitDelay* const D = UAbilityTask_WaitDelay::WaitDelay(this, SlamImpactDelay))
	{
		D->OnFinish.AddDynamic(this, &UDFBossAbility_GroundSlam::OnSlamImpact);
		D->ReadyForActivation();
	}
}

void UDFBossAbility_GroundSlam::OnSlamImpact()
{
	if (!GetAvatarActorFromActorInfo() || !GetAvatarActorFromActorInfo()->HasAuthority())
	{
		return;
	}
	ACharacter* const Ch = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	ADFBossBase* const Boss = Cast<ADFBossBase>(Ch);
	if (!Ch)
	{
		return;
	}

	const FVector Origin = Ch->GetActorLocation() - FVector(0.f, 0.f, 10.f);
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}

	FCollisionObjectQueryParams Obj(ECC_Pawn);
	FCollisionQueryParams Q(SCENE_QUERY_STAT(DF_BossSlam), false, Ch);
	Q.AddIgnoredActor(Ch);
	TArray<FOverlapResult> Overlaps;
	W->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, Obj, FCollisionShape::MakeSphere(SlamRadius), Q);

	const FGameplayEffectSpecHandle Spec = UDFGameplayEffectLibrary::MakeDamageEffect(
		SlamDamage, FDFGameplayTags::Effect_Damage_Physical, Ch);
	UAbilitySystemComponent* const Src = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Ch);
	if (!Spec.IsValid() || !Spec.Data || !Src)
	{
		if (Boss)
		{
			Boss->Multicast_BossLocalAttackFX(Origin, SlamCameraShake, SlamNiagara, CameraShakeInnerRadius, CameraShakeOuterRadius);
		}
		if (!AbilityMontage)
		{
			EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
		}
		return;
	}

	for (const FOverlapResult& O : Overlaps)
	{
		AActor* const HitA = O.GetActor();
		if (!HitA || HitA == Ch)
		{
			continue;
		}
		if (UAbilitySystemComponent* const Tgt = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitA))
		{
			Src->ApplyGameplayEffectSpecToTarget(*Spec.Data, Tgt);
		}
	}

	if (Boss)
	{
		Boss->Multicast_BossLocalAttackFX(Origin, SlamCameraShake, SlamNiagara, CameraShakeInnerRadius, CameraShakeOuterRadius);
	}
	if (!AbilityMontage)
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	}
}

void UDFBossAbility_GroundSlam::OnMontageEnd()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
