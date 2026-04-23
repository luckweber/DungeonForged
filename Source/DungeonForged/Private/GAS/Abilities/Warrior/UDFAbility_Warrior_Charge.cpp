// Source/DungeonForged/Private/GAS/Abilities/Warrior/UDFAbility_Warrior_Charge.cpp
#include "GAS/Abilities/Warrior/UDFAbility_Warrior_Charge.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitMovementModeChange.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/UDFLockOnComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"
#include "Characters/ADFEnemyBase.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Damage_Physical.h"
#include "GAS/Effects/UGE_Debuff_Stun.h"
#include "GAS/UDFAttributeSet.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"

static AActor* DFWarriorGetLockTarget(ACharacter* C)
{
	const ADFPlayerCharacter* const P = Cast<ADFPlayerCharacter>(C);
	return P && P->LockOnComponent ? P->LockOnComponent->GetCurrentTarget() : nullptr;
}

UDFAbility_Warrior_Charge::UDFAbility_Warrior_Charge()
{
	AbilityCost_Mana = 0.f;
	AbilityCost_Stamina = 25.f;
	BaseCooldown = 14.f;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UDFAbility_Warrior_Charge::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityTags.AddTag(FDFGameplayTags::Ability_Warrior_Charge);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
		BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Stunned);
	}
}

bool UDFAbility_Warrior_Charge::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		return false;
	}
	ACharacter* const C = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	AActor* const T = C ? DFWarriorGetLockTarget(C) : nullptr;
	if (!C || !T)
	{
		if (C)
		{
			FGameplayEventData Evt;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(C, FDFGameplayTags::Ability_Failed_Range, Evt);
		}
		return false;
	}
	const float D = FVector::Dist(C->GetActorLocation(), T->GetActorLocation());
	if (D < ChargeMinRange || D > ChargeMaxRange)
	{
		if (C)
		{
			FGameplayEventData Evt;
			Evt.EventMagnitude = D;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(C, FDFGameplayTags::Ability_Failed_Range, Evt);
		}
		return false;
	}
	return true;
}

void UDFAbility_Warrior_Charge::ActivateAbility(
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
	if (UAbilitySystemComponent* asc = GetAbilitySystemComponentFromActorInfo())
	{
		if (asc->GetOwner() && asc->GetOwner()->HasAuthority())
		{
			ApplyResourceCostsToOwner(asc);
		}
	}
	bChargeImpactDone = false;
	ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Char || !Char->HasAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	UAbilitySystemComponent* const ASC = GetAbilitySystemComponentFromActorInfo();
	AActor* const Target = DFWarriorGetLockTarget(Char);
	if (ASC && ChargeIFrameEffect)
	{
		FGameplayEffectContextHandle C = ASC->MakeEffectContext();
		IFrameHandle = ASC->ApplyGameplayEffectToSelf(ChargeIFrameEffect.GetDefaultObject(), 1.f, C);
	}
	if (Target)
	{
		const FVector Dir2D = (Target->GetActorLocation() - Char->GetActorLocation()).GetSafeNormal2D();
		Char->LaunchCharacter(FVector(Dir2D.X, Dir2D.Y, 0.f) * ChargeLaunchSpeed, true, true);
	}
	if (UAbilityTask_WaitMovementModeChange* M = UAbilityTask_WaitMovementModeChange::CreateWaitMovementModeChange(this, MOVE_Walking))
	{
		M->OnChange.AddDynamic(this, &UDFAbility_Warrior_Charge::OnChargeMovementChanged);
		M->ReadyForActivation();
	}
	if (UAbilityTask_WaitDelay* D = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(0.1f, ChargeFallbackDelay)))
	{
		D->OnFinish.AddDynamic(this, &UDFAbility_Warrior_Charge::OnChargeDelayEnd);
		D->ReadyForActivation();
	}
}

void UDFAbility_Warrior_Charge::OnChargeDelayEnd()
{
	TryChargeSlam();
}

void UDFAbility_Warrior_Charge::OnChargeMovementChanged(EMovementMode /*NewMode*/)
{
	TryChargeSlam();
}

void UDFAbility_Warrior_Charge::TryChargeSlam()
{
	DoChargeSlam();
}

void UDFAbility_Warrior_Charge::DoChargeSlam()
{
	if (bChargeImpactDone)
	{
		return;
	}
	bChargeImpactDone = true;
	RemoveIFrame();
	ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Char || !GetWorld() || !Char->HasAuthority())
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
		return;
	}
	UAbilitySystemComponent* const Source = GetAbilitySystemComponentFromActorInfo();
	AActor* const LockTarget = DFWarriorGetLockTarget(Char);
	FCollisionQueryParams Q(SCENE_QUERY_STAT(Charge), false, Char);
	FCollisionObjectQueryParams Obj(ECC_Pawn);
	const FVector C = Char->GetActorLocation();
	TArray<FOverlapResult> Ov;
	GetWorld()->OverlapMultiByObjectType(Ov, C, FQuat::Identity, Obj, FCollisionShape::MakeSphere(ChargeLandOverlap), Q);
	const float Str = Source->GetNumericAttribute(UDFAttributeSet::GetStrengthAttribute());
	for (FOverlapResult& R : Ov)
	{
		AActor* A = R.GetActor();
		if (!A || A == Char || !Cast<ADFEnemyBase>(A))
		{
			continue;
		}
		UAbilitySystemComponent* TASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(A);
		if (!TASC || !Source)
		{
			continue;
		}
		FGameplayEffectContextHandle Ctx = Source->MakeEffectContext();
		Ctx.AddSourceObject(this);
		Ctx.AddInstigator(Char, Char);
		{
			const FGameplayEffectSpecHandle S = Source->MakeOutgoingSpec(UGE_Damage_Physical::StaticClass(), 1.f, Ctx);
			if (S.IsValid() && S.Data.IsValid())
			{
				S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, 60.f + Str);
				Source->ApplyGameplayEffectSpecToTarget(*S.Data.Get(), TASC);
			}
		}
		if (LockTarget == A)
		{
			const FGameplayEffectSpecHandle S = Source->MakeOutgoingSpec(UGE_Debuff_Stun::StaticClass(), 1.f, Ctx);
			if (S.IsValid() && S.Data.IsValid())
			{
				S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, 1.2f);
				Source->ApplyGameplayEffectSpecToTarget(*S.Data.Get(), TASC);
			}
		}
	}
	if (ChargeSlamNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			Char, ChargeSlamNiagara, C, FRotator::ZeroRotator, FVector(1.f), true, true, ENCPoolMethod::None, true);
	}
	PlayHeavyShake();
	if (ChargeLandMontage)
	{
		if (UAnimInstance* AI = Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr)
		{
			AI->Montage_Play(ChargeLandMontage, 1.f);
		}
	}
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UDFAbility_Warrior_Charge::PlayHeavyShake() const
{
	ACharacter* const Av = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (APlayerController* const PC = Av ? Cast<APlayerController>(Av->GetController()) : nullptr)
	{
		if (HeavyCameraShake)
		{
			PC->ClientStartCameraShake(HeavyCameraShake, 1.2f, ECameraShakePlaySpace::UserDefined, FRotator::ZeroRotator);
		}
	}
}

void UDFAbility_Warrior_Charge::RemoveIFrame()
{
	UAbilitySystemComponent* const ASCRemove = GetAbilitySystemComponentFromActorInfo();
	if (IFrameHandle.IsValid() && ASCRemove)
	{
		ASCRemove->RemoveActiveGameplayEffect(IFrameHandle, 1);
		IFrameHandle = FActiveGameplayEffectHandle();
	}
}
