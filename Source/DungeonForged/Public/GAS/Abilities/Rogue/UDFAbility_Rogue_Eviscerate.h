// Source/DungeonForged/Public/GAS/Abilities/Rogue/UDFAbility_Rogue_Eviscerate.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Rogue_Eviscerate.generated.h"

class UAnimMontage;
class UNiagaraSystem;

UCLASS()
class DUNGEONFORGED_API UDFAbility_Rogue_Eviscerate : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbility_Rogue_Eviscerate();

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Rogue|Eviscerate")
	TObjectPtr<UAnimMontage> EviscerateMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Rogue|Eviscerate", meta = (ClampMin = "0.0"))
	float MeleeRange = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Rogue|Eviscerate|VFX")
	TObjectPtr<UNiagaraSystem> EviscSlashVFX;

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

	void DoEvisc();

	/** Filled in Activate after measuring combo points. */
	int32 PointsSpentCache = 0;
};
