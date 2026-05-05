// Source/DungeonForged/Public/GameModes/Nexus/UDFNexusClassSelectionWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UDFNexusClassSelectionWidget.generated.h"

class UButton;
class UImage;
class UTileView;
class UDFNexusClassListObject;

UCLASS(Blueprintable)
class DUNGEONFORGED_API UDFNexusClassSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Replace tile contents. Opcionalmente o Blueprint pode chamar com uma lista propria (ex.: filtro).
	 * Se nunca for chamado, @c NativeConstruct preenche a partir da @c DT de classes via subsistema (evita Nexus vazio).
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|UI")
	void RefreshUnlockedList(const TArray<UDFNexusClassListObject*>& InItems);

	UFUNCTION(BlueprintCallable, Category = "Nexus|UI")
	void SetSelectedClassRow(FName ClassRow);

	/** Disabled until a class is chosen. */
	UFUNCTION(BlueprintCallable, Category = "Nexus|UI")
	void ConfirmAndTravel();

	/** UI-only: hook for opening meta upgrades. */
	UFUNCTION(BlueprintCallable, Category = "Nexus|UI")
	void OpenMetaUpgradesOverlay();

protected:
	virtual void NativeConstruct() override;

	/** Mesmo nome de widget que @c UDFClassSelectionWidget quando o modo é @c SceneCaptureUMG (RT na Image). */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UImage> PreviewRenderTarget = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTileView> ClassTileView = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UButton> StartRunButton = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UButton> UpgradesButton = nullptr;

	FName CurrentSelected = NAME_None;

	void RefreshPreviewBrushFromSubsystem();

	/** Construtor da lista quando o WBP não chama RefreshUnlockedList explicitamente. */
	void RebuildClassTileItemsFromSubsystem();

	UFUNCTION()
	void OnStartRunClicked();

	UFUNCTION()
	void OnUpgradesClicked();

	void OnClassTileSelectionChanged(UObject* Item);
};
