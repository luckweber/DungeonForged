// Source/DungeonForged/Private/GAS/Abilities/Warrior/UDFAbility_Warrior_MeleeSwing.cpp
#include "GAS/Abilities/Warrior/UDFAbility_Warrior_MeleeSwing.h"

#include "Abilities/GameplayAbility.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Combat/UDFComboComponent.h"
#include "Combat/UDFMeleeAimComponent.h"
#include "Combat/DFAnimCombatLibrary.h"
#include "Animation/AnimInstance.h"
#include "Combat/UDFMeleeTraceComponent.h"
#include "GameFramework/Actor.h"
#include "GAS/DFGameplayTags.h"
#include "Combat/DFCombatDebug.h"
#include "DungeonForgedModule.h"

bool UDFAbility_Warrior_MeleeSwing::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	bool bSkipCooldownForComboChain = false;
	if (ActorInfo)
	{
		if (const ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(ActorInfo->AvatarActor.Get()))
		{
			if (const UDFComboComponent* const Combo = PC->Combo)
			{
				bSkipCooldownForComboChain = Combo->CurrentComboStep > 0
					|| Combo->LockedComboActivationStep > 0
					|| Combo->PendingComboActivationStep > 0
					|| Combo->bComboWindowActive
					|| Combo->bComboInputBuffered
					|| Combo->bComboChainAdvancePending;
			}
		}
	}
	if (!bSkipCooldownForComboChain)
	{
		return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
	}
	const float SavedCooldown = BaseCooldown;
	const_cast<UDFAbility_Warrior_MeleeSwing*>(this)->BaseCooldown = 0.f;
	const bool bCan = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
	const_cast<UDFAbility_Warrior_MeleeSwing*>(this)->BaseCooldown = SavedCooldown;
	return bCan;
}

UDFAbility_Warrior_MeleeSwing::UDFAbility_Warrior_MeleeSwing()
{
	bRetriggerInstancedAbility = true;
	AbilityCost_Mana = 0.f;
	AbilityCost_Stamina = 6.f;
	BaseCooldown = 0.f;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UDFAbility_Warrior_MeleeSwing::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Warrior_MeleeSwing);
		AbilityTags.AddTag(FDFGameplayTags::Ability_Attack);
		AbilityTags.AddTag(FDFGameplayTags::Ability_Attack_Melee);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Stunned);
		ActivationOwnedTags.AddTag(FDFGameplayTags::State_Attacking);
		if (BaseCooldown > 0.f)
		{
			bRetriggerInstancedAbility = false;
			BuildCooldownTagContainer(CachedCooldownTags);
			ActivationBlockedTags.AppendTags(CachedCooldownTags);
		}
	}
}

void UDFAbility_Warrior_MeleeSwing::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
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

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (ASC->GetOwner() && ASC->GetOwner()->HasAuthority())
		{
			ApplyResourceCostsToOwner(ASC);
		}
	}

	AActor* Avatar = ActorInfo->AvatarActor.Get();
	UDFComboComponent* Combo = nullptr;
	UAnimMontage* MontToPlay = nullptr;
	if (ADFPlayerCharacter* PC = Cast<ADFPlayerCharacter>(Avatar))
	{
		PushSwingCosmeticsToMeleeTrace(PC);
		Combo = PC->Combo;
		// AAA aim: snap-rotate to face the resolved target (lock-on > soft cone) and commit it so
		// UANS_DFMeleeWarp notify states warp to the same actor for the duration of the swing.
		if (UDFMeleeAimComponent* const Aim = PC->MeleeAim)
		{
			Aim->AcquireAndCommitTarget();
		}
	}
	int32 ComboStep = 0;
	int32 LockedStepForLog = -1;
	if (Combo)
	{
		LockedStepForLog = Combo->LockedComboActivationStep;
		ComboStep = Combo->ResolveComboStepForActivation();
		MontToPlay = Combo->ResolveDirectionalComboMontage(ComboStep);
	}
	if (!MontToPlay)
	{
		MontToPlay = AbilityMontage.Get();
	}

	if (!MontToPlay || !Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

#if !UE_BUILD_SHIPPING
	if (Combo && DFCombatDebug::IsChannelEnabled(DFCombatDebug::EChannel::Combo))
	{
		UE_LOG(LogDungeonForged, Log, TEXT("[Combo|GAS] Activate step=%d locked=%d montage=%s (fallback=%s)"),
			ComboStep,
			LockedStepForLog,
			*MontToPlay->GetName(),
			MontToPlay == AbilityMontage.Get() ? TEXT("yes") : TEXT("no"));
	}
#endif

	if (Combo)
	{
		Combo->ClearLockedComboStepAfterActivation(ComboStep);
	}

	if (Combo)
	{
		Combo->NotifyAbilitySwingMontageStarted(MontToPlay);
	}

	const bool bChainSwing = ComboStep > 0;
	const float ChainBlendIn = Combo ? Combo->ResolveChainBlendInForStep(ComboStep) : 0.08f;

	UAnimInstance* AnimInst = nullptr;
	if (ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		AnimInst = Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr;
	}

	if (bChainSwing && AnimInst && MontToPlay)
	{
		bPlayedMontageDirect = true;
		// bStopAllMontages=false: PrepareForComboChainActivation already stopped the previous swing with the
		// configured blend-out time. Letting StopAllMontages fire here would override that with the asset's
		// default blend-out and kill the cross-fade.
		const float Len = UDFAnimCombatLibrary::PlayMontageWithBlendIn(AnimInst, MontToPlay, 1.f, ChainBlendIn, false);
#if !UE_BUILD_SHIPPING
		if (Combo)
		{
			Combo->RecordChainMontageBlendIn(ChainBlendIn, MontToPlay);
		}
#endif
		if (Len <= 0.f)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UDFAbility_Warrior_MeleeSwing::OnDirectMontageEnded);
		AnimInst->Montage_SetEndDelegate(EndDelegate, MontToPlay);
	}
	else
	{
		UAbilityTask_PlayMontageAndWait* PlayTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, MontToPlay, 1.f, NAME_None, true, 1.f, 0.f, true);
		if (!PlayTask)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		PlayTask->OnCompleted.AddDynamic(this, &UDFAbility_Warrior_MeleeSwing::OnMontageEnd);
		PlayTask->OnBlendOut.AddDynamic(this, &UDFAbility_Warrior_MeleeSwing::OnMontageEnd);
		PlayTask->OnInterrupted.AddDynamic(this, &UDFAbility_Warrior_MeleeSwing::OnMontageEnd);
		PlayTask->OnCancelled.AddDynamic(this, &UDFAbility_Warrior_MeleeSwing::OnMontageEnd);
		PlayTask->ReadyForActivation();
	}
}

void UDFAbility_Warrior_MeleeSwing::OnDirectMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	(void)Montage;
	(void)bInterrupted;
	OnMontageEnd();
}

void UDFAbility_Warrior_MeleeSwing::OnMontageEnd()
{
	if (ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (PC->MeleeTrace)
		{
			PC->MeleeTrace->EndTrace();
		}
		if (UDFComboComponent* const Combo = PC->Combo)
		{
			Combo->NotifyAbilitySwingMontagePlaybackEnded();
			if (Combo->LockedComboActivationStep < 0
				&& Combo->PendingComboActivationStep < 0
				&& !Combo->bComboChainAdvancePending
				&& !Combo->bComboWindowActive
				&& !Combo->bComboInputBuffered)
			{
				Combo->ResetCombo();
			}
		}
		if (UDFMeleeAimComponent* const Aim = PC->MeleeAim)
		{
			Aim->ReleaseAttackTarget();
		}
	}
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UDFAbility_Warrior_MeleeSwing::PushSwingCosmeticsToMeleeTrace(ADFPlayerCharacter* const Player) const
{
	if (!Player || !Player->MeleeTrace)
	{
		return;
	}
	if (!Player->HasAuthority())
	{
		return;
	}

	FDFMeleeTraceCosmetics Cosmetics = Player->MeleeTrace->DefaultCosmetics;
	if (SwingSound)
	{
		Cosmetics.SwingSound = SwingSound;
	}
	if (SwingVFX)
	{
		Cosmetics.SwingVFX = SwingVFX;
	}
	if (!SwingFXSocketName.IsNone())
	{
		Cosmetics.SwingFXSocketName = SwingFXSocketName;
	}
	Cosmetics.SwingVFXScale = SwingVFXScale;
	if (ImpactSound)
	{
		Cosmetics.ImpactSound = ImpactSound;
	}
	if (ImpactVFX)
	{
		Cosmetics.ImpactVFX = ImpactVFX;
	}
	Cosmetics.ImpactVFXScale = ImpactVFXScale;
	Player->MeleeTrace->SetSwingCosmetics(Cosmetics);
}
