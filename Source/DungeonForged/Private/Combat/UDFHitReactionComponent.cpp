// Source/DungeonForged/Private/Combat/UDFHitReactionComponent.cpp
#include "Combat/UDFHitReactionComponent.h"
#include "Animation/UDFAnimInstance_Enemy.h"
#include "Characters/ADFEnemyBase.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Engine/Engine.h"
#include "FX/UDFCombatFeedbackTypes.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "UI/Combat/DFCombatTextTypes.h"
#include "UI/Combat/UDFCombatTextSubsystem.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"

UDFHitReactionComponent::UDFHitReactionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDFHitReactionComponent::OnHitReceived(
	const float DamageAmount,
	const float KnockbackMagnitude,
	const FVector HitDirection2D,
	AActor* const Instigator,
	const FVector HitLocation,
	const FVector HitNormal,
	const FGameplayTag DamageSourceTag)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (const ADFEnemyBase* const Enemy = Cast<ADFEnemyBase>(GetOwner()))
	{
		if (Enemy->HasDied() || Enemy->IsInDeathFlow())
		{
			return;
		}
	}
	FVector Dir = HitDirection2D;
	if (Dir.IsNearlyZero())
	{
		Dir = FVector::ForwardVector;
	}
	Dir.Z = 0.f;
	Dir.Normalize();

	const bool bIsKnockback = DamageAmount >= KnockbackThreshold;
	if (ACharacter* C = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* CMC = C->GetCharacterMovement())
		{
			if (bIsKnockback)
			{
				const FVector Impulse = Dir * (KnockbackMagnitude * KnockbackImpulseFromHit);
				CMC->AddImpulse(Impulse, true);
			}
		}
	}

	if (UAnimMontage* const ReactionMontage = ResolveHitMontage(DamageAmount, bIsKnockback, Dir, Instigator, DamageSourceTag))
	{
		PlayHitReaction(ReactionMontage, 1.f);
	}

	if (StaggerStunGameplayEffect && (!bIsKnockback) && (DamageAmount >= StaggerThreshold))
	{
		TryApplyStaggerStun(Instigator);
	}

	if (ADFPlayerCharacter* const Victim = Cast<ADFPlayerCharacter>(GetOwner()))
	{
		float MaxH = 1.f;
		if (UAbilitySystemComponent* const ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
		{
			MaxH = FMath::Max(1.f, ASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute()));
		}
		const float Pct = MaxH > 0.f ? (DamageAmount / MaxH) : 0.f;
		EDFHitFeedbackBand Band = EDFHitFeedbackBand::Light;
		if (bIsKnockback)
		{
			Band = EDFHitFeedbackBand::Knockback;
		}
		else if (Pct > 0.3f)
		{
			Band = EDFHitFeedbackBand::Critical;
		}
		else if (DamageAmount >= StaggerThreshold)
		{
			Band = EDFHitFeedbackBand::Heavy;
		}
		Victim->Client_HitFeedback(Band, Pct, Instigator);
	}

	const bool bUseImpact = !HitLocation.IsNearlyZero(1.f);
	FVector VfxPos = GetOwner()->GetActorLocation();
	if (bUseImpact)
	{
		VfxPos = HitLocation;
	}
	else if (ACharacter* Ch = Cast<ACharacter>(GetOwner()))
	{
		if (Ch->GetMesh())
		{
			VfxPos = Ch->GetMesh()->GetComponentLocation();
		}
	}
	const FRotator FacingVfx = bUseImpact ? FRotator((-HitNormal).Rotation())
		: FRotator(Dir.Rotation().Pitch, Dir.Rotation().Yaw, 0.f);
	if (!IsRunningDedicatedServer())
	{
		if (UWorld* const W = GetWorld())
		{
			if (UAbilitySystemComponent* const TASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
			{
				if (TASC->HasMatchingGameplayTag(FDFGameplayTags::State_Invulnerable)
					&& FDFGameplayTags::State_Invulnerable.IsValid())
				{
					if (UDFCombatTextSubsystem* const Cts = W->GetSubsystem<UDFCombatTextSubsystem>())
					{
						Cts->SpawnTextString(
							VfxPos + FVector(0.f, 0.f, 40.f), TEXT("IMMUNE"), ECombatTextType::Immune);
					}
				}
				else if (TASC->HasMatchingGameplayTag(FDFGameplayTags::State_Combat_Block)
					&& FDFGameplayTags::State_Combat_Block.IsValid())
				{
					if (UDFCombatTextSubsystem* const Cts = W->GetSubsystem<UDFCombatTextSubsystem>())
					{
						Cts->SpawnTextString(
							VfxPos + FVector(0.f, 0.f, 40.f), TEXT("BLOCK"), ECombatTextType::Block);
					}
				}
			}
		}
	}
	SpawnHitVFX(VfxPos, FacingVfx, DamageSourceTag);
	SpawnHitDecal(VfxPos, bUseImpact ? FRotator((-HitNormal).Rotation()) : FacingVfx);
}

UAnimMontage* UDFHitReactionComponent::PickDirectionalMontage(
	const ACharacter* const Victim,
	const FVector& HitDirectionFromAttacker,
	UAnimMontage* const Front,
	UAnimMontage* const Back,
	UAnimMontage* const Left,
	UAnimMontage* const Right,
	UAnimMontage* const Fallback)
{
	if (!Victim)
	{
		return Fallback;
	}
	const FVector2D Fwd(FVector2D(Victim->GetActorForwardVector().GetSafeNormal2D()));
	FVector2D ToHit = FVector2D(HitDirectionFromAttacker).GetSafeNormal();
	if (ToHit.IsNearlyZero(0.01f))
	{
		return Fallback ? Fallback : Front;
	}
	const float CrossZ = Fwd.X * ToHit.Y - Fwd.Y * ToHit.X;
	const float Dot = FMath::Clamp(FVector2D::DotProduct(Fwd, ToHit), -1.f, 1.f);
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(CrossZ, Dot));
	const float A = FMath::Abs(Yaw);
	if (A < 45.f)
	{
		return Front ? Front : Fallback;
	}
	if (A > 135.f)
	{
		return Back ? Back : Fallback;
	}
	if (Yaw > 0.f)
	{
		return Right ? Right : Fallback;
	}
	return Left ? Left : Fallback;
}

UAnimMontage* UDFHitReactionComponent::ResolveHitMontage(
	const float DamageAmount,
	const bool bIsKnockback,
	const FVector& HitDirection2D,
	AActor* const Instigator,
	const FGameplayTag DamageSourceTag) const
{
	if (DamageSourceTag.IsValid())
	{
		if (const TObjectPtr<UAnimMontage>* const Found = DamageSourceHitMontages.Find(DamageSourceTag))
		{
			if (*Found)
			{
				return Found->Get();
			}
		}
	}

	if (bIsKnockback)
	{
		return KnockbackMontage;
	}
	if (DamageAmount <= 0.f)
	{
		return nullptr;
	}

	const bool bHeavy = DamageAmount >= StaggerThreshold;
	if (bUseDirectionalHitReactions)
	{
		if (bPreferAnimInstanceDirectionalMontages)
		{
			if (ACharacter* const C = Cast<ACharacter>(GetOwner()))
			{
				if (USkeletalMeshComponent* const Skel = C->GetMesh())
				{
					if (UUDFAnimInstance_Enemy* const EnemyAnim = Cast<UUDFAnimInstance_Enemy>(Skel->GetAnimInstance()))
					{
						EnemyAnim->HitReactionDirection = HitDirection2D;
						EnemyAnim->HitFromWorldLocation =
							Instigator ? Instigator->GetActorLocation() : FVector::ZeroVector;
						if (UAnimMontage* const FromAnim = EnemyAnim->SelectHitMontage(HitDirection2D))
						{
							return FromAnim;
						}
					}
				}
			}
		}

		if (const ACharacter* const Victim = Cast<ACharacter>(GetOwner()))
		{
			UAnimMontage* const Fallback = bHeavy ? HeavyHitMontage : LightHitMontage;
			if (bHeavy)
			{
				return PickDirectionalMontage(
					Victim,
					HitDirection2D,
					HeavyHit_Front,
					HeavyHit_Back,
					HeavyHit_Left,
					HeavyHit_Right,
					PickDirectionalMontage(
						Victim,
						HitDirection2D,
						LightHit_Front,
						LightHit_Back,
						LightHit_Left,
						LightHit_Right,
						Fallback));
			}
			return PickDirectionalMontage(
				Victim,
				HitDirection2D,
				LightHit_Front,
				LightHit_Back,
				LightHit_Left,
				LightHit_Right,
				Fallback);
		}
	}

	if (bHeavy)
	{
		return HeavyHitMontage ? HeavyHitMontage : LightHitMontage;
	}
	return LightHitMontage;
}

void UDFHitReactionComponent::TryApplyStaggerStun(AActor* const InstigatorActor) const
{
	AActor* const O = GetOwner();
	UAbilitySystemComponent* const ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(O);
	if (!ASC)
	{
		return;
	}
	FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
	if (InstigatorActor)
	{
		Ctx.AddInstigator(InstigatorActor, InstigatorActor);
	}
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(
		StaggerStunGameplayEffect, StaggerEffectLevel, Ctx);
	if (Spec.IsValid() && Spec.Data)
	{
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void UDFHitReactionComponent::PlayHitReaction(UAnimMontage* const Montage, const float PlayRate)
{
	if (!Montage)
	{
		return;
	}
	if (ADFPlayerCharacter* const Player = Cast<ADFPlayerCharacter>(GetOwner()))
	{
		if (Player->HasAuthority())
		{
			Player->Multicast_PlayHitReactionMontage(Montage, PlayRate);
		}
		return;
	}
	if (ACharacter* C = Cast<ACharacter>(GetOwner()))
	{
		if (UAnimInstance* A = (C->GetMesh() ? C->GetMesh()->GetAnimInstance() : nullptr))
		{
			A->Montage_Play(Montage, PlayRate);
		}
	}
}

void UDFHitReactionComponent::SpawnHitVFX(
	const FVector Location,
	const FRotator NormalRotation,
	const FGameplayTag DamageSourceTag)
{
	if (IsRunningDedicatedServer())
	{
		return;
	}
	UNiagaraSystem* const Vfx = [this, DamageSourceTag]() -> UNiagaraSystem*
	{
		if (DamageSourceTag.IsValid())
		{
			if (const TObjectPtr<UNiagaraSystem>* const Found = DamageSourceHitImpactNiagara.Find(DamageSourceTag))
			{
				if (*Found)
				{
					return Found->Get();
				}
			}
		}
		return HitImpactNiagara;
	}();
	if (!Vfx)
	{
		return;
	}
	UNiagaraComponent* N = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		this, Vfx, Location, NormalRotation, FVector(1.f), true, true, ENCPoolMethod::None, true);
	(void)N;
}

void UDFHitReactionComponent::SpawnHitDecal(const FVector Location, const FRotator NormalRotation)
{
	if (IsRunningDedicatedServer() || !DecalMaterial)
	{
		return;
	}
	UGameplayStatics::SpawnDecalAtLocation(
		this, DecalMaterial, DecalSize, Location, NormalRotation, DecalLifespan);
}
