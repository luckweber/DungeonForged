// Source/DungeonForged/Public/GAS/Abilities/Warrior/UDFAbility_Warrior_WarCry.h
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Warrior_WarCry.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDFWarriorWarCry, AActor* /*Instigator*/, FVector /*WorldLocation*/);

UCLASS()
class DUNGEONFORGED_API UDFAbility_Warrior_WarCry : public UDFGameplayAbility
{
	GENERATED_BODY()
public:
	UDFAbility_Warrior_WarCry();
	static FOnDFWarriorWarCry OnWarriorWarCry;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|WarCry")
	TObjectPtr<class UAnimMontage> WarCryMontage;
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|WarCry")
	TObjectPtr<class UNiagaraSystem> WarCryNiagara;
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|WarCry")
	TObjectPtr<class USoundBase> WarCrySFX;
	/** +Strength from Data_Magnitude, Data_Duration. If null, uses UGE_Buff_DamageUp. */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|WarCry")
	TSubclassOf<class UGameplayEffect> WarCryDamageBuffClass;
	/** +MovementSpeed scale (typ. +0.15). If null, uses UGE_Buff_Speed (asset is +0.3). */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|WarCry")
	TSubclassOf<class UGameplayEffect> WarCrySpeedBuffClass;
	UPROPERTY(EditDefaultsOnly, Category = "DF|Ability|Warrior|WarCry", meta = (ClampMin = "0.0"))
	float WarCryRange = 800.f;

protected:
	virtual void PostInitProperties() override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	UFUNCTION()
	void OnMontageBlended();
	UFUNCTION()
	void OnMontageFinished();
	bool bWarCryApplied = false;
};
