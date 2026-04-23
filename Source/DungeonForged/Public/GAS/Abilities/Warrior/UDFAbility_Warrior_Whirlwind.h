// Source/DungeonForged/Public/GAS/Abilities/Warrior/UDFAbility_Warrior_Whirlwind.h
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayEffectTypes.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Warrior_Whirlwind.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFAbility_Warrior_Whirlwind : public UDFGameplayAbility
{
	GENERATED_BODY()
public:
	UDFAbility_Warrior_Whirlwind();

	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|Whirlwind")
	TObjectPtr<class UAnimMontage> WhirlwindMontage;
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|Whirlwind")
	TObjectPtr<class UAnimMontage> WhirlwindStopMontage;
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|Whirlwind")
	TObjectPtr<class UNiagaraSystem> WhirlwindHitSparks;
	/** Infinite duration; should grant State.Spinning and optional move slow. */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|Whirlwind")
	TSubclassOf<class UGameplayEffect> WhirlwindActiveEffect;
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|Whirlwind", meta = (ClampMin = "0.0"))
	float WhirlwindTickInterval = 0.3f;
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|Whirlwind", meta = (ClampMin = "0.0"))
	float WhirlwindMaxDuration = 3.f;
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|Whirlwind", meta = (ClampMin = "0.0"))
	float WhirlwindOverlapRadius = 200.f;

protected:
	virtual void PostInitProperties() override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION()
	void OnWhirlwindTickEvent(FGameplayEventData Payload);
	UFUNCTION()
	void OnWhirlwindInputReleased(float TimeHeld);
	UFUNCTION()
	void OnWhirlwindMaxTime();

	void FinishWhirlwind();
	void RemoveWhirlwindEffect();
	void ApplyWhirlwindDamageTick();

	FActiveGameplayEffectHandle WhirlwindActiveHandle;
	TMap<AActor*, float> LastHitTime;
	float CachedMaxWalkSpeed = 0.f;
	bool bHasWalkCache = false;
	bool bWhirlwindEnding = false;
};
