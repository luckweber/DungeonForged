// Source/DungeonForged/Public/GAS/UDFAttributeSet.h

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "UDFAttributeSet.generated.h"

#ifndef ATTRIBUTE_ACCESSORS
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)
#endif

/** Current health and max (for UI, bars, etc.) */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnUDFHealthChanged, float /*CurrentHealth*/, float /*MaxHealth*/);

/** Current mana and max */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnUDFManaChanged, float /*CurrentMana*/, float /*MaxMana*/);

/** Fired when health reaches zero from gameplay effects (and clamped to zero) — set once per death until revived */
DECLARE_MULTICAST_DELEGATE(FOnUDFOutOfHealth);

UCLASS(Blueprintable)
class DUNGEONFORGED_API UDFAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UDFAttributeSet();

	FOnUDFHealthChanged OnHealthChanged;
	FOnUDFManaChanged OnManaChanged;
	FOnUDFOutOfHealth OnOutOfHealth;

	//~ Resources

	UPROPERTY(BlueprintReadOnly, Category = "DF|Attributes|Vitals", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UDFAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "DF|Attributes|Vitals", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UDFAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category = "DF|Attributes|Vitals", ReplicatedUsing = OnRep_Mana)
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UDFAttributeSet, Mana)

	UPROPERTY(BlueprintReadOnly, Category = "DF|Attributes|Vitals", ReplicatedUsing = OnRep_MaxMana)
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UDFAttributeSet, MaxMana)

	UPROPERTY(BlueprintReadOnly, Category = "DF|Attributes|Vitals", ReplicatedUsing = OnRep_Stamina)
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UDFAttributeSet, Stamina)

	UPROPERTY(BlueprintReadOnly, Category = "DF|Attributes|Vitals", ReplicatedUsing = OnRep_MaxStamina)
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UDFAttributeSet, MaxStamina)

	//~ Offense

	UPROPERTY(BlueprintReadOnly, Category = "DF|Attributes|Combat", ReplicatedUsing = OnRep_Strength)
	FGameplayAttributeData Strength;
	ATTRIBUTE_ACCESSORS(UDFAttributeSet, Strength)

	UPROPERTY(BlueprintReadOnly, Category = "DF|Attributes|Combat", ReplicatedUsing = OnRep_Intelligence)
	FGameplayAttributeData Intelligence;
	ATTRIBUTE_ACCESSORS(UDFAttributeSet, Intelligence)

	UPROPERTY(BlueprintReadOnly, Category = "DF|Attributes|Combat", ReplicatedUsing = OnRep_Agility)
	FGameplayAttributeData Agility;
	ATTRIBUTE_ACCESSORS(UDFAttributeSet, Agility)

	//~ Mitigation

	UPROPERTY(BlueprintReadOnly, Category = "DF|Attributes|Combat", ReplicatedUsing = OnRep_Armor)
	FGameplayAttributeData Armor;
	ATTRIBUTE_ACCESSORS(UDFAttributeSet, Armor)

	UPROPERTY(BlueprintReadOnly, Category = "DF|Attributes|Combat", ReplicatedUsing = OnRep_MagicResist)
	FGameplayAttributeData MagicResist;
	ATTRIBUTE_ACCESSORS(UDFAttributeSet, MagicResist)

	//~ Secondary

	UPROPERTY(BlueprintReadOnly, Category = "DF|Attributes|Secondary", ReplicatedUsing = OnRep_CritChance)
	FGameplayAttributeData CritChance;
	ATTRIBUTE_ACCESSORS(UDFAttributeSet, CritChance)

	UPROPERTY(BlueprintReadOnly, Category = "DF|Attributes|Secondary", ReplicatedUsing = OnRep_CritMultiplier)
	FGameplayAttributeData CritMultiplier;
	ATTRIBUTE_ACCESSORS(UDFAttributeSet, CritMultiplier)

	UPROPERTY(BlueprintReadOnly, Category = "DF|Attributes|Secondary", ReplicatedUsing = OnRep_CooldownReduction)
	FGameplayAttributeData CooldownReduction;
	ATTRIBUTE_ACCESSORS(UDFAttributeSet, CooldownReduction)

	/** Additive spell amp (0 = none, 0.3 = +30% magic damage from TimeWarp, etc.). */
	UPROPERTY(BlueprintReadOnly, Category = "DF|Attributes|Secondary", ReplicatedUsing = OnRep_SpellDamageAmp)
	FGameplayAttributeData SpellDamageAmp;
	ATTRIBUTE_ACCESSORS(UDFAttributeSet, SpellDamageAmp)

	UPROPERTY(BlueprintReadOnly, Category = "DF|Attributes|Movement", ReplicatedUsing = OnRep_MovementSpeedMultiplier)
	FGameplayAttributeData MovementSpeedMultiplier;
	ATTRIBUTE_ACCESSORS(UDFAttributeSet, MovementSpeedMultiplier)

	/** Stamina cost per second while sprinting (used with periodic sprint-drain GEs). */
	UPROPERTY(BlueprintReadOnly, Category = "DF|Attributes|Movement", ReplicatedUsing = OnRep_SprintStaminaDrain)
	FGameplayAttributeData SprintStaminaDrain;
	ATTRIBUTE_ACCESSORS(UDFAttributeSet, SprintStaminaDrain)

	//~ UAttributeSet
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Mana(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxMana(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Strength(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Intelligence(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Agility(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Armor(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MagicResist(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_CritChance(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_CritMultiplier(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_CooldownReduction(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_SpellDamageAmp(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MovementSpeedMultiplier(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_SprintStaminaDrain(const FGameplayAttributeData& OldValue);

private:
	/** If Health <= 0, applies once until Health goes above zero again. */
	void HandleOutOfHealth();

	void ClampAttributePair(const FGameplayAttribute& CurrentAttribute, const FGameplayAttribute& MaxAttribute, float& NewValue) const;
	void OnMaxPossiblyReduced(
		const FGameplayAttribute& CurrentAttribute, const FGameplayAttribute& MaxAttribute, float NewMax);
	void TryBroadcastHealth();
	void TryBroadcastMana();

	bool bOutOfHealthBroadcasted = false;
};
