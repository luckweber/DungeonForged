// Source/DungeonForged/Private/GAS/Abilities/Boss/UDFBossAbility_PhaseTransitionSlam.cpp
#include "GAS/Abilities/Boss/UDFBossAbility_PhaseTransitionSlam.h"
#include "GAS/Abilities/Boss/DFBossAbilityCommons.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UDFGameplayEffectLibrary.h"
#include "GAS/UDFAttributeSet.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Boss/ADFBossBase.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"

UDFBossAbility_PhaseTransitionSlam::UDFBossAbility_PhaseTransitionSlam()
{
	bSourceObjectMustBeBoss = true;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseCooldown = 0.f;
}

void UDFBossAbility_PhaseTransitionSlam::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Boss_PhaseTransitionSlam);
	}
}

void UDFBossAbility_PhaseTransitionSlam::ActivateAbility(
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
	ADFBossBase* const Boss = Cast<ADFBossBase>(GetAvatarActorFromActorInfo());
	ACharacter* const Pl = DFBossAbilityCommons::GetFirstPlayerCharacter(GetWorld(), Boss);
	if (Pl && Boss)
	{
		if (UAbilitySystemComponent* const Pasc = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pl))
		{
			Pasc->AddLooseGameplayTag(FDFGameplayTags::UI_CinematicLock);
		}
		DFBossAbilityCommons::StunTarget(Boss->GetAbilitySystemComponent(), Pl, PlayerLockStunSeconds);
	}
	bEruptFired = false;
	UAnimMontage* M = PhaseTransitionSlamMontage ? PhaseTransitionSlamMontage : AbilityMontage;
	if (M && Boss && Boss->GetMesh())
	{
		if (UAbilityTask_PlayMontageAndWait* Play = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, NAME_None, M, 1.f, NAME_None, true, 1.f, 0.f, false))
		{
			Play->OnCompleted.AddDynamic(this, &UDFBossAbility_PhaseTransitionSlam::OnMontageEnd);
			Play->OnInterrupted.AddDynamic(this, &UDFBossAbility_PhaseTransitionSlam::OnMontageEnd);
			Play->OnCancelled.AddDynamic(this, &UDFBossAbility_PhaseTransitionSlam::OnMontageEnd);
			Play->ReadyForActivation();
		}
	}
	if (FDFGameplayTags::Event_Boss_PhaseErupt.IsValid())
	{
		if (UAbilityTask_WaitGameplayEvent* Ev = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, FDFGameplayTags::Event_Boss_PhaseErupt, nullptr, true, true))
		{
			Ev->EventReceived.AddDynamic(this, &UDFBossAbility_PhaseTransitionSlam::OnEruptEvent);
			Ev->ReadyForActivation();
		}
	}
}

void UDFBossAbility_PhaseTransitionSlam::OnEruptEvent(FGameplayEventData Data)
{
	(void)Data;
	if (bEruptFired)
	{
		return;
	}
	bEruptFired = true;
	DoRoomSlam();
}

void UDFBossAbility_PhaseTransitionSlam::OnMontageEnd()
{
	if (!bEruptFired)
	{
		DoRoomSlam();
		bEruptFired = true;
	}
	RemovePlayerStunLock();
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UDFBossAbility_PhaseTransitionSlam::RemovePlayerStunLock()
{
	ADFBossBase* const Boss = Cast<ADFBossBase>(GetAvatarActorFromActorInfo());
	ACharacter* const Pl = DFBossAbilityCommons::GetFirstPlayerCharacter(GetWorld(), Boss);
	if (Pl)
	{
		if (UAbilitySystemComponent* Pasc = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pl))
		{
			FGameplayTagContainer StunT;
			StunT.AddTag(FDFGameplayTags::State_Stunned);
			Pasc->RemoveActiveEffectsWithGrantedTags(StunT);
			Pasc->RemoveLooseGameplayTag(FDFGameplayTags::UI_CinematicLock, 0);
		}
	}
}

void UDFBossAbility_PhaseTransitionSlam::DoRoomSlam()
{
	ADFBossBase* const Boss = Cast<ADFBossBase>(GetAvatarActorFromActorInfo());
	if (!Boss || !Boss->HasAuthority() || !GetWorld())
	{
		return;
	}
	const FVector O = Boss->GetActorLocation();
	FCollisionObjectQueryParams Ob(ECC_Pawn);
	FCollisionQueryParams Q(SCENE_QUERY_STAT(DF_PhaseSlam), false, Boss);
	Q.AddIgnoredActor(Boss);
	TArray<FOverlapResult> Hits;
	GetWorld()->OverlapMultiByObjectType(Hits, O, FQuat::Identity, Ob, FCollisionShape::MakeSphere(RoomSlamRadius), Q);
	UAbilitySystemComponent* const Src = Boss->GetAbilitySystemComponent();
	if (!Src)
	{
		return;
	}
	ACharacter* const Pl = DFBossAbilityCommons::GetFirstPlayerCharacter(GetWorld(), Boss);
	if (!Pl)
	{
		return;
	}
	UAbilitySystemComponent* Tgt = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pl);
	if (!Tgt)
	{
		return;
	}
	const float MaxH = Tgt->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute());
	const float Dmg = FMath::Max(1.f, MaxH * HealthPercentDamage);
	const FGameplayEffectSpecHandle SpecH = UDFGameplayEffectLibrary::MakeDamageEffect(Dmg, FDFGameplayTags::Effect_Damage_True, Boss);
	if (SpecH.IsValid() && SpecH.Data)
	{
		Src->ApplyGameplayEffectSpecToTarget(*SpecH.Data, Tgt);
	}
	if (RoomShockwaveNiagara)
	{
		Boss->Multicast_BossLocalAttackFX(O, ExtremeCameraShake, RoomShockwaveNiagara, 0.f, 8000.f);
	}
	if (Pl && FDFGameplayTags::Event_Boss_WhiteFlash.IsValid())
	{
		FGameplayEventData Fd;
		Fd.EventMagnitude = ScreenWhiteFlashDuration;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Pl, FDFGameplayTags::Event_Boss_WhiteFlash, Fd);
	}
}
