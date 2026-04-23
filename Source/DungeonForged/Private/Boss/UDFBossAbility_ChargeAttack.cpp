// Source/DungeonForged/Private/Boss/UDFBossAbility_ChargeAttack.cpp
#include "Boss/UDFBossAbility_ChargeAttack.h"
#include "Boss/ADFBossBase.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UDFGameplayEffectLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Animation/AnimMontage.h"

UDFBossAbility_ChargeAttack::UDFBossAbility_ChargeAttack()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UDFBossAbility_ChargeAttack::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Physical_Charge);
	}
}

void UDFBossAbility_ChargeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo, nullptr))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (AbilityMontage)
	{
		if (UAbilityTask_PlayMontageAndWait* const M = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, AbilityMontage, 1.f, NAME_None, true, 1.f, 0.f, false))
		{
			M->OnCompleted.AddDynamic(this, &UDFBossAbility_ChargeAttack::OnMontageEnd);
			M->OnInterrupted.AddDynamic(this, &UDFBossAbility_ChargeAttack::OnMontageEnd);
			M->OnCancelled.AddDynamic(this, &UDFBossAbility_ChargeAttack::OnMontageEnd);
			M->ReadyForActivation();
		}
	}

	if (UAbilityTask_WaitDelay* const D = UAbilityTask_WaitDelay::WaitDelay(this, ChargeHitTime))
	{
		D->OnFinish.AddDynamic(this, &UDFBossAbility_ChargeAttack::OnChargeResolve);
		D->ReadyForActivation();
	}
}

void UDFBossAbility_ChargeAttack::OnChargeResolve()
{
	AActor* const Av = GetAvatarActorFromActorInfo();
	if (!Av || !Av->HasAuthority())
	{
		return;
	}
	ACharacter* const Ch = Cast<ACharacter>(Av);
	ADFBossBase* const Boss = Cast<ADFBossBase>(Ch);
	if (!Ch)
	{
		return;
	}
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}

	const FVector Start = Ch->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector End = Start + Ch->GetActorForwardVector() * ChargeDistance;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(DF_BossCharge), false, Ch);
	Q.AddIgnoredActor(Ch);
	TArray<FHitResult> Hits;
	FCollisionShape S = FCollisionShape::MakeSphere(SweepRadius);
	const bool bAny = W->SweepMultiByChannel(
		Hits, Start, End, FQuat::Identity, ECC_Pawn, S, Q);

	AActor* FirstHit = nullptr;
	if (bAny)
	{
		for (FHitResult& H : Hits)
		{
			AActor* const A = H.GetActor();
			if (A && A != Ch)
			{
				if (UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(A))
				{
					FirstHit = A;
					break;
				}
			}
		}
	}

	if (FirstHit)
	{
		const FGameplayEffectSpecHandle Spec = UDFGameplayEffectLibrary::MakeDamageEffect(
			ChargeDamage, FDFGameplayTags::Effect_Damage_Physical, Ch);
		if (UAbilitySystemComponent* const Src = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Ch))
		{
			if (UAbilitySystemComponent* const Tgt = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(FirstHit))
			{
				if (Spec.IsValid() && Spec.Data)
				{
					Src->ApplyGameplayEffectSpecToTarget(*Spec.Data, Tgt);
				}
			}
		}
	}
	else if (Boss && ChargeMissMontage)
	{
		Boss->Multicast_PlayLocalMontage(ChargeMissMontage, 1.f, NAME_None);
	}
	if (!AbilityMontage)
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	}
}

void UDFBossAbility_ChargeAttack::OnMontageEnd()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
