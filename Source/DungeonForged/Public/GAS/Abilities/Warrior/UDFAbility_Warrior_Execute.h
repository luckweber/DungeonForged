// Source/DungeonForged/Public/GAS/Abilities/Warrior/UDFAbility_Warrior_Execute.h
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Warrior_Execute.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFAbility_Warrior_Execute : public UDFGameplayAbility
{
	GENERATED_BODY()
public:
	UDFAbility_Warrior_Execute();

	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|Execute")
	TObjectPtr<class UAnimMontage> ExecuteMontage;
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|Execute")
	TObjectPtr<class UNiagaraSystem> DeathBlowNiagara;
	/** If set, applied to self on lethal execute (e.g. mana + cooldown). Otherwise +50 mana in code. */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|Execute")
	TSubclassOf<class UGameplayEffect> ExecuteKillBonusEffect;
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|Execute", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HealthThresholdFraction = 0.25f;
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|Execute", meta = (ClampMin = "0.0"))
	float HitStopTimeDilation = 0.1f;
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|Execute", meta = (ClampMin = "0.0"))
	float HitStopDurationSec = 0.1f;

protected:
	virtual void PostInitProperties() override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	UFUNCTION()
	void OnExecuteMontageEnd();
	UFUNCTION()
	void OnExecuteTraceEvent(FGameplayEventData Payload);
	UFUNCTION()
	void RestoreTimeDilationAfterHitStop();
	void DoExecuteHit();
	FTimerHandle HitStopTimer;
};
