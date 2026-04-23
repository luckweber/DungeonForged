// Source/DungeonForged/Public/GAS/DFGameplayTags.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
 * Project-native GameplayTags (AddNativeGameplayTag). Storage is static; assign once in RegisterGameplayTags.
 * @see UDFAssetManager::StartInitialLoading, UDungeonForgedGameInstance::Init
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
	/** Stun / most CC GEs with ApplicationTagRequirements should ignore targets with this tag. */
	static FGameplayTag State_CCIgnore;
	static FGameplayTag State_BossEnraged;
	/** Set on minions spawned by a boss encounter. */
	static FGameplayTag State_Spawned_Boss;
	static FGameplayTag State_BossVulnerable;

	//~ Event
	/** Listeners: lock arena doors, play NavMesh blockers, etc. */
	static FGameplayTag Event_Boss_DoorLock;
	/** Cinematic or trigger finished; restore gameplay. */
	static FGameplayTag Event_Boss_IntroComplete;
	static FGameplayTag Event_Ability_Fire_Launch;
	static FGameplayTag Event_Ability_Melee_Hit;
	static FGameplayTag Event_Ability_Montage_End;

	//~ Effect
	static FGameplayTag Effect_Damage_Physical;
	static FGameplayTag Effect_Damage_Magic;
	static FGameplayTag Effect_Damage_True;
	static FGameplayTag Effect_DoT_Fire;
	static FGameplayTag Effect_DoT_Poison;
	static FGameplayTag Effect_DoT_Bleed;
	static FGameplayTag Effect_Buff_Speed;
	static FGameplayTag Effect_Buff_DamageUp;
	static FGameplayTag Effect_Buff_Shield;
	static FGameplayTag Effect_Debuff_Slow;
	static FGameplayTag Effect_Debuff_Weaken;
	static FGameplayTag Effect_Debuff_ArmorBreak;

	//~ Data (SetByCaller)
	static FGameplayTag Data_Damage;
	static FGameplayTag Data_Healing;
	static FGameplayTag Data_Duration;
	static FGameplayTag Data_Cost;
	static FGameplayTag Data_Cooldown;
	static FGameplayTag Data_Magnitude;
	static FGameplayTag Data_Knockback;

	//~ UI
	static FGameplayTag UI_MenuOpen;
	static FGameplayTag UI_InventoryOpen;
	static FGameplayTag UI_AbilityMenuOpen;
	/** Gate gameplay input/abilities while boss intro cinematic runs. */
	static FGameplayTag UI_CinematicLock;
};
