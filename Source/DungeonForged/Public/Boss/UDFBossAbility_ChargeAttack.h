// Source/DungeonForged/Public/Boss/UDFBossAbility_ChargeAttack.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFBossAbility_ChargeAttack.generated.h"

struct FGameplayAbilitySpecHandle;
struct FGameplayAbilityActorInfo;
struct FGameplayAbilityActivationInfo;
struct FGameplayEventData;
class UAnimMontage;

UCLASS()
class DUNGEONFORGED_API UDFBossAbility_ChargeAttack : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFBossAbility_ChargeAttack();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnChargeResolve();

	UFUNCTION()
	void OnMontageEnd();

	virtual void PostInitProperties() override;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Charge")
	float ChargeDistance = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Charge")
	float SweepRadius = 80.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Charge")
	float ChargeDamage = 35.f;

	/** Seconds after activation to run the melee sweep (align with charge contact frame). */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Charge")
	float ChargeHitTime = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Charge")
	TObjectPtr<UAnimMontage> ChargeMissMontage;
};
