// Source/DungeonForged/Private/GAS/UDFAttributeSet.cpp

#include "GAS/UDFAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

UDFAttributeSet::UDFAttributeSet()
{
	// Baseline design-time defaults; replace or override with initialization effects
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitMana(50.f);
	InitMaxMana(50.f);
	InitStamina(100.f);
	InitMaxStamina(100.f);

	InitStrength(10.f);
	InitIntelligence(10.f);
	InitAgility(10.f);

	InitArmor(0.f);
	InitMagicResist(0.f);

	InitCritChance(0.05f);
	InitCritMultiplier(2.0f);
	InitCooldownReduction(0.f);
	InitMovementSpeedMultiplier(1.f);
	InitSprintStaminaDrain(20.f);
}

void UDFAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, Strength, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, Intelligence, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, Agility, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, MagicResist, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, CritChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, CritMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, CooldownReduction, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, MovementSpeedMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, SprintStaminaDrain, COND_None, REPNOTIFY_Always);
}

void UDFAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// Clamped “current” resources
	if (Attribute == GetHealthAttribute())
	{
		ClampAttributePair(GetHealthAttribute(), GetMaxHealthAttribute(), NewValue);
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		// Max must stay positive so UI / division stays sane; Health is clamped in PostAttributeChange if max drops
		NewValue = FMath::Max(NewValue, 1.f);
	}
	else if (Attribute == GetManaAttribute())
	{
		ClampAttributePair(GetManaAttribute(), GetMaxManaAttribute(), NewValue);
	}
	else if (Attribute == GetMaxManaAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetStaminaAttribute())
	{
		ClampAttributePair(GetStaminaAttribute(), GetMaxStaminaAttribute(), NewValue);
	}
	else if (Attribute == GetMaxStaminaAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	// Offense & mitigation — non-negative
	else if (Attribute == GetStrengthAttribute() || Attribute == GetIntelligenceAttribute() || Attribute == GetAgilityAttribute() ||
		Attribute == GetArmorAttribute() || Attribute == GetMagicResistAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	// 0..1 stats
	else if (Attribute == GetCritChanceAttribute() || Attribute == GetCooldownReductionAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 1.f);
	}
	else if (Attribute == GetCritMultiplierAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
	else if (Attribute == GetMovementSpeedMultiplierAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.1f);
	}
	else if (Attribute == GetSprintStaminaDrainAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}

void UDFAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// If max drops below current, snap current to new max
	if (Attribute == GetMaxHealthAttribute())
	{
		OnMaxPossiblyReduced(GetHealthAttribute(), GetMaxHealthAttribute(), NewValue);
	}
	if (Attribute == GetMaxManaAttribute())
	{
		OnMaxPossiblyReduced(GetManaAttribute(), GetMaxManaAttribute(), NewValue);
	}
	if (Attribute == GetMaxStaminaAttribute())
	{
		OnMaxPossiblyReduced(GetStaminaAttribute(), GetMaxStaminaAttribute(), NewValue);
	}

	if (Attribute == GetHealthAttribute() || Attribute == GetMaxHealthAttribute())
	{
		TryBroadcastHealth();
		if (GetHealth() > 0.f)
		{
			bOutOfHealthBroadcasted = false;
		}
		HandleOutOfHealth();
	}
	else if (Attribute == GetManaAttribute() || Attribute == GetMaxManaAttribute())
	{
		TryBroadcastMana();
	}
}

void UDFAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// GAS: instant/duration changes are applied; react after evaluation (damage, buffs, etc.)
	// Out-of-health is detected here and in PostAttributeChange so direct sets and GEs are both covered
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// Final clamp in case a GE bypasses PreAttributeChange edge cases
		if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
		{
			const float Clamped = FMath::Clamp(GetHealth(), 0.f, GetMaxHealth());
			if (!FMath::IsNearlyEqual(Clamped, GetHealth()))
			{
				ASC->SetNumericAttributeBase(GetHealthAttribute(), Clamped);
			}
		}
		HandleOutOfHealth();
	}
}

void UDFAttributeSet::ClampAttributePair(
	const FGameplayAttribute& CurrentAttribute, const FGameplayAttribute& MaxAttribute, float& NewValue) const
{
	const UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}
	const float MaxV = ASC->GetNumericAttribute(MaxAttribute);
	NewValue = FMath::Clamp(NewValue, 0.f, FMath::Max(MaxV, 0.f));
}

void UDFAttributeSet::OnMaxPossiblyReduced(
	const FGameplayAttribute& CurrentAttribute, const FGameplayAttribute& /*MaxAttribute*/, float NewMax)
{
	if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
	{
		const float Current = ASC->GetNumericAttribute(CurrentAttribute);
		if (Current > NewMax)
		{
			ASC->SetNumericAttributeBase(CurrentAttribute, NewMax);
		}
	}
}

void UDFAttributeSet::HandleOutOfHealth()
{
	if (GetHealth() > 0.f)
	{
		return;
	}
	if (bOutOfHealthBroadcasted)
	{
		return;
	}
	bOutOfHealthBroadcasted = true;
	OnOutOfHealth.Broadcast();
}

void UDFAttributeSet::TryBroadcastHealth()
{
	OnHealthChanged.Broadcast(GetHealth(), GetMaxHealth());
}

void UDFAttributeSet::TryBroadcastMana()
{
	OnManaChanged.Broadcast(GetMana(), GetMaxMana());
}

//~ OnReps
void UDFAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDFAttributeSet, Health, OldValue);
}
void UDFAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDFAttributeSet, MaxHealth, OldValue);
}
void UDFAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDFAttributeSet, Mana, OldValue);
}
void UDFAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDFAttributeSet, MaxMana, OldValue);
}
void UDFAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDFAttributeSet, Stamina, OldValue);
}
void UDFAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDFAttributeSet, MaxStamina, OldValue);
}
void UDFAttributeSet::OnRep_Strength(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDFAttributeSet, Strength, OldValue);
}
void UDFAttributeSet::OnRep_Intelligence(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDFAttributeSet, Intelligence, OldValue);
}
void UDFAttributeSet::OnRep_Agility(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDFAttributeSet, Agility, OldValue);
}
void UDFAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDFAttributeSet, Armor, OldValue);
}
void UDFAttributeSet::OnRep_MagicResist(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDFAttributeSet, MagicResist, OldValue);
}
void UDFAttributeSet::OnRep_CritChance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDFAttributeSet, CritChance, OldValue);
}
void UDFAttributeSet::OnRep_CritMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDFAttributeSet, CritMultiplier, OldValue);
}
void UDFAttributeSet::OnRep_CooldownReduction(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDFAttributeSet, CooldownReduction, OldValue);
}
void UDFAttributeSet::OnRep_MovementSpeedMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDFAttributeSet, MovementSpeedMultiplier, OldValue);
}
void UDFAttributeSet::OnRep_SprintStaminaDrain(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDFAttributeSet, SprintStaminaDrain, OldValue);
}
