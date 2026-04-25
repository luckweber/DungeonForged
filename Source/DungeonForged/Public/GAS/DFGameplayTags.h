// Source/DungeonForged/Public/GAS/DFGameplayTags.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
 * Project-native GameplayTags (AddNativeGameplayTag). Storage is static; assign once in RegisterGameplayTags.
 * @see UDFAssetManager::StartInitialLoading, UDFGameInstance::Init
 */
struct DUNGEONFORGED_API FDFGameplayTags
{
	/** Call once; safe to call again (no-op if already registered). */
	static void RegisterGameplayTags();

	/** Legacy alias. */
	static void InitializeNativeGameplayTags() { RegisterGameplayTags(); }

	/** Resolves Data.Damage; RequestGameplayTag fallback if Register has not run yet. */
	static FGameplayTag ResolveDataDamageTag();
	/** Resolves Data.Knockback; RequestGameplayTag fallback if Register has not run yet. */
	static FGameplayTag ResolveDataKnockbackTag();

	//~ Ability (hierarchical roots; used for block/cancel queries)
	static FGameplayTag Ability_Parent;
	/** Parent of Ability.Attack.Melee / Ranged; matches "Ability.Attack.*". */
	static FGameplayTag Ability_Attack;
	static FGameplayTag Ability_Fire;
	static FGameplayTag Ability_Ice;
	static FGameplayTag Ability_Attack_Melee;
	static FGameplayTag Ability_Attack_Ranged;
	static FGameplayTag Ability_Fire_Fireball;
	static FGameplayTag Ability_Fire_FlameStrike;
	static FGameplayTag Ability_Ice_FrostBolt;
	static FGameplayTag Ability_Ice_Blizzard;
	static FGameplayTag Ability_Physical_Charge;
	static FGameplayTag Ability_Physical_Whirlwind;
	static FGameplayTag Ability_Slot_1;
	static FGameplayTag Ability_Slot_2;
	static FGameplayTag Ability_Slot_3;
	static FGameplayTag Ability_Slot_4;
	static FGameplayTag Ability_Passive_HealthRegen;
	static FGameplayTag Ability_Passive_ManaRegen;
	static FGameplayTag Ability_Passive_Warrior_Fortitude;
	static FGameplayTag Ability_Passive_Warrior_Retaliation;
	static FGameplayTag Ability_Passive_Mage_ArcaneMastery;
	static FGameplayTag Ability_Passive_Mage_ManaVortex;
	static FGameplayTag Ability_Passive_Rogue_Predator;
	static FGameplayTag Ability_Passive_Rogue_BleedMastery;
	static FGameplayTag Ability_Movement_Sprint;
	static FGameplayTag Ability_Movement_Dodge;
	// Warrior
	static FGameplayTag Ability_Warrior_ShieldBash;
	static FGameplayTag Ability_Warrior_WarCry;
	static FGameplayTag Ability_Warrior_Whirlwind;
	static FGameplayTag Ability_Warrior_IronSkin;
	static FGameplayTag Ability_Warrior_Charge;
	static FGameplayTag Ability_Warrior_Execute;
	/** Failsafe when activation blocked by range (e.g. charge). */
	static FGameplayTag Ability_Failed_Range;
	// Mage
	static FGameplayTag Ability_Mage;
	static FGameplayTag Ability_Mage_FrostBolt;
	static FGameplayTag Ability_Mage_BlizzardStorm;
	static FGameplayTag Ability_Mage_ArcaneBarrage;
	static FGameplayTag Ability_Mage_TimeWarp;
	static FGameplayTag Ability_Mage_ManaShield;
	static FGameplayTag Ability_Mage_Teleport;
	// Rogue
	static FGameplayTag Ability_Rogue;
	static FGameplayTag Ability_Rogue_Backstab;
	static FGameplayTag Ability_Rogue_FanOfKnives;
	static FGameplayTag Ability_Rogue_ShadowStep;
	static FGameplayTag Ability_Rogue_Eviscerate;
	static FGameplayTag Ability_Rogue_Vanish;
	static FGameplayTag Ability_Rogue_SmokeScreen;
	// Universal (roguelike pickups — all classes)
	static FGameplayTag Ability_Universal_HealthPotion;
	static FGameplayTag Ability_Universal_SecondWind;
	static FGameplayTag Ability_Universal_BattleHymn;
	static FGameplayTag Ability_Universal_Siphon;
	static FGameplayTag Ability_Universal_Berserk;
	static FGameplayTag Ability_Universal_CallLightning;
	/** Parent tag for cooldown GEs; used by TimeWarp purge. */
	static FGameplayTag Ability_Cooldown;
	/** Per-skill universal cooldown (asset tags on GEs). */
	static FGameplayTag Ability_Cooldown_HealthPotion;
	static FGameplayTag Ability_Cooldown_SecondWind;
	static FGameplayTag Ability_Cooldown_BattleHymn;
	static FGameplayTag Ability_Cooldown_Siphon;
	static FGameplayTag Ability_Cooldown_Berserk;
	static FGameplayTag Ability_Cooldown_CallLightning;
	// Boss (granted only to ADFBossBase; see bSourceObjectMustBeBoss on ability CDOs)
	static FGameplayTag Ability_Boss_TerrorShout;
	static FGameplayTag Ability_Boss_MeteorStrike;
	static FGameplayTag Ability_Boss_VoidBarrier;
	static FGameplayTag Ability_Boss_PhaseTransitionSlam;
	static FGameplayTag Ability_Boss_EnragePulse;
	static FGameplayTag Ability_Cooldown_Boss_TerrorShout;
	static FGameplayTag Ability_Cooldown_Boss_MeteorStrike;
	static FGameplayTag Ability_Cooldown_Boss_VoidBarrier;
	static FGameplayTag Ability_Cooldown_Boss_EnragePulse;
	/** AN_PhaseErupt: phase slam room-wide hit frame. */
	static FGameplayTag Event_Boss_PhaseErupt;
	static FGameplayTag Event_Boss_WhiteFlash;
	static FGameplayTag Effect_Debuff_Terrified;
	static FGameplayTag State_Universal_SecondWindAvailable;
	static FGameplayTag State_Berserk;
	static FGameplayTag Effect_Buff_BattleHymn;
	/** Siphon trace frame (AN_TraceStart). */
	static FGameplayTag Event_Universal_Siphon_Trace;
	/** Potion uses remaining: payload EventMagnitude = charges. */
	static FGameplayTag Event_Universal_HealthPotion_Charges;
	/** For HUD/FX: second wind proc. */
	static FGameplayTag Event_Universal_SecondWind_Activated;
	static FGameplayTag Event_Ability_Mage_FrostTrace;
	static FGameplayTag Event_Ability_Mage_ArcaneTrace;
	static FGameplayTag State_ManaShieldActive;
	static FGameplayTag Effect_DoT_Frost;
	static FGameplayTag Buff_Mage_TimeWarpHaste;
	static FGameplayTag Event_Ability_Whirlwind_Tick;
	static FGameplayTag Event_Warrior_ShieldBash_Trace;
	static FGameplayTag Event_Warrior_Execute_Trace;
	/** For AI: war cry played (flee / react). */
	static FGameplayTag Event_Warrior_WarCry;
	/** Off-hand must be shield. */
	static FGameplayTag Equipment_OffHand_Shield;
	static FGameplayTag State_Spinning;

	//~ State
	static FGameplayTag State_Dead;
	static FGameplayTag State_Stunned;
	static FGameplayTag State_Rooted;
	static FGameplayTag State_Silenced;
	static FGameplayTag State_Invulnerable;
	static FGameplayTag State_Targeting;
	static FGameplayTag State_InCombat;
	static FGameplayTag State_Sprinting;
	static FGameplayTag State_Dodging;
	static FGameplayTag State_Attacking;
	static FGameplayTag State_Casting;
	/** Stun / most CC: UTargetTagRequirementsGameplayEffectComponent should ignore targets with this tag. */
	static FGameplayTag State_CCIgnore;
	static FGameplayTag State_BossEnraged;
	/** Set on minions spawned by a boss encounter. */
	static FGameplayTag State_Spawned_Boss;
	static FGameplayTag State_BossVulnerable;
	static FGameplayTag State_Invisible;
	static FGameplayTag State_Stealthed;
	static FGameplayTag State_Concealed;
	static FGameplayTag Buff_Rogue_Ambush;
	static FGameplayTag Buff_Rogue_KillingSpree;

	//~ Event
	/** Listeners: lock arena doors, play NavMesh blockers, etc. */
	static FGameplayTag Event_Boss_DoorLock;
	/** Cinematic or trigger finished; restore gameplay. */
	static FGameplayTag Event_Boss_IntroComplete;
	/** Stealth / vanish: AI and audio listen for full stealth entry. */
	static FGameplayTag Event_Stealth_Entered;
	/** AN_TraceStart-style sync; Backstab, Fan of Knives, Eviscerate. */
	static FGameplayTag Event_Rogue_Backstab_Trace;
	static FGameplayTag Event_Rogue_FanOfKnives_Trace;
	static FGameplayTag Event_Rogue_Eviscerate_Trace;
	static FGameplayTag Event_Ability_Fire_Launch;
	static FGameplayTag Event_Ability_Melee_Hit;
	static FGameplayTag Event_Ability_Montage_End;
	static FGameplayTag Event_Hit_Received;
	static FGameplayTag Event_Ability_Kill;
	static FGameplayTag Event_Passive_Rogue_BleedApplied;

	//~ Effect
	static FGameplayTag Effect_Damage_Physical;
	static FGameplayTag Effect_Damage_Magic;
	static FGameplayTag Effect_Damage_True;
	/** Set on damage execution context when a crit is rolled. */
	static FGameplayTag Effect_Critical;
	/** SetByCaller: 1.0 if crit, 0 otherwise (read in AttributeSet for combat text). */
	static FGameplayTag Data_CriticalHit;
	/** For combat text "BLOCK" when the target is actively guarding (e.g. shield up). */
	static FGameplayTag State_Combat_Block;
	static FGameplayTag Effect_DoT_Fire;
	static FGameplayTag Effect_DoT_Poison;
	static FGameplayTag Effect_DoT_Bleed;
	static FGameplayTag Effect_Buff_Speed;
	static FGameplayTag Effect_Buff_DamageUp;
	static FGameplayTag Effect_Buff_Shield;
	static FGameplayTag Effect_Debuff_Slow;
	static FGameplayTag Effect_Debuff_Weaken;
	static FGameplayTag Effect_Debuff_ArmorBreak;
	static FGameplayTag Effect_Debuff_Blinded;
	/** Replaces single active level-stat-scaling GameplayEffect (infinite) per level. */
	static FGameplayTag Effect_Buff_LevelStatScaling;
	//~ Elemental (damage spec dynamic tags; query in UI/ASC)
	static FGameplayTag Effect_Element_Fire;
	static FGameplayTag Effect_Element_Ice;
	static FGameplayTag Effect_Element_Water;
	static FGameplayTag Effect_Element_Lightning;
	static FGameplayTag Effect_Element_Earth;
	static FGameplayTag Effect_Element_Arcane;
	static FGameplayTag Effect_Element_Physical;
	static FGameplayTag Effect_Element_True;
	/** Stacked with wet/water for Electrocute reaction (optional; can use Effect_Debuff_Slow from water). */
	static FGameplayTag State_Elemental_Wet;
	/** Procedural Melt / Steam / Electrocute tags for reactions (grant + FX hooks). */
	static FGameplayTag Effect_Reaction_Melt;
	static FGameplayTag Effect_Reaction_Steam;
	static FGameplayTag Effect_Reaction_Electrocute;

	//~ Data (SetByCaller)
	static FGameplayTag Data_Damage;
	static FGameplayTag Data_Healing;
	static FGameplayTag Data_Duration;
	static FGameplayTag Data_Cost;
	static FGameplayTag Data_Cooldown;
	static FGameplayTag Data_Magnitude;
	static FGameplayTag Data_Knockback;
	/** UGE_LevelUp_StatScaling: additive to MaxHealth from level curve. */
	static FGameplayTag Data_LevelUp_MaxHealthAdd;
	static FGameplayTag Data_LevelUp_MaxManaAdd;
	static FGameplayTag Data_LevelUp_StrengthAdd;
	static FGameplayTag Data_LevelUp_IntelligenceAdd;
	static FGameplayTag Data_LevelUp_AgilityAdd;

	//~ Character / progression (native Character.Level.1..N also registered in RegisterGameplayTags)
	static FGameplayTag Character_Level;

	//~ UI
	static FGameplayTag UI_MenuOpen;
	static FGameplayTag UI_InventoryOpen;
	static FGameplayTag UI_AbilityMenuOpen;
	/** Class selection (Nexus / new run) — pause-adjacent; query on ASC for input lock. */
	static FGameplayTag UI_ClassSelectionOpen;
	/** Gate gameplay input/abilities while boss intro cinematic runs. */
	static FGameplayTag UI_CinematicLock;
};
