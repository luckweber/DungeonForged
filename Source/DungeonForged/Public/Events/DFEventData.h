// Source/DungeonForged/Public/Events/DFEventData.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "DFEventData.generated.h"

class UGameplayEffect;
class UTexture2D;

UENUM(BlueprintType)
enum class EEventOutcomeType : uint8
{
	None UMETA(Hidden),
	/** Apply heal GE; SetByCaller `Data.Healing` = `OutcomeValue` (or percent if `bOutcomeValueIsPercentOfMax`). */
	Heal,
	/** True damage: `Data.Damage` = `OutcomeValue` (or % max HP if `bOutcomeValueIsPercentOfMax`). */
	Damage,
	/** `AddRunGold` with amount = `FMath::RoundToInt(OutcomeValue)`. */
	Gold,
	/** `AbilityRowName` in DT_Abilities; `UDFAbilitySelectionSubsystem::GrantSelectedAbility`. */
	AddAbility,
	/** Remove one random entry from the run’s granted-abilities list, then re-grant. */
	RemoveAbility,
	/** Apply `EffectClass` to the player (duration policy comes from the GE asset, often Infinite). */
	AddEffect,
	/** No mechanical effect. */
	Nothing,
	/** Picks a small positive roll (heal, gold) — uses internal weights. */
	RandomGood,
	/** Picks a small negative roll (damage, gold loss) — uses internal weights. */
	RandomBad,
	/**
	 * `AbilityRowName` = row in `UDFRunManager::ItemDataTable` (DT_Items);
	 * `OutcomeValue` = quantity (default 1).
	 */
	AddItem,
	/**
	 * `UDFRunManager::MulEnemyOutgoingDamageScale(OutcomeValue)` (e.g. 1.2f = +20% for enemies this run).
	 */
	ScaleEnemyDamage,
	/**
	 * Blueprint / level hook: `UDFRandomEventSubsystem::OnRequestSpecialEncounter` broadcasts
	 * with `EventRowName` and `OutcomeValue` (designer id).
	 */
	SpawnSpecialEncounter,
	/**
	 * 50/50: Heal with `OutcomeValue` (and `bOutcomeValueIsPercentOfMax`) **or** Damage with
	 * `SecondaryOutcomeValue` (same percent rules). e.g. fountain "drink".
	 */
	RandomHealOrDamage,
	/** Remove one random granted ability, then grant one random (same rarity rules as `TryGrantRandomAbilityByRarity`). */
	SwapRandomAbility
};

USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFEventChoice
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	FText ChoiceText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	FText OutcomeText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	EEventOutcomeType OutcomeType = EEventOutcomeType::Nothing;

	/** Meaning depends on `OutcomeType` and `bOutcomeValueIsPercentOfMax` (heal/damage as % of max health). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (ClampMin = "0.0"))
	float OutcomeValue = 0.f;

	/**
	 * When true, Heal and Damage use `OutcomeValue` as 0..100 percent of **max** health
	 * (and gold still uses the rounded absolute for Gold type).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	bool bOutcomeValueIsPercentOfMax = false;

	/** e.g. `RandomHealOrDamage` bad branch magnitude; also used by some designer-tuned double outcomes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (ClampMin = "0.0"))
	float SecondaryOutcomeValue = 0.f;

	/** For `AddAbility` — `DT_Abilities` row name. For `AddItem` — `DT_Items` row. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	FName AbilityRowName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	TSubclassOf<UGameplayEffect> EffectClass = nullptr;

	/** For weighted AI hints / future automation (not shown to the player in base UI). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (ClampMin = "0.0"))
	float ChoiceWeight = 1.f;

	/** If non-empty, the choice is only available when the player’s ASC has all of these tags. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	FGameplayTagContainer RequiredTags;

	/**
	 * If &gt; 0, the player’s Agility (from UDFAttributeSet) must be **strictly greater** than this
	 * (e.g. 40 = needs Agility &gt; 40 for steal checks).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (ClampMin = "0.0"))
	float RequiredAgility = 0.f;
};

/**
 * `DT_RandomEvents` row. Examples (author in DataTable asset):
 * - MysteriousAltar, WanderingMerchant, AncientFountain, FallenHero, DemonPact (see design doc in source control).
 */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFRandomEventRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	FText EventTitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (MultiLine = "true"))
	FText EventDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	TObjectPtr<UTexture2D> EventIllustration = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	TArray<FDFEventChoice> Choices;

	/** Inclusive: this row may roll when `CurrentFloor >= MinFloor`. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (ClampMin = "1"))
	int32 MinFloor = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (ClampMin = "0.0"))
	float Weight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	bool bCanRepeat = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	FGameplayTagContainer EventTags;
};
