// Source/DungeonForged/Private/GAS/Abilities/Boss/UDFBossAbility_MeteorStrike.cpp
#include "GAS/Abilities/Boss/UDFBossAbility_MeteorStrike.h"
#include "GAS/Abilities/Boss/DFBossAbilityCommons.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Cooldown_Boss_MeteorStrike.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Boss/ADFBossBase.h"
#include "Boss/ADFMeteorImpactActor.h"
#include "Boss/ADFMeteorWarningDecal.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

UDFBossAbility_MeteorStrike::UDFBossAbility_MeteorStrike()
{
	bSourceObjectMustBeBoss = true;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseCooldown = 45.f;
	CooldownClass = UGE_Cooldown_Boss_MeteorStrike::StaticClass();
	WarningDecalClass = ADFMeteorWarningDecal::StaticClass();
	ImpactClass = ADFMeteorImpactActor::StaticClass();
}

void UDFBossAbility_MeteorStrike::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Boss_MeteorStrike);
	}
}

bool UDFBossAbility_MeteorStrike::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) || !ActorInfo)
	{
		return false;
	}
	if (const ADFBossBase* B = Cast<ADFBossBase>(ActorInfo->AvatarActor.Get()))
	{
		return B->CurrentPhase >= 2;
	}
	return false;
}

void UDFBossAbility_MeteorStrike::ActivateAbility(
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
	DFBossAbilityCommons::ApplySetCallerCooldown(ActorInfo->AbilitySystemComponent.Get(), CooldownClass, 45.f);

	ADFBossBase* const Boss = Cast<ADFBossBase>(GetAvatarActorFromActorInfo());
	ACharacter* const Player = DFBossAbilityCommons::GetFirstPlayerCharacter(GetWorld(), Boss);
	if (!Boss || !GetWorld() || !WarningDecalClass || !ImpactClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	FVector Target = Boss->GetActorLocation() + Boss->GetActorForwardVector() * 200.f;
	if (Player)
	{
		Target = Player->GetActorLocation() + Player->GetVelocity() * PlayerLeadSeconds;
	}
	FActorSpawnParameters Sp;
	Sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Sp.Owner = Boss;
	Sp.Instigator = Boss;
	(void)(GetWorld()->SpawnActor<ADFMeteorWarningDecal>(WarningDecalClass, FTransform(FRotator(-90.f, 0.f, 0.f), Target), Sp));
	PendingMeteorAim = Target;
	if (UAbilityTask_WaitDelay* D = UAbilityTask_WaitDelay::WaitDelay(this, TelegraphTime))
	{
		D->OnFinish.AddDynamic(this, &UDFBossAbility_MeteorStrike::OnTelegraphEnd);
		D->ReadyForActivation();
	}
}

void UDFBossAbility_MeteorStrike::OnTelegraphEnd()
{
	ADFBossBase* const Boss = Cast<ADFBossBase>(GetAvatarActorFromActorInfo());
	if (!GetWorld() || !Boss || !ImpactClass)
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
		return;
	}
	FActorSpawnParameters S;
	S.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	S.Owner = Boss;
	S.Instigator = Boss;
	if (ADFMeteorImpactActor* I = GetWorld()->SpawnActor<ADFMeteorImpactActor>(ImpactClass, PendingMeteorAim, FRotator::ZeroRotator, S))
	{
		I->InitializeImpact(Boss, PendingMeteorAim, 500.f, 1000.f);
	}
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}