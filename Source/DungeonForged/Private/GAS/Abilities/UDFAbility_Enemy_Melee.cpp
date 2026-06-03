// Source/DungeonForged/Private/GAS/Abilities/UDFAbility_Enemy_Melee.cpp

#include "GAS/Abilities/UDFAbility_Enemy_Melee.h"
#include "Characters/ADFEnemyBase.h"
#include "Combat/AN/AN_SendGameplayEvent.h"
#include "Combat/UDFCombatDirectorSubsystem.h"
#include "Combat/UDFHitReactionComponent.h"
#include "Combat/UDFMeleeAimComponent.h"
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
#include "Combat/DFCombatDebug.h"
#include "DrawDebugHelpers.h"
#include "FX/UDFCombatFeedbackLibrary.h"
#include "DungeonForgedModule.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "WorldCollision.h"

#if !UE_BUILD_SHIPPING
static TAutoConsoleVariable<int32> CVarDF_DebugEnemyMelee(
	TEXT("df.DebugEnemyMelee"),
	0,
	TEXT("DungeonForged: debug UDFAbility_Enemy_Melee hit volume.\n")
	TEXT(" 0: Off (also enable df.DebugCombat 32)\n")
	TEXT(" 1: Log + on-screen\n")
	TEXT(" 2: Log + draw hit sphere + lines to targets"),
	ECVF_Cheat);

static bool IsEnemyMeleeDebugLogEnabled()
{
	return CVarDF_DebugEnemyMelee.GetValueOnGameThread() > 0
		|| DFCombatDebug::IsChannelEnabled(DFCombatDebug::EChannel::EnemyMelee);
}

static bool IsEnemyMeleeDebugDrawEnabled()
{
	return CVarDF_DebugEnemyMelee.GetValueOnGameThread() >= 2
		|| DFCombatDebug::IsChannelEnabled(DFCombatDebug::EChannel::EnemyMelee);
}
#endif

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

/** Reads UAN_SendGameplayEvent time on the montage asset (same idea as MeleeTrace ScheduleAuthorityTraceWindows). */
bool FindSendGameplayEventNotifyTime(UAnimMontage* const Montage, const FGameplayTag EventTag, float& OutSec)
{
	OutSec = -1.f;
	if (!Montage || !EventTag.IsValid())
	{
		return false;
	}
	for (const FAnimNotifyEvent& Ev : Montage->Notifies)
	{
		if (!Ev.Notify)
		{
			continue;
		}
		const UAN_SendGameplayEvent* const SendNotify = Cast<UAN_SendGameplayEvent>(Ev.Notify);
		if (!SendNotify || SendNotify->EventTag != EventTag)
		{
			continue;
		}
		const float T = Ev.GetTriggerTime();
		if (OutSec < 0.f || T < OutSec)
		{
			OutSec = T;
		}
	}
	return OutSec >= 0.f;
}

#if !UE_BUILD_SHIPPING
static void EnemyMeleeDebugLog(const ACharacter* Char, const FString& Line, const FColor Color = FColor::Orange)
{
	DF_LOG(Log, "%s", *Line);
	if (GEngine && IsEnemyMeleeDebugLogEnabled())
	{
		DF_SCREEN(Color, 2.f, "%s", *Line);
	}
	(void)Char;
}

static void DrawEnemyMeleeHitVolume(
	UWorld* const World,
	const ACharacter* Char,
	const FVector& Center,
	const float Radius,
	const TArray<AActor*>& HitTargets,
	const bool bAnyDamage)
{
	if (!World || !IsEnemyMeleeDebugDrawEnabled())
	{
		return;
	}
	const float Duration = 2.f;
	const FColor VolColor = bAnyDamage ? FColor::Orange : FColor(255, 128, 0, 128);
	DrawDebugSphere(World, Center, Radius, 16, VolColor, false, Duration, 0, 1.5f);
	if (Char)
	{
		const FVector Origin = Char->GetActorLocation();
		DrawDebugLine(World, Origin, Center, FColor::Yellow, false, Duration, 0, 1.f);
		DrawDebugString(
			World,
			Center + FVector(0.f, 0.f, Radius + 12.f),
			FString::Printf(TEXT("EnemyMelee R=%.0f"), Radius),
			nullptr,
			FColor::Orange,
			Duration,
			true,
			1.f);
	}
	for (AActor* const Target : HitTargets)
	{
		if (!Target)
		{
			continue;
		}
		DrawDebugLine(World, Center, Target->GetActorLocation(), FColor::Green, false, Duration, 0, 2.f);
		DrawDebugSphere(World, Target->GetActorLocation(), 24.f, 8, FColor::Green, false, Duration, 0, 1.5f);
	}
}
#endif
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
		if (FDFGameplayTags::Event_Ability_Melee_Hit.IsValid())
		{
			HitGameplayEventTag = FDFGameplayTags::Event_Ability_Melee_Hit;
		}
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
	bHoldsAttackToken = false;
	bHoldsRangedCastToken = false;
	if (ADFEnemyBase* const Enemy = Cast<ADFEnemyBase>(GetAvatarActorFromActorInfo()))
	{
		if (UWorld* const World = GetWorld())
		{
			if (UDFCombatDirectorSubsystem* const Director = World->GetSubsystem<UDFCombatDirectorSubsystem>())
			{
				const bool bIsRangedAttack = FDFGameplayTags::Ability_Attack_Ranged.IsValid()
					&& AbilityTags.HasTag(FDFGameplayTags::Ability_Attack_Ranged);
				if (bIsRangedAttack)
				{
					if (!Director->RequestRangedCastToken(Enemy))
					{
						EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
						return;
					}
					bHoldsRangedCastToken = true;
				}
				else
				{
					if (!Director->RequestAttackToken(Enemy))
					{
						EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
						return;
					}
					bHoldsAttackToken = true;
				}
			}
		}

		// AAA aim: snap-rotate to face the target and commit it as ManualTarget so the montage's
		// UANS_DFMeleeWarp notify states warp to the same actor. Fixes "attacks with back to player".
		if (UDFMeleeAimComponent* const Aim = Enemy->MeleeAim)
		{
			Aim->AcquireAndCommitTarget();
		}
	}
	ActiveParallelTasks = 0;
	bHitWindowDone = false;
	bMontageFinishHandled = false;

	const FGameplayTag ResolvedHitTag = HitGameplayEventTag.IsValid()
		? HitGameplayEventTag
		: FDFGameplayTags::Event_Ability_Melee_Hit;

	float MontageHitSec = -1.f;
	const bool bScheduledFromMontage = AbilityMontage && FindSendGameplayEventNotifyTime(AbilityMontage, ResolvedHitTag, MontageHitSec);
	const bool bUseGameplayEventHit = !bScheduledFromMontage && HitGameplayEventTag.IsValid() && AbilityMontage;

	// 1) Hit window: montage notify time (authoritative) > runtime GameplayEvent > legacy fixed delay.
	if (bScheduledFromMontage)
	{
#if !UE_BUILD_SHIPPING
		DF_LOG(Log, "[EnemyMelee] Hit scheduled from montage %s at %.3fs (tag=%s)",
			*AbilityMontage->GetName(), MontageHitSec, *ResolvedHitTag.ToString());
#endif
		if (MontageHitSec <= 0.001f)
		{
			ExecuteHitWindow();
		}
		else if (UAbilityTask_WaitDelay* const D = UAbilityTask_WaitDelay::WaitDelay(this, MontageHitSec))
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
	}
	else if (bUseGameplayEventHit)
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
#if !UE_BUILD_SHIPPING
		DF_LOG(Warning, "[EnemyMelee] No DF Send Gameplay Event on montage for %s — instant hit. Add notify or tune HitWindowDelay.",
			AbilityMontage ? *AbilityMontage->GetName() : TEXT("(no montage)"));
#endif
		ExecuteHitWindow();
	}
	else if (UAbilityTask_WaitDelay* const D = UAbilityTask_WaitDelay::WaitDelay(this, HitWindowDelay))
	{
#if !UE_BUILD_SHIPPING
		DF_LOG(Log, "[EnemyMelee] Hit via HitWindowDelay=%.3fs (add UAN_SendGameplayEvent %s on montage for sync)",
			HitWindowDelay, *ResolvedHitTag.ToString());
#endif
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

void UDFAbility_Enemy_Melee::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	if (ADFEnemyBase* const Enemy = Cast<ADFEnemyBase>(GetAvatarActorFromActorInfo()))
	{
		if (UDFMeleeAimComponent* const Aim = Enemy->MeleeAim)
		{
			Aim->ReleaseAttackTarget();
		}
		if (bHoldsAttackToken)
		{
			if (UWorld* const World = GetWorld())
			{
				if (UDFCombatDirectorSubsystem* const Director = World->GetSubsystem<UDFCombatDirectorSubsystem>())
				{
					Director->ReleaseAttackToken(Enemy);
				}
			}
		}
		if (bHoldsRangedCastToken)
		{
			if (UWorld* const World = GetWorld())
			{
				if (UDFCombatDirectorSubsystem* const Director = World->GetSubsystem<UDFCombatDirectorSubsystem>())
				{
					Director->ReleaseRangedCastToken(Enemy);
				}
			}
		}
	}
	bHoldsAttackToken = false;
	bHoldsRangedCastToken = false;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
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
	const bool bOverlapped = GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, Obj, Sphere, Q);

	TArray<AActor*> DamagedTargets;
	TSet<AActor*> Seen;
	if (bOverlapped)
	{
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
				if (DamageSourceTag.IsValid())
				{
					S.Data->AddDynamicAssetTag(DamageSourceTag);
				}
				SourceASC->ApplyGameplayEffectSpecToTarget(*S.Data.Get(), TASC);
				ApplyBonusOnHitEffects(SourceASC, TASC, Ctx);
				FVector HitDir = A->GetActorLocation() - Char->GetActorLocation();
				HitDir.Z = 0.f;
				HitDir.Normalize();
				FHitResult HitResult;
				HitResult.ImpactPoint = A->GetActorLocation();
				HitResult.ImpactNormal = -HitDir.GetSafeNormal();
				HitResult.bBlockingHit = true;
				UDFCombatFeedbackLibrary::DispatchProjectileHitConfirmed(
					this, Char, A, HitResult, Dmg, 0.f, DamageSourceTag);
				PlayImpactCosmetics(Char, A->GetActorLocation(), Char->GetActorLocation() - A->GetActorLocation());
				DamagedTargets.Add(A);
			}
		}
	}

#if !UE_BUILD_SHIPPING
	if (IsEnemyMeleeDebugLogEnabled())
	{
		FString TargetList = TEXT("(none)");
		if (DamagedTargets.Num() > 0)
		{
			TargetList = DamagedTargets[0]->GetName();
			for (int32 Idx = 1; Idx < DamagedTargets.Num(); ++Idx)
			{
				TargetList += TEXT(", ") + DamagedTargets[Idx]->GetName();
			}
		}
		EnemyMeleeDebugLog(
			Char,
			FString::Printf(
				TEXT("[EnemyMelee|Hit] %s dmg=%.1f center=%s R=%.0f off=%.0f overlaps=%d hit=%d targets=%s"),
				*Char->GetName(),
				Dmg,
				*Center.ToCompactString(),
				MeleeRadius,
				MeleeForwardOffset,
				Overlaps.Num(),
				DamagedTargets.Num(),
				*TargetList),
			DamagedTargets.Num() > 0 ? FColor::Green : FColor::Orange);
	}
	if (IsEnemyMeleeDebugDrawEnabled() || bDrawDebugHitVolume)
	{
		DrawEnemyMeleeHitVolume(GetWorld(), Char, Center, MeleeRadius, DamagedTargets, DamagedTargets.Num() > 0);
	}
#endif
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
