// Source/DungeonForged/Private/Boss/UDFBossAbility_SummonMinions.cpp
#include "Boss/UDFBossAbility_SummonMinions.h"
#include "Boss/ADFBossBase.h"
#include "Characters/ADFEnemyBase.h"
#include "GAS/DFGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

UDFBossAbility_SummonMinions::UDFBossAbility_SummonMinions()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UDFBossAbility_SummonMinions::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Ice_Blizzard);
	}
}

void UDFBossAbility_SummonMinions::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo, nullptr))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACharacter* const Ch = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	ADFBossBase* const Boss = Cast<ADFBossBase>(Ch);
	UWorld* const W = GetWorld();
	if (!Ch || !Boss || !W || !MinionClass || !Ch->HasAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	USkeletalMeshComponent* const Sk = Ch->GetMesh();
	if (!Sk)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const int32 Cap = 6;
	for (const FName& Sock : SpawnSocketNames)
	{
		if (Boss->GetLivingMinionCount() >= Cap)
		{
			break;
		}
		FVector Loc = Ch->GetActorLocation();
		FRotator Rot = Ch->GetActorRotation();
		if (Sk->DoesSocketExist(Sock))
		{
			const FTransform T = Sk->GetSocketTransform(Sock, ERelativeTransformSpace::RTS_World);
			Loc = T.GetLocation();
			Rot = T.Rotator();
		}
		FActorSpawnParameters P;
		P.Instigator = Ch;
		P.Owner = Ch;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		ADFEnemyBase* const S = W->SpawnActor<ADFEnemyBase>(MinionClass, Loc, Rot, P);
		if (!S)
		{
			continue;
		}
		if (MinionDataTable && MinionRowName != NAME_None)
		{
			S->InitializeFromDataTable(MinionDataTable, MinionRowName);
		}
		Boss->RegisterSpawnedMinion(S);
	}

	if (AbilityMontage)
	{
		if (UAbilityTask_PlayMontageAndWait* const M = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, AbilityMontage, 1.f, NAME_None, true, 1.f, 0.f, false))
		{
			M->OnCompleted.AddDynamic(this, &UDFBossAbility_SummonMinions::OnSummonMontageCompleted);
			M->OnInterrupted.AddDynamic(this, &UDFBossAbility_SummonMinions::OnSummonMontageInterrupted);
			M->OnCancelled.AddDynamic(this, &UDFBossAbility_SummonMinions::OnSummonMontageInterrupted);
			M->ReadyForActivation();
		}
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UDFBossAbility_SummonMinions::OnSummonMontageCompleted()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UDFBossAbility_SummonMinions::OnSummonMontageInterrupted()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}
