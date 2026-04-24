// Source/DungeonForged/Private/GAS/DFGameplayTags.cpp
#include "GAS/DFGameplayTags.h"
#include "GameplayTagsManager.h"

#define DF_TAG(Member) FDFGameplayTags::Member = M.AddNativeGameplayTag

FGameplayTag FDFGameplayTags::Ability_Parent;
FGameplayTag FDFGameplayTags::Ability_Attack;
FGameplayTag FDFGameplayTags::Ability_Fire;
FGameplayTag FDFGameplayTags::Ability_Ice;
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
FGameplayTag FDFGameplayTags::Ability_Passive_Warrior_Fortitude;
FGameplayTag FDFGameplayTags::Ability_Passive_Warrior_Retaliation;
FGameplayTag FDFGameplayTags::Ability_Passive_Mage_ArcaneMastery;
FGameplayTag FDFGameplayTags::Ability_Passive_Mage_ManaVortex;
FGameplayTag FDFGameplayTags::Ability_Passive_Rogue_Predator;
FGameplayTag FDFGameplayTags::Ability_Passive_Rogue_BleedMastery;
FGameplayTag FDFGameplayTags::Ability_Movement_Sprint;
FGameplayTag FDFGameplayTags::Ability_Movement_Dodge;
FGameplayTag FDFGameplayTags::Ability_Warrior_ShieldBash;
FGameplayTag FDFGameplayTags::Ability_Warrior_WarCry;
FGameplayTag FDFGameplayTags::Ability_Warrior_Whirlwind;
FGameplayTag FDFGameplayTags::Ability_Warrior_IronSkin;
FGameplayTag FDFGameplayTags::Ability_Warrior_Charge;
FGameplayTag FDFGameplayTags::Ability_Warrior_Execute;
FGameplayTag FDFGameplayTags::Ability_Failed_Range;
FGameplayTag FDFGameplayTags::Ability_Mage;
FGameplayTag FDFGameplayTags::Ability_Mage_FrostBolt;
FGameplayTag FDFGameplayTags::Ability_Mage_BlizzardStorm;
FGameplayTag FDFGameplayTags::Ability_Mage_ArcaneBarrage;
FGameplayTag FDFGameplayTags::Ability_Mage_TimeWarp;
FGameplayTag FDFGameplayTags::Ability_Mage_ManaShield;
FGameplayTag FDFGameplayTags::Ability_Mage_Teleport;
FGameplayTag FDFGameplayTags::Ability_Rogue;
FGameplayTag FDFGameplayTags::Ability_Rogue_Backstab;
FGameplayTag FDFGameplayTags::Ability_Rogue_FanOfKnives;
FGameplayTag FDFGameplayTags::Ability_Rogue_ShadowStep;
FGameplayTag FDFGameplayTags::Ability_Rogue_Eviscerate;
FGameplayTag FDFGameplayTags::Ability_Rogue_Vanish;
FGameplayTag FDFGameplayTags::Ability_Rogue_SmokeScreen;
FGameplayTag FDFGameplayTags::Ability_Universal_HealthPotion;
FGameplayTag FDFGameplayTags::Ability_Universal_SecondWind;
FGameplayTag FDFGameplayTags::Ability_Universal_BattleHymn;
FGameplayTag FDFGameplayTags::Ability_Universal_Siphon;
FGameplayTag FDFGameplayTags::Ability_Universal_Berserk;
FGameplayTag FDFGameplayTags::Ability_Universal_CallLightning;
FGameplayTag FDFGameplayTags::Ability_Cooldown;
FGameplayTag FDFGameplayTags::Ability_Cooldown_HealthPotion;
FGameplayTag FDFGameplayTags::Ability_Cooldown_SecondWind;
FGameplayTag FDFGameplayTags::Ability_Cooldown_BattleHymn;
FGameplayTag FDFGameplayTags::Ability_Cooldown_Siphon;
FGameplayTag FDFGameplayTags::Ability_Cooldown_Berserk;
FGameplayTag FDFGameplayTags::Ability_Cooldown_CallLightning;
FGameplayTag FDFGameplayTags::Ability_Boss_TerrorShout;
FGameplayTag FDFGameplayTags::Ability_Boss_MeteorStrike;
FGameplayTag FDFGameplayTags::Ability_Boss_VoidBarrier;
FGameplayTag FDFGameplayTags::Ability_Boss_PhaseTransitionSlam;
FGameplayTag FDFGameplayTags::Ability_Boss_EnragePulse;
FGameplayTag FDFGameplayTags::Ability_Cooldown_Boss_TerrorShout;
FGameplayTag FDFGameplayTags::Ability_Cooldown_Boss_MeteorStrike;
FGameplayTag FDFGameplayTags::Ability_Cooldown_Boss_VoidBarrier;
FGameplayTag FDFGameplayTags::Ability_Cooldown_Boss_EnragePulse;
FGameplayTag FDFGameplayTags::Event_Boss_PhaseErupt;
FGameplayTag FDFGameplayTags::Event_Boss_WhiteFlash;
FGameplayTag FDFGameplayTags::Effect_Debuff_Terrified;
FGameplayTag FDFGameplayTags::State_Universal_SecondWindAvailable;
FGameplayTag FDFGameplayTags::State_Berserk;
FGameplayTag FDFGameplayTags::Effect_Buff_BattleHymn;
FGameplayTag FDFGameplayTags::Event_Universal_Siphon_Trace;
FGameplayTag FDFGameplayTags::Event_Universal_HealthPotion_Charges;
FGameplayTag FDFGameplayTags::Event_Universal_SecondWind_Activated;
FGameplayTag FDFGameplayTags::Event_Ability_Mage_FrostTrace;
FGameplayTag FDFGameplayTags::Event_Ability_Mage_ArcaneTrace;
FGameplayTag FDFGameplayTags::State_ManaShieldActive;
FGameplayTag FDFGameplayTags::Effect_DoT_Frost;
FGameplayTag FDFGameplayTags::Buff_Mage_TimeWarpHaste;
FGameplayTag FDFGameplayTags::Event_Ability_Whirlwind_Tick;
FGameplayTag FDFGameplayTags::Event_Warrior_ShieldBash_Trace;
FGameplayTag FDFGameplayTags::Event_Warrior_Execute_Trace;
FGameplayTag FDFGameplayTags::Event_Warrior_WarCry;
FGameplayTag FDFGameplayTags::Equipment_OffHand_Shield;
FGameplayTag FDFGameplayTags::State_Spinning;
FGameplayTag FDFGameplayTags::State_Dead;
FGameplayTag FDFGameplayTags::State_Stunned;
FGameplayTag FDFGameplayTags::State_Rooted;
FGameplayTag FDFGameplayTags::State_Silenced;
FGameplayTag FDFGameplayTags::State_Invulnerable;
FGameplayTag FDFGameplayTags::State_Targeting;
FGameplayTag FDFGameplayTags::State_InCombat;
FGameplayTag FDFGameplayTags::State_Sprinting;
FGameplayTag FDFGameplayTags::State_Dodging;
FGameplayTag FDFGameplayTags::State_Attacking;
FGameplayTag FDFGameplayTags::State_Casting;
FGameplayTag FDFGameplayTags::State_CCIgnore;
FGameplayTag FDFGameplayTags::State_BossEnraged;
FGameplayTag FDFGameplayTags::State_Spawned_Boss;
FGameplayTag FDFGameplayTags::State_BossVulnerable;
FGameplayTag FDFGameplayTags::State_Invisible;
FGameplayTag FDFGameplayTags::State_Stealthed;
FGameplayTag FDFGameplayTags::State_Concealed;
FGameplayTag FDFGameplayTags::Buff_Rogue_Ambush;
FGameplayTag FDFGameplayTags::Buff_Rogue_KillingSpree;
FGameplayTag FDFGameplayTags::Event_Boss_DoorLock;
FGameplayTag FDFGameplayTags::Event_Boss_IntroComplete;
FGameplayTag FDFGameplayTags::Event_Stealth_Entered;
FGameplayTag FDFGameplayTags::Event_Rogue_Backstab_Trace;
FGameplayTag FDFGameplayTags::Event_Rogue_FanOfKnives_Trace;
FGameplayTag FDFGameplayTags::Event_Rogue_Eviscerate_Trace;
FGameplayTag FDFGameplayTags::Effect_Damage_Physical;
FGameplayTag FDFGameplayTags::Effect_Damage_Magic;
FGameplayTag FDFGameplayTags::Effect_Damage_True;
FGameplayTag FDFGameplayTags::Effect_Critical;
FGameplayTag FDFGameplayTags::Data_CriticalHit;
FGameplayTag FDFGameplayTags::State_Combat_Block;
FGameplayTag FDFGameplayTags::Effect_DoT_Fire;
FGameplayTag FDFGameplayTags::Effect_DoT_Poison;
FGameplayTag FDFGameplayTags::Effect_DoT_Bleed;
FGameplayTag FDFGameplayTags::Effect_Buff_Speed;
FGameplayTag FDFGameplayTags::Effect_Buff_DamageUp;
FGameplayTag FDFGameplayTags::Effect_Buff_Shield;
FGameplayTag FDFGameplayTags::Effect_Debuff_Slow;
FGameplayTag FDFGameplayTags::Effect_Debuff_Weaken;
FGameplayTag FDFGameplayTags::Effect_Debuff_ArmorBreak;
FGameplayTag FDFGameplayTags::Effect_Debuff_Blinded;
FGameplayTag FDFGameplayTags::Effect_Element_Fire;
FGameplayTag FDFGameplayTags::Effect_Element_Ice;
FGameplayTag FDFGameplayTags::Effect_Element_Water;
FGameplayTag FDFGameplayTags::Effect_Element_Lightning;
FGameplayTag FDFGameplayTags::Effect_Element_Earth;
FGameplayTag FDFGameplayTags::Effect_Element_Arcane;
FGameplayTag FDFGameplayTags::Effect_Element_Physical;
FGameplayTag FDFGameplayTags::Effect_Element_True;
FGameplayTag FDFGameplayTags::State_Elemental_Wet;
FGameplayTag FDFGameplayTags::Effect_Reaction_Melt;
FGameplayTag FDFGameplayTags::Effect_Reaction_Steam;
FGameplayTag FDFGameplayTags::Effect_Reaction_Electrocute;
FGameplayTag FDFGameplayTags::Event_Ability_Fire_Launch;
FGameplayTag FDFGameplayTags::Event_Ability_Melee_Hit;
FGameplayTag FDFGameplayTags::Event_Ability_Montage_End;
FGameplayTag FDFGameplayTags::Event_Hit_Received;
FGameplayTag FDFGameplayTags::Event_Ability_Kill;
FGameplayTag FDFGameplayTags::Event_Passive_Rogue_BleedApplied;
FGameplayTag FDFGameplayTags::Data_Damage;
FGameplayTag FDFGameplayTags::Data_Healing;
FGameplayTag FDFGameplayTags::Data_Duration;
FGameplayTag FDFGameplayTags::Data_Cost;
FGameplayTag FDFGameplayTags::Data_Cooldown;
FGameplayTag FDFGameplayTags::Data_Magnitude;
FGameplayTag FDFGameplayTags::Data_Knockback;
FGameplayTag FDFGameplayTags::Effect_Buff_LevelStatScaling;
FGameplayTag FDFGameplayTags::Data_LevelUp_MaxHealthAdd;
FGameplayTag FDFGameplayTags::Data_LevelUp_MaxManaAdd;
FGameplayTag FDFGameplayTags::Data_LevelUp_StrengthAdd;
FGameplayTag FDFGameplayTags::Data_LevelUp_IntelligenceAdd;
FGameplayTag FDFGameplayTags::Data_LevelUp_AgilityAdd;
FGameplayTag FDFGameplayTags::Character_Level;
FGameplayTag FDFGameplayTags::UI_MenuOpen;
FGameplayTag FDFGameplayTags::UI_InventoryOpen;
FGameplayTag FDFGameplayTags::UI_AbilityMenuOpen;
FGameplayTag FDFGameplayTags::UI_CinematicLock;

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

	DF_TAG(Ability_Parent)(FName("Ability"), FString("Root tag for all abilities (cancel / filter)."));
	DF_TAG(Ability_Attack)(FName("Ability.Attack"), FString("Parent of melee/ranged basic attacks and strike abilities."));
	DF_TAG(Ability_Fire)(FName("Ability.Fire"), FString("Fire magic ability subtree."));
	DF_TAG(Ability_Ice)(FName("Ability.Ice"), FString("Ice magic ability subtree."));
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
	DF_TAG(Ability_Passive_Warrior_Fortitude)(FName("Ability.Passive.Warrior.Fortitude"), FString("Max HP/Armor; wounded enrage (periodic)."));
	DF_TAG(Ability_Passive_Warrior_Retaliation)(FName("Ability.Passive.Warrior.Retaliation"), FString("Thorns-like riposte on big hits."));
	DF_TAG(Ability_Passive_Mage_ArcaneMastery)(FName("Ability.Passive.Mage.ArcaneMastery"), FString("Int / CDR / max mana from level."));
	DF_TAG(Ability_Passive_Mage_ManaVortex)(FName("Ability.Passive.Mage.ManaVortex"), FString("Ability kills restore mana (event)."));
	DF_TAG(Ability_Passive_Rogue_Predator)(FName("Ability.Passive.Rogue.Predator"), FString("Move speed and crit from predator stance."));
	DF_TAG(Ability_Passive_Rogue_BleedMastery)(FName("Ability.Passive.Rogue.BleedMastery"), FString("Augment bleeds, armor break on multi-bleed."));
	DF_TAG(Ability_Movement_Sprint)(FName("Ability.Movement.Sprint"), FString("Sprint (hold) movement ability."));
	DF_TAG(Ability_Movement_Dodge)(FName("Ability.Movement.Dodge"), FString("Dodge / roll."));

	DF_TAG(Ability_Warrior_ShieldBash)(FName("Ability.Warrior.ShieldBash"), FString("Warrior: shield bash."));
	DF_TAG(Ability_Warrior_WarCry)(FName("Ability.Warrior.WarCry"), FString("Warrior: war cry AOE buff."));
	DF_TAG(Ability_Warrior_Whirlwind)(FName("Ability.Warrior.Whirlwind"), FString("Warrior: channeled spin."));
	DF_TAG(Ability_Warrior_IronSkin)(FName("Ability.Warrior.IronSkin"), FString("Warrior: defensive damage reduction."));
	DF_TAG(Ability_Warrior_Charge)(FName("Ability.Warrior.Charge"), FString("Warrior: charging rush to target."));
	DF_TAG(Ability_Warrior_Execute)(FName("Ability.Warrior.Execute"), FString("Warrior: execute on low health."));
	DF_TAG(Ability_Failed_Range)(FName("Ability.Failed.Range"), FString("Ability failed: range (UI / feedback)."));
	DF_TAG(Ability_Mage)(FName("Ability.Mage"), FString("Mage class abilities."));
	DF_TAG(Ability_Mage_FrostBolt)(FName("Ability.Mage.FrostBolt"), FString("Mage: frost bolt slow/freeze."));
	DF_TAG(Ability_Mage_BlizzardStorm)(FName("Ability.Mage.BlizzardStorm"), FString("Mage: ground AOE blizzard DoT."));
	DF_TAG(Ability_Mage_ArcaneBarrage)(FName("Ability.Mage.ArcaneBarrage"), FString("Mage: arcane charge missiles."));
	DF_TAG(Ability_Mage_TimeWarp)(FName("Ability.Mage.TimeWarp"), FString("Mage: reset cooldowns + haste window."));
	DF_TAG(Ability_Mage_ManaShield)(FName("Ability.Mage.ManaShield"), FString("Mage: mana absorbs damage."));
	DF_TAG(Ability_Mage_Teleport)(FName("Ability.Mage.Teleport"), FString("Mage: blink / spellsteal bump."));
	DF_TAG(Ability_Rogue)(FName("Ability.Rogue"), FString("Rogue / Assassin class abilities."));
	DF_TAG(Ability_Rogue_Backstab)(FName("Ability.Rogue.Backstab"), FString("Rogue: positional backstab (builder)."));
	DF_TAG(Ability_Rogue_FanOfKnives)(FName("Ability.Rogue.FanOfKnives"), FString("Rogue: 360° knife volley (AoE)."));
	DF_TAG(Ability_Rogue_ShadowStep)(FName("Ability.Rogue.ShadowStep"), FString("Rogue: blink behind target."));
	DF_TAG(Ability_Rogue_Eviscerate)(FName("Ability.Rogue.Eviscerate"), FString("Rogue: finisher, consumes combo points."));
	DF_TAG(Ability_Rogue_Vanish)(FName("Ability.Rogue.Vanish"), FString("Rogue: long combat stealth break."));
	DF_TAG(Ability_Rogue_SmokeScreen)(FName("Ability.Rogue.SmokeScreen"), FString("Rogue: thrown smoke cloud."));
	DF_TAG(Ability_Universal_HealthPotion)(FName("Ability.Universal.HealthPotion"), FString("Consumable potion: charges, heal %."));
	DF_TAG(Ability_Universal_SecondWind)(FName("Ability.Universal.SecondWind"), FString("One-shot cheat death: revive at 25% HP."));
	DF_TAG(Ability_Universal_BattleHymn)(FName("Ability.Universal.BattleHymn"), FString("All-stats burst + CDR + crit."));
	DF_TAG(Ability_Universal_Siphon)(FName("Ability.Universal.Siphon"), FString("True damage + lifesteal."));
	DF_TAG(Ability_Universal_Berserk)(FName("Ability.Universal.Berserk"), FString("Berserk mode: huge stats, self-bleed."));
	DF_TAG(Ability_Universal_CallLightning)(FName("Ability.Universal.CallLightning"), FString("Ground targeted lightning strike."));
	DF_TAG(Ability_Cooldown)(FName("Ability.Cooldown"), FString("On cooldown GameplayEffect asset tags; children per skill."));
	DF_TAG(Ability_Cooldown_HealthPotion)(FName("Ability.Cooldown.HealthPotion"), FString("Potion 30s CD."));
	DF_TAG(Ability_Cooldown_SecondWind)(FName("Ability.Cooldown.SecondWind"), FString("Second Wind re-proc block 120s."));
	DF_TAG(Ability_Cooldown_BattleHymn)(FName("Ability.Cooldown.BattleHymn"), FString("Battle Hymn CD."));
	DF_TAG(Ability_Cooldown_Siphon)(FName("Ability.Cooldown.Siphon"), FString("Siphon CD."));
	DF_TAG(Ability_Cooldown_Berserk)(FName("Ability.Cooldown.Berserk"), FString("Berserk CD."));
	DF_TAG(Ability_Cooldown_CallLightning)(FName("Ability.Cooldown.CallLightning"), FString("Call Lightning CD."));
	DF_TAG(Ability_Boss_TerrorShout)(FName("Ability.Boss.TerrorShout"), FString("Boss: fear shout AOE + debuff."));
	DF_TAG(Ability_Boss_MeteorStrike)(FName("Ability.Boss.MeteorStrike"), FString("Boss: telegraphed player meteor (phase 2+)."));
	DF_TAG(Ability_Boss_VoidBarrier)(FName("Ability.Boss.VoidBarrier"), FString("Boss: void invuln + orbs (phase 2+)."));
	DF_TAG(Ability_Boss_PhaseTransitionSlam)(FName("Ability.Boss.PhaseTransitionSlam"), FString("Boss: unavoidable inter-phase slam (cinematic)."));
	DF_TAG(Ability_Boss_EnragePulse)(FName("Ability.Boss.EnragePulse"), FString("Boss: enrage AOE pressure."));
	DF_TAG(Ability_Cooldown_Boss_TerrorShout)(FName("Ability.Cooldown.Boss.TerrorShout"), FString("Terror Shout 30s CD."));
	DF_TAG(Ability_Cooldown_Boss_MeteorStrike)(FName("Ability.Cooldown.Boss.MeteorStrike"), FString("Meteor 45s CD."));
	DF_TAG(Ability_Cooldown_Boss_VoidBarrier)(FName("Ability.Cooldown.Boss.VoidBarrier"), FString("Void barrier 60s CD."));
	DF_TAG(Ability_Cooldown_Boss_EnragePulse)(FName("Ability.Cooldown.Boss.EnragePulse"), FString("Enrage pulse 8s CD."));
	DF_TAG(Event_Boss_PhaseErupt)(FName("Event.Boss.PhaseErupt"), FString("AN_PhaseErupt: execute slam room damage + FX."));
	DF_TAG(Event_Boss_WhiteFlash)(FName("Event.Boss.WhiteFlash"), FString("Payload EventMagnitude = seconds; post-process flash."));
	DF_TAG(Effect_Debuff_Terrified)(FName("Effect.Debuff.Terrified"), FString("Fear: slow, restricted control."));
	DF_TAG(State_Universal_SecondWindAvailable)(FName("State.Universal.SecondWindAvailable"), FString("Can proc Second Wind (pickup / grant)."));
	DF_TAG(State_Berserk)(FName("State.Berserk"), FString("Berserk ultimate window."));
	DF_TAG(Effect_Buff_BattleHymn)(FName("Effect.Buff.BattleHymn"), FString("Battle Hymn buff window."));
	DF_TAG(Event_Universal_Siphon_Trace)(FName("Event.Universal.Siphon.Trace"), FString("Siphon hit frame (anim notify)."));
	DF_TAG(Event_Universal_HealthPotion_Charges)(FName("Event.Universal.HealthPotion.Charges"), FString("Payload: EventMagnitude = remaining charges."));
	DF_TAG(Event_Universal_SecondWind_Activated)(FName("Event.Universal.SecondWind.Activated"), FString("Second Wind proc: HUD/FX broadcast."));
	DF_TAG(Event_Ability_Mage_FrostTrace)(FName("Event.Ability.Mage.Frost.Trace"), FString("Frost bolt release (anim notify)."));
	DF_TAG(Event_Ability_Mage_ArcaneTrace)(FName("Event.Ability.Mage.Arcane.Trace"), FString("Arcane barrage missiles spawn."));
	DF_TAG(State_ManaShieldActive)(FName("State.ManaShieldActive"), FString("Taking damage to mana first."));
	DF_TAG(Effect_DoT_Frost)(FName("Effect.DoT.Frost"), FString("Frost DoT / shatter mark."));
	DF_TAG(Buff_Mage_TimeWarpHaste)(FName("Buff.Mage.TimeWarpHaste"), FString("Time warp int amp + CDR window."));
	DF_TAG(Event_Ability_Whirlwind_Tick)(FName("Event.Ability.Whirlwind.Tick"), FString("Whirlwind damage tick (anim notify)."));
	DF_TAG(Event_Warrior_ShieldBash_Trace)(FName("Event.Warrior.ShieldBash.Trace"), FString("Sync AN_TraceStart for shield bash box."));
	DF_TAG(Event_Warrior_Execute_Trace)(FName("Event.Warrior.Execute.Trace"), FString("Sync AN_TraceStart for execute."));
	DF_TAG(Event_Warrior_WarCry)(FName("Event.Warrior.WarCry"), FString("AI / audio: war cry roar event."));
	DF_TAG(Equipment_OffHand_Shield)(FName("Equipment.OffHand.Shield"), FString("Off-hand is a shield (requirement)."));
	DF_TAG(State_Spinning)(FName("State.Spinning"), FString("Whirlwind or spin attack active."));

	DF_TAG(State_Dead)(FName("State.Dead"), FString("Actor is dead; cannot act."));
	DF_TAG(State_Stunned)(FName("State.Stunned"), FString("Stun — no actions."));
	DF_TAG(State_Rooted)(FName("State.Rooted"), FString("Root — cannot move."));
	DF_TAG(State_Silenced)(FName("State.Silenced"), FString("Cannot use abilities."));
	DF_TAG(State_Invulnerable)(FName("State.Invulnerable"), FString("Immune to damage / harmful effects."));
	DF_TAG(State_Targeting)(FName("State.Targeting"), FString("In targeting mode (e.g. ability aim)."));
	DF_TAG(State_InCombat)(FName("State.InCombat"), FString("In combat; regen, UI, music."));
	DF_TAG(State_Sprinting)(FName("State.Sprinting"), FString("Sprint movement."));
	DF_TAG(State_Dodging)(FName("State.Dodging"), FString("Invuln window / roll."));
	DF_TAG(State_Attacking)(FName("State.Attacking"), FString("In melee/weapon attack window; upper body or combo."));
	DF_TAG(State_Casting)(FName("State.Casting"), FString("Charging or casting a spell/ability with cast anim."));
	DF_TAG(State_CCIgnore)(FName("State.CCIgnore"), FString("CC effects (stun) should not apply (enrage, bosses)."));
	DF_TAG(State_BossEnraged)(FName("State.BossEnraged"), FString("Boss enrage / soft taunt for UI and AI."));
	DF_TAG(State_Spawned_Boss)(FName("State.Spawned.Boss"), FString("Minion from a boss encounter."));
	DF_TAG(State_BossVulnerable)(FName("State.BossVulnerable"), FString("Stumble / long recovery window."));
	DF_TAG(State_Invisible)(FName("State.Invisible"), FString("Not rendered / untarget for some systems."));
	DF_TAG(State_Stealthed)(FName("State.Stealthed"), FString("Stealth movement / opener rules."));
	DF_TAG(State_Concealed)(FName("State.Concealed"), FString("In smoke or heavy concealment."));
	DF_TAG(Buff_Rogue_Ambush)(FName("Buff.Rogue.Ambush"), FString("Next ambush hit from vanish: bonus damage + combo."));
	DF_TAG(Buff_Rogue_KillingSpree)(FName("Buff.Rogue.KillingSpree"), FString("5-CP Eviscerate proc: haste/agi window."));

	DF_TAG(Event_Boss_DoorLock)(FName("Event.Boss.DoorLock"), FString("Lock arena exit doors — subscribe in BP/Actor."));

	DF_TAG(Event_Boss_IntroComplete)(FName("Event.Boss.IntroComplete"), FString("Boss intro cinematic finished."));

	DF_TAG(Event_Stealth_Entered)(FName("Event.Stealth.Entered"), FString("Player entered full stealth (AI/FX)."));
	DF_TAG(Event_Rogue_Backstab_Trace)(FName("Event.Rogue.Backstab.Trace"), FString("Backstab hit frame (AN_TraceStart)."));
	DF_TAG(Event_Rogue_FanOfKnives_Trace)(FName("Event.Rogue.FanOfKnives.Trace"), FString("Fan of Knives release frame."));
	DF_TAG(Event_Rogue_Eviscerate_Trace)(FName("Event.Rogue.Eviscerate.Trace"), FString("Eviscerate hit frame."));

	DF_TAG(Effect_Damage_Physical)(FName("Effect.Damage.Physical"), FString("Physical damage effect (asset tag)."));
	DF_TAG(Effect_Damage_Magic)(FName("Effect.Damage.Magic"), FString("Magic damage effect (asset tag)."));
	DF_TAG(Effect_Damage_True)(FName("Effect.Damage.True"), FString("True damage effect (asset tag)."));
	DF_TAG(Effect_Critical)(FName("Effect.Critical"), FString("This damage instance is a critical hit (execution + combat text)."));
	DF_TAG(Data_CriticalHit)(FName("Data.CriticalHit"), FString("SetByCaller 1.0/0.0 set by UDFDamageCalculation (crit)."));
	DF_TAG(State_Combat_Block)(FName("State.Combat.Block"), FString("Target is blocking; floating text shows BLOCK on hit."));

	DF_TAG(Effect_DoT_Fire)(FName("Effect.DoT.Fire"), FString("Damage over time — fire."));
	DF_TAG(Effect_DoT_Poison)(FName("Effect.DoT.Poison"), FString("DoT — poison."));
	DF_TAG(Effect_DoT_Bleed)(FName("Effect.DoT.Bleed"), FString("DoT — bleed."));
	DF_TAG(Effect_Buff_Speed)(FName("Effect.Buff.Speed"), FString("Movement or attack speed buff."));
	DF_TAG(Effect_Buff_DamageUp)(FName("Effect.Buff.DamageUp"), FString("Damage amplification."));
	DF_TAG(Effect_Buff_Shield)(FName("Effect.Buff.Shield"), FString("Absorption shield."));
	DF_TAG(Effect_Debuff_Slow)(FName("Effect.Debuff.Slow"), FString("Movement slow."));
	DF_TAG(Effect_Debuff_Weaken)(FName("Effect.Debuff.Weaken"), FString("Reduced outgoing damage or stats."));
	DF_TAG(Effect_Debuff_ArmorBreak)(FName("Effect.Debuff.ArmorBreak"), FString("Reduced mitigation."));
	DF_TAG(Effect_Debuff_Blinded)(FName("Effect.Debuff.Blinded"), FString("Severe aim/sight debuff."));

	DF_TAG(Effect_Element_Fire)(FName("Effect.Element.Fire"), FString("This damage instance is fire-element (affinity / UI)."));
	DF_TAG(Effect_Element_Ice)(FName("Effect.Element.Ice"), FString("Ice element."));
	DF_TAG(Effect_Element_Water)(FName("Effect.Element.Water"), FString("Water element."));
	DF_TAG(Effect_Element_Lightning)(FName("Effect.Element.Lightning"), FString("Lightning element."));
	DF_TAG(Effect_Element_Earth)(FName("Effect.Element.Earth"), FString("Earth element."));
	DF_TAG(Effect_Element_Arcane)(FName("Effect.Element.Arcane"), FString("Arcane — neutral matrix; bypasses resist table."));
	DF_TAG(Effect_Element_Physical)(FName("Effect.Element.Physical"), FString("Physical/weapon element on damage."));
	DF_TAG(Effect_Element_True)(FName("Effect.Element.True"), FString("True — bypasses affinity + resist."));

	DF_TAG(State_Elemental_Wet)(FName("State.Elemental.Wet"), FString("Soaked; pairs with slow for electrocute reaction."));
	DF_TAG(Effect_Reaction_Melt)(FName("Effect.Reaction.Melt"), FString("Melt: fire + frost reaction."));
	DF_TAG(Effect_Reaction_Steam)(FName("Effect.Reaction.Steam"), FString("Steam: ice on burning target."));
	DF_TAG(Effect_Reaction_Electrocute)(FName("Effect.Reaction.Electrocute"), FString("Electrocute: lightning on wet/slow."));

	DF_TAG(Event_Ability_Fire_Launch)(FName("Event.Ability.Fire.Launch"), FString("Anim notify: fire projectile release."));
	DF_TAG(Event_Ability_Melee_Hit)(FName("Event.Ability.Melee.Hit"), FString("Melee impact / damage window."));
	DF_TAG(Event_Ability_Montage_End)(FName("Event.Ability.Montage.End"), FString("Ability montage finished."));
	DF_TAG(Event_Hit_Received)(FName("Event.Hit.Received"), FString("On damage: victim; EventMagnitude = damage this execute."));
	DF_TAG(Event_Ability_Kill)(FName("Event.Ability.Kill"), FString("Killer: lethal when context has gameplay ability (Mana Vortex, etc.)."));
	DF_TAG(Event_Passive_Rogue_BleedApplied)(FName("Event.Passive.Rogue.BleedApplied"), FString("Rogue: bleed was applied; payload Target=enemy."));

	DF_TAG(Data_Damage)(FName("Data.Damage"), FString("SetByCaller / damage magnitude."));
	DF_TAG(Data_Healing)(FName("Data.Healing"), FString("SetByCaller — healing."));
	DF_TAG(Data_Duration)(FName("Data.Duration"), FString("SetByCaller — duration."));
	DF_TAG(Data_Cost)(FName("Data.Cost"), FString("SetByCaller — mana/stamina cost."));
	DF_TAG(Data_Cooldown)(FName("Data.Cooldown"), FString("SetByCaller — cooldown seconds."));
	DF_TAG(Data_Magnitude)(FName("Data.Magnitude"), FString("SetByCaller — generic magnitude (buff/debuff)."));
	DF_TAG(Data_Knockback)(FName("Data.Knockback"), FString("SetByCaller — melee knockback / impulse scale."));

	DF_TAG(Effect_Buff_LevelStatScaling)(FName("Effect.Buff.LevelStatScaling"), FString("Infinite per-level max-vitals and primary stat bonus (replaced on level up)."));
	DF_TAG(Data_LevelUp_MaxHealthAdd)(FName("Data.LevelUp.MaxHealthAdd"), FString("SetByCaller — additive max health from level table."));
	DF_TAG(Data_LevelUp_MaxManaAdd)(FName("Data.LevelUp.MaxManaAdd"), FString("SetByCaller — additive max mana from level table."));
	DF_TAG(Data_LevelUp_StrengthAdd)(FName("Data.LevelUp.StrengthAdd"), FString("SetByCaller — additive strength from level table."));
	DF_TAG(Data_LevelUp_IntelligenceAdd)(FName("Data.LevelUp.IntelligenceAdd"), FString("SetByCaller — additive intelligence from level table."));
	DF_TAG(Data_LevelUp_AgilityAdd)(FName("Data.LevelUp.AgilityAdd"), FString("SetByCaller — additive agility from level table."));
	DF_TAG(Character_Level)(FName("Character.Level"), FString("Level tier tags: Character.Level.N for exact level."));
	for (int32 L = 1; L <= 30; ++L)
	{
		M.AddNativeGameplayTag(
			FName(*FString::Printf(TEXT("Character.Level.%d"), L)),
			FString::Printf(TEXT("Character has reached at least level %d."), L));
	}

	DF_TAG(UI_MenuOpen)(FName("UI.MenuOpen"), FString("Pause / main menu visible."));
	DF_TAG(UI_InventoryOpen)(FName("UI.InventoryOpen"), FString("Inventory screen up."));
	DF_TAG(UI_AbilityMenuOpen)(FName("UI.AbilityMenuOpen"), FString("Ability offer / roguelike pick UI."));

	DF_TAG(UI_CinematicLock)(FName("UI.CinematicLock"), FString("Input and abilities locked for cinematic."));

	GDF_RegisteredNativeGameplayTags = true;
}

#undef DF_TAG
