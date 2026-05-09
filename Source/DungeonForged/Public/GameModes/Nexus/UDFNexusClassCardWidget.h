// Source/DungeonForged/Public/GameModes/Nexus/UDFNexusClassCardWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "UDFNexusClassCardWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UTexture2D;

/**
 * Tile no TileView: icone + nome + opcional moldura (Border_icone) com estados normal / hover / seleccionado.
 */
UCLASS()
class DUNGEONFORGED_API UDFNexusClassCardWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Nexus|UI")
	void SetClassData(
		const FName& ClassRow,
		const FText& Name,
		bool bLocked,
		const FText& LockText,
		UTexture2D* Portrait = nullptr);

	FName GetClassRow() const { return ClassRow; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	/** Aplica FrameBrush* em Border_icone conforme hover + seleção do TileView. */
	void RefreshCardFrame();

	/** Imagem vazia num brush = esse brush nao conta; usa-se o proximo fallback na cadeia. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nexus|Card|Frame")
	FSlateBrush FrameBrushNormal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nexus|Card|Frame")
	FSlateBrush FrameBrushHover;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nexus|Card|Frame")
	FSlateBrush FrameBrushSelected;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UImage> ClassArt = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> ClassName = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UImage> LockOverlay = nullptr;

	/** Moldura no BP; o nome do widget tem de ser exactamente Border_icone para o BindWidgetOptional. */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_icone = nullptr;

	UPROPERTY(Transient)
	uint8 bListEntryHovered : 1 = false;

	UPROPERTY(Transient)
	uint8 bListEntrySelected : 1 = false;

	FName ClassRow = NAME_None;

	FSlateBrush ResolveFrameBrush() const;
};
