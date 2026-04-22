// Source/DungeonForged/Public/Characters/ADFPlayerState.h

#pragma once

#include "CoreMinimal.h"
#include "DFDataTableStructs.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "ADFPlayerState.generated.h"

class UAbilitySystemComponent;
class UDataTable;
class UDFAttributeSet;

UCLASS()
class DUNGEONFORGED_API ADFPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ADFPlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UDFAttributeSet> AttributeSet;

	virtual void BeginPlay() override;

	/** Server / authority: grants every valid row in the table to this PlayerState's ASC. */
	UFUNCTION(BlueprintCallable, Category = "GAS")
	void GrantAbilitiesFromDataTable(UDataTable* AbilityTable);

	/**
	 * Server / authority: applies the startup Gameplay Effect from the named row (typically attribute base values).
	 * Call after InitAbilityActorInfo on the avatar so attributes are registered.
	 */
	UFUNCTION(BlueprintCallable, Category = "GAS")
	void InitializeAttributesFromDataTable(UDataTable* AttributeTable, FName RowName);
};
