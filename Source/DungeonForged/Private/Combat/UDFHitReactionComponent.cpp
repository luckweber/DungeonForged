// Source/DungeonForged/Private/Combat/UDFHitReactionComponent.cpp
#include "Combat/UDFHitReactionComponent.h"
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
	const FVector HitNormal)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
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
				if (KnockbackMontage)
				{
					PlayHitReaction(KnockbackMontage, 1.f);
				}
			}
		}
	}

	if (!bIsKnockback)
	{
		if (DamageAmount >= StaggerThreshold)
		{
			if (HeavyHitMontage)
			{
				PlayHitReaction(HeavyHitMontage, 1.f);
			}
			else if (LightHitMontage)
			{
				PlayHitReaction(LightHitMontage, 1.f);
			}
		}
		else if (DamageAmount > 0.f)
		{
			if (LightHitMontage)
			{
				PlayHitReaction(LightHitMontage, 1.f);
			}
		}
	}

	if (StaggerStunGameplayEffect && (!bIsKnockback) && (DamageAmount >= StaggerThreshold))
	{
		TryApplyStaggerStun(Instigator);
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
	SpawnHitVFX(VfxPos, FacingVfx);
	SpawnHitDecal(VfxPos, bUseImpact ? FRotator((-HitNormal).Rotation()) : FacingVfx);
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
	if (ACharacter* C = Cast<ACharacter>(GetOwner()))
	{
		if (UAnimInstance* A = (C->GetMesh() ? C->GetMesh()->GetAnimInstance() : nullptr))
		{
			A->Montage_Play(Montage, PlayRate);
		}
	}
}

void UDFHitReactionComponent::SpawnHitVFX(const FVector Location, const FRotator NormalRotation)
{
	if (IsRunningDedicatedServer() || !HitImpactNiagara)
	{
		return;
	}
	UNiagaraComponent* N = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		this, HitImpactNiagara, Location, NormalRotation, FVector(1.f), true, true, ENCPoolMethod::None, true);
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
