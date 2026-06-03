// Source/DungeonForged/Public/GAS/Abilities/DFAbility_Dodge.h
#pragma once

#include "CoreMinimal.h"
#include "Combat/DFDodgeTypes.h"
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Dodge|Unarmed")
	FDFDodgeAnimSet UnarmedAnimSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Dodge|Armed")
	FDFDodgeAnimSet ArmedAnimSet;

	/** @deprecated Legacy single montage. Final fallback when both sets fail. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Dodge|Legacy")
	TObjectPtr<UAnimMontage> DodgeMontage;

	/** Local-space input speed (cm/s) below which direction snaps to Backward. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Dodge", meta = (ClampMin = "0.0"))
	float DirectionalInputThreshold = 80.f;

	/** When montage has root motion, skip CMC MoveToForce so anim RM is not overridden. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Dodge|RootMotion")
	bool bPreferAnimRootMotion = true;

	/** Yaw-align to dodge direction before montage (recommended for per-direction roll assets). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Dodge|RootMotion")
	bool bRotateToDodgeDirection = true;

	UFUNCTION(BlueprintCallable, Category = "Ability|DF|Dodge")
	EDFDodgeDirection ResolveDodgeDirection() const;

	UFUNCTION(BlueprintCallable, Category = "Ability|DF|Dodge")
	FVector ResolveDodgeDirectionWorld() const;

	UFUNCTION(BlueprintCallable, Category = "Ability|DF|Dodge")
	UAnimMontage* ResolveDodgeMontage(EDFDodgeDirection Direction) const;

protected:
	virtual void PostInitProperties() override;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnDodgeDurationElapsed();

	UFUNCTION()
	void OnDodgeMontageCompleted();

	UFUNCTION()
	void OnDodgeMontageCancelled();

	bool IsOwnerArmed() const;

	float GetEffectiveDodgeStaminaCost() const;

	virtual float GetAbilityStaminaCost() const override;
};
