// Source/DungeonForged/Public/GAS/Abilities/Rogue/UDFAbility_Rogue_Vanish.h
#pragma once

#include "CoreMinimal.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_Rogue_Vanish.generated.h"

class UAnimMontage;
class UDFAttributeSet;
class UNiagaraSystem;
class AActor;
class UGameplayAbility;
class UMaterialInstanceDynamic;

UCLASS()
class DUNGEONFORGED_API UDFAbility_Rogue_Vanish : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbility_Rogue_Vanish();

	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Rogue|Vanish|VFX")
	TObjectPtr<UNiagaraSystem> VanishPuffVFX;

	/** 0-1, multiplied into mesh (WoW-style partial fade). */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|DF|Rogue|Vanish|Visual", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StealthMeshOpacity = 0.3f;

protected:
	virtual void PostInitProperties() override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION()
	void OnWaitFinished();

	UFUNCTION()
	void OnAnyAbility(UGameplayAbility* A);

	void OnHealthForVanish(float NewHealth, float MaxHealth);
	void UnbindAll();
	void RestoreMeshOpacity();
	void BreakVanish(bool bFromAttack);

	FActiveGameplayEffectHandle VanishHandle;
	float LastHealthSnapshot = 0.f;
	/** Stacked DMI for restore (slot index). */
	TArray<TObjectPtr<UMaterialInstanceDynamic>> MeshOpacities;
	bool bTearingDown = false;
	UDFAttributeSet* BoundAttr = nullptr;
};
