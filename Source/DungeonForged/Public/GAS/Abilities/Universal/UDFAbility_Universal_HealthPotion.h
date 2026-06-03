#pragma once
#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Universal_HealthPotion.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFAbility_Universal_HealthPotion : public UDFGameplayAbility
{
	GENERATED_BODY()
public:
	UDFAbility_Universal_HealthPotion();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Universal", meta = (ClampMin = "1"))
	int32 StartingCharges = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Universal", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float HealFractionOfMaxHealth = 0.4f;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void PostInitProperties() override;

	void BroadcastCharges(UAbilitySystemComponent& ASC, const FGameplayAbilitySpecHandle Handle) const;
};
