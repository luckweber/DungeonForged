// Source/DungeonForged/Public/GameModes/MainMenu/UDFSplashScreenUserWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UDFSplashScreenUserWidget.generated.h"

class UImage;
class AHUD;

USTRUCT(BlueprintType)
struct FDFSplashPhaseConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Splash")
	float FadeInSeconds = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Splash")
	float HoldSeconds = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Splash")
	float FadeOutSeconds = 0.8f;
};

UCLASS(Blueprintable)
class DUNGEONFORGED_API UDFSplashScreenUserWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	/** 0: Epic / UE, 1: studio, 2: title. Fewer than 3: remaining steps skipped. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|MainMenu|Splash")
	TArray<UTexture2D*> SplashImages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|MainMenu|Splash")
	TArray<float> HoldDurations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|MainMenu|Splash")
	TArray<FDFSplashPhaseConfig> PhaseConfig;

	/** The HUD that owns this full-screen pass. */
	UFUNCTION(BlueprintCallable, Category = "DF|MainMenu|Splash")
	void SetOwnerHUD(AHUD* InHUD);

	UFUNCTION(BlueprintCallable, Category = "DF|MainMenu|Splash")
	void StartSplashFlow();

	UFUNCTION(BlueprintCallable, Category = "DF|MainMenu|Splash")
	void PlayNextSplash();

	UFUNCTION(BlueprintCallable, Category = "DF|MainMenu|Splash")
	void SkipSplashes();

	/** Remove self and hand off to the HUD. */
	UFUNCTION(BlueprintCallable, Category = "DF|MainMenu|Splash")
	void ShowMainMenu();

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(
		const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UFUNCTION(BlueprintNativeEvent, Category = "DF|MainMenu|Splash")
	void OnSplashIndexChanged(int32 Index, UTexture2D* Texture);
	virtual void OnSplashIndexChanged_Implementation(int32 Index, UTexture2D* Texture);

	/** Title card: optional scale; implement in WBP. */
	UFUNCTION(BlueprintNativeEvent, Category = "DF|MainMenu|Splash")
	void OnTitleCardShown(UTexture2D* TitleTexture);
	virtual void OnTitleCardShown_Implementation(UTexture2D* TitleTexture) {}

	/** Blueprint may drive opacity on @c BindWidget. */
	UFUNCTION(BlueprintNativeEvent, Category = "DF|MainMenu|Splash")
	void ApplySplashVisibleAlpha(float Opacity, int32 Index);
	virtual void ApplySplashVisibleAlpha_Implementation(float Opacity, int32 Index);

	void SchedulePhaseTimers();
	void AdvanceAfterFadeInHold();
	void AdvanceAfterFadeOut();
	float GetDefaultHold() const;
	FDFSplashPhaseConfig GetOrCreatePhaseConfig(int32 Index) const;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "DF|MainMenu|Splash", meta = (AllowPrivateAccess = true))
	int32 CurrentSplash = 0;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UImage> SplashImage = nullptr;

	/** Optional subtitle on title card. */
	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> SubtitleText = nullptr;

	FTimerHandle PhaseTimer1;
	FTimerHandle PhaseTimer2;
	FTimerHandle PhaseTimer3;
	double ShownTimeSeconds = 0.0;

	TWeakObjectPtr<AHUD> OwnerHUD;

	UFUNCTION()
	void OnFadeInEnd();

	UFUNCTION()
	void OnHoldEnd();

	UFUNCTION()
	void OnFadeOutEnd();
};
