// Source/DungeonForged/Public/GAS/Abilities/Warrior/UDFAbility_Warrior_IronSkin.h
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayEffectTypes.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Warrior_IronSkin.generated.h"

UCLASS()
class DUNGEONFORGED_API UDFAbility_Warrior_IronSkin : public UDFGameplayAbility
{
	GENERATED_BODY()
public:
	UDFAbility_Warrior_IronSkin();

	/** 6s armor/MR/invuln windows — configure in asset (GE_IronSkin). */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|IronSkin")
	TSubclassOf<class UGameplayEffect> IronSkinEffect;
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|IronSkin")
	TObjectPtr<class UAnimMontage> IronSkinMontage;
	/** Shimmer on cast. */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|IronSkin")
	TObjectPtr<class UNiagaraSystem> IronSkinVFX;
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|IronSkin")
	TObjectPtr<class UNiagaraSystem> IronSkinShatterVFX;

protected:
	virtual void PostInitProperties() override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnIronSkinEffectRemoved(const FGameplayEffectRemovalInfo& InInfo);
};
