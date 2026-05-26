// Source/DungeonForged/Private/GAS/Abilities/UDFAbility_AirDash.cpp
#include "GAS/Abilities/UDFAbility_AirDash.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Characters/UDFCharacterMovementComponent.h"
#include "Combat/DFDodgeTypes.h"
#include "Combat/DFJumpDebug.h"
#include "DFAssetManager.h"
#include "Data/UDFCombatTuningData.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameFramework/Character.h"
#include "GameFramework/RootMotionSource.h"

UDFAbility_AirDash::UDFAbility_AirDash()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	AbilityCost_Stamina = 15.f;
}

void UDFAbility_AirDash::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Movement_AirDash);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Stunned);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dodging);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Exhausted);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_AirDashing);
	}
}

float UDFAbility_AirDash::GetEffectiveAirDashStaminaCost() const
{
	if (const UDFCombatTuningData* const Tuning = UDFAssetManager::GetCombatTuningDataSafe())
	{
		if (Tuning->AirDashStaminaCost > 0.f)
		{
			return Tuning->AirDashStaminaCost;
		}
	}
	return AbilityCost_Stamina;
}

bool UDFAbility_AirDash::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const ACharacter* const Char = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	const UDFCharacterMovementComponent* const CMC = Char
		? Cast<UDFCharacterMovementComponent>(Char->GetCharacterMovement())
		: nullptr;
	if (!CMC || !CMC->IsFalling())
	{
		return false;
	}
	if (CMC->bAirDodgeUsedThisJump)
	{
		return false;
	}

	const float Cost = GetEffectiveAirDashStaminaCost();
	if (Cost > 0.f && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		if (const UDFAttributeSet* const Attrs = ActorInfo->AbilitySystemComponent->GetSet<UDFAttributeSet>())
		{
			if (Attrs->GetStamina() < Cost)
			{
				return false;
			}
		}
	}
	return true;
}

FVector UDFAbility_AirDash::ResolveAirDashDirectionWorld() const
{
	const FGameplayAbilityActorInfo* const Info = GetCurrentActorInfo();
	const ACharacter* const Char = Info ? Cast<ACharacter>(Info->AvatarActor.Get()) : nullptr;
	if (!Char)
	{
		return FVector::ForwardVector;
	}
	const UDFCharacterMovementComponent* const CMC = Cast<UDFCharacterMovementComponent>(Char->GetCharacterMovement());
	const FVector LocalInput = CMC
		? DFResolveLocalMovementIntent(Char, CMC, 80.f)
		: FVector::ForwardVector;
	const FRotator YawRot(0.f, Char->GetActorRotation().Yaw, 0.f);
	if (LocalInput.SizeSquared2D() < 80.f * 80.f)
	{
		return Char->GetActorForwardVector();
	}
	return YawRot.RotateVector(LocalInput.GetSafeNormal2D());
}

void UDFAbility_AirDash::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
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

	AbilityCost_Stamina = GetEffectiveAirDashStaminaCost();
	ACharacter* const Char = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	UDFCharacterMovementComponent* const CMC = Char ? Cast<UDFCharacterMovementComponent>(Char->GetCharacterMovement()) : nullptr;
	if (!Char || !CMC)
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
		if (FDFGameplayTags::State_AirDashing.IsValid())
		{
			ASC->AddLooseGameplayTag(FDFGameplayTags::State_AirDashing);
		}
		if (bGrantIFrames && FDFGameplayTags::State_Invulnerable.IsValid())
		{
			const float IFrameDur = UDFAssetManager::GetCombatTuningDataSafe()
				? UDFAssetManager::GetCombatTuningDataSafe()->AirDashIFrameDuration
				: 0.15f;
			ASC->AddLooseGameplayTag(FDFGameplayTags::State_Invulnerable);
			FTimerHandle IFrameHandle;
			Char->GetWorldTimerManager().SetTimer(
				IFrameHandle,
				[ASC]()
				{
					if (ASC)
					{
						ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Invulnerable);
					}
				},
				IFrameDur,
				false);
		}
	}

	CMC->NotifyAirDashPerformed();

	const float DashDist = UDFAssetManager::GetCombatTuningDataSafe()
		? UDFAssetManager::GetCombatTuningDataSafe()->AirDashDistance
		: CMC->AirDashDistance;
	const float DashDur = UDFAssetManager::GetCombatTuningDataSafe()
		? UDFAssetManager::GetCombatTuningDataSafe()->AirDashDuration
		: CMC->AirDashDuration;

	const FVector Dir = ResolveAirDashDirectionWorld().GetSafeNormal2D();
	const float SavedGravity = CMC->GravityScale;
	CMC->GravityScale = 0.f;
	(void)SavedGravity;

	if (FRootMotionSource_MoveToForce* const RMS = new FRootMotionSource_MoveToForce())
	{
		RMS->InstanceName = FName(TEXT("DF_AirDash"));
		RMS->AccumulateMode = ERootMotionAccumulateMode::Override;
		RMS->Priority = 250;
		RMS->Duration = DashDur;
		RMS->StartLocation = Char->GetActorLocation();
		RMS->TargetLocation = Char->GetActorLocation() + Dir * DashDist;
		RMS->bRestrictSpeedToExpected = true;
		CMC->ApplyRootMotionSource(TSharedPtr<FRootMotionSource>(RMS));
	}

	if (AirDashMontage)
	{
		UAbilityTask_PlayMontageAndWait* const MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, AirDashMontage, 1.f, NAME_None, false, 1.f);
		MontageTask->OnCompleted.AddDynamic(this, &UDFAbility_AirDash::OnAirDashFinished);
		MontageTask->OnInterrupted.AddDynamic(this, &UDFAbility_AirDash::OnAirDashFinished);
		MontageTask->OnCancelled.AddDynamic(this, &UDFAbility_AirDash::OnAirDashFinished);
		MontageTask->ReadyForActivation();
	}

	UAbilityTask_WaitDelay* const Wait = UAbilityTask_WaitDelay::WaitDelay(this, DashDur);
	Wait->OnFinish.AddDynamic(this, &UDFAbility_AirDash::OnAirDashFinished);
	Wait->ReadyForActivation();

	CMC->Velocity = Dir * (DashDist / FMath::Max(DashDur, 0.01f));
	CMC->Velocity.Z = FMath::Min(CMC->Velocity.Z, 0.f);

	DFJumpDebug::Logf(TEXT("AirDash dir=%s dist=%.0f dur=%.2fs"), *Dir.ToCompactString(), DashDist, DashDur);
}

void UDFAbility_AirDash::OnAirDashFinished()
{
	if (IsActive())
	{
		if (ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			if (UDFCharacterMovementComponent* const CMC = Cast<UDFCharacterMovementComponent>(Char->GetCharacterMovement()))
			{
				CMC->GravityScale = CMC->IsFalling() && CMC->Velocity.Z < 0.f
					? CMC->DFGravityScale * CMC->DFFallGravityMultiplier
					: CMC->DFGravityScale;
				CMC->RemoveRootMotionSource(FName(TEXT("DF_AirDash")));
			}
		}
	}
	if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (FDFGameplayTags::State_AirDashing.IsValid())
		{
			ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_AirDashing);
		}
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
