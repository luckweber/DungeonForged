// Source/DungeonForged/Public/Input/DFInputConfig.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "InputAction.h"
#include "DFInputConfig.generated.h"

/**
 * GAS "local" input id — passed to FGameplayAbilitySpec::InputID and
 * UAbilitySystemComponent::AbilityLocalInputPressed/Released. Keep grant
 * InputID values in sync (same int32) when using input-based activation.
 */
UENUM(BlueprintType)
enum class EDFAbilityInput : uint8
{
	None = 0,
	/** Skill bar 1 (e.g. Q) */
	Ability1 = 1,
	/** Skill bar 2 (e.g. E) */
	Ability2 = 2,
	/** Skill bar 3 (e.g. R) */
	Ability3 = 3,
	/** Skill bar 4 (e.g. F) */
	Ability4 = 4,
	/** Basic attack */
	Attack = 5,
	/** Interact (e.g. G) */
	Interact = 6,
	/** Sprint (hold) */
	Sprint = 7,
	/** Dodge / evade GAs */
	Dodge = 8,
	/** Target lock / soft lock */
	LockOn = 9,
	/** Inventory / character sheet */
	ToggleInventory = 10,
};

USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFInputAction
{
	GENERATED_BODY()

	/** The Enhanced Input action asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> Action = nullptr;

	/**
	 * Project tag for this binding (e.g. Input.Action.Move). Used to resolve actions from code
	 * and to document intent; not required for GAS routing.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (Categories = "Input"))
	FGameplayTag InputTag;

	/**
	 * When > 0, used as the int32 for AbilityLocalInputPressed/Released and should match
	 * the InputID on granted FGameplayAbilitySpec. When 0, BindAbilityInputFromConfig falls
	 * back to 1-based array index for the AbilityInputActions list only.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|GAS", meta = (ClampMin = "0", UIMin = "0"))
	int32 GameplayInputId = 0;
};

UCLASS(BlueprintType)
class DUNGEONFORGED_API UDFInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Non-ability: move, look, jump, camera zoom, UI, etc. (lookup by tag). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TArray<FDFInputAction> NativeInputActions;

	/** Abilities: each row's Action is bound to GAS local input; use GameplayInputId or list order. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|GAS")
	TArray<FDFInputAction> AbilityInputActions;

	/** First NativeInputActions entry whose InputTag matches (including parent tags). */
	const UInputAction* FindNativeInputActionByTag(const FGameplayTag& Tag) const;
};
