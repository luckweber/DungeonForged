// Source/DungeonForged/Public/Combat/UDFHitReactionComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "UDFHitReactionComponent.generated.h"

class UAnimMontage;
class UNiagaraSystem;
class UMaterialInterface;
class AActor;
class ACharacter;
class UGameplayEffect;

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFHitReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFHitReactionComponent();

	// All montage/VFX assets are stored as soft refs so the CDO of the owning Character BP
	// does not load them. They are resolved at BeginPlay via @c EnsureHitReactionAssetsLoaded.

	UPROPERTY(EditAnywhere, Category = "Combat|HitReaction")
	TSoftObjectPtr<UAnimMontage> LightHitMontage;

	UPROPERTY(EditAnywhere, Category = "Combat|HitReaction")
	TSoftObjectPtr<UAnimMontage> HeavyHitMontage;

	UPROPERTY(EditAnywhere, Category = "Combat|HitReaction")
	TSoftObjectPtr<UAnimMontage> KnockbackMontage;

	UPROPERTY(Transient) TObjectPtr<UAnimMontage> ResolvedLightHitMontage;
	UPROPERTY(Transient) TObjectPtr<UAnimMontage> ResolvedHeavyHitMontage;
	UPROPERTY(Transient) TObjectPtr<UAnimMontage> ResolvedKnockbackMontage;

	/**
	 * When true, plays Front/Back/Left/Right montages from @a HitDirection2D (attacker → victim, XY).
	 * If the mesh uses @c UUDFAnimInstance_Enemy and @a bPreferAnimInstanceDirectionalMontages is set,
	 * montages are taken from the AnimBP (HitReact_Front, etc.) instead of the fields below.
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|HitReaction|Directional")
	bool bUseDirectionalHitReactions = false;

	UPROPERTY(EditAnywhere, Category = "Combat|HitReaction|Directional",
		meta = (EditCondition = "bUseDirectionalHitReactions"))
	bool bPreferAnimInstanceDirectionalMontages = true;

	UPROPERTY(EditAnywhere, Category = "Combat|HitReaction|Directional",
		meta = (EditCondition = "bUseDirectionalHitReactions"))
	TSoftObjectPtr<UAnimMontage> LightHit_Front;

	UPROPERTY(EditAnywhere, Category = "Combat|HitReaction|Directional",
		meta = (EditCondition = "bUseDirectionalHitReactions"))
	TSoftObjectPtr<UAnimMontage> LightHit_Back;

	UPROPERTY(EditAnywhere, Category = "Combat|HitReaction|Directional",
		meta = (EditCondition = "bUseDirectionalHitReactions"))
	TSoftObjectPtr<UAnimMontage> LightHit_Left;

	UPROPERTY(EditAnywhere, Category = "Combat|HitReaction|Directional",
		meta = (EditCondition = "bUseDirectionalHitReactions"))
	TSoftObjectPtr<UAnimMontage> LightHit_Right;

	UPROPERTY(EditAnywhere, Category = "Combat|HitReaction|Directional",
		meta = (EditCondition = "bUseDirectionalHitReactions"))
	TSoftObjectPtr<UAnimMontage> HeavyHit_Front;

	UPROPERTY(EditAnywhere, Category = "Combat|HitReaction|Directional",
		meta = (EditCondition = "bUseDirectionalHitReactions"))
	TSoftObjectPtr<UAnimMontage> HeavyHit_Back;

	UPROPERTY(EditAnywhere, Category = "Combat|HitReaction|Directional",
		meta = (EditCondition = "bUseDirectionalHitReactions"))
	TSoftObjectPtr<UAnimMontage> HeavyHit_Left;

	UPROPERTY(EditAnywhere, Category = "Combat|HitReaction|Directional",
		meta = (EditCondition = "bUseDirectionalHitReactions"))
	TSoftObjectPtr<UAnimMontage> HeavyHit_Right;

	UPROPERTY(Transient) TObjectPtr<UAnimMontage> ResolvedLightHit_Front;
	UPROPERTY(Transient) TObjectPtr<UAnimMontage> ResolvedLightHit_Back;
	UPROPERTY(Transient) TObjectPtr<UAnimMontage> ResolvedLightHit_Left;
	UPROPERTY(Transient) TObjectPtr<UAnimMontage> ResolvedLightHit_Right;
	UPROPERTY(Transient) TObjectPtr<UAnimMontage> ResolvedHeavyHit_Front;
	UPROPERTY(Transient) TObjectPtr<UAnimMontage> ResolvedHeavyHit_Back;
	UPROPERTY(Transient) TObjectPtr<UAnimMontage> ResolvedHeavyHit_Left;
	UPROPERTY(Transient) TObjectPtr<UAnimMontage> ResolvedHeavyHit_Right;

	UPROPERTY(EditAnywhere, Category = "Combat|HitReaction", meta = (ClampMin = "0.0"))
	float StaggerThreshold = 30.f;

	/** Heavy hit damage that retargets AI aggro to the attacker (co-op tanking). */
	UPROPERTY(EditAnywhere, Category = "Combat|HitReaction|AI", meta = (ClampMin = "0.0"))
	float AggroSwitchDamageThreshold = 40.f;

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
	TSoftObjectPtr<UNiagaraSystem> HitImpactNiagara;

	UPROPERTY(Transient) TObjectPtr<UNiagaraSystem> ResolvedHitImpactNiagara;

	UPROPERTY(EditAnywhere, Category = "Combat|VFX", meta = (ClampMin = "0.0"))
	float HitVFXMaxDrawDistance = 0.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Decal")
	TSoftObjectPtr<UMaterialInterface> DecalMaterial;

	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> ResolvedDecalMaterial;

	UPROPERTY(EditAnywhere, Category = "Combat|Decal", meta = (ClampMin = "1.0"))
	FVector DecalSize = FVector(32.f, 64.f, 64.f);

	UPROPERTY(EditAnywhere, Category = "Combat|Decal", meta = (ClampMin = "0.0"))
	float DecalLifespan = 8.f;

	/** Optional per-damage-source hit montage overrides (Damage.Source.Slash / Blunt / Pierce). */
	UPROPERTY(EditAnywhere, Category = "Combat|HitReaction|DamageSource", meta = (Categories = "Damage.Source"))
	TMap<FGameplayTag, TSoftObjectPtr<UAnimMontage>> DamageSourceHitMontages;

	/** Optional per-damage-source impact VFX overrides. */
	UPROPERTY(EditAnywhere, Category = "Combat|VFX|DamageSource", meta = (Categories = "Damage.Source"))
	TMap<FGameplayTag, TSoftObjectPtr<UNiagaraSystem>> DamageSourceHitImpactNiagara;

	/** Optional per-bone hit montage overrides (B4). */
	UPROPERTY(EditAnywhere, Category = "Combat|HitReaction|BodyPart")
	TMap<FName, TSoftObjectPtr<UAnimMontage>> BoneHitMontages;

	UPROPERTY(Transient) TMap<FGameplayTag, TObjectPtr<UAnimMontage>> ResolvedDamageSourceHitMontages;
	UPROPERTY(Transient) TMap<FGameplayTag, TObjectPtr<UNiagaraSystem>> ResolvedDamageSourceHitImpactNiagara;
	UPROPERTY(Transient) TMap<FName, TObjectPtr<UAnimMontage>> ResolvedBoneHitMontages;

	UFUNCTION(BlueprintCallable, Category = "Combat|HitReaction")
	void OnHitReceived(
		float DamageAmount,
		float KnockbackMagnitude,
		FVector HitDirection2D,
		AActor* Instigator,
		FVector HitLocation = FVector::ZeroVector,
		FVector HitNormal = FVector::UpVector,
		FGameplayTag DamageSourceTag = FGameplayTag(),
		FName HitBoneName = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Combat|HitReaction")
	void PlayHitReaction(UAnimMontage* Montage, float PlayRate = 1.f);

	UFUNCTION(BlueprintCallable, Category = "Combat|VFX")
	void SpawnHitVFX(FVector Location, FRotator NormalRotation, FGameplayTag DamageSourceTag = FGameplayTag());

	UFUNCTION(BlueprintCallable, Category = "Combat|VFX")
	void SpawnHitDecal(FVector Location, FRotator NormalRotation);

protected:
	virtual void BeginPlay() override;

	/** Triggers async load of all soft refs and fills Resolved* caches on completion. */
	void EnsureHitReactionAssetsLoaded();

	/** Repopulates Resolved* caches by reading currently-loaded soft pointers. */
	void RefreshResolvedHitReactionAssets();

	/** Streamable handle for the async load kicked off in @c EnsureHitReactionAssetsLoaded. */
	TSharedPtr<struct FStreamableHandle> HitReactionStreamHandle;

	/** Stagger only for heavy band (Stagger..Knockback) if StaggerStun is set. */
	void TryApplyStaggerStun(AActor* InstigatorActor) const;

	UAnimMontage* ResolveHitMontage(
		float DamageAmount,
		bool bIsKnockback,
		const FVector& HitDirection2D,
		AActor* Instigator,
		FGameplayTag DamageSourceTag,
		FName HitBoneName = NAME_None) const;

	static UAnimMontage* PickDirectionalMontage(
		const ACharacter* Victim,
		const FVector& HitDirectionFromAttacker,
		UAnimMontage* Front,
		UAnimMontage* Back,
		UAnimMontage* Left,
		UAnimMontage* Right,
		UAnimMontage* Fallback);
};
