// Source/DungeonForged/Private/GAS/Abilities/Mage/UDFAbility_Mage_BlizzardStorm.cpp
#include "GAS/Abilities/Mage/UDFAbility_Mage_BlizzardStorm.h"
#include "Combat/DFBlizzardZone.h"
#include "Combat/DFGroundTargetActor.h"
#include "GAS/DFGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "GameFramework/Character.h"

UDFAbility_Mage_BlizzardStorm::UDFAbility_Mage_BlizzardStorm()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	AbilityCost_Mana = 80.f;
	BaseCooldown = 30.f;
}

void UDFAbility_Mage_BlizzardStorm::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Mage_BlizzardStorm);
	}
}

void UDFAbility_Mage_BlizzardStorm::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
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
	const TObjectPtr<UAnimMontage> M = BlizzardCastMontage ? BlizzardCastMontage : AbilityMontage;
	if (!M || !ActorInfo)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (UAbilityTask_PlayMontageAndWait* const T = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, M, 1.f, NAME_None, true, 1.f, 0.f, false))
	{
		T->OnCompleted.AddDynamic(this, &UDFAbility_Mage_BlizzardStorm::OnMontageForChannelingDone);
		T->OnInterrupted.AddDynamic(this, &UDFAbility_Mage_BlizzardStorm::OnMontageInterruptedForBlizzard);
		T->OnCancelled.AddDynamic(this, &UDFAbility_Mage_BlizzardStorm::OnMontageInterruptedForBlizzard);
		T->ReadyForActivation();
	}
}

void UDFAbility_Mage_BlizzardStorm::OnMontageForChannelingDone()
{
	if (!GroundTargetActorClass)
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
		return;
	}
	if (UAbilityTask_WaitTargetData* const W = UAbilityTask_WaitTargetData::WaitTargetData(
			this, NAME_None, EGameplayTargetingConfirmation::UserConfirmed, GroundTargetActorClass))
	{
		W->ValidData.AddDynamic(this, &UDFAbility_Mage_BlizzardStorm::OnTargetDataReady);
		W->Cancelled.AddDynamic(this, &UDFAbility_Mage_BlizzardStorm::OnTargetCancelled);
		W->ReadyForActivation();
	}
}

void UDFAbility_Mage_BlizzardStorm::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data)
{
	if (!GetCurrentActorInfo() || !GetCurrentActorInfo()->IsNetAuthority() || !BlizzardZoneClass)
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
		return;
	}
	const FHitResult H = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(Data, 0);
	FVector Loc = H.bBlockingHit || H.ImpactPoint != FVector::ZeroVector ? H.ImpactPoint : (H.Location != FVector::ZeroVector ? H.Location : H.TraceEnd);
	if (ACharacter* const C = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		FActorSpawnParameters P;
		P.Instigator = C;
		P.Owner = C;
		if (UWorld* const W = GetWorld())
		{
			W->SpawnActor<ADFBlizzardZone>(BlizzardZoneClass, Loc, FRotator::ZeroRotator, P);
		}
	}
	if (UAbilityTask_WaitDelay* const D = UAbilityTask_WaitDelay::WaitDelay(this, 8.f))
	{
		D->OnFinish.AddDynamic(this, &UDFAbility_Mage_BlizzardStorm::OnBlizzardAbilityEnd);
		D->ReadyForActivation();
	}
}

void UDFAbility_Mage_BlizzardStorm::OnMontageInterruptedForBlizzard()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

void UDFAbility_Mage_BlizzardStorm::OnTargetCancelled(const FGameplayAbilityTargetDataHandle& /*Data*/)
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

void UDFAbility_Mage_BlizzardStorm::OnBlizzardAbilityEnd()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
