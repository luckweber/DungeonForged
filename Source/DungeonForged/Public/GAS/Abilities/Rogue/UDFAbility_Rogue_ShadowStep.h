// Source/DungeonForged/Public/GAS/Abilities/Rogue/UDFAbility_Rogue_ShadowStep.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "UDFAbility_Rogue_ShadowStep.generated.h"

class UNiagaraSystem;

UCLASS()
class DUNGEONFORGED_API UDFAbility_Rogue_ShadowStep : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbility_Rogue_ShadowStep();

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Rogue|ShadowStep", meta = (ClampMin = "0.0"))
	float DistanceBehind = 120.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Rogue|ShadowStep", meta = (ClampMin = "0.0"))
	float ZOffset = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Rogue|ShadowStep|VFX")
	TObjectPtr<UNiagaraSystem> TrailVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Rogue|ShadowStep|VFX")
	TObjectPtr<UNiagaraSystem> ArrivalVFX;

protected:
	virtual void PostInitProperties() override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	FActiveGameplayEffectHandle ShadowStepEffectHandle;
};
