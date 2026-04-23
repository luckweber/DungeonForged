// Source/DungeonForged/Public/GAS/Abilities/Passive/UDFAbilityPassive.h
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbilityPassive.generated.h"

/**
 * Base for DungeonForged passives: auto-activates on grant, no cost/montage (see OnGiveAbility / ActivateAbility overrides).
 * Subtypes implement Aura-style persistent GE and/or event-driven loops; store ActiveEffectHandle and clear in OnRemoveAbility.
 */
UCLASS(Abstract, Blueprintable)
class DUNGEONFORGED_API UDFAbilityPassive : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbilityPassive();

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

protected:
	/** Invoked after UGameplayAbility::Activate (skips UDF's Commit + montage). */
	virtual void OnPassiveAbilityActivated(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo& ActivationInfo,
		const FGameplayEventData* TriggerEventData) {}
};
