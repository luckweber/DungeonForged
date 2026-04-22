// Source/DungeonForged/Public/Data/DFDataTableStructs.h
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Engine/DataTable.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "GameplayTagContainer.h"
#include "GAS/UDFGameplayAbility.h"
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
	Weapon     UMETA(DisplayName = "Weapon"),
	Armor      UMETA(DisplayName = "Armor"),
	Helmet     UMETA(DisplayName = "Helmet"),
	Ring       UMETA(DisplayName = "Ring"),
	Amulet     UMETA(DisplayName = "Amulet"),
	Consumable UMETA(DisplayName = "Consumable"),
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
