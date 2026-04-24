// Source/DungeonForged/Public/Debug/UDFGASDebugOverlayWidget.h

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "UDFGASDebugOverlayWidget.generated.h"

class UTextBlock;
class UScrollBox;
class UPanelWidget;
class UVerticalBox;
class UCanvasPanel;
class UAbilitySystemComponent;
class UDFAttributeSet;

UCLASS()
class DUNGEONFORGED_API UDFGASDebugOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	/** Pooled row texts for dynamic sections (tags / effects / abilities). */
	TArray<TObjectPtr<UTextBlock>> TagLinePool;
	TArray<TObjectPtr<UTextBlock>> EffectLinePool;
	TArray<TObjectPtr<UTextBlock>> AbilityLinePool;
	float TimeSinceRefresh = 0.f;
	static constexpr float RefreshInterval = 0.2f;

	TObjectPtr<UScrollBox> TagsScroll;
	TObjectPtr<UScrollBox> EffectsScroll;
	TObjectPtr<UVerticalBox> AttributesBox;
	TObjectPtr<UVerticalBox> AbilitiesBox;
	TObjectPtr<UTextBlock> PerfText;

	void EnsureWidgetsBuilt();
	void RefreshContent();
	static FLinearColor ColorForTag(const FGameplayTag& T);
	void EnsureTextPool(
		int32 MinCount, TArray<TObjectPtr<UTextBlock>>& Pool, UPanelWidget* Parent, const FName BaseName);
};
