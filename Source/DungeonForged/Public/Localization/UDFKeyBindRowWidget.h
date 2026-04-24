// Source/DungeonForged/Public/Localization/UDFKeyBindRowWidget.h
#pragma once

#include "CoreMinimal.h"
#include "UI/UDFUserWidgetBase.h"

DECLARE_DELEGATE_OneParam(FOnKeyBindRequest, FName);

#include "UDFKeyBindRowWidget.generated.h"

class UButton;
class UTextBlock;

/**
 * C++ base for WBP_KeyBindRow: one Enhanced Input mappable row (label + rebind).
 * MappingName must match the mappable "MappingName" in your IMC / Input Action.
 */
UCLASS()
class DUNGEONFORGED_API UDFKeyBindRowWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "DF|Input")
	FName MappingName = NAME_None;

	UFUNCTION(BlueprintCallable, Category = "DF|Input")
	void SetCurrentKeyText(const FText& T);

	UFUNCTION(BlueprintCallable, Category = "DF|Input")
	const FName& GetMappingName() const { return MappingName; }

	UFUNCTION(BlueprintCallable, Category = "DF|Input")
	void SetActionLabelText(const FText& T);

	/** C++-bound from the options screen when a row is created. */
	FOnKeyBindRequest OnRequestRebind;

	UFUNCTION()
	void HandleKeyButtonPressed();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ActionLabel = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> KeyButton = nullptr;

	/** If absent, the current key is not shown in C++ (add a child Text in the button in WBP). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KeyNameText = nullptr;
};
