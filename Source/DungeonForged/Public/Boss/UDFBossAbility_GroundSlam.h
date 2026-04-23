// Source/DungeonForged/Public/Boss/UDFBossAbility_GroundSlam.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFBossAbility_GroundSlam.generated.h"

struct FGameplayAbilitySpecHandle;
struct FGameplayAbilityActorInfo;
struct FGameplayAbilityActivationInfo;
struct FGameplayEventData;
class UCameraShakeBase;
class UNiagaraSystem;

UCLASS()
class DUNGEONFORGED_API UDFBossAbility_GroundSlam : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFBossAbility_GroundSlam();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnSlamImpact();

	UFUNCTION()
	void OnMontageEnd();

	virtual void PostInitProperties() override;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Slam")
	float SlamRadius = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Slam")
	float SlamDamage = 40.f;

	/** Seconds after activation that the slam damage + FX fire (tune to anim). */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Slam")
	float SlamImpactDelay = 0.45f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Slam")
	TSubclassOf<UCameraShakeBase> SlamCameraShake;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Slam")
	TObjectPtr<UNiagaraSystem> SlamNiagara;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Slam")
	float CameraShakeInnerRadius = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Slam")
	float CameraShakeOuterRadius = 2500.f;
};
