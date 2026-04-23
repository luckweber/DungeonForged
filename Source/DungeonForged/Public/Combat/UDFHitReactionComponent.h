// Source/DungeonForged/Public/Combat/UDFHitReactionComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UDFHitReactionComponent.generated.h"

class UAnimMontage;
class UNiagaraSystem;
class UMaterialInterface;
class AActor;
class UGameplayEffect;

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFHitReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFHitReactionComponent();

	UPROPERTY(EditAnywhere, Category = "Combat|HitReaction")
	TObjectPtr<UAnimMontage> LightHitMontage;

	UPROPERTY(EditAnywhere, Category = "Combat|HitReaction")
	TObjectPtr<UAnimMontage> HeavyHitMontage;

	UPROPERTY(EditAnywhere, Category = "Combat|HitReaction")
	TObjectPtr<UAnimMontage> KnockbackMontage;

	UPROPERTY(EditAnywhere, Category = "Combat|HitReaction", meta = (ClampMin = "0.0"))
	float StaggerThreshold = 30.f;

	UPROPERTY(EditAnywhere, Category = "Combat|HitReaction", meta = (ClampMin = "0.0"))
	float KnockbackThreshold = 60.f;

	/** If damage &gt; KnockbackThreshold, this impulse scale is used with the direction. */
	UPROPERTY(EditAnywhere, Category = "Combat|HitReaction", meta = (ClampMin = "0.0"))
	float KnockbackImpulseFromHit = 1.f;

	/** Gameplay effect with a duration; typically grants `State.Stunned` for stagger. */
	UPROPERTY(EditAnywhere, Category = "Combat|GAS")
	TSubclassOf<UGameplayEffect> StaggerStunGameplayEffect;

	/** Stun effect level for Apply. */
	UPROPERTY(EditAnywhere, Category = "Combat|GAS", meta = (ClampMin = "1.0"))
	float StaggerEffectLevel = 1.f;

	/** VFX on hit point (dedicated server skips spawn). */
	UPROPERTY(EditAnywhere, Category = "Combat|VFX")
	TObjectPtr<UNiagaraSystem> HitImpactNiagara;

	UPROPERTY(EditAnywhere, Category = "Combat|VFX", meta = (ClampMin = "0.0"))
	float HitVFXMaxDrawDistance = 0.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Decal")
	TObjectPtr<UMaterialInterface> DecalMaterial;

	UPROPERTY(EditAnywhere, Category = "Combat|Decal", meta = (ClampMin = "1.0"))
	FVector DecalSize = FVector(32.f, 64.f, 64.f);

	UPROPERTY(EditAnywhere, Category = "Combat|Decal", meta = (ClampMin = "0.0"))
	float DecalLifespan = 8.f;

	UFUNCTION(BlueprintCallable, Category = "Combat|HitReaction")
	void OnHitReceived(
		float DamageAmount,
		float KnockbackMagnitude,
		FVector HitDirection2D,
		AActor* Instigator,
		FVector HitLocation = FVector::ZeroVector,
		FVector HitNormal = FVector::UpVector);

	UFUNCTION(BlueprintCallable, Category = "Combat|HitReaction")
	void PlayHitReaction(UAnimMontage* Montage, float PlayRate = 1.f);

	UFUNCTION(BlueprintCallable, Category = "Combat|VFX")
	void SpawnHitVFX(FVector Location, FRotator NormalRotation);

	UFUNCTION(BlueprintCallable, Category = "Combat|VFX")
	void SpawnHitDecal(FVector Location, FRotator NormalRotation);

protected:
	/** Stagger only for heavy band (Stagger..Knockback) if StaggerStun is set. */
	void TryApplyStaggerStun(AActor* InstigatorActor) const;
};
