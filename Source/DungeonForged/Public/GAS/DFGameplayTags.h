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

	//~ Ability
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

	//~ Effect
	static FGameplayTag Effect_DoT_Fire;
	static FGameplayTag Effect_DoT_Poison;
	static FGameplayTag Effect_DoT_Bleed;
	static FGameplayTag Effect_Buff_Speed;
	static FGameplayTag Effect_Buff_DamageUp;
	static FGameplayTag Effect_Buff_Shield;
	static FGameplayTag Effect_Debuff_Slow;
	static FGameplayTag Effect_Debuff_Weaken;
	static FGameplayTag Effect_Debuff_ArmorBreak;

	//~ Event
	static FGameplayTag Event_Ability_Fire_Launch;
	static FGameplayTag Event_Ability_Melee_Hit;
	static FGameplayTag Event_Ability_Montage_End;

	//~ Data (SetByCaller)
	static FGameplayTag Data_Damage;
	static FGameplayTag Data_Healing;
	static FGameplayTag Data_Duration;
	static FGameplayTag Data_Knockback;

	//~ UI
	static FGameplayTag UI_MenuOpen;
	static FGameplayTag UI_InventoryOpen;
	static FGameplayTag UI_AbilityMenuOpen;
};
