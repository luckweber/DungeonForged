// Source/DungeonForged/Private/GAS/Abilities/Rogue/UDFAbility_Rogue_Eviscerate.cpp
#include "GAS/Abilities/Rogue/UDFAbility_Rogue_Eviscerate.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AnimMontage.h"
#include "Camera/UDFLockOnComponent.h"
#include "Characters/ADFEnemyBase.h"
#include "Characters/ADFPlayerCharacter.h"
#include "CollisionQueryParams.h"
#include "Combat/UDFComboPointsComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/DFRogueGAS.h"
#include "GAS/Effects/UGE_Buff_Rogue_KillingSpree.h"
#include "GAS/Effects/UGE_Damage_Physical.h"
#include "GAS/Effects/UGE_DoT_Bleed.h"
#include "GAS/UDFAttributeSet.h"
#include "GAS/UDFPassivesGASEvents.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "WorldCollision.h"

UDFAbility_Rogue_Eviscerate::UDFAbility_Rogue_Eviscerate()
{
	AbilityCost_Mana = 0.f;
	AbilityCost_Stamina = 0.f;
	BaseCooldown = 1.f;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UDFAbility_Rogue_Eviscerate::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Rogue_Eviscerate);
		AbilityTags.AddTag(FDFGameplayTags::Ability_Rogue);
		AbilityTags.AddTag(FDFGameplayTags::Ability_Attack_Melee);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
		ActivationOwnedTags.AddTag(FDFGameplayTags::State_Attacking);
	}
}

bool UDFAbility_Rogue_Eviscerate::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) || !ActorInfo)
	{
		return false;
	}
	if (const ADFPlayerCharacter* const P = Cast<ADFPlayerCharacter>(ActorInfo->AvatarActor.Get()))
	{
		if (P->ComboPoints && P->ComboPoints->GetComboPoints() >= 1)
		{
			return true;
		}
	}
	return false;
}

void UDFAbility_Rogue_Eviscerate::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	(void)TriggerEventData;
	PointsSpentCache = 0;
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
	ADFPlayerCharacter* const P = Cast<ADFPlayerCharacter>(ActorInfo->AvatarActor.Get());
	UDFComboPointsComponent* const Combo = P ? P->ComboPoints : nullptr;
	if (Combo)
	{
		PointsSpentCache = Combo->GetComboPoints();
		Combo->ResetComboPoints();
	}
	if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (ASC->GetOwner() && ASC->GetOwner()->HasAuthority())
		{
			ApplyResourceCostsToOwner(ASC);
		}
	}
	const TObjectPtr<UAnimMontage> M = EviscerateMontage ? EviscerateMontage : AbilityMontage;
	if (!M)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (UAbilityTask_PlayMontageAndWait* const T = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, M, 1.f, NAME_None, true, 1.f, 0.f, true))
	{
		T->OnCompleted.AddDynamic(this, &UDFAbility_Rogue_Eviscerate::OnMontageEnd);
		T->OnBlendOut.AddDynamic(this, &UDFAbility_Rogue_Eviscerate::OnMontageEnd);
		T->OnInterrupted.AddDynamic(this, &UDFAbility_Rogue_Eviscerate::OnMontageEnd);
		T->OnCancelled.AddDynamic(this, &UDFAbility_Rogue_Eviscerate::OnMontageEnd);
		T->ReadyForActivation();
	}
	if (UAbilityTask_WaitGameplayEvent* E = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, FDFGameplayTags::Event_Rogue_Eviscerate_Trace, nullptr, true, true))
	{
		E->EventReceived.AddDynamic(this, &UDFAbility_Rogue_Eviscerate::OnTraceEvent);
		E->ReadyForActivation();
	}
}

void UDFAbility_Rogue_Eviscerate::OnTraceEvent(FGameplayEventData /*Payload*/)
{
	DoEvisc();
}

void UDFAbility_Rogue_Eviscerate::DoEvisc()
{
	ACharacter* const C = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!C || !C->HasAuthority() || !GetWorld())
	{
		return;
	}
	UAbilitySystemComponent* const Src = GetAbilitySystemComponentFromActorInfo();
	ADFPlayerCharacter* const P = Cast<ADFPlayerCharacter>(C);
	if (!Src || !P)
	{
		return;
	}
	const int32 Pts = FMath::Clamp(PointsSpentCache, 1, 5);
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
		FCollisionQueryParams Q(SCENE_QUERY_STAT(Evisc), false, C);
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Q) && IsValid(Hit.GetActor()) && Cast<ADFEnemyBase>(Hit.GetActor()))
		{
			Target = Hit.GetActor();
		}
	}
	if (!IsValid(Target))
	{
		return;
	}
	UAbilitySystemComponent* const Tasc = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!Tasc)
	{
		return;
	}
	const float Base = Agi * 1.5f;
	const float Bonus = Pts * Agi * 0.8f;
	const float Total = Base + Bonus;
	const float Sbc = DF_Rogue_CompensatePhysicalSetBy(Src, Total);
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
	{
		const float BleedDur = 2.f + static_cast<float>(Pts) * 1.f;
		const FGameplayEffectContextHandle Cx = DF_Rogue_EffectContext(Src, C, nullptr);
		FGameplayEffectSpecHandle B = Src->MakeOutgoingSpec(UGE_DoT_Bleed::StaticClass(), 1.f, Cx);
		if (B.IsValid() && B.Data && FDFGameplayTags::Data_Duration.IsValid() && FDFGameplayTags::Data_Damage.IsValid())
		{
			B.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, BleedDur);
			// UDFMMC_BleedDoT: -Data.Damage*0.2 per tick; want tick=Agi*0.3*Pts => D=Agi*1.5*Pts
			const float BleedD = Agi * 1.5f * Pts;
			B.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, FMath::Max(0.01f, BleedD));
			B = UAbilitySystemBlueprintLibrary::SetDuration(B, BleedDur);
			if (B.IsValid() && B.Data)
			{
				Src->ApplyGameplayEffectSpecToTarget(*B.Data, Tasc);
				UDFPassivesGASEvents::DispatchRogueBleedApplied(C, Target, Src);
			}
		}
	}
	if (Pts == 5)
	{
		const FGameplayEffectContextHandle Kx = Src->MakeEffectContext();
		if (Kx.IsValid())
		{
			const FGameplayEffectSpecHandle K = Src->MakeOutgoingSpec(UGE_Buff_Rogue_KillingSpree::StaticClass(), 1.f, Kx);
			if (K.IsValid() && K.Data && FDFGameplayTags::Data_Duration.IsValid() && FDFGameplayTags::Data_Magnitude.IsValid())
			{
				K.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, 5.f);
				K.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Magnitude, Agi * 0.4f);
				Src->ApplyGameplayEffectSpecToSelf(*K.Data);
			}
		}
	}
	if (EviscSlashVFX && GetWorld())
	{
		const float Scale = 0.5f + 0.5f * (static_cast<float>(Pts) / 5.f);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, EviscSlashVFX, Target->GetActorLocation() + FVector(0, 0, 50.f), C->GetActorRotation(), FVector(Scale), true, true, ENCPoolMethod::AutoRelease, true);
	}
}

void UDFAbility_Rogue_Eviscerate::OnMontageEnd()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
