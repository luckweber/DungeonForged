// Source/DungeonForged/Public/GAS/Abilities/Warrior/UDFAbility_Warrior_Charge.h
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Engine/EngineTypes.h"
#include "GameplayEffectTypes.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Warrior_Charge.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFAbility_Warrior_Charge : public UDFGameplayAbility
{
	GENERATED_BODY()
public:
	UDFAbility_Warrior_Charge();

	/** e.g. UGE with State.Invulnerable 0.5s */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|Charge")
	TSubclassOf<class UGameplayEffect> ChargeIFrameEffect;
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|Charge")
	TObjectPtr<class UAnimMontage> ChargeLandMontage;
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|Charge")
	TObjectPtr<class UNiagaraSystem> ChargeSlamNiagara;
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|Charge")
	TSubclassOf<class UCameraShakeBase> HeavyCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|Charge", meta = (ClampMin = "0.0"))
	float ChargeMinRange = 400.f;
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|Charge", meta = (ClampMin = "0.0"))
	float ChargeMaxRange = 2000.f;
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|Charge", meta = (ClampMin = "0.0"))
	float ChargeLandOverlap = 120.f;
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|Charge", meta = (ClampMin = "0.0"))
	float ChargeFallbackDelay = 0.6f;
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|Charge", meta = (ClampMin = "0.0"))
	float ChargeLaunchSpeed = 2200.f;

protected:
	virtual void PostInitProperties() override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	UFUNCTION()
	void OnChargeDelayEnd();
	UFUNCTION()
	void OnChargeMovementChanged(EMovementMode NewMode);
	void TryChargeSlam();
	void DoChargeSlam();
	void PlayHeavyShake() const;
	void RemoveIFrame();

	bool bChargeImpactDone = false;
	FActiveGameplayEffectHandle IFrameHandle;
};
