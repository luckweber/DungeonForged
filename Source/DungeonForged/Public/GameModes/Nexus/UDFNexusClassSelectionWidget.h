// Source/DungeonForged/Public/GameModes/Nexus/UDFNexusClassSelectionWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UDFNexusClassSelectionWidget.generated.h"

class UButton;
class UTileView;
class UDFNexusClassListObject;

UCLASS(Blueprintable)
class DUNGEONFORGED_API UDFNexusClassSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Populates the tile with @a UDFNexusClassListObject entries (Blueprint may override). */
	UFUNCTION(BlueprintCallable, Category = "Nexus|UI")
	void RefreshUnlockedList(const TArray<UDFNexusClassListObject*>& InItems);

	UFUNCTION(BlueprintCallable, Category = "Nexus|UI")
	void SetSelectedClassRow(FName ClassRow) { CurrentSelected = ClassRow; }

	/** Disabled until a class is chosen. */
	UFUNCTION(BlueprintCallable, Category = "Nexus|UI")
	void ConfirmAndTravel();

	/** UI-only: hook for opening meta upgrades. */
	UFUNCTION(BlueprintCallable, Category = "Nexus|UI")
	void OpenMetaUpgradesOverlay();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTileView> ClassTileView = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UButton> StartRunButton = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UButton> UpgradesButton = nullptr;

	FName CurrentSelected = NAME_None;

	UFUNCTION()
	void OnStartRunClicked();

	UFUNCTION()
	void OnUpgradesClicked();
};
