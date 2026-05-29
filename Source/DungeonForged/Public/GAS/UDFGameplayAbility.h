// Source/DungeonForged/Public/GAS/UDFGameplayAbility.h

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "UDFGameplayAbility.generated.h"

class UAbilitySystemComponent;
class UDFAttributeSet;
class UAnimMontage;

UENUM(BlueprintType)
enum class EAbilityActivationPolicy : uint8
{
	OnInputTriggered UMETA(DisplayName = "On Input Triggered (held / repeat)"),
	OnInputStarted UMETA(DisplayName = "On Input Started (press)"),
	Passive UMETA(DisplayName = "Passive (no direct input)"),
};

/**
 * Base GameplayAbility for DungeonForged.
 *
 * UGameplayAbility already has: AbilityTags, CancelAbilitiesWithTag, BlockAbilitiesWithTag
 * (set those on the CDO). AdditionalAutoMergeTags is appended to AbilityTags on the CDO in PostInit.
 */
UCLASS(Blueprintable, BlueprintType)
class DUNGEONFORGED_API UDFGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UDFGameplayAbility();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Tags")
	FGameplayTagContainer AdditionalAutoMergeTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF")
	EAbilityActivationPolicy ActivationPolicy = EAbilityActivationPolicy::OnInputTriggered;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Animation")
	TObjectPtr<UAnimMontage> AbilityMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Cost", meta = (ClampMin = "0.0"))
	float AbilityCost_Mana = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Cost", meta = (ClampMin = "0.0"))
	float AbilityCost_Stamina = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Cooldown", meta = (ClampMin = "0.0"))
	float BaseCooldown = 0.f;

	/** If true, activation requires the avatar to be an ADFBossBase (boss-only attacks). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Boss")
	bool bSourceObjectMustBeBoss = false;

	/** When true, participates in optional global ability cooldown (B11). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Cooldown")
	bool bUseGlobalAbilityCooldown = false;

	/** 0 = use @c UDFCombatTuningData::GlobalAbilityGCD. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Cooldown", meta = (ClampMin = "0.0", EditCondition = "bUseGlobalAbilityCooldown"))
	float GlobalAbilityGCDOverride = 0.f;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** Effective mana cost (override for tuning-driven costs, e.g. dodge). */
	UFUNCTION(BlueprintPure, Category = "Ability|DF|Cost")
	virtual float GetAbilityManaCost() const;

	/** Effective stamina cost (override for tuning-driven costs). */
	UFUNCTION(BlueprintPure, Category = "Ability|DF|Cost")
	virtual float GetAbilityStaminaCost() const;

	virtual bool CheckCost(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ApplyCost(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	virtual const FGameplayTagContainer* GetCooldownTags() const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** UGameplayAbility already exposes a no-arg `GetAbilitySystemComponentFromActorInfo` — this is a static helper for an arbitrary FGameplayAbilityActorInfo. */
	UFUNCTION(BlueprintPure, Category = "Ability|DF")
	static UAbilitySystemComponent* GetASCForActorInfo(const FGameplayAbilityActorInfo& ActorInfo);

	/** Resolves the montage on the ability owner's ASC; ties into GAS via UAbilitySystemGlobals (see .cpp). */
	UFUNCTION(BlueprintCallable, Category = "Ability|DF|Animation")
	float PlayAbilityMontage(float InPlayRate = 1.f, FName StartSectionName = NAME_None);

	UFUNCTION(BlueprintNativeEvent, Category = "Ability|DF", meta = (DisplayName = "On Ability Activated (After Commit)"))
	void K2_OnAbilityActivated(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo& ActorInfo,
		const FGameplayAbilityActivationInfo& ActivationInfo);

protected:
	virtual void PostInitProperties() override;

	virtual void ApplyCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	/** Built on demand for CheckCooldown / ApplyCooldown (CDO and instanced copies). */
	UPROPERTY()
	mutable FGameplayTagContainer CachedCooldownTags;

	void BuildCooldownTagContainer(FGameplayTagContainer& OutTags) const;
	void EnsureCooldownTagsCached() const;
	bool IsOwnerOnAbilityCooldown(const UAbilitySystemComponent& ASC) const;

	bool CheckSingleResourceCost(
		const FGameplayAbilityActorInfo* ActorInfo,
		const UAbilitySystemComponent& ASC,
		TSubclassOf<UGameplayEffect> CostEffectClass,
		FGameplayAttribute ResourceAttribute,
		float CostMagnitude,
		FGameplayTagContainer* OptionalRelevantTags) const;

	void ApplySingleResourceCost(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		TSubclassOf<UGameplayEffect> CostEffectClass,
		float CostMagnitude) const;
};
