// Source/DungeonForged/Public/UI/ClassSelection/UDFClassListEntryWidget.h
#pragma once

#include "CoreMinimal.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFClassListEntryWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UDFClassSelectionWidget;

UCLASS()
class DUNGEONFORGED_API UDFClassListEntryWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	/** Called from parent list builder. */
	void InitializeEntry(const FName InClassRow, UDFClassSelectionWidget* InOwner, bool bInUnlocked, bool bInSelected);
	FName GetClassRow() const { return ClassRow; }

	UFUNCTION()
	void OnSelectPressed();

	UFUNCTION()
	void OnHovered();
	UFUNCTION()
	void OnUnhovered();

	void RefreshVisual();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UImage> ClassPortrait = nullptr;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> ClassName = nullptr;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> ClassArchetype = nullptr;
	/** 5 optional skull/pip images; bind Pip0..Pip4 in BP. */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UImage> DifficultyPip0;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UImage> DifficultyPip1;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UImage> DifficultyPip2;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UImage> DifficultyPip3;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UImage> DifficultyPip4;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UImage> LockOverlay = nullptr;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> UnlockHintText = nullptr;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UButton> SelectButton = nullptr;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UImage> SelectionBorder;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UImage> EntryBackground;

	FName ClassRow;
	UPROPERTY(Transient)
	TObjectPtr<UDFClassSelectionWidget> OwnerScreen = nullptr;
	bool bUnlocked = true;
	bool bSelected = false;
};
