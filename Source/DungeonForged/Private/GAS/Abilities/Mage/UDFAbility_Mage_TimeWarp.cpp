// Source/DungeonForged/Private/GAS/Abilities/Mage/UDFAbility_Mage_TimeWarp.cpp
#include "GAS/Abilities/Mage/UDFAbility_Mage_TimeWarp.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Buff_TimeWarpHaste.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

UDFAbility_Mage_TimeWarp::UDFAbility_Mage_TimeWarp()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	AbilityCost_Mana = 60.f;
	BaseCooldown = 60.f;
	TimeWarpHasteClass = UGE_Buff_TimeWarpHaste::StaticClass();
}

void UDFAbility_Mage_TimeWarp::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Mage_TimeWarp);
	}
}

void UDFAbility_Mage_TimeWarp::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!K2_CommitAbilityCost())
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
	const TObjectPtr<UAnimMontage> M = TimeWarpMontage ? TimeWarpMontage : AbilityMontage;
	if (!M)
	{
		ApplyTimeWarpEffects();
		K2_CommitAbilityCooldown(true, true);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	if (ACharacter* const C = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (SelfTimeNiagara)
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(
				SelfTimeNiagara, C->GetMesh(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, FVector(1.f), EAttachLocation::KeepRelativeOffset, false, ENCPoolMethod::None, true, true);
		}
	}
	if (UAbilityTask_PlayMontageAndWait* const T = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, M, 1.f, NAME_None, true, 1.2f, 0.f, false))
	{
		T->OnCompleted.AddDynamic(this, &UDFAbility_Mage_TimeWarp::OnMontageDone);
		T->OnInterrupted.AddDynamic(this, &UDFAbility_Mage_TimeWarp::OnMontageDone);
		T->OnCancelled.AddDynamic(this, &UDFAbility_Mage_TimeWarp::OnMontageDone);
		T->ReadyForActivation();
	}
}

void UDFAbility_Mage_TimeWarp::OnMontageDone()
{
	ApplyTimeWarpEffects();
	if (ACharacter* const C = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (PulseNiagara)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				this, PulseNiagara, C->GetActorLocation(), FRotator::ZeroRotator, FVector(1.5f), true, true, ENCPoolMethod::None, true);
		}
	}
	K2_CommitAbilityCooldown(true, true);
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UDFAbility_Mage_TimeWarp::ApplyTimeWarpEffects()
{
	UAbilitySystemComponent* const ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || !ASC->GetOwner() || !ASC->GetOwner()->HasAuthority())
	{
		return;
	}
	FGameplayTagContainer T;
	T.AddTag(FDFGameplayTags::Ability_Cooldown);
	ASC->RemoveActiveEffectsWithAppliedTags(T);
	if (TimeWarpHasteClass)
	{
		const FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
		ASC->ApplyGameplayEffectToSelf(TimeWarpHasteClass.GetDefaultObject(), 1.f, Ctx);
	}
}
