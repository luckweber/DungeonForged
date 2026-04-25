// Source/DungeonForged/Public/UI/ClassSelection/UDFClassSelectionWidget.h
#pragma once

#include "CoreMinimal.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFClassSelectionWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UScrollBox;
class UWrapBox;
class UPanelWidget;
class UDFClassListEntryWidget;
class UDFClassStatBarWidget;
class UDFAbilityPreviewIconWidget;
class UDFClassAbilityTooltipWidget;
class UDFActiveChallengeIndicatorWidget;
class UDFPartnerClassPreviewWidget;

UCLASS()
class DUNGEONFORGED_API UDFClassSelectionWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "DF|ClassSelection")
	void SelectClassByRow(FName ClassRow);

	UFUNCTION(BlueprintCallable, Category = "DF|ClassSelection")
	void RefreshAll();

	/** Optional: designer sets for challenge + confirm dialog (Prompt 52). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|ClassSelection|Challenge")
	bool bHasActiveChallenge = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|ClassSelection|Challenge")
	FText ActiveChallengeName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|ClassSelection|Challenge")
	FText ActiveChallengeModifierSummary;

	/** Prompt 70: when true, co-op confirm rules apply (hide if false). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|ClassSelection|Coop")
	bool bIsCoopSession = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|ClassSelection|Coop")
	FText CoopPartnerDisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|ClassSelection|Coop")
	bool bLocalPlayerLockedIn = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|ClassSelection|Coop")
	bool bPartnerLockedIn = false;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& InMouseEvent) override;

	void BuildClassList();
	void FillDetailsPanel();
	void FillAbilityRow();
	void ClearChildPanel(class UPanelWidget* Box);
	void UpdateCoopUi();
	void UpdateConfirmButton();

	UFUNCTION() void OnBackClicked();
	UFUNCTION() void OnConfirmClicked();

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UScrollBox> ClassListScroll = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "DF|ClassSelection")
	TSubclassOf<UDFClassListEntryWidget> ClassEntryWidgetClass;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UImage> PreviewRenderTarget = nullptr;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> SelectedClassName = nullptr;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UImage> PreviewBorderImage;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> ClassTitle = nullptr;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> ClassDescription = nullptr;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> PlaystyleTag = nullptr;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UImage> DifficultyBarImage;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DifficultyLabel;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BaseStatsHeader;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<class UVerticalBox> StatBarsBox;
	UPROPERTY(EditDefaultsOnly, Category = "DF|ClassSelection")
	TSubclassOf<UDFClassStatBarWidget> StatBarWidgetClass;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AbilitiesHeader;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> StartingAbilitiesScroll;
	UPROPERTY(EditDefaultsOnly, Category = "DF|ClassSelection")
	TSubclassOf<UDFAbilityPreviewIconWidget> AbilityIconWidgetClass;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MetaUpgradesHeader;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> MetaUpgradesWrap;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HistoryHeader;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RunCountText;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BestFloorText;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WinCountText;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NoHistoryText;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UButton> BackButton = nullptr;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UButton> ConfirmButton = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UDFActiveChallengeIndicatorWidget> ActiveChallengePanel;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CoopStatusText;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UDFPartnerClassPreviewWidget> PartnerClassPreview;

	UPROPERTY(Transient) TObjectPtr<UDFClassAbilityTooltipWidget> AbilityTooltip;

	bool bDraggingPreview = false;
	FVector2D LastMouseScreen = FVector2D::ZeroVector;
};
