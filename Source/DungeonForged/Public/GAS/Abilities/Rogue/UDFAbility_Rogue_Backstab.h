// Source/DungeonForged/Public/GAS/Abilities/Rogue/UDFAbility_Rogue_Backstab.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Rogue_Backstab.generated.h"

class UAnimMontage;
class UDFComboPointsComponent;
class AActor;

UCLASS()
class DUNGEONFORGED_API UDFAbility_Rogue_Backstab : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbility_Rogue_Backstab();

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Rogue|Backstab")
	TObjectPtr<UAnimMontage> BackstabMontage;

	/** GCD between rogue builders that share this (seconds). */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Rogue|Backstab", meta = (ClampMin = "0.0"))
	float GlobalGCD = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Rogue|Backstab", meta = (ClampMin = "0.0"))
	float MeleeRange = 180.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Rogue|Backstab", meta = (ClampMin = "0.0"))
	float BaseDamage = 10.f;

protected:
	virtual void PostInitProperties() override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnMontageEnd();

	UFUNCTION()
	void OnTraceEvent(FGameplayEventData Payload);

	void DoBackstab();

	static UDFComboPointsComponent* GetCombo(AActor* From);

	/** For GCD (written in ActivateAbility). */
	double LastGCD = -1e10;
};
