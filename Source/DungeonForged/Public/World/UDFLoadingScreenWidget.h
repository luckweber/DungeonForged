// Source/DungeonForged/Public/World/UDFLoadingScreenWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "World/DFWorldTypes.h"
#include "UDFLoadingScreenWidget.generated.h"

class UImage;
class UTextBlock;
class UProgressBar;
class UHorizontalBox;

/**
 * C++ base for WBP_LoadingScreen. Subclass in UMG (parent = this class) and match @c BindWidget names.
 * Fake progress, fade, and optional next-floor layout are driven from @ref UDFLoadingScreenSubsystem.
 */
UCLASS(Abstract, Blueprintable)
class DUNGEONFORGED_API UDFLoadingScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ResetForTravel();

	/** 0–1 primary bar; holds near 0.85 with stutter until @a bSnapComplete. */
	void SetLoadingProgress(float Pct, bool bSnapComplete);

	void SetLoadingTitleText(const FText& T);
	void SetFlavorText(const FText& T);
	void SetTip(const FText& Label, const FText& Body);
	void SetFloorCopy(int32 Floor, int32 MaxFloors, const FText& DifficultyLine);
	void SetRunProgress(float Pct01);
	void SetEnemyPreviewTextures(int32 Count, class UTexture2D* A, class UTexture2D* B, class UTexture2D* C);

	/** Visual alpha 0–1 for fade in/out; implemented in BP or here if using color. */
	UFUNCTION(BlueprintCallable, Category = "DF|Loading")
	void SetRootVisualAlpha(float Alpha);

	/** Plays @a FadeOutAnim if bound (typical 0.5s). Called from @ref UDFLoadingScreenSubsystem. */
	UFUNCTION(BlueprintCallable, Category = "DF|Loading")
	void PlayFadeOutAnimation();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> BackgroundArt = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> LogoImage = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LoadingTitle = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> FlavorText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> LoadingBar = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TipLabel = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TipText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> EnemyPreview = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FloorNumber = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FloorDifficulty = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> RunProgress = nullptr;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> FadeInAnim = nullptr;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> FadeOutAnim = nullptr;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> LogoBreathAnim = nullptr;

private:
	float TargetProgress = 0.f;
	float DisplayedProgress = 0.f;
	float StutterTimer = 0.f;
	uint8 bSnapRequest : 1 = false;
	uint8 bBreathStarted : 1 = false;
};
