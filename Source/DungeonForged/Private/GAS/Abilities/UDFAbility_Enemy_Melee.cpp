// Source/DungeonForged/Private/GAS/Abilities/UDFAbility_Enemy_Melee.cpp

#include "GAS/Abilities/UDFAbility_Enemy_Melee.h"
#include "Characters/ADFEnemyBase.h"
#include "Combat/UDFHitReactionComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Damage_Physical.h"
#include "GAS/UDFAttributeSet.h"
#include "GameplayEffectTypes.h"
#include "Animation/AnimMontage.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "WorldCollision.h"

namespace
{
bool IsDFEnemyMeleeTargetAlive(UAbilitySystemComponent* const TargetASC)
{
	if (!TargetASC)
	{
		return false;
	}
	if (FDFGameplayTags::State_Dead.IsValid() && TargetASC->HasMatchingGameplayTag(FDFGameplayTags::State_Dead))
	{
		return false;
	}
	const FGameplayAttribute HealthAttribute = UDFAttributeSet::GetHealthAttribute();
	return !HealthAttribute.IsValid() || TargetASC->GetNumericAttribute(HealthAttribute) > 0.f;
}
} // namespace

UDFAbility_Enemy_Melee::UDFAbility_Enemy_Melee()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	AbilityCost_Mana = 0.f;
	AbilityCost_Stamina = 0.f;
	ActivationPolicy = EAbilityActivationPolicy::Passive; // not bound to input; AI activates by tag
}

void UDFAbility_Enemy_Melee::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Attack_Melee);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
		ActivationOwnedTags.AddTag(FDFGameplayTags::State_Attacking);
	}
}

void UDFAbility_Enemy_Melee::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
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
	ActiveParallelTasks = 0;
	bHitWindowDone = false;
	bMontageFinishHandled = false;
	const bool bUseGameplayEventHit = HitGameplayEventTag.IsValid() && AbilityMontage;

	// 1) Hit window: prefer montage GameplayEvent for exact frame timing; otherwise use the legacy delay.
	if (bUseGameplayEventHit)
	{
		if (UAbilityTask_WaitGameplayEvent* const HitEventTask =
				UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HitGameplayEventTag, nullptr, true, true))
		{
			HitEventTask->EventReceived.AddDynamic(this, &UDFAbility_Enemy_Melee::OnHitGameplayEvent);
			HitEventTask->ReadyForActivation();
		}
		else
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
	}
	else if (HitWindowDelay <= 0.001f)
	{
		ExecuteHitWindow();
	}
	else if (UAbilityTask_WaitDelay* const D = UAbilityTask_WaitDelay::WaitDelay(this, HitWindowDelay))
	{
		++ActiveParallelTasks;
		D->OnFinish.AddDynamic(this, &UDFAbility_Enemy_Melee::OnHitWindowElapsed);
		D->ReadyForActivation();
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2) Optional montage: one parallel end signal. If CreatePlayMontageAndWait fails (skeleton/ASC),
	//    fall back to UDF::PlayAbilityMontage + delay — otherwise the ability ends in the same frame
	//    and the BT re-fires "DF MeleeAttack" in a loop.
	if (AbilityMontage)
	{
		bool bMontageEndTaskAdded = false;
		if (UAbilityTask_PlayMontageAndWait* const M = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, NAME_None, AbilityMontage, 1.f, NAME_None, true, 1.f, 0.f, true))
		{
			++ActiveParallelTasks;
			bMontageEndTaskAdded = true;
			M->OnCompleted.AddDynamic(this, &UDFAbility_Enemy_Melee::OnMontageOrInstantFinished);
			M->OnInterrupted.AddDynamic(this, &UDFAbility_Enemy_Melee::OnMontageOrInstantFinished);
			M->OnCancelled.AddDynamic(this, &UDFAbility_Enemy_Melee::OnMontageOrInstantFinished);
			M->ReadyForActivation();
		}
		if (!bMontageEndTaskAdded)
		{
			const float Played = PlayAbilityMontage(1.f, NAME_None);
			float T = (Played > 0.01f) ? Played : AbilityMontage->GetPlayLength();
			if (T <= 0.01f)
			{
				T = 0.5f;
			}
			if (UAbilityTask_WaitDelay* const Dm = UAbilityTask_WaitDelay::WaitDelay(this, T))
			{
				++ActiveParallelTasks;
				Dm->OnFinish.AddDynamic(this, &UDFAbility_Enemy_Melee::OnMontageOrInstantFinished);
				Dm->ReadyForActivation();
			}
		}
	}
	if (ActiveParallelTasks == 0)
	{
		if (!bUseGameplayEventHit)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		}
	}
}

void UDFAbility_Enemy_Melee::OnHitWindowElapsed()
{
	if (bHitWindowDone)
	{
		return;
	}
	ExecuteHitWindow();
	TryEndWhenIdle();
}

void UDFAbility_Enemy_Melee::OnHitGameplayEvent(FGameplayEventData Payload)
{
	(void)Payload;
	ExecuteHitWindow();
	if (ActiveParallelTasks == 0)
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	}
}

void UDFAbility_Enemy_Melee::OnMontageOrInstantFinished()
{
	if (bMontageFinishHandled)
	{
		return;
	}
	bMontageFinishHandled = true;
	TryEndWhenIdle();
}

void UDFAbility_Enemy_Melee::TryEndWhenIdle()
{
	--ActiveParallelTasks;
	if (ActiveParallelTasks > 0)
	{
		return;
	}
	// No montage: end after the single delay (hit window was the only task, ActiveParallel might be wrong)
	// We increment for Delay + maybe Montage. On last finish, end.
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UDFAbility_Enemy_Melee::ExecuteHitWindow()
{
	if (bHitWindowDone)
	{
		return;
	}
	bHitWindowDone = true;
	if (ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (Char->HasAuthority())
		{
			PlayAttackCosmetics(Char);
		}
	}
	ApplyDamageToOverlappingTargets();
}

void UDFAbility_Enemy_Melee::ApplyDamageToOverlappingTargets()
{
	ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* const SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!Char || !SourceASC || !GetWorld() || !Char->HasAuthority())
	{
		return;
	}
	const FGameplayAttribute StrAttr = UDFAttributeSet::GetStrengthAttribute();
	const float Str = StrAttr.IsValid() ? SourceASC->GetNumericAttribute(StrAttr) : 0.f;
	const float Dmg = FMath::Max(0.f, Str * FMath::Max(0.f, DamageFromStrengthScale) + FMath::Max(0.f, AddedDamage));
	if (Dmg <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector Fwd = Char->GetActorForwardVector();
	const FVector Center = Char->GetActorLocation() + Fwd * MeleeForwardOffset;
	const FCollisionObjectQueryParams Obj(ECC_Pawn);
	FCollisionQueryParams Q(SCENE_QUERY_STAT(EnemyMelee), false, Char);
	Q.AddIgnoredActor(Char);
	FCollisionShape const Sphere = FCollisionShape::MakeSphere(MeleeRadius);
	TArray<FOverlapResult> Overlaps;
	if (!GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, Obj, Sphere, Q))
	{
		return;
	}
	TSet<AActor*> Seen;
	for (const FOverlapResult& H : Overlaps)
	{
		AActor* const A = H.GetActor();
		if (!A || Seen.Contains(A) || A == Char)
		{
			continue;
		}
		Seen.Add(A);
		// Do not hit other AI enemies; adjust if you want friendly damage.
		if (Cast<ADFEnemyBase>(A) && A != Char)
		{
			continue;
		}
		UAbilitySystemComponent* const TASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(A);
		if (!IsDFEnemyMeleeTargetAlive(TASC))
		{
			continue;
		}
		FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
		Ctx.AddSourceObject(this);
		Ctx.AddInstigator(Char, Char);
		const FGameplayEffectSpecHandle S = SourceASC->MakeOutgoingSpec(UGE_Damage_Physical::StaticClass(), 1.f, Ctx);
		if (S.IsValid() && S.Data && FDFGameplayTags::Data_Damage.IsValid())
		{
			S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, Dmg);
			SourceASC->ApplyGameplayEffectSpecToTarget(*S.Data.Get(), TASC);
			ApplyBonusOnHitEffects(SourceASC, TASC, Ctx);
			if (UDFHitReactionComponent* const Hit = A->FindComponentByClass<UDFHitReactionComponent>())
			{
				FVector HitDir = A->GetActorLocation() - Char->GetActorLocation();
				HitDir.Z = 0.f;
				HitDir.Normalize();
				const FVector HitLocation = A->GetActorLocation();
				Hit->OnHitReceived(Dmg, 0.f, HitDir, Char, HitLocation, -HitDir.GetSafeNormal());
			}
			PlayImpactCosmetics(Char, A->GetActorLocation(), Char->GetActorLocation() - A->GetActorLocation());
		}
	}
}

void UDFAbility_Enemy_Melee::ApplyBonusOnHitEffects(
	UAbilitySystemComponent* const SourceASC,
	UAbilitySystemComponent* const TargetASC,
	FGameplayEffectContextHandle const& Ctx) const
{
	if (!SourceASC || !TargetASC || BonusOnHitEffects.Num() == 0)
	{
		return;
	}
	for (FDFEnemyMeleeBonusOnHitEffect const& Entry : BonusOnHitEffects)
	{
		if (!Entry.EffectClass || Entry.Chance <= 0.f || FMath::FRand() > Entry.Chance)
		{
			continue;
		}
		const float Level = static_cast<float>(FMath::Max(1, Entry.EffectLevel));
		const FGameplayEffectSpecHandle Bonus = SourceASC->MakeOutgoingSpec(Entry.EffectClass, Level, Ctx);
		if (!Bonus.IsValid() || !Bonus.Data)
		{
			continue;
		}
		const float EffDur =
			Entry.SetByCallerDuration > KINDA_SMALL_NUMBER ? Entry.SetByCallerDuration : BonusOnHitDefaultSetByCallerDuration;
		const float EffDmg =
			Entry.SetByCallerDamage > KINDA_SMALL_NUMBER ? Entry.SetByCallerDamage : BonusOnHitDefaultSetByCallerDamage;

		if (FDFGameplayTags::Data_Duration.IsValid() && EffDur > KINDA_SMALL_NUMBER)
		{
			Bonus.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, EffDur);
		}
		if (FDFGameplayTags::Data_Damage.IsValid() && EffDmg > KINDA_SMALL_NUMBER)
		{
			Bonus.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, EffDmg);
		}
		SourceASC->ApplyGameplayEffectSpecToTarget(*Bonus.Data.Get(), TargetASC);
	}
}

void UDFAbility_Enemy_Melee::PlayAttackCosmetics(ACharacter* const SourceCharacter) const
{
	if (!SourceCharacter || (!AttackSound && !AttackVFX))
	{
		return;
	}
	if (ADFEnemyBase* const Enemy = Cast<ADFEnemyBase>(SourceCharacter))
	{
		Enemy->Multicast_PlayEnemyCosmeticCue(
			AttackSound,
			AttackVFX,
			AttackFXSocketName,
			SourceCharacter->GetActorLocation(),
			SourceCharacter->GetActorRotation(),
			AttackVFXScale,
			true);
	}
}

void UDFAbility_Enemy_Melee::PlayImpactCosmetics(
	ACharacter* const SourceCharacter,
	const FVector& HitLocation,
	const FVector& HitNormal) const
{
	if (!SourceCharacter || (!ImpactSound && !ImpactVFX))
	{
		return;
	}
	if (ADFEnemyBase* const Enemy = Cast<ADFEnemyBase>(SourceCharacter))
	{
		const FRotator ImpactRotation = HitNormal.IsNearlyZero() ? FRotator::ZeroRotator : HitNormal.GetSafeNormal().Rotation();
		Enemy->Multicast_PlayEnemyCosmeticCue(
			ImpactSound,
			ImpactVFX,
			NAME_None,
			HitLocation,
			ImpactRotation,
			ImpactVFXScale,
			false);
	}
}
