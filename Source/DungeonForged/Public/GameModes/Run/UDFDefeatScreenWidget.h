// Source/DungeonForged/Public/GameModes/Run/UDFDefeatScreenWidget.h
#pragma once

#include "CoreMinimal.h"
#include "GameModes/Run/DFRunTypes.h"
#include "Engine/DataTable.h"
#include "Blueprint/UserWidget.h"
#include "UDFDefeatScreenWidget.generated.h"

class UTextBlock;
class UImage;
class UButton;
class UProgressBar;
class UMaterialInterface;

UCLASS(Blueprintable, Abstract)
class DUNGEONFORGED_API UDFDefeatScreenWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	UFUNCTION(BlueprintCallable, Category = "Run|UI")
	void SetDefeatData(FDFRunSummary const& Summary, FText const& DefeatCause, FText const& OptionalTip);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> BackgroundArt;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> FloorText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CauseText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> MetaXPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TipText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ReturnNexus;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> PlayAgain;

	/** e.g. @c M_PostProcess_Desat — optional MID on @c BackgroundArt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|UI")
	TObjectPtr<UMaterialInterface> DesaturationPostProcessMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Data")
	TObjectPtr<UDataTable> TipsTable = nullptr;

	UFUNCTION()
	void HandleReturnNexus();
	UFUNCTION()
	void HandlePlayAgain();
};
