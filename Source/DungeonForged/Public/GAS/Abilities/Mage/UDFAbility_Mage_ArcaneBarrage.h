// Source/DungeonForged/Public/GAS/Abilities/Mage/UDFAbility_Mage_ArcaneBarrage.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Mage_ArcaneBarrage.generated.h"

class UAbilitySystemComponent;
struct FGameplayEffectContextHandle;
class ADFArcaneMissileProjectile;

UCLASS()
class DUNGEONFORGED_API UDFAbility_Mage_ArcaneBarrage : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbility_Mage_ArcaneBarrage();

	/** 3 volleys before regen. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|Arcane", meta = (ClampMin = "1"))
	int32 MaxCharges = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|Arcane", meta = (ClampMin = "0.0"))
	float ChargeRechargeTime = 6.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|Arcane", meta = (ClampMin = "0.0"))
	float GlobalCastCooldown = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|Arcane")
	TSubclassOf<ADFArcaneMissileProjectile> MissileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|Arcane")
	TObjectPtr<UAnimMontage> ArcaneQuickCastMontage;

	/** Per-missile from arcane / overload (defaults to UGE_Damage_Magic in projectile). */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|Arcane")
	TSubclassOf<class UGameplayEffect> OverloadExtraDamageClass;

	/** Replicated bookkeeping on authority. */
	void NotifyArcaneMissileHit(AActor* Target, UAbilitySystemComponent* TargetASC, const FGameplayEffectContextHandle& Ctx);

protected:
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnTraceEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnCastMontageEnd();

	UFUNCTION()
	void OnRechargeTick();

	virtual void PostInitProperties() override;

	int32 RemainingCharges = 3;
	double LastGlobalCast = -1.0;
	FTimerHandle RechargeHandle;
	TMap<TWeakObjectPtr<AActor>, int32> SalvoHitCount;
};
