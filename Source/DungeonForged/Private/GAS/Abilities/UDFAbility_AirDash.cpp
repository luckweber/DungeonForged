// Source/DungeonForged/Private/GAS/Abilities/UDFAbility_AirDash.cpp
#include "GAS/Abilities/UDFAbility_AirDash.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/UDFAnimInstance.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Characters/UDFCharacterMovementComponent.h"
#include "Combat/DFAirDashDebug.h"
#include "Combat/DFDodgeDebug.h"
#include "Combat/DFDodgeTypes.h"
#include "DFAssetManager.h"
#include "Data/UDFCombatTuningData.h"
#include "Equipment/DFEquipmentTypes.h"
#include "Equipment/UDFEquipmentComponent.h"
#include "Engine/Engine.h"
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

bool UDFAbility_AirDash::IsOwnerArmed() const
{
	const FGameplayAbilityActorInfo* const Info = GetCurrentActorInfo();
	if (!Info)
	{
		return false;
	}
	const ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(Info->AvatarActor.Get());
	if (!PC || !PC->Equipment)
	{
		return false;
	}
	return !PC->Equipment->IsSlotEmpty(EEquipmentSlot::Weapon);
}

EDFDodgeDirection UDFAbility_AirDash::ResolveAirDashDirection() const
{
	const FGameplayAbilityActorInfo* const Info = GetCurrentActorInfo();
	if (!Info)
	{
		return EDFDodgeDirection::Forward;
	}
	const ACharacter* const Char = Cast<ACharacter>(Info->AvatarActor.Get());
	if (!Char)
	{
		return EDFDodgeDirection::Forward;
	}
	const UDFCharacterMovementComponent* const CMC = Cast<UDFCharacterMovementComponent>(Char->GetCharacterMovement());
	if (!CMC)
	{
		return EDFDodgeDirection::Forward;
	}

	const FVector LocalInput = DFResolveLocalMovementIntent(Char, CMC, DirectionalInputThreshold);
	const float ThresholdSq = DirectionalInputThreshold * DirectionalInputThreshold;
	if (LocalInput.SizeSquared2D() < ThresholdSq)
	{
		DFAirDashDebug::Logf(TEXT("ResolveDir fallback Forward (weak input vel2D=%.1f)"),
			Char->GetVelocity().Size2D());
		return EDFDodgeDirection::Forward;
	}

	const EDFDodgeDirection Dir = DFSnapLocalInputToDodgeDirection(LocalInput.GetSafeNormal2D());
	DFAirDashDebug::Logf(TEXT("ResolveDir %s local=(%.2f,%.2f) lastInputWorld=%s"),
		DFDodgeDebug::DirectionName(Dir), LocalInput.X, LocalInput.Y,
		*CMC->GetLastInputVector().ToCompactString());
	return Dir;
}

FVector UDFAbility_AirDash::ResolveAirDashDirectionWorld() const
{
	const FGameplayAbilityActorInfo* const Info = GetCurrentActorInfo();
	if (!Info)
	{
		return FVector::ForwardVector;
	}
	const ACharacter* const Char = Cast<ACharacter>(Info->AvatarActor.Get());
	if (!Char)
	{
		return FVector::ForwardVector;
	}
	const FRotator YawRot(0.f, Char->GetActorRotation().Yaw, 0.f);
	return YawRot.RotateVector(DFGetDodgeDirectionLocalVector(ResolveAirDashDirection()));
}

UAnimMontage* UDFAbility_AirDash::ResolveAirDashMontage(const EDFDodgeDirection Direction) const
{
	const bool bArmed = IsOwnerArmed();
	const FDFDodgeAnimSet& Primary = bArmed ? ArmedAnimSet : UnarmedAnimSet;
	const FDFDodgeAnimSet& Secondary = bArmed ? UnarmedAnimSet : ArmedAnimSet;

	TArray<EDFDodgeDirection> TryOrder;
	DFGetDodgeDirectionResolveOrder(Direction, TryOrder);

	for (const EDFDodgeDirection Dir : TryOrder)
	{
		if (UAnimMontage* const M = Primary.Resolve(Dir))
		{
			return M;
		}
	}
	for (const EDFDodgeDirection Dir : TryOrder)
	{
		if (UAnimMontage* const M = Secondary.Resolve(Dir))
		{
			return M;
		}
	}
	return AirDashMontage.Get();
}

float UDFAbility_AirDash::GetAbilityStaminaCost() const
{
	return GetEffectiveAirDashStaminaCost();
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

float UDFAbility_AirDash::GetActiveMontagePosition() const
{
	const ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Char || !ActiveAirDashMontage)
	{
		return 0.f;
	}
	if (UAnimInstance* const Anim = Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr)
	{
		return Anim->Montage_GetPosition(ActiveAirDashMontage);
	}
	return 0.f;
}

float UDFAbility_AirDash::GetActiveMontageLength() const
{
	return ActiveAirDashMontage ? ActiveAirDashMontage->GetPlayLength() : 0.f;
}

void UDFAbility_AirDash::ApplyProgrammaticAirDashDisplacement(ACharacter* const Char,
	UDFCharacterMovementComponent* const CMC, const FVector& DashDirWorld, const float DashDist,
	const float DashDur) const
{
	if (!Char || !CMC)
	{
		return;
	}

	if (FRootMotionSource_MoveToForce* const RMS = new FRootMotionSource_MoveToForce())
	{
		RMS->InstanceName = FName(TEXT("DF_AirDash"));
		RMS->AccumulateMode = ERootMotionAccumulateMode::Override;
		RMS->Priority = 250;
		RMS->Duration = DashDur;
		RMS->StartLocation = Char->GetActorLocation();
		RMS->TargetLocation = Char->GetActorLocation() + DashDirWorld * DashDist;
		RMS->bRestrictSpeedToExpected = true;
		CMC->ApplyRootMotionSource(TSharedPtr<FRootMotionSource>(RMS));
	}

	CMC->Velocity = DashDirWorld * (DashDist / FMath::Max(DashDur, 0.01f));
	CMC->Velocity.Z = 0.f;
}

void UDFAbility_AirDash::VerifyMontagePlayback(ACharacter* const Char, UAnimMontage* const Montage,
	const bool bWantsAnimRootMotion, const FVector& DashDirWorld, const float DashDist, const float DashDur)
{
	if (!IsActive() || !Char || !Montage)
	{
		return;
	}

	UAnimInstance* const Anim = Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr;
	const bool bIsPlaying = Anim && (Anim->Montage_IsPlaying(Montage) || Anim->IsAnyMontagePlaying());
	const float Pos = Anim && Anim->Montage_IsPlaying(Montage) ? Anim->Montage_GetPosition(Montage) : 0.f;
	DFAirDashDebug::DumpMontagePlayback(Montage, Montage->GetPlayLength(), bIsPlaying, Pos);

	if (bIsPlaying)
	{
		return;
	}

	DFAirDashDebug::Logf(
		TEXT("Montage NOT playing — open '%s' and set Slot to '%s' (must match AnimBP slot node). See dodge montages for reference."),
		*Montage->GetName(),
		*MontageSlotName.ToString());

	const UDFCharacterMovementComponent* const CMC = Cast<UDFCharacterMovementComponent>(Char->GetCharacterMovement());
	if (!bFallbackToProgrammaticIfMontageFails || bProgrammaticFallbackApplied || (CMC && CMC->IsAirDashDriveActive()))
	{
		return;
	}

	if (UDFCharacterMovementComponent* const MutableCMC = const_cast<UDFCharacterMovementComponent*>(CMC))
	{
		bProgrammaticFallbackApplied = true;
		DFAirDashDebug::Log(TEXT("Applying programmatic fallback displacement"));
		ApplyProgrammaticAirDashDisplacement(Char, MutableCMC, DashDirWorld, DashDist, DashDur);
		(void)bWantsAnimRootMotion;
	}
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
		DFAirDashDebug::DumpCanActivateFail(CMC, TEXT("not falling"));
		return false;
	}
	if (CMC->bAirDodgeUsedThisJump)
	{
		DFAirDashDebug::DumpCanActivateFail(CMC, TEXT("already used this jump"));
		return false;
	}
	if (CMC->GetAirDashCooldownRemaining() > 0.f)
	{
		DFAirDashDebug::DumpCanActivateFail(CMC, TEXT("cooldown"));
		return false;
	}

	const float Cost = GetEffectiveAirDashStaminaCost();
	if (Cost > 0.f && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		if (const UDFAttributeSet* const Attrs = ActorInfo->AbilitySystemComponent->GetSet<UDFAttributeSet>())
		{
			if (Attrs->GetStamina() < Cost)
			{
				DFAirDashDebug::DumpCanActivateFail(CMC, TEXT("stamina"));
				return false;
			}
		}
	}
	return true;
}

void UDFAbility_AirDash::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	(void)TriggerEventData;
	bAirDashFinishRequested = false;
	ActiveAirDashMontage = nullptr;
	bProgrammaticFallbackApplied = false;

	if (!ActorInfo)
	{
		EndAbility(Handle, nullptr, ActivationInfo, true, true);
		return;
	}
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo, nullptr))
	{
		DFAirDashDebug::Log(TEXT("Activate FAIL CommitAbility"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACharacter* const Char = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	UDFCharacterMovementComponent* const CMC = Char ? Cast<UDFCharacterMovementComponent>(Char->GetCharacterMovement()) : nullptr;
	if (!Char || !CMC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const EDFDodgeDirection Direction = ResolveAirDashDirection();
	CMC->LastDodgeDirection = Direction;
	UAnimMontage* const PickedMontage = ResolveAirDashMontage(Direction);
	ActiveAirDashMontage = PickedMontage;
	const FVector DashDirWorld = ResolveAirDashDirectionWorld().GetSafeNormal2D();
	const bool bArmed = IsOwnerArmed();

	if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponentFromActorInfo())
	{
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
	const float DriveDur = UDFAssetManager::GetCombatTuningDataSafe()
		? UDFAssetManager::GetCombatTuningDataSafe()->AirDashDuration
		: CMC->AirDashDuration;
	float AbilityDur = DriveDur;

	const bool bMontageHasRootMotion = PickedMontage && DFMontageHasRootMotion(PickedMontage);
	const bool bUseAnimRootMotion = bPreferAnimRootMotion && bMontageHasRootMotion;
	const bool bUseProgrammaticDisplacement = !bUseAnimRootMotion;

	const float LockedZ = Char->GetActorLocation().Z;
	if (PickedMontage)
	{
		AbilityDur = FMath::Max(DriveDur, PickedMontage->GetPlayLength());
		DFAirDashDebug::DumpMontageSlot(PickedMontage, MontageSlotName);
	}

	CMC->BeginAirDashDrive(DashDirWorld, DashDist, DriveDur, LockedZ);
	DFAirDashDebug::Logf(TEXT("Timing drive=%.2fs ability=%.2fs dist=%.0f speed=%.0f"),
		DriveDur, AbilityDur, DashDist, DashDist / FMath::Max(DriveDur, 0.01f));

	if (bSuppressAnimRootMotionDuringDash)
	{
		SavedAnimRootMotionTranslationScale = Char->GetAnimRootMotionTranslationScale();
		Char->SetAnimRootMotionTranslationScale(0.f);
		if (UAnimInstance* const Anim = Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr)
		{
			SavedAnimRootMotionMode = static_cast<uint8>(Anim->RootMotionMode);
			bRestoredAnimRootMotionMode = false;
			Anim->RootMotionMode = ERootMotionMode::IgnoreRootMotion;
		}
	}

	bool bSkipRotateForLockOn = false;
	if (const UDFCombatTuningData* const Tuning = UDFAssetManager::GetCombatTuningDataSafe())
	{
		bSkipRotateForLockOn = Tuning->bDodgeKeepFacingTargetOnLockOn;
	}
	bool bIsLockedOn = false;
	if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponentFromActorInfo())
	{
		bIsLockedOn = ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Targeting);
	}
	if (bRotateToDashDirection && !(bSkipRotateForLockOn && bIsLockedOn) && !DashDirWorld.IsNearlyZero())
	{
		FRotator FaceRot = DashDirWorld.GetSafeNormal().Rotation();
		FaceRot.Pitch = 0.f;
		FaceRot.Roll = 0.f;
		Char->SetActorRotation(FaceRot);
	}

	DFAirDashDebug::DumpActivate(Char, CMC, Direction, PickedMontage, DashDirWorld, DashDist, AbilityDur,
		bMontageHasRootMotion, bUseAnimRootMotion, true, bLockAltitudeDuringDash);

	if (DFAirDashDebug::IsDrawEnabled())
	{
		DFAirDashDebug::DrawPlanarArrow(Char->GetWorld(), Char->GetActorLocation(), DashDirWorld, DashDist,
			FColor::Orange, AbilityDur);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, AbilityDur, FColor::Orange,
				FString::Printf(TEXT("AirDash %s (%s) drive=%.2fs anim=%.2fs"),
					DFDodgeDebug::DirectionName(Direction),
					bArmed ? TEXT("armed") : TEXT("unarmed"),
					DriveDur,
					AbilityDur));
		}
	}

	if (PickedMontage)
	{
		UAnimInstance* const Anim = Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr;
		const FName AssetSlot = DFGetMontagePrimarySlotName(PickedMontage);
		bool bUsedDynamicSlotMontage = false;
		if (Anim && MontageSlotName != NAME_None && AssetSlot != MontageSlotName)
		{
			if (UAnimSequenceBase* const Seq = DFGetPrimaryMontageSequence(PickedMontage))
			{
				if (UAnimMontage* const DynamicMontage = Anim->PlaySlotAnimationAsDynamicMontage(
						Seq, MontageSlotName, 0.05f, 0.08f, 1.f, 1, -1.f, 0.f))
				{
					bUsedDynamicSlotMontage = true;
					DFAirDashDebug::Logf(
						TEXT("Dynamic montage slot='%s' seq='%s' len=%.2f (asset slot='%s')"),
						*MontageSlotName.ToString(),
						*Seq->GetName(),
						DynamicMontage->GetPlayLength(),
						*AssetSlot.ToString());
				}
			}
		}

		if (!bUsedDynamicSlotMontage)
		{
			if (UAbilityTask_PlayMontageAndWait* const MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
					this, NAME_None, PickedMontage, 1.f, NAME_None, true, 1.f, 0.f, true))
			{
				MontageTask->OnCompleted.AddDynamic(this, &UDFAbility_AirDash::OnAirDashMontageCompleted);
				MontageTask->OnInterrupted.AddDynamic(this, &UDFAbility_AirDash::OnAirDashMontageInterrupted);
				MontageTask->OnCancelled.AddDynamic(this, &UDFAbility_AirDash::OnAirDashMontageCancelled);
				MontageTask->ReadyForActivation();
			}
		}

		FTimerHandle MontageVerifyHandle;
		TWeakObjectPtr<UDFAbility_AirDash> WeakThis(this);
		TWeakObjectPtr<ACharacter> WeakChar(Char);
		TWeakObjectPtr<UAnimMontage> WeakMontage(PickedMontage);
		const float VerifyDashDist = DashDist;
		const float VerifyDashDur = AbilityDur;
		const FVector VerifyDir = DashDirWorld;
		const bool bVerifyAnimRM = bUseAnimRootMotion;
		Char->GetWorldTimerManager().SetTimer(
			MontageVerifyHandle,
			[WeakThis, WeakChar, WeakMontage, bVerifyAnimRM, VerifyDir, VerifyDashDist, VerifyDashDur]()
			{
				if (UDFAbility_AirDash* const Self = WeakThis.Get())
				{
					Self->VerifyMontagePlayback(
						WeakChar.Get(), WeakMontage.Get(), bVerifyAnimRM, VerifyDir, VerifyDashDist, VerifyDashDur);
				}
			},
			0.05f,
			false);
	}

	if (UAbilityTask_WaitDelay* const Wait = UAbilityTask_WaitDelay::WaitDelay(this, AbilityDur))
	{
		Wait->OnFinish.AddDynamic(this, &UDFAbility_AirDash::OnAirDashFinished);
		Wait->ReadyForActivation();
	}
	else
	{
		FinishAirDash(TEXT("WaitTaskFailed"));
	}
}

void UDFAbility_AirDash::FinishAirDash(const TCHAR* const Reason)
{
	if (bAirDashFinishRequested || !IsActive())
	{
		return;
	}
	bAirDashFinishRequested = true;

	ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UDFCharacterMovementComponent* const CMC = Char ? Cast<UDFCharacterMovementComponent>(Char->GetCharacterMovement()) : nullptr;
	DFAirDashDebug::DumpFinish(Reason, Char, CMC, GetActiveMontagePosition(), GetActiveMontageLength());

	if (Char && CMC)
	{
		if (CMC->IsFalling())
		{
			if (UAnimInstance* const Anim = Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr)
			{
				if (UUDFAnimInstance* const DFAnim = Cast<UUDFAnimInstance>(Anim))
				{
					DFAnim->NotifyAirDashEndedWhileAirborne();
				}
				if (MontageSlotName != NAME_None)
				{
					Anim->StopSlotAnimation(0.08f, MontageSlotName);
				}
			}
		}

		CMC->EndAirDashDrive(CMC->AirDashExitVelocityRetain);
		CMC->RemoveRootMotionSource(FName(TEXT("DF_AirDash")));
		if (bSuppressAnimRootMotionDuringDash)
		{
			Char->SetAnimRootMotionTranslationScale(SavedAnimRootMotionTranslationScale);
			if (UAnimInstance* const Anim = Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr)
			{
				Anim->RootMotionMode = static_cast<ERootMotionMode::Type>(SavedAnimRootMotionMode);
				bRestoredAnimRootMotionMode = true;
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
	ActiveAirDashMontage = nullptr;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UDFAbility_AirDash::OnAirDashFinished()
{
	FinishAirDash(TEXT("WaitDelay"));
}

void UDFAbility_AirDash::OnAirDashMontageCompleted()
{
	ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAnimInstance* const Anim = Char && Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr;
	const float Pos = Anim && ActiveAirDashMontage ? Anim->Montage_GetPosition(ActiveAirDashMontage) : 0.f;
	DFAirDashDebug::DumpMontageEvent(TEXT("Completed"), ActiveAirDashMontage, Pos, GetActiveMontageLength());
}

void UDFAbility_AirDash::OnAirDashMontageInterrupted()
{
	DFAirDashDebug::DumpMontageEvent(TEXT("Interrupted"), ActiveAirDashMontage, GetActiveMontagePosition(),
		GetActiveMontageLength());
}

void UDFAbility_AirDash::OnAirDashMontageCancelled()
{
	DFAirDashDebug::DumpMontageEvent(TEXT("Cancelled"), ActiveAirDashMontage, GetActiveMontagePosition(),
		GetActiveMontageLength());
}
