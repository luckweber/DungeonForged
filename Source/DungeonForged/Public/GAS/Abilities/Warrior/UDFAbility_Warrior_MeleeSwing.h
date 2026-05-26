// Source/DungeonForged/Public/GAS/Abilities/Warrior/UDFAbility_Warrior_MeleeSwing.h
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Warrior_MeleeSwing.generated.h"

struct FGameplayAbilityActorInfo;
class USoundBase;
class UNiagaraSystem;

/** GAS melee basic: selects montage from combo by CurrentComboStep, otherwise AbilityMontage. */
UCLASS()
class DUNGEONFORGED_API UDFAbility_Warrior_MeleeSwing : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbility_Warrior_MeleeSwing();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	virtual void PostInitProperties() override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnMontageEnd();

	UFUNCTION()
	void OnDirectMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY(Transient)
	bool bPlayedMontageDirect = false;

	/** Plays at trace start (AN_TraceStart), aligned with enemy AttackSound/VFX. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Warrior|FX")
	TObjectPtr<USoundBase> SwingSound = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Warrior|FX")
	TObjectPtr<UNiagaraSystem> SwingVFX = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Warrior|FX")
	FName SwingFXSocketName = FName(TEXT("weapon_end"));

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Warrior|FX")
	FVector SwingVFXScale = FVector(1.f);

	/** Plays at each damaged target (in addition to victim HitReaction VFX). */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Warrior|FX")
	TObjectPtr<USoundBase> ImpactSound = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Warrior|FX")
	TObjectPtr<UNiagaraSystem> ImpactVFX = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Warrior|FX")
	FVector ImpactVFXScale = FVector(1.f);

	void PushSwingCosmeticsToMeleeTrace(class ADFPlayerCharacter* Player) const;
};
