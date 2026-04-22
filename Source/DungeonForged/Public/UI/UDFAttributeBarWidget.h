// Source/DungeonForged/Public/UI/UDFAttributeBarWidget.h
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayEffectTypes.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFAttributeBarWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS(Blueprintable, BlueprintType)
class DUNGEONFORGED_API UDFAttributeBarWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

public:
	UDFAttributeBarWidget(const FObjectInitializer& ObjectInitializer);

	/** e.g. UDFAttributeSet::GetHealthAttribute() */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|UI|GAS")
	FGameplayAttribute TrackedAttribute;

	/** e.g. UDFAttributeSet::GetMaxHealthAttribute(); leave unset for bar fill against 1.0 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|UI|GAS")
	FGameplayAttribute MaxAttribute;

protected:
	virtual void NativeConstruct() override;

	void OnAttributeChanged(const FOnAttributeChangeData& Data);

	void RefreshFromASC();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> AttributeBar = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ValueText = nullptr;
};
