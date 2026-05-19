// Source/DungeonForged/Public/GAS/Abilities/Warrior/UDFAbility_Warrior_HeavyAttack.h
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Warrior_HeavyAttack.generated.h"

struct FGameplayAbilityActorInfo;

/** Charged primary melee (hold LMB). Montage from @c UDFComboComponent::ResolveHeavyAttackMontage. */
UCLASS()
class DUNGEONFORGED_API UDFAbility_Warrior_HeavyAttack : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbility_Warrior_HeavyAttack();

protected:
	virtual void PostInitProperties() override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnMontageEnd();
};
