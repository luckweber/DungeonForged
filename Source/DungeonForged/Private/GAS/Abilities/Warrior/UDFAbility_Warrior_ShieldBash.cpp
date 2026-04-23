// Source/DungeonForged/Private/GAS/Abilities/Warrior/UDFAbility_Warrior_ShieldBash.cpp
#include "GAS/Abilities/Warrior/UDFAbility_Warrior_ShieldBash.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraShakeBase.h"
#include "Characters/ADFEnemyBase.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Buff_Shield.h"
#include "GAS/Effects/UGE_Damage_Physical.h"
#include "GAS/Effects/UGE_Debuff_Stun.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Kismet/GameplayStatics.h"
#include "WorldCollision.h"

UDFAbility_Warrior_ShieldBash::UDFAbility_Warrior_ShieldBash()
{
	AbilityCost_Mana = 0.f;
	AbilityCost_Stamina = 20.f;
	BaseCooldown = 8.f;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UDFAbility_Warrior_ShieldBash::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Warrior_ShieldBash);
		ActivationOwnedTags.AddTag(FDFGameplayTags::State_Attacking);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Stunned);
	}
}

bool UDFAbility_Warrior_ShieldBash::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.Get())
	{
		return false;
	}
	if (!ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(FDFGameplayTags::Equipment_OffHand_Shield))
	{
		return false;
	}
	return true;
}

void UDFAbility_Warrior_ShieldBash::ActivateAbility(
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
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (ASC->GetOwner() && ASC->GetOwner()->HasAuthority())
		{
			ApplyResourceCostsToOwner(ASC);
		}
	}

	const TObjectPtr<UAnimMontage> Mont = ShieldBashMontage ? ShieldBashMontage : AbilityMontage;
	if (!Mont)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (UAbilityTask_PlayMontageAndWait* M = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Mont, 1.f, NAME_None, true, 1.f, 0.f, true))
	{
		M->OnCompleted.AddDynamic(this, &UDFAbility_Warrior_ShieldBash::OnMontageEnd);
		M->OnBlendOut.AddDynamic(this, &UDFAbility_Warrior_ShieldBash::OnMontageEnd);
		M->OnInterrupted.AddDynamic(this, &UDFAbility_Warrior_ShieldBash::OnMontageEnd);
		M->OnCancelled.AddDynamic(this, &UDFAbility_Warrior_ShieldBash::OnMontageEnd);
		M->ReadyForActivation();
	}
	if (UAbilityTask_WaitGameplayEvent* E =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FDFGameplayTags::Event_Warrior_ShieldBash_Trace, nullptr, true, true))
	{
		E->EventReceived.AddDynamic(this, &UDFAbility_Warrior_ShieldBash::OnTraceGameplayEvent);
		E->ReadyForActivation();
	}
}

void UDFAbility_Warrior_ShieldBash::OnTraceGameplayEvent(FGameplayEventData /*Payload*/)
{
	DoShieldBashBoxTrace();
	PlayLightCameraShake();
}

void UDFAbility_Warrior_ShieldBash::DoShieldBashBoxTrace() const
{
	ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* const SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!Char || !SourceASC || !GetWorld() || !Char->HasAuthority())
	{
		return;
	}
	const FVector Fwd = Char->GetActorForwardVector();
	const FVector Center = Char->GetActorLocation() + Fwd * 100.f;
	const FCollisionObjectQueryParams ObjParams(ECC_Pawn);
	FCollisionQueryParams Q(SCENE_QUERY_STAT(ShieldBash), false, Char);
	Q.AddIgnoredActor(Char);
	FCollisionShape const Box = FCollisionShape::MakeBox(FVector(25.f, 25.f, 40.f));
	const FQuat Rot = Fwd.Rotation().Quaternion();
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, Center, Rot, ObjParams, Box, Q);
	TSet<AActor*> Seen;
	for (const FOverlapResult& H : Overlaps)
	{
		AActor* A = H.GetActor();
		if (!A || Seen.Contains(A))
		{
			continue;
		}
		Seen.Add(A);
		if (!Cast<ADFEnemyBase>(A))
		{
			continue;
		}
		UAbilitySystemComponent* TASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(A);
		if (!TASC)
		{
			continue;
		}
		(void)SourceASC->GetNumericAttribute(UDFAttributeSet::GetStrengthAttribute());
		FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
		Ctx.AddSourceObject(this);
		Ctx.AddInstigator(Char, Char);
		{
			const FGameplayEffectSpecHandle S = SourceASC->MakeOutgoingSpec(UGE_Damage_Physical::StaticClass(), 1.f, Ctx);
			if (S.IsValid() && S.Data.IsValid())
			{
				S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, 40.f);
				SourceASC->ApplyGameplayEffectSpecToTarget(*S.Data.Get(), TASC);
			}
		}
		{
			const FGameplayEffectSpecHandle S = SourceASC->MakeOutgoingSpec(UGE_Debuff_Stun::StaticClass(), 1.f, Ctx);
			if (S.IsValid() && S.Data.IsValid())
			{
				S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, 1.5f);
				SourceASC->ApplyGameplayEffectSpecToTarget(*S.Data.Get(), TASC);
			}
		}
	}
	{
		FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
		const FGameplayEffectSpecHandle S = SourceASC->MakeOutgoingSpec(UGE_Buff_Shield::StaticClass(), 1.f, Ctx);
		if (S.IsValid() && S.Data.IsValid())
		{
			S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, 0.5f);
			SourceASC->ApplyGameplayEffectSpecToSelf(*S.Data.Get());
		}
	}
}

void UDFAbility_Warrior_ShieldBash::PlayLightCameraShake() const
{
	ACharacter* const Av = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	APlayerController* const PC = Av ? Cast<APlayerController>(Av->GetController()) : nullptr;
	if (PC && LightCameraShake)
	{
		PC->ClientStartCameraShake(LightCameraShake, 1.f, ECameraShakePlaySpace::UserDefined, FRotator::ZeroRotator);
	}
}

void UDFAbility_Warrior_ShieldBash::OnMontageEnd()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
