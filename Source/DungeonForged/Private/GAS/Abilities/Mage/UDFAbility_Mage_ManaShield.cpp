// Source/DungeonForged/Private/GAS/Abilities/Mage/UDFAbility_Mage_ManaShield.cpp
#include "GAS/Abilities/Mage/UDFAbility_Mage_ManaShield.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_ManaShield_Active.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/Character.h"
#include "Components/SceneComponent.h"

UDFAbility_Mage_ManaShield::UDFAbility_Mage_ManaShield()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	AbilityCost_Mana = 0.f;
	BaseCooldown = 20.f;
	ManaShieldActiveClass = UGE_ManaShield_Active::StaticClass();
}

void UDFAbility_Mage_ManaShield::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Mage_ManaShield);
	}
}

void UDFAbility_Mage_ManaShield::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo)
	{
		EndAbility(Handle, nullptr, ActivationInfo, true, true);
		return;
	}
	UAbilitySystemComponent* const ASC = GetAbilitySystemComponentFromActorInfo();
	ACharacter* const C = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!ASC || !C)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (ASC->HasMatchingGameplayTag(FDFGameplayTags::State_ManaShieldActive))
	{
		if (ActorInfo->IsNetAuthority())
		{
			FGameplayTagContainer T;
			T.AddTag(FDFGameplayTags::State_ManaShieldActive);
			ASC->RemoveActiveEffectsWithGrantedTags(T);
		}
		if (ActiveShieldNiagara.IsValid())
		{
			ActiveShieldNiagara->DestroyComponent();
			ActiveShieldNiagara = nullptr;
		}
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	if (!K2_CommitAbilityCost() || !ManaShieldActiveClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (ActorInfo->IsNetAuthority())
	{
		const FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
		ActiveShieldHandle = ASC->ApplyGameplayEffectToSelf(ManaShieldActiveClass.GetDefaultObject(), 1.f, Ctx);
	}
	if (USceneComponent* const Attach = C->GetMesh() ? Cast<USceneComponent>(C->GetMesh()) : C->GetRootComponent())
	{
		if (ShieldLoopNiagara)
		{
			UNiagaraComponent* const N = 			UNiagaraFunctionLibrary::SpawnSystemAttached(
				ShieldLoopNiagara, Attach, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, FVector(1.f), EAttachLocation::SnapToTarget, false, ENCPoolMethod::None, true, true);
			ActiveShieldNiagara = N;
		}
	}
	AActor* const TagSource = C;
	if (UAbilityTask_WaitGameplayTagRemoved* const R = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(
			this, FDFGameplayTags::State_ManaShieldActive, TagSource, true))
	{
		R->Removed.AddDynamic(this, &UDFAbility_Mage_ManaShield::OnShieldRemoved);
		R->ReadyForActivation();
	}
}

void UDFAbility_Mage_ManaShield::OnShieldRemoved()
{
	if (ActiveShieldNiagara.IsValid())
	{
		if (ACharacter* const C = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			if (ShieldBreakNiagara)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					this, ShieldBreakNiagara, C->GetActorLocation(), FRotator::ZeroRotator, FVector(1.f), true, true, ENCPoolMethod::None, true);
			}
		}
		ActiveShieldNiagara->DestroyComponent();
		ActiveShieldNiagara = nullptr;
	}
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
