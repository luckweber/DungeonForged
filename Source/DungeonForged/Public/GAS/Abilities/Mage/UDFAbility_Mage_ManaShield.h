// Source/DungeonForged/Public/GAS/Abilities/Mage/UDFAbility_Mage_ManaShield.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Mage_ManaShield.generated.h"

struct FGameplayEventData;
class UGameplayEffect;
struct FActiveGameplayEffectHandle;
class UNiagaraComponent;
class UNiagaraSystem;

UCLASS()
class DUNGEONFORGED_API UDFAbility_Mage_ManaShield : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbility_Mage_ManaShield();

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|ManaShield")
	TSubclassOf<UGameplayEffect> ManaShieldActiveClass;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|ManaShield")
	TObjectPtr<UNiagaraSystem> ShieldLoopNiagara = nullptr;

	/** Optional: Niagara when shield is broken (mana to 0). */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Mage|ManaShield")
	TObjectPtr<UNiagaraSystem> ShieldBreakNiagara = nullptr;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnShieldRemoved();

	virtual void PostInitProperties() override;

private:
	TWeakObjectPtr<UNiagaraComponent> ActiveShieldNiagara;
	FActiveGameplayEffectHandle ActiveShieldHandle;
};
