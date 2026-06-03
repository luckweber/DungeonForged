// Source/DungeonForged/Public/Data/DFDataTableStructs.h
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Engine/DataTable.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Combat/DFMeleeTraceTypes.h"
#include "NiagaraSystem.h"
#include "GAS/UDFGameplayAbility.h"
#include "Equipment/DFEquipmentTypes.h"
#include "Animation/DFAnimSetTypes.h"
#include "Combat/DFComboDirectionalTypes.h"
#include "DFDataTableStructs.generated.h"

class AActor;
class ADFPlayerCharacter;
class APawn;
class UBehaviorTree;
class UGameplayAbility;
class UGameplayEffect;

UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	Common    UMETA(DisplayName = "Common"),
	Uncommon  UMETA(DisplayName = "Uncommon"),
	Rare      UMETA(DisplayName = "Rare"),
	Epic      UMETA(DisplayName = "Epic"),
	Legendary UMETA(DisplayName = "Legendary"),
};

UENUM(BlueprintType)
enum class EItemType : uint8
{
	/** Legacy order preserved for DT rows (0-6), new gear types 7-11. */
	Weapon		= 0 UMETA(DisplayName = "Weapon"),
	Armor		= 1 UMETA(DisplayName = "Armor (legacy, use target slot)"),
	Helmet		= 2 UMETA(DisplayName = "Helmet"),
	Ring		= 3 UMETA(DisplayName = "Ring"),
	Amulet		= 4 UMETA(DisplayName = "Amulet"),
	Consumable	= 5 UMETA(DisplayName = "Consumable"),
	Currency	= 6 UMETA(DisplayName = "Currency (Gold)"),
	OffHand		= 7 UMETA(DisplayName = "OffHand"),
	Chest		= 8 UMETA(DisplayName = "Chest"),
	Legs		= 9 UMETA(DisplayName = "Legs"),
	Boots		= 10 UMETA(DisplayName = "Boots"),
	Gloves		= 11 UMETA(DisplayName = "Gloves"),
};

UENUM(BlueprintType)
enum class EEnemyTier : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Elite  UMETA(DisplayName = "Elite"),
	Boss   UMETA(DisplayName = "Boss"),
};

UENUM(BlueprintType)
enum class EDFEnemyArchetype : uint8
{
	Grunt UMETA(DisplayName = "Grunt"),
	Tank UMETA(DisplayName = "Tank"),
	Skirmisher UMETA(DisplayName = "Skirmisher"),
	Caster UMETA(DisplayName = "Caster"),
	Berserker UMETA(DisplayName = "Berserker"),
	Healer UMETA(DisplayName = "Healer"),
	Spawner UMETA(DisplayName = "Spawner"),
	Shielder UMETA(DisplayName = "Shielder"),
	Sniper UMETA(DisplayName = "Sniper"),
	Bomber UMETA(DisplayName = "Bomber"),
};

/** Row in @c DT_ClassUnlock (see @c UDFClassSelectionDeveloperSettings::ClassUnlockDataTable). */
UENUM(BlueprintType)
enum class EDFClassUnlockRule : uint8
{
	AlwaysUnlocked UMETA(DisplayName = "Always unlocked"),
	MinimumTotalRuns UMETA(DisplayName = "Minimum total runs completed"),
	MinimumTotalWins UMETA(DisplayName = "Minimum total wins"),
	MinimumMetaLevel UMETA(DisplayName = "Minimum meta level"),
	/** Unlocks only via @c UDFSaveGame::UnlockedClasses (Nexus / meta purchase). */
	SaveUnlockListOnly UMETA(DisplayName = "Save unlock list only"),
};

/** Unlock rule for class selection. Row name or @c AdditionalClassRowNames must match @c DT_Class row names. */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFClassUnlockTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Extra DT_Class row names that share this rule (e.g. Guerreiro + Warrior). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unlock")
	TArray<FName> AdditionalClassRowNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unlock")
	EDFClassUnlockRule UnlockRule = EDFClassUnlockRule::AlwaysUnlocked;

	/** Threshold for runs / wins / meta level rules. Ignored for @c AlwaysUnlocked and @c SaveUnlockListOnly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unlock", meta = (ClampMin = "0"))
	int32 RequiredValue = 0;

	/** Shown on the lock overlay when locked. Empty = auto text from @c UnlockRule. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unlock|UI")
	FText UnlockHint;
};

/** DataTable: grant a UDF gameplay ability; used e.g. with ADFPlayerState::GrantAbilitiesFromDataTable. */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFAbilityTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Abilities")
	TSubclassOf<UDFGameplayAbility> AbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Abilities", meta = (Categories = "Ability"))
	FGameplayTag AbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Abilities")
	int32 AbilityLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Abilities|UI")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Abilities|UI")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Abilities|UI")
	FText Description;

	/** Rarity is used for between-floor 1-of-3 roll weighting (see UDFAbilitySelectionSubsystem). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Abilities|UI")
	EItemRarity Rarity = EItemRarity::Common;

	/**
	 * If non-empty, the player must already have a granted ability whose @c AbilityTag matches
	 * one of these tags (build synergy / archetype pools).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Abilities|Draft")
	FGameplayTagContainer RequiresSynergyTags;

	/**
	 * Excludes this row from offers when the player already has a granted ability whose
	 * @c AbilityTag matches any tag here (dedup / exclusion).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Abilities|Draft")
	FGameplayTagContainer ExcludedIfGrantedTags;

	/** Optional text for the selection card (GAS cost/cooldown can be separate). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Abilities|UI")
	FText DisplayCooldown = FText::GetEmpty();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Abilities|UI")
	FText DisplayCost = FText::GetEmpty();

	/**
	 * If >= 0, used as FGameplayAbilitySpec InputID when granted (pairs with AbilityInputActions[].GameplayInputId).
	 * If < 0 (default), InputID stays list position capped at 3 — see UDFRunManager::GrantAbilitiesForCurrentRun.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Abilities|Input", meta = (ClampMin = "-1", ClampMax = "63"))
	int32 GameplayAbilityInputID = INDEX_NONE;
};

/** One GameplayEffect that initializes base attributes (e.g. modifiers on UDFAttributeSet); used with InitializeAttributesFromDataTable. */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFAttributeInitTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TSubclassOf<UGameplayEffect> StartupGameplayEffect;
};

/** Weighted montage variant with optional tag filters (AAA variety). */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFComboVariant
{
	GENERATED_BODY()

	/** Soft ref — resolved lazily by callers (e.g. UDFComboComponent::ApplyComboStepData) via LoadSynchronous on equip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
	TSoftObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Weight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
	FGameplayTagContainer RequiredAttackerTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
	FGameplayTagContainer BlockedAttackerTags;
};

/** One combo chain step: light swing plus optional heavy finisher branch montage. */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFComboStep
{
	GENERATED_BODY()

	/** Legacy single montage; used when @c LightVariants is empty. Soft-loaded on equip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
	TSoftObjectPtr<UAnimMontage> LightMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
	TArray<FDFComboVariant> LightVariants;

	/** Played instead of light when the player holds primary during the combo window (heavy finisher branch). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
	TSoftObjectPtr<UAnimMontage> HeavyBranchMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
	TArray<FDFComboVariant> HeavyBranchVariants;

	/** Per-step chain blend-in (seconds). Negative = use @c UDFComboComponent::ComboChainMontageBlendInTime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo", meta = (ClampMin = "-1.0"))
	float ChainBlendInTime = -1.f;

	/** Per-step chain blend-out (seconds). Negative = use component default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo", meta = (ClampMin = "-1.0"))
	float ChainBlendOutTime = -1.f;

	/** 255 = use @c UDFComboComponent::ComboChainBlendOption. Otherwise @c EAlphaBlendOption value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo", meta = (ClampMin = "0", ClampMax = "255"))
	uint8 ChainBlendOptionOverride = 255;

	/**
	 * Start time (seconds) when this step chains (step > 0). Blends into this pose — do not use Montage_SetPosition.
	 * Keep small (often 0.05–0.12); values near 0.25+ usually feel abrupt and fight the cross-fade.
	 * 0 = play from start. Tip: set slightly before AN_TraceStart on the montage timeline.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float ChainStartTimeOffset = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Launch")
	bool bIsLauncher = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Launch", meta = (EditCondition = "bIsLauncher"))
	FVector LaunchVelocity = FVector(0.f, 0.f, 700.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Launch", meta = (EditCondition = "bIsLauncher"))
	FVector SelfLaunchVelocity = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Launch",
		meta = (EditCondition = "bIsLauncher", ClampMin = "0.0", ClampMax = "1.0"))
	float TargetGravityScale = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Launch",
		meta = (EditCondition = "bIsLauncher", ClampMin = "0.1"))
	float HangtimeSeconds = 1.5f;
};

/**
 * Standalone combo definition row (e.g. DT_Combos).
 * Exports cleanly as CSV/JSON — montage refs become asset paths in the export.
 *
 * Typical workflow:
 *   1. Edit in editor (DT_Combos row editor) OR export as JSON, edit externally, reimport.
 *   2. Reference from FDFItemTableRow::WeaponMeleeComboRow (preferred) or paste inline into
 *      WeaponMeleeComboSteps (legacy).
 *   3. Multiple items can point to the same combo row — share "Sword_Basic_Chain" across
 *      every 1H sword variant without copy-pasting montages.
 */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFComboTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Display name shown in the row editor / debug overlays. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
	FText DisplayName;

	/** Optional weapon tag filter (e.g. Weapon.Sword.1H) — informational; not enforced at resolution time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo", meta = (Categories = "Weapon"))
	FGameplayTagContainer WeaponTagFilter;

	/** Optional pipeline metadata; survives JSON/CSV round-trip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Meta")
	FString Author;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Meta", meta = (ClampMin = "1"))
	int32 Version = 1;

	/** The actual chain steps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
	TArray<FDFComboStep> Steps;

	/** Optional aerial juggle chain (used after launcher / while @c bAerialComboActive). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Aerial")
	TArray<FDFComboStep> AerialSteps;

	/**
	 * Optional 8-way montage overrides per step index (parallel to @c Steps).
	 * When a step entry is configured, it overrides legacy Backward/Side arrays for that step.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Directional")
	TArray<FDFComboDirectionalMontageSet> DirectionalStepOverrides;
};

/** Item definition row (e.g. DT_Items). */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFItemTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemRarity Rarity = EItemRarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemType ItemType = EItemType::Weapon;

	/** 1 = non-stackable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "1"))
	int32 MaxStack = 1;

	/**
	 * Per-unit mass for encumbrance (optional). 0 = no contribution.
	 * @see UDFInventoryComponent::MaxCarryWeight — 0 disables the limit entirely.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Encumbrance", meta = (ClampMin = "0"))
	float ItemWeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Visuals")
	TObjectPtr<UStaticMesh> ItemMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Visuals")
	TObjectPtr<UTexture2D> Icon = nullptr;

	/** Equipment slot (authoritative for gear validation). Filled in DT rows for armor/weapons. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Equipment")
	EEquipmentSlot TargetEquipmentSlot = EEquipmentSlot::None;

	/** For modular paper-doll / leader-pose (Armor parts). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Visuals")
	TObjectPtr<USkeletalMesh> ItemSkeletalMesh = nullptr;

	/** Optional; shown on paper-doll / slot widgets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "0"))
	int32 ItemLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|GAS")
	TMap<FGameplayAttribute, float> AttributeModifiers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|GAS")
	TSubclassOf<UGameplayEffect> OnEquipEffect;

	/**
	 * When this item occupies the Weapon slot, the Ability System grants this class on equip (server),
	 * revokes it on unequip, and TryActivateGrantedWeaponMeleeAbility uses it instead of Warrior.MeleeSwing.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Combat")
	TSubclassOf<UGameplayAbility> WeaponMeleeGameplayAbility;

	/** Combo montages for basic melee while this weapon is equipped (overrides armed fallback class row / BP baseline). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Combat")
	TArray<TSoftObjectPtr<UAnimMontage>> WeaponMeleeComboMontages;

	/**
	 * Preferred: handle to a row in DT_Combos. Reusable across items and exports as JSON/CSV.
	 * When set (DataTable + RowName both valid), it overrides @c WeaponMeleeComboSteps below.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Combat",
		meta = (RowType = "/Script/DungeonForged.DFComboTableRow"))
	FDataTableRowHandle WeaponMeleeComboRow;

	/**
	 * Legacy / inline fallback: per-step combo (light + optional heavy finisher).
	 * Used when @c WeaponMeleeComboRow is empty. When non-empty, overrides @c WeaponMeleeComboMontages.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Combat|Legacy")
	TArray<FDFComboStep> WeaponMeleeComboSteps;

	/** Loops on primary-attack hold before heavy tier commits (optional). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Combat|Heavy")
	TSoftObjectPtr<UAnimMontage> WeaponChargeWindupMontage;

	/** Optional bridge montage on heavy release after windup; falls back to heavy montage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Combat|Heavy")
	TSoftObjectPtr<UAnimMontage> WeaponHeavyChargeReleaseMontage;

	/** e.g. Weapon.Sword.1H, Weapon.Axe — applied to ASC while equipped. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Combat", meta = (Categories = "Weapon"))
	FGameplayTagContainer WeaponTags;

	/** Hit reaction / damage typing: Damage.Source.Slash, Blunt, Pierce. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Combat", meta = (Categories = "Damage.Source"))
	FGameplayTag WeaponDamageSourceTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Combat")
	TSoftObjectPtr<UAnimMontage> WeaponHeavyAttackMontage;

	/** Played when the player held primary attack past @c UDFComboComponent::MaxHeavyChargeThreshold (the biggest swing this weapon owns). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Combat")
	TSoftObjectPtr<UAnimMontage> WeaponMaxHeavyAttackMontage;

	/** Overrides UDFMeleeTraceComponent::BaseDamage while equipped; leave 0 to use character defaults. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Combat", meta = (ClampMin = "0.0"))
	float WeaponMeleeBaseDamage = 0.f;

	/** When true, @c WeaponMeleeTraceShape overrides tuning-data weapon-tag lookup (B2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Combat|Trace")
	bool bOverrideMeleeTraceShape = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Combat|Trace", meta = (EditCondition = "bOverrideMeleeTraceShape"))
	EDFMeleeTraceShape WeaponMeleeTraceShape = EDFMeleeTraceShape::Sphere;

	/** Overrides melee damage gameplay effect subclass while equipped; null = revert to pawn defaults. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Combat")
	TSubclassOf<UGameplayEffect> WeaponMeleeDamageGameplayEffect;

	/**
	 * Linked animation layer (ALS / Elder-style) while this item is in the Weapon slot.
	 * UUDFAnimInstance syncs LinkAnimClassLayers / Unlink when equip changes; null = no layer from this item.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Animation")
	TSubclassOf<UAnimInstance> WeaponLinkedAnimLayerClass;

	/**
	 * When equipped in Weapon slot, copied to UUDFAnimInstance::ActiveAnimSet (if valid); otherwise reverts to class DefaultAnimSet.
	 * Prefer Break ActiveAnimSet in AnimGraph for dynamic Blend Space refs.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Animation")
	FUDAnimSet WeaponAnimSet;
};

USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FName RowName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ClampMin = "0"))
	int32 Quantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	bool bIsEquipped = false;
};

/** Spawner / bestiary style enemy row. */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFEnemyTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	TSubclassOf<AActor> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	FText EnemyName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat", meta = (ClampMin = "0.0"))
	float BaseHealth = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat", meta = (ClampMin = "0.0"))
	float BaseDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat", meta = (ClampMin = "0.0"))
	float BaseArmor = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Rewards", meta = (ClampMin = "0.0"))
	float ExperienceReward = 0.f;

	/** Run gold dropped to the killing player (rand in [Min, Max], inclusive). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Rewards", meta = (ClampMin = "0"))
	int32 GoldDropMin = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Rewards", meta = (ClampMin = "0"))
	int32 GoldDropMax = 0;

	/** Row names in DT_Items (or your item table) that this enemy can roll for loot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Loot")
	TArray<FName> LootTableRows;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|GAS", meta = (Categories = "Ability"))
	TArray<FGameplayTag> GrantedAbilities;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	EEnemyTier Tier = EEnemyTier::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI")
	EDFEnemyArchetype Archetype = EDFEnemyArchetype::Grunt;

	/**
	 * When > 0, `ADFEnemyBase::InitializeFromDataTable` sets `UCharacterMovementComponent::MaxWalkSpeed`
	 * and replicates it to simulating clients. When 0, Character / Blueprint `Max Walk Speed` defaults are left unchanged.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement", meta = (ClampMin = "0.0"))
	float MaxWalkSpeed = 0.f;

	/** Used by ADFEnemyBase::BeginPlay to run the brain (set AutoPossessAI = PlacedInWorldOrSpawned and use AAIController). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI")
	TObjectPtr<UBehaviorTree> AIBehaviorTree;

	/** Melee “in range” / BT decorator threshold (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI", meta = (ClampMin = "0.0"))
	float MeleeRange = 200.f;

	/** Ranged “in range” / BT threshold (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI", meta = (ClampMin = "0.0"))
	float RangedRange = 2000.f;

	/** Service “attack range” (sphere / proximity) for target acquisition (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI", meta = (ClampMin = "0.0"))
	float AttackRange = 600.f;

	/**
	 * Optional: row in `DT_EnemyElemental` (or your `FDFElementalAffinityRow` table).
	 * Filled on `ADFEnemyBase` if `ElementalAffinityTable` / BP default is set.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Elemental")
	FName ElementalAffinityRowName;

	/** If set, used instead of the enemy's `DefaultElementalAffinityTable` for this row. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Elemental")
	TObjectPtr<UDataTable> ElementalAffinityTableOverride = nullptr;

	/** World locations for UDFBTTask_FindPatrolPoint; cyclic index in blackboard. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI")
	TArray<FVector> PatrolPathPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI")
	TArray<TSoftObjectPtr<UAnimMontage>> TauntMontages;

	/**
	 * Opcional: reproduzida uma vez ao aplicar a linha no spawn (`InitializeFromDataTable`).
	 * Multicast para todos os clientes; no servidor a árvore de comportamento pode esperar até o fim (ver @a bDelayAIUntilSpawnBirthMontageFinishes).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Spawn")
	TSoftObjectPtr<UAnimMontage> SpawnBirthMontage;

	/** Se true e `SpawnBirthMontage` estiver definida, o servidor adia `RunBehaviorTree` até acabar a duração da montagem. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Spawn")
	bool bDelayAIUntilSpawnBirthMontageFinishes = true;
};

/** Row in a DataTable of loot pools: references multiple item row names in DT_Items. Used by chests. */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFLootPoolTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** FNames of rows in your item table (FDFItemTableRow) for this pool. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Loot")
	TArray<FName> ItemRowNames;
};

/** Character class / archetype (starting mesh, base stats, ability row names in DT_Abilities). */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFClassTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class")
	FText ClassName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class")
	FText ClassDescription;

	/** e.g. "Tanque Corpo a Corpo" (class selection UI). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|UI")
	FText ClassArchetype;

	/** 1-5; drives skull pips. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|UI", meta = (ClampMin = "1", ClampMax = "5"))
	int32 DifficultyPips = 3;

	/** e.g. "Agressivo" (pill badge in details). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|UI")
	FText PlaystyleTag;

	/** Class-themed UI: title, borders, name under preview. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|UI")
	FLinearColor ClassColor = FLinearColor(0.9f, 0.75f, 0.2f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|UI")
	TObjectPtr<UTexture2D> ClassPortrait = nullptr;

	/** Optional idle loop in the 3D class preview; leave null to keep mesh bind pose. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|UI|Preview")
	TSoftObjectPtr<UAnimSequence> ClassPreviewIdle;

	/** Optional burst when swapping class on the turntable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|UI|Preview|VFX")
	TSoftObjectPtr<UNiagaraSystem> ClassChangeVFX;

	/**
	 * Preview mesh socket for @ref ClassChangeVFX (e.g. pelvis, spine_03). If None, spawns at mesh origin + @ref ClassChangeVFXRelativeTransform.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|UI|Preview|VFX")
	FName ClassChangeVFXAttachSocket = NAME_None;

	/** Local offset from socket or mesh origin when spawning class-change VFX in selection preview. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|UI|Preview|VFX")
	FTransform ClassChangeVFXRelativeTransform = FTransform(FVector(0.f, 0.f, 40.f));

	/** Multiplies or drives a simple tint on preview MID (setup in BP or future cosmetic hook). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|UI|Preview")
	FLinearColor PreviewCosmeticTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|Visuals")
	TObjectPtr<USkeletalMesh> CharacterMesh = nullptr;

	/**
	 * Run pawn Blueprint (e.g. BP_JCHero_Character for Knight, BP_ElfCharacter for Warrior).
	 * @c ADFRunGameMode::GetDefaultPawnClassForController. If unset, uses game mode default + @ref CharacterMesh swap.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|Pawn")
	TSoftClassPtr<ADFPlayerCharacter> CharacterClass;

	/**
	 * Nexus hub pawn Blueprint. @c ADFNexusGameMode::GetDefaultPawnClassForController.
	 * If unset, uses @c ADFNexusGameMode::NexusPawnClass + optional @ref CharacterMesh on the body mesh.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|Pawn")
	TSoftClassPtr<APawn> NexusCharacterClass;

	/** Row names in DT_Abilities for starting grants. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|Abilities")
	TArray<FName> StartingAbilities;

	/**
	 * Row name in DT_Items: granted to the inventory and equipped in the Weapon paper-doll slot once per run when the run begins.
	 * Only applied when @ref UDFRunManager has not marked the starter weapon as granted yet.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|Starting Gear")
	FName StartingWeaponItemRow;

	/**
	 * While the weapon slot is empty, TryActivatePrimaryMeleeGameplayAbility activates this tag first
	 * (typically a GA granted via StartingAbilities / DT_Abilities). If unset, Warrior.MeleeSwing is used when granted.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|Combat", meta = (Categories = "Ability"))
	FGameplayTag DefaultUnarmedMeleeAbilityTag;

	/** Unarmed combo montages (soco) when Weapon slot is empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|Combat")
	TArray<TSoftObjectPtr<UAnimMontage>> UnarmedMeleeComboMontages;

	/** Fallback armed montages when the equipped weapon row has no WeaponMeleeComboMontages. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|Combat")
	TArray<TSoftObjectPtr<UAnimMontage>> ArmedMeleeComboMontagesFallback;

	/** Per-step armed combo when weapon row has no @c WeaponMeleeComboSteps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|Combat")
	TArray<FDFComboStep> ArmedMeleeComboStepsFallback;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|Combat|Heavy")
	TSoftObjectPtr<UAnimMontage> ArmedChargeWindupMontageFallback;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|Combat|Heavy")
	TSoftObjectPtr<UAnimMontage> ArmedHeavyChargeReleaseMontageFallback;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|Combat")
	TSoftObjectPtr<UAnimMontage> ArmedHeavyAttackMontageFallback;

	/** Max-tier heavy fallback when the weapon row has no @c WeaponMaxHeavyAttackMontage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|Combat")
	TSoftObjectPtr<UAnimMontage> ArmedMaxHeavyAttackMontageFallback;

	/** Optional per-step Backward / Side directional combo overrides (used when owner is moving in that direction). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|Combat|Directional")
	TArray<TSoftObjectPtr<UAnimMontage>> ArmedBackwardMeleeComboMontagesFallback;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|Combat|Directional")
	TArray<TSoftObjectPtr<UAnimMontage>> ArmedSideMeleeComboMontagesFallback;

	/** Optional 8-way per-step overrides (class-wide fallback when combo row has no directional data). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|Combat|Directional")
	TArray<FDFComboDirectionalMontageSet> ArmedDirectionalMeleeComboMontagesFallback;

	/** Class-wide aerial juggle steps when weapon combo row has no @c AerialSteps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|Combat|Aerial")
	TArray<FDFComboStep> ArmedAerialMeleeComboStepsFallback;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|GAS")
	TMap<FGameplayAttribute, float> BaseAttributeValues;
};

/** Procedural or authored dungeon floor composition. */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFDungeonFloorRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon|Floor", meta = (ClampMin = "0"))
	int32 FloorNumber = 0;

	/** Row names in your enemy table valid for this floor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon|Floor")
	TArray<FName> PossibleEnemyRows;

	/**
	 * When true, SpawnEnemies does not spawn grunts or boss from this row — use ADFEnemySpawnerActor / PCG only.
	 * Avoids origin fallback stacking with manual spawners and clears PCG warnings for authored encounters.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon|Floor")
	bool bUseManualEnemyLayoutOnly = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon|Floor", meta = (ClampMin = "0"))
	int32 MinEnemies = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon|Floor", meta = (ClampMin = "0"))
	int32 MaxEnemies = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon|Floor")
	bool bHasBoss = false;

	/** Row name in the enemy table when bHasBoss is true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon|Floor|Boss", meta = (EditCondition = "bHasBoss"))
	FName BossEnemyRow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon|Floor", meta = (ClampMin = "0.0"))
	float DifficultyMultiplier = 1.f;
};
