// Source/DungeonForged/Private/GAS/Abilities/Warrior/UDFAbility_Warrior_IronSkin.cpp
#include "GAS/Abilities/Warrior/UDFAbility_Warrior_IronSkin.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEffectRemoved.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GAS/DFGameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

UDFAbility_Warrior_IronSkin::UDFAbility_Warrior_IronSkin()
{
	AbilityCost_Mana = 35.f;
	AbilityCost_Stamina = 0.f;
	BaseCooldown = 25.f;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UDFAbility_Warrior_IronSkin::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Warrior_IronSkin);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Stunned);
	}
}

void UDFAbility_Warrior_IronSkin::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	(void)TriggerEventData;
	if (!ActorInfo)
	{
		EndAbility(Handle, nullptr, ActivationInfo, true, true);
		return;
	}
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo, nullptr))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	UAbilitySystemComponent* const ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC && ASC->GetOwner() && ASC->GetOwner()->HasAuthority())
	{
		ApplyResourceCostsToOwner(ASC);
	}
	if (!IronSkinEffect)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	FActiveGameplayEffectHandle ActiveHandle;
	if (ASC)
	{
		const FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
		ActiveHandle = ASC->ApplyGameplayEffectToSelf(IronSkinEffect.GetDefaultObject(), 1.f, Ctx);
	}
	ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (Char && Char->GetMesh() && IronSkinVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			IronSkinVFX, Char->GetMesh(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true, true);
	}
	if (UAnimMontage* M = IronSkinMontage ? IronSkinMontage : AbilityMontage)
	{
		if (UAbilityTask_PlayMontageAndWait* T = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, NAME_None, M, 1.f, NAME_None, true, 1.f, 0.f, true))
		{
			T->ReadyForActivation();
		}
	}
	UAbilityTask_WaitGameplayEffectRemoved* const WaitRemoved =
		ActiveHandle.IsValid() ? UAbilityTask_WaitGameplayEffectRemoved::WaitForGameplayEffectRemoved(this, ActiveHandle) : nullptr;
	if (WaitRemoved)
	{
		WaitRemoved->OnRemoved.AddDynamic(this, &UDFAbility_Warrior_IronSkin::OnIronSkinEffectRemoved);
		WaitRemoved->ReadyForActivation();
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UDFAbility_Warrior_IronSkin::OnIronSkinEffectRemoved(const FGameplayEffectRemovalInfo& /*InInfo*/)
{
	if (ACharacter* const C = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (IronSkinShatterVFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				this, IronSkinShatterVFX, C->GetActorLocation() + FVector(0.f, 0.f, 50.f), FRotator::ZeroRotator, FVector(1.f), true, true, ENCPoolMethod::None, true);
		}
	}
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
