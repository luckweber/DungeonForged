// Source/DungeonForged/Public/GAS/Abilities/Boss/UDFBossAbility_TerrorShout.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFBossAbility_TerrorShout.generated.h"

class UAnimMontage;
class UCameraShakeBase;
class UNiagaraSystem;
class UGameplayEffect;

UCLASS()
class DUNGEONFORGED_API UDFBossAbility_TerrorShout : public UDFGameplayAbility
{
	GENERATED_BODY()
public:
	UDFBossAbility_TerrorShout();
	virtual void PostInitProperties() override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnMontageFinished();

	UFUNCTION()
	void RunShoutAoe();

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Terror")
	TObjectPtr<UAnimMontage> BossShoutMontage;

	/** 1200 uu sphere. */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Terror", meta = (ClampMin = "1"))
	float AoeRadius = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Terror")
	TSubclassOf<UGameplayEffect> TerrorDebuffClass;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Terror")
	TSubclassOf<UGameplayEffect> WeakenDebuffClass;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Terror")
	TSubclassOf<UGameplayEffect> CooldownClass;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Terror")
	TSubclassOf<UCameraShakeBase> HeavyCameraShake;

	/** 1.5s per spec. */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Terror", meta = (ClampMin = "0"))
	float CameraShakeDurationScale = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Terror", meta = (ClampMin = "0"))
	float ShakeInner = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Terror", meta = (ClampMin = "0"))
	float ShakeOuter = 5000.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Terror")
	TObjectPtr<UNiagaraSystem> ShockwaveNiagara;
};
