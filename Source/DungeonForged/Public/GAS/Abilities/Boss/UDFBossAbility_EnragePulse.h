// Source/DungeonForged/Public/GAS/Abilities/Boss/UDFBossAbility_EnragePulse.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFBossAbility_EnragePulse.generated.h"

class UAnimMontage;
class UCameraShakeBase;
class UNiagaraSystem;
class UGameplayEffect;

UCLASS()
class DUNGEONFORGED_API UDFBossAbility_EnragePulse : public UDFGameplayAbility
{
	GENERATED_BODY()
public:
	UDFBossAbility_EnragePulse();
	virtual void PostInitProperties() override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnMontageEnd();

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Enrage")
	TObjectPtr<UAnimMontage> EnragePulseMontage;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Enrage", meta = (ClampMin = "1"))
	float PulseRadius = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Enrage")
	TSubclassOf<UGameplayEffect> CooldownClass;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Enrage")
	TObjectPtr<UNiagaraSystem> PulseNiagara;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Boss|Enrage")
	TSubclassOf<UCameraShakeBase> KineticCameraShake;
};
