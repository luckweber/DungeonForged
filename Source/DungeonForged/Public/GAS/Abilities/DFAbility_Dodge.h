// Source/DungeonForged/Public/GAS/Abilities/DFAbility_Dodge.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "DFAbility_Dodge.generated.h"

struct FGameplayEventData;
struct FGameplayAbilityActorInfo;
class UAnimMontage;

UCLASS()
class DUNGEONFORGED_API UDFAbility_Dodge : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbility_Dodge();

	/** CDO: set Cost/Cooldown via CostGameplayEffectClass and CooldownGameplayEffectClass (e.g. GE_Cost_Dodge, GE_Cooldown_Dodge). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Dodge")
	TObjectPtr<UAnimMontage> DodgeMontage;

protected:
	virtual void PostInitProperties() override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnDodgeDurationElapsed();

	UFUNCTION()
	void OnDodgeMontageCompleted();

	UFUNCTION()
	void OnDodgeMontageCancelled();
};
