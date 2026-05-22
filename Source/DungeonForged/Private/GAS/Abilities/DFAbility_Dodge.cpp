// Source/DungeonForged/Private/GAS/Abilities/DFAbility_Dodge.cpp
#include "GAS/Abilities/DFAbility_Dodge.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Characters/UDFCharacterMovementComponent.h"
#include "Combat/DFDodgeDebug.h"
#include "Combat/UDFComboComponent.h"
#include "Engine/Engine.h"
#include "DFAssetManager.h"
#include "Data/UDFCombatTuningData.h"
#include "Equipment/DFEquipmentTypes.h"
#include "Equipment/UDFEquipmentComponent.h"
#include "FX/UDFScreenEffectsComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameFramework/Character.h"

UDFAbility_Dodge::UDFAbility_Dodge()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	AbilityCost_Mana = 0.f;
	AbilityCost_Stamina = 20.f;
	AbilityMontage = nullptr;
}

void UDFAbility_Dodge::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Movement_Dodge);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Stunned);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dodging);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Exhausted);
		CancelAbilitiesWithTag.AddTag(FDFGameplayTags::Ability_Attack_Melee);
	}
}

bool UDFAbility_Dodge::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		if (DFDodgeDebug::IsLogEnabled() && OptionalRelevantTags && !OptionalRelevantTags->IsEmpty())
		{
			DFDodgeDebug::Logf(TEXT("CanActivate FAIL (GAS) tags=%s"), *OptionalRelevantTags->ToStringSimple());
		}
		return false;
	}
	const float StaminaCost = GetEffectiveDodgeStaminaCost();
	if (StaminaCost > 0.f && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		if (const UDFAttributeSet* const Attrs = ActorInfo->AbilitySystemComponent->GetSet<UDFAttributeSet>())
		{
			if (Attrs->GetStamina() < StaminaCost)
			{
				DFDodgeDebug::Logf(TEXT("CanActivate FAIL stamina=%.1f need=%.1f"),
					Attrs->GetStamina(), StaminaCost);
				return false;
			}
		}
	}
	return true;
}

float UDFAbility_Dodge::GetEffectiveDodgeStaminaCost() const
{
	if (const UDFCombatTuningData* const Tuning = UDFAssetManager::GetCombatTuningDataSafe())
	{
		if (Tuning->DodgeStaminaCost > 0.f)
		{
			return Tuning->DodgeStaminaCost;
		}
	}
	return AbilityCost_Stamina;
}

bool UDFAbility_Dodge::IsOwnerArmed() const
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

EDFDodgeDirection UDFAbility_Dodge::ResolveDodgeDirection() const
{
	const FGameplayAbilityActorInfo* const Info = GetCurrentActorInfo();
	if (!Info)
	{
		return EDFDodgeDirection::Backward;
	}
	const ACharacter* const Char = Cast<ACharacter>(Info->AvatarActor.Get());
	if (!Char)
	{
		return EDFDodgeDirection::Backward;
	}
	const UDFCharacterMovementComponent* const CMC = Cast<UDFCharacterMovementComponent>(Char->GetCharacterMovement());
	if (!CMC)
	{
		return EDFDodgeDirection::Backward;
	}

	const FVector LocalInput = DFResolveLocalMovementIntent(Char, CMC, DirectionalInputThreshold);
	const float ThresholdSq = DirectionalInputThreshold * DirectionalInputThreshold;
	if (LocalInput.SizeSquared2D() < ThresholdSq)
	{
		DFDodgeDebug::Logf(TEXT("ResolveDir fallback Backward (weak input vel2D=%.1f)"),
			Char->GetVelocity().Size2D());
		return EDFDodgeDirection::Backward;
	}

	const EDFDodgeDirection Dir = DFSnapLocalInputToDodgeDirection(LocalInput.GetSafeNormal2D());
	DFDodgeDebug::Logf(TEXT("ResolveDir %s local=(%.2f,%.2f) lastInputWorld=%s"),
		DFDodgeDebug::DirectionName(Dir), LocalInput.X, LocalInput.Y,
		*CMC->GetLastInputVector().ToCompactString());
	return Dir;
}

FVector UDFAbility_Dodge::ResolveDodgeDirectionWorld() const
{
	const FGameplayAbilityActorInfo* const Info = GetCurrentActorInfo();
	if (!Info)
	{
		return FVector::BackwardVector;
	}
	const ACharacter* const Char = Cast<ACharacter>(Info->AvatarActor.Get());
	if (!Char)
	{
		return FVector::BackwardVector;
	}
	const FRotator YawRot(0.f, Char->GetActorRotation().Yaw, 0.f);
	return YawRot.RotateVector(DFGetDodgeDirectionLocalVector(ResolveDodgeDirection()));
}

UAnimMontage* UDFAbility_Dodge::ResolveDodgeMontage(const EDFDodgeDirection Direction) const
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
	return DodgeMontage.Get();
}

void UDFAbility_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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
		DFDodgeDebug::Log(TEXT("Activate FAIL CommitAbility"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AbilityCost_Stamina = GetEffectiveDodgeStaminaCost();

	ACharacter* const Char = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	UDFCharacterMovementComponent* const CMC = Char ? Cast<UDFCharacterMovementComponent>(Char->GetCharacterMovement()) : nullptr;
	if (!CMC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const EDFDodgeDirection Direction = ResolveDodgeDirection();
	CMC->LastDodgeDirection = Direction;
	UAnimMontage* const PickedMontage = ResolveDodgeMontage(Direction);
	const FVector DodgeDirWorld = ResolveDodgeDirectionWorld();
	const bool bArmed = IsOwnerArmed();

	DFDodgeDebug::Logf(TEXT("Activate dir=%s armed=%d staminaCost=%.1f montage=%s cdRem=%.2f"),
		DFDodgeDebug::DirectionName(Direction),
		bArmed ? 1 : 0,
		AbilityCost_Stamina,
		PickedMontage ? *PickedMontage->GetName() : TEXT("(none)"),
		CMC->GetDodgeCooldownRemaining());

	if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (ASC->GetOwner() && ASC->GetOwner()->HasAuthority())
		{
			ApplyResourceCostsToOwner(ASC);
		}
	}

	const bool bMontageHasRootMotion = PickedMontage && DFMontageHasRootMotion(PickedMontage);
	const bool bUseAnimRootMotion = bPreferAnimRootMotion && bMontageHasRootMotion;
	const bool bUseProgrammaticDisplacement = !bUseAnimRootMotion;

	if (bRotateToDodgeDirection && !DodgeDirWorld.IsNearlyZero())
	{
		FRotator FaceRot = DodgeDirWorld.GetSafeNormal().Rotation();
		FaceRot.Pitch = 0.f;
		FaceRot.Roll = 0.f;
		Char->SetActorRotation(FaceRot);
	}

	CMC->PerformDodge(DodgeDirWorld, bUseProgrammaticDisplacement);

	DFDodgeDebug::Logf(TEXT("RootMotion montageRM=%d useAnimRM=%d programmatic=%d"),
		bMontageHasRootMotion ? 1 : 0, bUseAnimRootMotion ? 1 : 0, bUseProgrammaticDisplacement ? 1 : 0);

	if (DFDodgeDebug::IsDrawEnabled() && Char)
	{
		const FVector Start = Char->GetActorLocation();
		DFDodgeDebug::DrawDodgeArrow(Char->GetWorld(), Start, DodgeDirWorld, CMC->DodgeDistance, FColor::Cyan, CMC->DodgeDuration);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, CMC->DodgeDuration, FColor::Cyan,
				FString::Printf(TEXT("Dodge %s (%s)"), DFDodgeDebug::DirectionName(Direction), bArmed ? TEXT("armed") : TEXT("unarmed")));
		}
	}

	float D = FMath::Max(0.01f, CMC->DodgeDuration);
	if (bUseAnimRootMotion && PickedMontage)
	{
		D = FMath::Max(0.01f, PickedMontage->GetPlayLength());
	}

	if (ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(Char))
	{
		if (UDFComboComponent* const Combo = PC->Combo)
		{
			Combo->ResetCombo();
		}
		if (PC->IsLocallyControlled() && PC->ScreenEffects)
		{
			PC->ScreenEffects->ApplyDodgeJuice(D);
		}
	}

	if (PickedMontage)
	{
		if (UAbilityTask_PlayMontageAndWait* const MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, NAME_None, PickedMontage, 1.f, NAME_None, true, 1.f, 0.f, true))
		{
			MontageTask->OnCompleted.AddDynamic(this, &UDFAbility_Dodge::OnDodgeMontageCompleted);
			MontageTask->OnInterrupted.AddDynamic(this, &UDFAbility_Dodge::OnDodgeMontageCancelled);
			MontageTask->OnCancelled.AddDynamic(this, &UDFAbility_Dodge::OnDodgeMontageCancelled);
			MontageTask->ReadyForActivation();
		}
	}

	if (UAbilityTask_WaitDelay* const Wait = UAbilityTask_WaitDelay::WaitDelay(this, D))
	{
		Wait->OnFinish.AddDynamic(this, &UDFAbility_Dodge::OnDodgeDurationElapsed);
		Wait->ReadyForActivation();
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UDFAbility_Dodge::OnDodgeDurationElapsed()
{
	DFDodgeDebug::Log(TEXT("EndAbility (duration elapsed)"));
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UDFAbility_Dodge::OnDodgeMontageCompleted()
{
}

void UDFAbility_Dodge::OnDodgeMontageCancelled()
{
}
