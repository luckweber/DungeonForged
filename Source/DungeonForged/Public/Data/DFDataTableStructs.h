// Source/DungeonForged/Public/Data/DFDataTableStructs.h
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Engine/DataTable.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimMontage.h"
#include "GAS/UDFGameplayAbility.h"
#include "Equipment/DFEquipmentTypes.h"
#include "DFDataTableStructs.generated.h"

class AActor;
class UBehaviorTree;
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

	/** Optional text for the selection card (GAS cost/cooldown can be separate). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Abilities|UI")
	FText DisplayCooldown = FText::GetEmpty();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Abilities|UI")
	FText DisplayCost = FText::GetEmpty();
};

/** One GameplayEffect that initializes base attributes (e.g. modifiers on UDFAttributeSet); used with InitializeAttributesFromDataTable. */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFAttributeInitTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TSubclassOf<UGameplayEffect> StartupGameplayEffect;
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
	TArray<TObjectPtr<UAnimMontage>> TauntMontages;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|Visuals")
	TObjectPtr<USkeletalMesh> CharacterMesh = nullptr;

	/** Row names in DT_Abilities for starting grants. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|Abilities")
	TArray<FName> StartingAbilities;

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
