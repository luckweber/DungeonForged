// Source/DungeonForged/Public/GAS/Abilities/DFAbility_Sprint.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "DFAbility_Sprint.generated.h"

struct FGameplayAbilityActorInfo;
struct FGameplayEventData;
class UDFCharacterMovementComponent;
class UGameplayEffect;

UCLASS()
class DUNGEONFORGED_API UDFAbility_Sprint : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbility_Sprint();

	/** If set, CMC will not do numeric tick drain; this GE is removed on End. Otherwise only CMC::TickSprintStamina runs. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Sprint")
	TSubclassOf<UGameplayEffect> SprintStaminaDrainEffectClass;

protected:
	virtual void PostInitProperties() override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Optional: C++ cancel path; prefer IA_Sprint Completed + CancelAbilities(Ability.Movement.Sprint) from character. */
	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	/** Clears CMC + drain GE. */
	void StopSprint(const FGameplayAbilitySpecHandle& Handle, const FGameplayAbilityActorInfo& ActorInfo,
		const FGameplayAbilityActivationInfo& ActivationInfo);

	void RemoveDrainEffectFromASC(class UAbilitySystemComponent* ASC);

	/** Filled when SprintStaminaDrainEffectClass is applied. */
	FActiveGameplayEffectHandle SprintStaminaDrainHandle;
};
