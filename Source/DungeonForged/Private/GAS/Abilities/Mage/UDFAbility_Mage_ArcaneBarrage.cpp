// Source/DungeonForged/Private/GAS/Abilities/Mage/UDFAbility_Mage_ArcaneBarrage.cpp
#include "GAS/Abilities/Mage/UDFAbility_Mage_ArcaneBarrage.h"
#include "Camera/UDFLockOnComponent.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Combat/DFArcaneMissileProjectile.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GAS/Effects/UGE_Damage_Magic.h"
#include "GAS/Effects/UGE_Debuff_Silence.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"

UDFAbility_Mage_ArcaneBarrage::UDFAbility_Mage_ArcaneBarrage()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	AbilityCost_Mana = 15.f;
	BaseCooldown = 0.f;
	OverloadExtraDamageClass = UGE_Damage_Magic::StaticClass();
}

void UDFAbility_Mage_ArcaneBarrage::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Mage_ArcaneBarrage);
	}
}

void UDFAbility_Mage_ArcaneBarrage::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);
	RemainingCharges = MaxCharges;
}

bool UDFAbility_Mage_ArcaneBarrage::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	if (RemainingCharges <= 0)
	{
		return false;
	}
	const AActor* const Av = ActorInfo && ActorInfo->AvatarActor.IsValid() ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!Av)
	{
		return false;
	}
	if (UWorld* const W = Av->GetWorld())
	{
		const double T = W->GetTimeSeconds();
		if (LastGlobalCast >= 0.0 && (T - LastGlobalCast) < double(GlobalCastCooldown))
		{
			return false;
		}
	}
	return true;
}

void UDFAbility_Mage_ArcaneBarrage::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CheckCost(Handle, ActorInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (RemainingCharges <= 0)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	RemainingCharges--;
	if (UWorld* const W = GetWorld())
	{
		const double T = W->GetTimeSeconds();
		LastGlobalCast = T;
	}
	if (UAbilitySystemComponent* const asc = GetAbilitySystemComponentFromActorInfo())
	{
		if (asc->GetOwner() && asc->GetOwner()->HasAuthority())
		{
			ApplyResourceCostsToOwner(asc);
		}
	}
	if (RemainingCharges < MaxCharges)
	{
		if (UWorld* const W = GetWorld())
		{
			if (!W->GetTimerManager().IsTimerActive(RechargeHandle))
			{
				W->GetTimerManager().SetTimer(
					RechargeHandle, this, &UDFAbility_Mage_ArcaneBarrage::OnRechargeTick, ChargeRechargeTime, true, ChargeRechargeTime);
			}
		}
	}
	const TObjectPtr<UAnimMontage> M = ArcaneQuickCastMontage ? ArcaneQuickCastMontage : AbilityMontage;
	if (!M)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (UAbilityTask_PlayMontageAndWait* const T = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, M, 1.f, NAME_None, true, 0.4f, 0.f, false))
	{
		T->OnCompleted.AddDynamic(this, &UDFAbility_Mage_ArcaneBarrage::OnCastMontageEnd);
		T->OnInterrupted.AddDynamic(this, &UDFAbility_Mage_ArcaneBarrage::OnCastMontageEnd);
		T->OnCancelled.AddDynamic(this, &UDFAbility_Mage_ArcaneBarrage::OnCastMontageEnd);
		T->ReadyForActivation();
	}
	if (UAbilityTask_WaitGameplayEvent* const W = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, FDFGameplayTags::Event_Ability_Mage_ArcaneTrace, nullptr, true, true))
	{
		W->EventReceived.AddDynamic(this, &UDFAbility_Mage_ArcaneBarrage::OnTraceEvent);
		W->ReadyForActivation();
	}
	SalvoHitCount.Reset();
}

void UDFAbility_Mage_ArcaneBarrage::OnTraceEvent(FGameplayEventData /*Payload*/)
{
	const FGameplayAbilityActorInfo* const Info = GetCurrentActorInfo();
	if (!Info || !Info->IsNetAuthority() || !MissileClass)
	{
		return;
	}
	ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Char)
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
	const FTransform Sx = Sk->DoesSocketExist(FName("hand_r")) ? Sk->GetSocketTransform(FName("hand_r"), RTS_World) : FTransform(Char->GetActorRotation(), Char->GetActorLocation());
	const FRotator BaseRot = Sx.Rotator();
	if (UWorld* const W = GetWorld())
	{
		for (int32 I = -1; I <= 1; ++I)
		{
			const FRotator Spread(BaseRot.Pitch, BaseRot.Yaw + (I * 5.f), BaseRot.Roll);
			const FTransform T(Spread, Sx.GetLocation());
			ADFArcaneMissileProjectile* const Miss = W->SpawnActorDeferred<ADFArcaneMissileProjectile>(MissileClass, T, Char, Char,
				ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
			if (IsValid(Miss))
			{
				Miss->SourceAbility = this;
				Miss->HomingTarget = Homing;
				Miss->FinishSpawning(T);
			}
		}
	}
}

void UDFAbility_Mage_ArcaneBarrage::NotifyArcaneMissileHit(AActor* Target, UAbilitySystemComponent* TargetASC, const FGameplayEffectContextHandle& Ctx)
{
	if (!GetAbilitySystemComponentFromActorInfo() || !IsValid(Target) || !TargetASC)
	{
		return;
	}
	UAbilitySystemComponent* const Source = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(OverloadExtraDamageClass))
	{
		return;
	}
	int32& C = SalvoHitCount.FindOrAdd(Target);
	C++;
	if (C < 3)
	{
		return;
	}
	const float Intel = Source->GetNumericAttribute(UDFAttributeSet::GetIntelligenceAttribute());
	const float Sbc = Intel * 0.7f; // 0.5+0.7 = 1.2*I
	const FGameplayEffectSpecHandle S = Source->MakeOutgoingSpec(OverloadExtraDamageClass, 1.f, Ctx);
	if (S.IsValid() && S.Data && FDFGameplayTags::Data_Damage.IsValid())
	{
		S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, Sbc);
		Source->ApplyGameplayEffectSpecToTarget(*S.Data, TargetASC);
	}
	{
		const FGameplayEffectSpecHandle S2 = Source->MakeOutgoingSpec(UGE_Debuff_Silence::StaticClass(), 1.f, Ctx);
		if (S2.IsValid() && S2.Data && FDFGameplayTags::Data_Duration.IsValid())
		{
			S2.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, 2.f);
			Source->ApplyGameplayEffectSpecToTarget(*S2.Data, TargetASC);
		}
	}
	C = 0;
}

void UDFAbility_Mage_ArcaneBarrage::OnCastMontageEnd()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UDFAbility_Mage_ArcaneBarrage::OnRechargeTick()
{
	if (RemainingCharges >= MaxCharges)
	{
		if (UWorld* const W = GetWorld())
		{
			W->GetTimerManager().ClearTimer(RechargeHandle);
		}
		return;
	}
	RemainingCharges++;
}