// Source/DungeonForged/Public/UI/UDFInGameHUDWidget.h
#pragma once

#include "CoreMinimal.h"
#include "UI/UDFUserWidgetBase.h"
#include "UI/Status/UDFStatusEffectBarWidget.h"
#include "UDFInGameHUDWidget.generated.h"

class UTextBlock;
class UImage;
class UWidgetAnimation;

UCLASS(Blueprintable, Abstract)
class DUNGEONFORGED_API UDFInGameHUDWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleReplicatedRunGold(int32 NewTotal);

	/** Short pop on the label when gold increases. */
	void PlayGoldPulse();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GoldText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CoinIcon = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> GoldChangePulseAnim = nullptr;

	/** WBP: optional player buff / debuff status rows (Binds in Designer if present). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDFStatusEffectBarWidget> DFStatusEffectBar = nullptr;

	int32 LastGoldShown = 0;
};
