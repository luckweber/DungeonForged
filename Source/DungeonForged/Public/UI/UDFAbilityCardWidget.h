// Source/DungeonForged/Public/UI/UDFAbilityCardWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/DFDataTableStructs.h"
#include "UDFAbilityCardWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDFAbilityCardSelect);

UCLASS()
class DUNGEONFORGED_API UDFAbilityCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetupCard(const FDFAbilityTableRow& Row, const FName InRowName);

	FName GetRowName() const { return RowKey; }

	/** Fired when the card's select button is pressed. */
	UPROPERTY(BlueprintAssignable, Category = "DF|Rogue")
	FOnDFAbilityCardSelect OnCardSelectClicked;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> AbilityIcon = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AbilityName = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RarityLabel = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Description = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CooldownText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CostText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SelectButton = nullptr;

	UFUNCTION()
	void HandleSelectPressed();

	void ApplyRarityStyle(EItemRarity Rarity) const;

	FName RowKey = NAME_None;
};
