// Source/DungeonForged/Private/GAS/DFAbility_Fireball.cpp
#include "GAS/DFAbility_Fireball.h"
#include "GAS/DFGameplayTags.h"
#include "Combat/DFFireballProjectile.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayEffectTypes.h"

UDFAbility_Fireball::UDFAbility_Fireball()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	// Manual mana handled by cost GE: keep base check from ignoring wrong values
	AbilityCost_Mana = 0.f;
}

void UDFAbility_Fireball::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Fire_Fireball);
	}
}

void UDFAbility_Fireball::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo, nullptr))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!IsValid(AbilityMontage) || !ActorInfo)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* const MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, AbilityMontage, 1.f, NAME_None, true, 1.f, 0.f, false);
	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	MontageTask->OnCompleted.AddDynamic(this, &UDFAbility_Fireball::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UDFAbility_Fireball::OnMontageInterruptedOrCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &UDFAbility_Fireball::OnMontageInterruptedOrCancelled);
	MontageTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* const EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FDFGameplayTags::Event_Ability_Fire_Launch, nullptr, true, true);
	if (!EventTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	EventTask->EventReceived.AddDynamic(this, &UDFAbility_Fireball::OnFireLaunchEvent);
	EventTask->ReadyForActivation();
}

void UDFAbility_Fireball::OnFireLaunchEvent(FGameplayEventData Payload)
{
	(void)Payload;
	const FGameplayAbilityActorInfo* const Info = GetCurrentActorInfo();
	if (!Info || !Info->IsNetAuthority())
	{
		return;
	}
	ACharacter* const Char = GetAvatarActorFromActorInfo() ? Cast<ACharacter>(GetAvatarActorFromActorInfo()) : nullptr;
	if (!Char || !FireballProjectileClass)
	{
		return;
	}
	USkeletalMeshComponent* const Sk = Char->GetMesh();
	if (!IsValid(Sk))
	{
		return;
	}
	const FTransform SocketXform = Sk->DoesSocketExist(MuzzleSocketName)
		? Sk->GetSocketTransform(MuzzleSocketName, ERelativeTransformSpace::RTS_World)
		: FTransform(Char->GetActorRotation(), Char->GetActorLocation());
	FActorSpawnParameters Params;
	Params.Instigator = Char;
	Params.Owner = Char;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}
	World->SpawnActor<ADFFireballProjectile>(FireballProjectileClass, SocketXform.GetLocation(), SocketXform.Rotator(), Params);
}

void UDFAbility_Fireball::OnMontageCompleted()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UDFAbility_Fireball::OnMontageInterruptedOrCancelled()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}
