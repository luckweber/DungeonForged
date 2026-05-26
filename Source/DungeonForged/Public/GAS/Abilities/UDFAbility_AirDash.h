// Source/DungeonForged/Public/GAS/Abilities/UDFAbility_AirDash.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_AirDash.generated.h"

struct FGameplayAbilityActorInfo;
struct FGameplayEventData;
class UAnimMontage;

UCLASS()
class DUNGEONFORGED_API UDFAbility_AirDash : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbility_AirDash();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|AirDash")
	TObjectPtr<UAnimMontage> AirDashMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|AirDash")
	bool bGrantIFrames = true;

protected:
	virtual void PostInitProperties() override;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnAirDashFinished();

	float GetEffectiveAirDashStaminaCost() const;
	FVector ResolveAirDashDirectionWorld() const;
};
