// Source/DungeonForged/Private/GAS/Abilities/Mage/UDFAbility_Mage_FrostBolt.cpp
#include "GAS/Abilities/Mage/UDFAbility_Mage_FrostBolt.h"
#include "Camera/UDFLockOnComponent.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Combat/DFFrostBoltProjectile.h"
#include "GAS/DFGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

UDFAbility_Mage_FrostBolt::UDFAbility_Mage_FrostBolt()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	AbilityCost_Mana = 25.f;
	BaseCooldown = 4.f;
}

void UDFAbility_Mage_FrostBolt::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Mage_FrostBolt);
	}
}

void UDFAbility_Mage_FrostBolt::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo, nullptr))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (UAbilitySystemComponent* const asc = GetAbilitySystemComponentFromActorInfo())
	{
		if (asc->GetOwner() && asc->GetOwner()->HasAuthority())
		{
			ApplyResourceCostsToOwner(asc);
		}
	}
	const TObjectPtr<UAnimMontage> M = FrostBoltCastMontage ? FrostBoltCastMontage : AbilityMontage;
	if (!M || !ActorInfo)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (UAbilityTask_PlayMontageAndWait* const MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, M, 1.f, NAME_None, true, 1.f, 0.f, false))
	{
		MontageTask->OnCompleted.AddDynamic(this, &UDFAbility_Mage_FrostBolt::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &UDFAbility_Mage_FrostBolt::OnMontageInterruptedOrCancelled);
		MontageTask->OnCancelled.AddDynamic(this, &UDFAbility_Mage_FrostBolt::OnMontageInterruptedOrCancelled);
		MontageTask->ReadyForActivation();
	}
	if (UAbilityTask_WaitGameplayEvent* const Ev = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, FDFGameplayTags::Event_Ability_Mage_FrostTrace, nullptr, true, true))
	{
		Ev->EventReceived.AddDynamic(this, &UDFAbility_Mage_FrostBolt::OnFrostTraceEvent);
		Ev->ReadyForActivation();
	}
}

void UDFAbility_Mage_FrostBolt::OnFrostTraceEvent(FGameplayEventData /*Payload*/)
{
	const FGameplayAbilityActorInfo* const Info = GetCurrentActorInfo();
	if (!Info || !Info->IsNetAuthority())
	{
		return;
	}
	ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Char || !FrostProjectileClass)
	{
		return;
	}
	USkeletalMeshComponent* const Sk = Char->GetMesh();
	if (!Sk)
	{
		return;
	}
	AActor* Homing = nullptr;
	if (const ADFPlayerCharacter* const P = Cast<ADFPlayerCharacter>(Char))
	{
		if (P->LockOnComponent)
		{
			Homing = P->LockOnComponent->GetCurrentTarget();
		}
	}
	const FTransform SocketXform = Sk->DoesSocketExist(MuzzleSocketName)
		? Sk->GetSocketTransform(MuzzleSocketName, ERelativeTransformSpace::RTS_World)
		: FTransform(Char->GetActorRotation(), Char->GetActorLocation());
		if (UWorld* const W = GetWorld())
	{
		const FTransform T(SocketXform.Rotator(), SocketXform.GetLocation());
		ADFFrostBoltProjectile* const Proj = W->SpawnActorDeferred<ADFFrostBoltProjectile>(FrostProjectileClass, T, Char, Char,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (IsValid(Proj))
		{
			Proj->HomingTarget = Homing;
			Proj->FinishSpawning(T);
		}
	}
}

void UDFAbility_Mage_FrostBolt::OnMontageCompleted()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UDFAbility_Mage_FrostBolt::OnMontageInterruptedOrCancelled()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}
