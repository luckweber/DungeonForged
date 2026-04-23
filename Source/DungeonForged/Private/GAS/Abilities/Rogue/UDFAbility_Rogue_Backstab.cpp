// Source/DungeonForged/Private/GAS/Abilities/Rogue/UDFAbility_Rogue_Backstab.cpp
#include "GAS/Abilities/Rogue/UDFAbility_Rogue_Backstab.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AnimMontage.h"
#include "Camera/UDFLockOnComponent.h"
#include "Characters/ADFPlayerCharacter.h"
#include "CollisionQueryParams.h"
#include "Combat/UDFComboPointsComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/DFRogueGAS.h"
#include "GAS/Effects/UGE_Damage_Physical.h"
#include "GAS/Effects/UGE_DoT_Bleed.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayTagContainer.h"
#include "HAL/PlatformTime.h"
#include "Characters/ADFEnemyBase.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "WorldCollision.h"

UDFAbility_Rogue_Backstab::UDFAbility_Rogue_Backstab()
{
	AbilityCost_Mana = 0.f;
	AbilityCost_Stamina = 15.f;
	BaseCooldown = 0.f;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	LastGCD = -1e20;
}

void UDFAbility_Rogue_Backstab::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Rogue_Backstab);
		AbilityTags.AddTag(FDFGameplayTags::Ability_Rogue);
		AbilityTags.AddTag(FDFGameplayTags::Ability_Attack_Melee);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Stunned);
		ActivationOwnedTags.AddTag(FDFGameplayTags::State_Attacking);
	}
}

UDFComboPointsComponent* UDFAbility_Rogue_Backstab::GetCombo(AActor* const From)
{
	if (const ADFPlayerCharacter* const P = Cast<ADFPlayerCharacter>(From))
	{
		return P->ComboPoints;
	}
	return nullptr;
}

bool UDFAbility_Rogue_Backstab::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) || !ActorInfo)
	{
		return false;
	}
	if (FPlatformTime::Seconds() - LastGCD < static_cast<double>(GlobalGCD) - KINDA_SMALL_NUMBER)
	{
		return false;
	}
	return true;
}

void UDFAbility_Rogue_Backstab::ActivateAbility(
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
	if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (ASC->GetOwner() && ASC->GetOwner()->HasAuthority())
		{
			ApplyResourceCostsToOwner(ASC);
		}
	}
	LastGCD = FPlatformTime::Seconds();
	const TObjectPtr<UAnimMontage> M = BackstabMontage ? BackstabMontage : AbilityMontage;
	if (!M)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (UAbilityTask_PlayMontageAndWait* const T = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, M, 1.f, NAME_None, true, 1.f, 0.f, true))
	{
		T->OnCompleted.AddDynamic(this, &UDFAbility_Rogue_Backstab::OnMontageEnd);
		T->OnBlendOut.AddDynamic(this, &UDFAbility_Rogue_Backstab::OnMontageEnd);
		T->OnInterrupted.AddDynamic(this, &UDFAbility_Rogue_Backstab::OnMontageEnd);
		T->OnCancelled.AddDynamic(this, &UDFAbility_Rogue_Backstab::OnMontageEnd);
		T->ReadyForActivation();
	}
	if (UAbilityTask_WaitGameplayEvent* E = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, FDFGameplayTags::Event_Rogue_Backstab_Trace, nullptr, true, true))
	{
		E->EventReceived.AddDynamic(this, &UDFAbility_Rogue_Backstab::OnTraceEvent);
		E->ReadyForActivation();
	}
}

void UDFAbility_Rogue_Backstab::OnTraceEvent(FGameplayEventData /*Payload*/)
{
	DoBackstab();
}

void UDFAbility_Rogue_Backstab::DoBackstab()
{
	ACharacter* const C = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!C || !C->HasAuthority() || !GetWorld())
	{
		return;
	}
	UAbilitySystemComponent* const Src = GetAbilitySystemComponentFromActorInfo();
	UDFComboPointsComponent* const ComboC = GetCombo(C);
	ADFPlayerCharacter* const P = Cast<ADFPlayerCharacter>(C);
	if (!Src || !P)
	{
		return;
	}
	const float Agi = Src->GetNumericAttribute(UDFAttributeSet::GetAgilityAttribute());
	AActor* Target = nullptr;
	if (P->LockOnComponent)
	{
		Target = P->LockOnComponent->GetCurrentTarget();
	}
	if (!IsValid(Target))
	{
		const FVector Start = C->GetActorLocation() + FVector(0, 0, 50.f);
		const FVector End = Start + C->GetActorForwardVector() * MeleeRange;
		FCollisionQueryParams Q(SCENE_QUERY_STAT(Backstab), false, C);
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Q) && IsValid(Hit.GetActor()) && Cast<ADFEnemyBase>(Hit.GetActor()))
		{
			Target = Hit.GetActor();
		}
	}
	if (!IsValid(Target) || !Target->GetRootComponent())
	{
		return;
	}
	const FVector TForward = Target->GetActorForwardVector();
	const FVector CForward = C->GetActorForwardVector();
	const float D = FVector::DotProduct(CForward, TForward);
	const bool bBehind = D > 0.5f;
	const float Dmg = bBehind ? (Agi * 2.f + BaseDamage) : (Agi * 1.f + BaseDamage) * 0.5f;
	const bool bAmbush = Src->HasMatchingGameplayTag(FDFGameplayTags::Buff_Rogue_Ambush);
	float Sbc = DF_Rogue_CompensatePhysicalSetBy(Src, Dmg);
	if (bAmbush)
	{
		Sbc *= 2.f;
	}
	UAbilitySystemComponent* const Tasc = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!Tasc)
	{
		return;
	}
	{
		const FVector HitLoc = Target->GetActorLocation();
		const FVector HitNorm = (HitLoc - C->GetActorLocation()).GetSafeNormal();
		const FHitResult H(Target, nullptr, HitLoc, HitNorm.IsNearlyZero() ? FVector::UpVector : HitNorm);
		const FGameplayEffectContextHandle Ctx = DF_Rogue_EffectContext(Src, C, &H);
		const FGameplayEffectSpecHandle S = Src->MakeOutgoingSpec(UGE_Damage_Physical::StaticClass(), 1.f, Ctx);
		if (S.IsValid() && S.Data && FDFGameplayTags::Data_Damage.IsValid())
		{
			S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, Sbc);
			Src->ApplyGameplayEffectSpecToTarget(*S.Data, Tasc);
		}
	}
	if (bAmbush)
	{
		FGameplayTagContainer Amb;
		Amb.AddTag(FDFGameplayTags::Buff_Rogue_Ambush);
		Src->RemoveActiveEffectsWithGrantedTags(Amb);
		if (ComboC)
		{
			ComboC->AddComboPoints(3);
		}
	}
	{
		const FGameplayEffectContextHandle Ctx2 = DF_Rogue_EffectContext(Src, C, nullptr);
		const FGameplayEffectSpecHandle B = Src->MakeOutgoingSpec(UGE_DoT_Bleed::StaticClass(), 1.f, Ctx2);
		if (B.IsValid() && B.Data && FDFGameplayTags::Data_Duration.IsValid() && FDFGameplayTags::Data_Damage.IsValid())
		{
			B.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, 3.f);
			B.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, Agi * 0.1f);
			Src->ApplyGameplayEffectSpecToTarget(*B.Data, Tasc);
		}
	}
	if (ComboC)
	{
		ComboC->AddComboPoints(bBehind ? 2 : 1);
	}
}

void UDFAbility_Rogue_Backstab::OnMontageEnd()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
