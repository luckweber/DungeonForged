// Source/DungeonForged/Private/GAS/DFGameplayTags.cpp
#include "GAS/DFGameplayTags.h"
#include "GameplayTagsManager.h"

#define DF_TAG(Member) FDFGameplayTags::Member = M.AddNativeGameplayTag

FGameplayTag FDFGameplayTags::Ability_Attack_Melee;
FGameplayTag FDFGameplayTags::Ability_Attack_Ranged;
FGameplayTag FDFGameplayTags::Ability_Fire_Fireball;
FGameplayTag FDFGameplayTags::Ability_Fire_FlameStrike;
FGameplayTag FDFGameplayTags::Ability_Ice_FrostBolt;
FGameplayTag FDFGameplayTags::Ability_Ice_Blizzard;
FGameplayTag FDFGameplayTags::Ability_Physical_Charge;
FGameplayTag FDFGameplayTags::Ability_Physical_Whirlwind;
FGameplayTag FDFGameplayTags::Ability_Slot_1;
FGameplayTag FDFGameplayTags::Ability_Slot_2;
FGameplayTag FDFGameplayTags::Ability_Slot_3;
FGameplayTag FDFGameplayTags::Ability_Slot_4;
FGameplayTag FDFGameplayTags::Ability_Passive_HealthRegen;
FGameplayTag FDFGameplayTags::Ability_Passive_ManaRegen;
FGameplayTag FDFGameplayTags::State_Dead;
FGameplayTag FDFGameplayTags::State_Stunned;
FGameplayTag FDFGameplayTags::State_Rooted;
FGameplayTag FDFGameplayTags::State_Silenced;
FGameplayTag FDFGameplayTags::State_Invulnerable;
FGameplayTag FDFGameplayTags::State_Targeting;
FGameplayTag FDFGameplayTags::State_InCombat;
FGameplayTag FDFGameplayTags::State_Sprinting;
FGameplayTag FDFGameplayTags::State_Dodging;
FGameplayTag FDFGameplayTags::Effect_DoT_Fire;
FGameplayTag FDFGameplayTags::Effect_DoT_Poison;
FGameplayTag FDFGameplayTags::Effect_DoT_Bleed;
FGameplayTag FDFGameplayTags::Effect_Buff_Speed;
FGameplayTag FDFGameplayTags::Effect_Buff_DamageUp;
FGameplayTag FDFGameplayTags::Effect_Buff_Shield;
FGameplayTag FDFGameplayTags::Effect_Debuff_Slow;
FGameplayTag FDFGameplayTags::Effect_Debuff_Weaken;
FGameplayTag FDFGameplayTags::Effect_Debuff_ArmorBreak;
FGameplayTag FDFGameplayTags::Event_Ability_Fire_Launch;
FGameplayTag FDFGameplayTags::Event_Ability_Melee_Hit;
FGameplayTag FDFGameplayTags::Event_Ability_Montage_End;
FGameplayTag FDFGameplayTags::Data_Damage;
FGameplayTag FDFGameplayTags::Data_Healing;
FGameplayTag FDFGameplayTags::Data_Duration;
FGameplayTag FDFGameplayTags::Data_Knockback;
FGameplayTag FDFGameplayTags::UI_MenuOpen;
FGameplayTag FDFGameplayTags::UI_InventoryOpen;
FGameplayTag FDFGameplayTags::UI_AbilityMenuOpen;

static bool GDF_RegisteredNativeGameplayTags = false;

FGameplayTag FDFGameplayTags::ResolveDataDamageTag()
{
	if (FDFGameplayTags::Data_Damage.IsValid())
	{
		return FDFGameplayTags::Data_Damage;
	}
	return FGameplayTag::RequestGameplayTag(FName("Data.Damage"), false);
}

FGameplayTag FDFGameplayTags::ResolveDataKnockbackTag()
{
	if (FDFGameplayTags::Data_Knockback.IsValid())
	{
		return FDFGameplayTags::Data_Knockback;
	}
	return FGameplayTag::RequestGameplayTag(FName("Data.Knockback"), false);
}

void FDFGameplayTags::RegisterGameplayTags()
{
	if (GDF_RegisteredNativeGameplayTags)
	{
		return;
	}

	UGameplayTagsManager& M = UGameplayTagsManager::Get();

	DF_TAG(Ability_Attack_Melee)(FName("Ability.Attack.Melee"), FString("Melee auto-attack or weapon swing."));
	DF_TAG(Ability_Attack_Ranged)(FName("Ability.Attack.Ranged"), FString("Ranged basic attack or projectile-based attack."));
	DF_TAG(Ability_Fire_Fireball)(FName("Ability.Fire.Fireball"), FString("Fireball ability."));
	DF_TAG(Ability_Fire_FlameStrike)(FName("Ability.Fire.FlameStrike"), FString("Line or cone fire strike."));
	DF_TAG(Ability_Ice_FrostBolt)(FName("Ability.Ice.FrostBolt"), FString("Single-target ice bolt."));
	DF_TAG(Ability_Ice_Blizzard)(FName("Ability.Ice.Blizzard"), FString("AoE ice storm."));
	DF_TAG(Ability_Physical_Charge)(FName("Ability.Physical.Charge"), FString("Rush / charge forward."));
	DF_TAG(Ability_Physical_Whirlwind)(FName("Ability.Physical.Whirlwind"), FString("Spinning blade / AoE physical."));
	DF_TAG(Ability_Slot_1)(FName("Ability.Slot.1"), FString("Input slot 1 mapping."));
	DF_TAG(Ability_Slot_2)(FName("Ability.Slot.2"), FString("Input slot 2 mapping."));
	DF_TAG(Ability_Slot_3)(FName("Ability.Slot.3"), FString("Input slot 3 mapping."));
	DF_TAG(Ability_Slot_4)(FName("Ability.Slot.4"), FString("Input slot 4 mapping."));
	DF_TAG(Ability_Passive_HealthRegen)(FName("Ability.Passive.HealthRegen"), FString("Passive health regeneration."));
	DF_TAG(Ability_Passive_ManaRegen)(FName("Ability.Passive.ManaRegen"), FString("Passive mana regeneration."));

	DF_TAG(State_Dead)(FName("State.Dead"), FString("Actor is dead; cannot act."));
	DF_TAG(State_Stunned)(FName("State.Stunned"), FString("Stun — no actions."));
	DF_TAG(State_Rooted)(FName("State.Rooted"), FString("Root — cannot move."));
	DF_TAG(State_Silenced)(FName("State.Silenced"), FString("Cannot use abilities."));
	DF_TAG(State_Invulnerable)(FName("State.Invulnerable"), FString("Immune to damage / harmful effects."));
	DF_TAG(State_Targeting)(FName("State.Targeting"), FString("In targeting mode (e.g. ability aim)."));
	DF_TAG(State_InCombat)(FName("State.InCombat"), FString("In combat; regen, UI, music."));
	DF_TAG(State_Sprinting)(FName("State.Sprinting"), FString("Sprint movement."));
	DF_TAG(State_Dodging)(FName("State.Dodging"), FString("Invuln window / roll."));

	DF_TAG(Effect_DoT_Fire)(FName("Effect.DoT.Fire"), FString("Damage over time — fire."));
	DF_TAG(Effect_DoT_Poison)(FName("Effect.DoT.Poison"), FString("DoT — poison."));
	DF_TAG(Effect_DoT_Bleed)(FName("Effect.DoT.Bleed"), FString("DoT — bleed."));
	DF_TAG(Effect_Buff_Speed)(FName("Effect.Buff.Speed"), FString("Movement or attack speed buff."));
	DF_TAG(Effect_Buff_DamageUp)(FName("Effect.Buff.DamageUp"), FString("Damage amplification."));
	DF_TAG(Effect_Buff_Shield)(FName("Effect.Buff.Shield"), FString("Absorption shield."));
	DF_TAG(Effect_Debuff_Slow)(FName("Effect.Debuff.Slow"), FString("Movement slow."));
	DF_TAG(Effect_Debuff_Weaken)(FName("Effect.Debuff.Weaken"), FString("Reduced outgoing damage or stats."));
	DF_TAG(Effect_Debuff_ArmorBreak)(FName("Effect.Debuff.ArmorBreak"), FString("Reduced mitigation."));

	DF_TAG(Event_Ability_Fire_Launch)(FName("Event.Ability.Fire.Launch"), FString("Anim notify: fire projectile release."));
	DF_TAG(Event_Ability_Melee_Hit)(FName("Event.Ability.Melee.Hit"), FString("Melee impact / damage window."));
	DF_TAG(Event_Ability_Montage_End)(FName("Event.Ability.Montage.End"), FString("Ability montage finished."));

	DF_TAG(Data_Damage)(FName("Data.Damage"), FString("SetByCaller / damage magnitude."));
	DF_TAG(Data_Healing)(FName("Data.Healing"), FString("SetByCaller — healing."));
	DF_TAG(Data_Duration)(FName("Data.Duration"), FString("SetByCaller — duration."));
	DF_TAG(Data_Knockback)(FName("Data.Knockback"), FString("SetByCaller — melee knockback / impulse scale."));

	DF_TAG(UI_MenuOpen)(FName("UI.MenuOpen"), FString("Pause / main menu visible."));
	DF_TAG(UI_InventoryOpen)(FName("UI.InventoryOpen"), FString("Inventory screen up."));
	DF_TAG(UI_AbilityMenuOpen)(FName("UI.AbilityMenuOpen"), FString("Ability offer / roguelike pick UI."));

	GDF_RegisteredNativeGameplayTags = true;
}

#undef DF_TAG
