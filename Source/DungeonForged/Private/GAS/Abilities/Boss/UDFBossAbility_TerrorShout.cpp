// Source/DungeonForged/Private/GAS/Abilities/Boss/UDFBossAbility_TerrorShout.cpp
#include "GAS/Abilities/Boss/UDFBossAbility_TerrorShout.h"
#include "GAS/Abilities/Boss/DFBossAbilityCommons.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UDFGameplayEffectLibrary.h"
#include "GAS/Effects/UGE_Cooldown_Boss_TerrorShout.h"
#include "GAS/Effects/UGE_Debuff_TerrorStruck.h"
#include "GAS/Effects/UGE_Debuff_Weaken_Boss.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Boss/ADFBossBase.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"

UDFBossAbility_TerrorShout::UDFBossAbility_TerrorShout()
{
	bSourceObjectMustBeBoss = true;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseCooldown = 30.f;
	TerrorDebuffClass = UGE_Debuff_TerrorStruck::StaticClass();
	WeakenDebuffClass = UGE_Debuff_Weaken_Boss::StaticClass();
	CooldownClass = UGE_Cooldown_Boss_TerrorShout::StaticClass();
}

void UDFBossAbility_TerrorShout::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Boss_TerrorShout);
	}
}

void UDFBossAbility_TerrorShout::ActivateAbility(
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
	DFBossAbilityCommons::ApplySetCallerCooldown(
		ActorInfo->AbilitySystemComponent.Get(), CooldownClass, 30.f);

	UAnimMontage* M = BossShoutMontage ? BossShoutMontage : AbilityMontage;
	if (M)
	{
		if (UAbilityTask_PlayMontageAndWait* T = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, NAME_None, M, 1.f, NAME_None, true, 1.f, 0.f, false))
		{
			T->OnCompleted.AddDynamic(this, &UDFBossAbility_TerrorShout::OnMontageFinished);
			T->OnInterrupted.AddDynamic(this, &UDFBossAbility_TerrorShout::OnMontageFinished);
			T->OnCancelled.AddDynamic(this, &UDFBossAbility_TerrorShout::OnMontageFinished);
			T->ReadyForActivation();
			return;
		}
	}
	OnMontageFinished();
}

void UDFBossAbility_TerrorShout::OnMontageFinished()
{
	RunShoutAoe();
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UDFBossAbility_TerrorShout::RunShoutAoe()
{
	ACharacter* const Ch = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	ADFBossBase* const Boss = Cast<ADFBossBase>(Ch);
	if (!Ch || !Ch->HasAuthority() || !Boss)
	{
		return;
	}
	const FVector O = Ch->GetActorLocation();
	if (UWorld* W = Ch->GetWorld())
	{
		FCollisionObjectQueryParams Obj(ECC_Pawn);
		FCollisionQueryParams Q(SCENE_QUERY_STAT(DF_TerrorShout), false, Ch);
		Q.AddIgnoredActor(Ch);
		TArray<FOverlapResult> Overlaps;
		W->OverlapMultiByObjectType(Overlaps, O, FQuat::Identity, Obj, FCollisionShape::MakeSphere(AoeRadius), Q);
		UAbilitySystemComponent* const Src = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Ch);
		if (!Src)
		{
			return;
		}
		for (const FOverlapResult& R : Overlaps)
		{
			AActor* const HitA = R.GetActor();
			if (!HitA)
			{
				continue;
			}
			if (UAbilitySystemComponent* Tgt = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitA))
			{
				if (TerrorDebuffClass)
				{
					const FGameplayEffectSpecHandle H = Src->MakeOutgoingSpec(TerrorDebuffClass, 1.f, Src->MakeEffectContext());
					if (H.IsValid() && H.Data.IsValid() && FDFGameplayTags::Data_Duration.IsValid())
					{
						H.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, 3.f);
						Src->ApplyGameplayEffectSpecToTarget(*H.Data, Tgt);
					}
				}
				if (WeakenDebuffClass)
				{
					UDFGameplayEffectLibrary::ApplyEffectToTarget(Ch, HitA, WeakenDebuffClass);
				}
			}
		}
	}
	Boss->Multicast_BossLocalAttackFX(O, HeavyCameraShake, ShockwaveNiagara, ShakeInner, ShakeOuter);
}
