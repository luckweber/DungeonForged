// Source/DungeonForged/Public/GAS/Abilities/Mage/UDFAbility_Mage_TimeWarp.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Mage_TimeWarp.generated.h"

struct FGameplayEventData;
class UGameplayEffect;

UCLASS()
class DUNGEONFORGED_API UDFAbility_Mage_TimeWarp : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbility_Mage_TimeWarp();

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|TimeWarp")
	TSubclassOf<UGameplayEffect> TimeWarpHasteClass;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|TimeWarp")
	TObjectPtr<UAnimMontage> TimeWarpMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|TimeWarp")
	TObjectPtr<class UNiagaraSystem> SelfTimeNiagara = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|TimeWarp")
	TObjectPtr<UNiagaraSystem> PulseNiagara = nullptr;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnMontageDone();

	void ApplyTimeWarpEffects();

	virtual void PostInitProperties() override;
};
