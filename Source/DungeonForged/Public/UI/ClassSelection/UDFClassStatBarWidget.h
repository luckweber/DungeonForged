// Source/DungeonForged/Public/UI/ClassSelection/UDFClassStatBarWidget.h
#pragma once

#include "CoreMinimal.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFClassStatBarWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS()
class DUNGEONFORGED_API UDFClassStatBarWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "DF|ClassSelection|UI")
	void SetData(const FText& Label, float NormalizedFill, int32 ValueDisplay, FLinearColor FillColor);
protected:
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> StatLabel;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UProgressBar> StatBar;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> StatValue;
};
