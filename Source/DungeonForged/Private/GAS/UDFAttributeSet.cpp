// Source/DungeonForged/Private/GAS/UDFAttributeSet.cpp

#include "GAS/UDFAttributeSet.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Buff_Shield.h"
#include "GAS/Effects/UGE_Cooldown_Universal_SecondWind.h"
#include "GAS/UDFPassivesGASEvents.h"
#include "AbilitySystemBlueprintLibrary.h"
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

	InitCharacterLevel(1.f);
	InitStrength(10.f);
	InitIntelligence(10.f);
	InitAgility(10.f);

	InitArmor(0.f);
	InitMagicResist(0.f);

	InitCritChance(0.05f);
	InitCritMultiplier(2.0f);
	InitCooldownReduction(0.f);
	InitSpellDamageAmp(0.f);
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
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, CharacterLevel, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, Strength, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, Intelligence, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, Agility, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, MagicResist, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, CritChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, CritMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, CooldownReduction, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, SpellDamageAmp, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, MovementSpeedMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDFAttributeSet, SprintStaminaDrain, COND_None, REPNOTIFY_Always);
}

void UDFAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// Clamped “current” resources
	if (Attribute == GetHealthAttribute())
	{
		if (UAbilitySystemComponent* const ASC = GetOwningAbilitySystemComponent())
		{
			if (NewValue <= 0.f && ASC->GetOwner() && ASC->GetOwner()->HasAuthority() && FDFGameplayTags::State_Universal_SecondWindAvailable.IsValid()
				&& FDFGameplayTags::Ability_Cooldown_SecondWind.IsValid() && ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Universal_SecondWindAvailable)
				&& !ASC->HasMatchingGameplayTag(FDFGameplayTags::Ability_Cooldown_SecondWind))
			{
				const float MaxH = FMath::Max(1.f, ASC->GetNumericAttribute(GetMaxHealthAttribute()));
				NewValue = FMath::Max(1.f, MaxH * 0.25f);
				bSecondWindRescue = true;
			}
		}
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
	else if (Attribute == GetCharacterLevelAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
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
	else if (Attribute == GetSpellDamageAmpAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 3.f);
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
		if (Attribute == GetHealthAttribute() && bSecondWindRescue)
		{
			ProcessSecondWindAftermath();
		}
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
		if (UAbilitySystemComponent* const ASC = GetOwningAbilitySystemComponent())
		{
			const float Mag = Data.EvaluatedData.Magnitude;
			if (Mag < 0.f)
			{
				UDFPassivesGASEvents::DispatchHitReceived(ASC, Data.EffectSpec, -Mag);
			}
			if (Mag < 0.f && ASC->HasMatchingGameplayTag(FDFGameplayTags::State_ManaShieldActive))
			{
				const float Dmg = -Mag;
				const float CurrentMana = ASC->GetNumericAttribute(GetManaAttribute());
				const float ManaPortion = 0.7f * Dmg;
				const float ManaToDrain = FMath::Min(CurrentMana, ManaPortion);
				const float HLoss = 0.3f * Dmg + FMath::Max(0.f, ManaPortion - CurrentMana);
				if (Dmg > KINDA_SMALL_NUMBER)
				{
					const float HealthNow = GetHealth();
					const float NewHealth = FMath::Clamp(HealthNow + (Dmg - HLoss), 0.f, GetMaxHealth());
					ASC->SetNumericAttributeBase(GetHealthAttribute(), NewHealth);
					if (ManaToDrain > KINDA_SMALL_NUMBER)
					{
						ASC->ApplyModToAttribute(GetManaAttribute(), EGameplayModOp::Additive, -ManaToDrain);
					}
					if (ASC->GetNumericAttribute(GetManaAttribute()) <= KINDA_SMALL_NUMBER)
					{
						FGameplayTagContainer T;
						T.AddTag(FDFGameplayTags::State_ManaShieldActive);
						ASC->RemoveActiveEffectsWithGrantedTags(T);
					}
				}
			}
		}
		// Final clamp in case a GE bypasses PreAttributeChange edge cases
		if (UAbilitySystemComponent* const ASC2 = GetOwningAbilitySystemComponent())
		{
			const float Clamped = FMath::Clamp(GetHealth(), 0.f, GetMaxHealth());
			if (!FMath::IsNearlyEqual(Clamped, GetHealth()))
			{
				ASC2->SetNumericAttributeBase(GetHealthAttribute(), Clamped);
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

void UDFAttributeSet::ProcessSecondWindAftermath()
{
	bSecondWindRescue = false;
	UAbilitySystemComponent* const ASC = GetOwningAbilitySystemComponent();
	AActor* const Owner = ASC ? ASC->GetOwner() : nullptr;
	if (!ASC || !Owner || !Owner->HasAuthority() || !ASC->GetAvatarActor())
	{
		return;
	}
	AActor* const Av = ASC->GetAvatarActor();
	const FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
	{
		const FGameplayEffectSpecHandle S = ASC->MakeOutgoingSpec(UGE_Buff_Shield::StaticClass(), 1.f, Ctx);
		if (S.IsValid() && S.Data.IsValid())
		{
			S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, 3.f);
			ASC->ApplyGameplayEffectSpecToSelf(*S.Data.Get());
		}
	}
	{
		const FGameplayEffectSpecHandle S = ASC->MakeOutgoingSpec(UGE_Cooldown_Universal_SecondWind::StaticClass(), 1.f, Ctx);
		if (S.IsValid() && S.Data.IsValid())
		{
			S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Cooldown, 120.f);
			ASC->ApplyGameplayEffectSpecToSelf(*S.Data.Get());
		}
	}
	if (FDFGameplayTags::State_Universal_SecondWindAvailable.IsValid())
	{
		FGameplayTagContainer Granted;
		Granted.AddTag(FDFGameplayTags::State_Universal_SecondWindAvailable);
		ASC->RemoveActiveEffectsWithGrantedTags(Granted);
		ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Universal_SecondWindAvailable, 0);
	}
	bOutOfHealthBroadcasted = false;
	OnSecondWind.Broadcast();
	{
		FGameplayEventData E;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Av, FDFGameplayTags::Event_Universal_SecondWind_Activated, E);
	}
}

//~ OnReps
void UDFAttributeSet::OnRep_CharacterLevel(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDFAttributeSet, CharacterLevel, OldValue);
}
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
void UDFAttributeSet::OnRep_SpellDamageAmp(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDFAttributeSet, SpellDamageAmp, OldValue);
}
void UDFAttributeSet::OnRep_MovementSpeedMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDFAttributeSet, MovementSpeedMultiplier, OldValue);
}
void UDFAttributeSet::OnRep_SprintStaminaDrain(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDFAttributeSet, SprintStaminaDrain, OldValue);
}
