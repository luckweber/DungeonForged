// Source/DungeonForged/Private/GAS/Abilities/Boss/UDFBossAbility_EnragePulse.cpp
#include "GAS/Abilities/Boss/UDFBossAbility_EnragePulse.h"
#include "GAS/Abilities/Boss/DFBossAbilityCommons.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Cooldown_Boss_EnragePulse.h"
#include "GAS/Effects/UGE_Debuff_Slow.h"
#include "GAS/Effects/UGE_Damage_Physical.h"
#include "GAS/UDFAttributeSet.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Boss/ADFBossBase.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"

UDFBossAbility_EnragePulse::UDFBossAbility_EnragePulse()
{
	bSourceObjectMustBeBoss = true;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseCooldown = 8.f;
	CooldownClass = UGE_Cooldown_Boss_EnragePulse::StaticClass();
}

void UDFBossAbility_EnragePulse::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Boss_EnragePulse);
	}
}

bool UDFBossAbility_EnragePulse::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) || !ActorInfo)
	{
		return false;
	}
	if (const ADFBossBase* B = Cast<ADFBossBase>(ActorInfo->AvatarActor.Get()))
	{
		return B->bIsEnraged;
	}
	return false;
}

void UDFBossAbility_EnragePulse::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	(void)TriggerEventData;
	if (!ActorInfo)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo, nullptr))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	DFBossAbilityCommons::ApplySetCallerCooldown(ActorInfo->AbilitySystemComponent.Get(), CooldownClass, 8.f);
	UAnimMontage* M = EnragePulseMontage ? EnragePulseMontage : AbilityMontage;
	if (M)
	{
		if (UAbilityTask_PlayMontageAndWait* T = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, NAME_None, M, 1.f, NAME_None, true, 1.f, 0.f, false))
		{
			T->OnCompleted.AddDynamic(this, &UDFBossAbility_EnragePulse::OnMontageEnd);
			T->OnInterrupted.AddDynamic(this, &UDFBossAbility_EnragePulse::OnMontageEnd);
			T->OnCancelled.AddDynamic(this, &UDFBossAbility_EnragePulse::OnMontageEnd);
			T->ReadyForActivation();
		}
	}
	else
	{
		OnMontageEnd();
	}
}

void UDFBossAbility_EnragePulse::OnMontageEnd()
{
	ACharacter* const Ch = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	ADFBossBase* const Boss = Cast<ADFBossBase>(Ch);
	if (Ch && Ch->HasAuthority() && Boss)
	{
		const FVector O = Ch->GetActorLocation();
		FCollisionObjectQueryParams Obj(ECC_Pawn);
		FCollisionQueryParams Q(SCENE_QUERY_STAT(DF_EnragePulse), false, Ch);
		Q.AddIgnoredActor(Ch);
		TArray<FOverlapResult> Overlaps;
		if (UWorld* W = GetWorld())
		{
			W->OverlapMultiByObjectType(Overlaps, O, FQuat::Identity, Obj, FCollisionShape::MakeSphere(PulseRadius), Q);
		}
		UAbilitySystemComponent* const Src = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Ch);
		if (Src)
		{
			const float Str = Src->GetNumericAttribute(UDFAttributeSet::GetStrengthAttribute());
			const float SetBy = Str; // (SetBy + 0.5*Str) from exec = 1.5*Str pre-mitigation
			for (const FOverlapResult& R : Overlaps)
			{
				AActor* const HitA = R.GetActor();
				if (!HitA || HitA == Ch)
				{
					continue;
				}
				if (UAbilitySystemComponent* Tgt = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitA))
				{
					const FGameplayEffectSpecHandle S = Src->MakeOutgoingSpec(UGE_Damage_Physical::StaticClass(), 1.f, Src->MakeEffectContext());
					if (S.IsValid() && S.Data.IsValid() && FDFGameplayTags::Data_Damage.IsValid())
					{
						S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, SetBy);
						Src->ApplyGameplayEffectSpecToTarget(*S.Data, Tgt);
					}
					const FGameplayEffectSpecHandle Sl = Src->MakeOutgoingSpec(UGE_Debuff_Slow::StaticClass(), 1.f, Src->MakeEffectContext());
					if (Sl.IsValid() && Sl.Data.IsValid() && FDFGameplayTags::Data_Duration.IsValid())
					{
						Sl.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, 1.f);
						Src->ApplyGameplayEffectSpecToTarget(*Sl.Data, Tgt);
					}
				}
			}
		}
		if (PulseNiagara)
		{
			Boss->Multicast_BossLocalAttackFX(O, KineticCameraShake, PulseNiagara, 0.f, 5000.f);
		}
	}
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
