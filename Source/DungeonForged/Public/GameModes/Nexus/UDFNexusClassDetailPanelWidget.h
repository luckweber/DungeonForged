// Source/DungeonForged/Public/GameModes/Nexus/UDFNexusClassDetailPanelWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UDFNexusClassDetailPanelWidget.generated.h"

class UTextBlock;
class UProgressBar;

/**
 * Painel de detalhe da classe (texto da DT_Class + barras de atributo normalizadas).
 * Use @c UDFNexusClassCardWidget apenas como tile (ícone + nome no TileView).
 */
UCLASS(Blueprintable)
class DUNGEONFORGED_API UDFNexusClassDetailPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Lê @c DT_Class via subsistema + @c GetStatBarScalesForClass. */
	UFUNCTION(BlueprintCallable, Category = "Nexus|UI")
	void RefreshForClass(FName ClassRow);

protected:
	void ClearOptionalWidgets();

	/** Fallback quando o TextBlock no WBP não tem o nome exacto @c DetailClassName (ex.: @c ClassName). */
	UTextBlock* ResolveClassTitleTextBlock() const;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailClassName = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailDescription = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailArchetype = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailPlaystyle = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailDifficulty = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> BarStrength = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> BarInt = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> BarAgi = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> BarDefense = nullptr;
};
