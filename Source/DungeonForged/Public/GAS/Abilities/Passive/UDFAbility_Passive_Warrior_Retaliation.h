// Source/DungeonForged/Public/GAS/Abilities/Passive/UDFAbility_Passive_Warrior_Retaliation.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/Passive/UDFAbilityPassive.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "UDFAbility_Passive_Warrior_Retaliation.generated.h"

class UNiagaraSystem;

UCLASS()
class DUNGEONFORGED_API UDFAbility_Passive_Warrior_Retaliation : public UDFAbilityPassive
{
	GENERATED_BODY()

public:
	UDFAbility_Passive_Warrior_Retaliation();

	virtual void PostInitProperties() override;

	UPROPERTY(EditDefaultsOnly, Category = "Passive|VFX")
	TObjectPtr<UNiagaraSystem> RetaliationRiposteVFX;

protected:
	virtual void OnPassiveAbilityActivated(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo& ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnHitReceived(FGameplayEventData EventData);
};
