// Source/DungeonForged/Public/World/UDFLoadingScreenSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "World/DFWorldTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UDFLoadingScreenSubsystem.generated.h"

class UUserWidget;
class UDFLoadingScreenWidget;
class UDataTable;
class UProgressBar;
class UTextBlock;

/** Fullscreen loading UX + dicas opcionais via DT. Project Settings: Dungeon Forged | Loading Screen (@c UDFLoadingScreenDeveloperSettings). */
UCLASS()
class DUNGEONFORGED_API UDFLoadingScreenSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(EditAnywhere, Category = "DF|Loading")
	TSubclassOf<UUserWidget> LoadingScreenClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ActiveLoadingScreen;

	/**
	 * Enforced in @ref HideLoadingScreen. Também pode sobrescrever em
	 * Edit -> Project Settings -> Dungeon Forged | Loading Screen.
	 */
	UPROPERTY(EditAnywhere, Category = "DF|Loading", meta = (ClampMin = "0.0"))
	float MinLoadingTime = 2.f;

	float LoadingStartTime = 0.f;

	/**
	 * Full-screen load UI (Z 100) + progress behaviour. Pairs with @ref HideLoadingScreen on map load.
	 * @param Reason drives title/flavor (designer WBP or future data-driven rows).
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|Loading")
	void ShowLoadingScreen(ETravelReason Reason, int32 NextFloorNumber = 1, int32 MaxFloors = 10);

	UFUNCTION(BlueprintCallable, Category = "DF|Loading")
	void HideLoadingScreen();
protected:
	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);
	void ApplyDeveloperLoadingDefaults();
	void CancelLoadingProgressTicker();
	bool OnLoadingProgressTicker(float DeltaTime);
	void FinishHideAfterMinTime();
	void OnFadeOutRemoveWidget();
	void ClearFallbackBindings();
	void TryDiscoverFallbackControls(UUserWidget* ScreenRoot);
	void FallbackSetLoadingProgress(float Pct, bool bSnapComplete);

	UPROPERTY(Transient)
	TObjectPtr<UDFLoadingScreenWidget> ActiveLoadingCxx = nullptr;

	/** Resolvido de @c UDFLoadingScreenDeveloperSettings::TipsTable em @ref ApplyDeveloperLoadingDefaults. */
	UPROPERTY(Transient)
	TObjectPtr<UDataTable> CachedTipsTable = nullptr;

	/** CoreTicker sobrevive a OpenLevel onde o TimerManager do UWorld anterior some. */
	FTSTicker::FDelegateHandle ProgressTickerHandle;

	FTimerHandle MinTimeTimer;
	FTimerHandle FadeTimer;
	FDelegateHandle PostLoadMapHandle;

	int32 LerpStepIndex = 0;
	static constexpr int32 LerpStepCount = 30; // alvo de fase 1 (0 -> 90% ao longo de ~LerpStepCount ticks)
	bool bMapLoaded = false;
	/** Progresso apresentado (interp suave); fase 1 persegue alvo falso, fase 2 vai a 1. */
	float CurrentFilledPct = 0.f;
	ETravelReason ShownReason = ETravelReason::FirstLaunch;

	/** Widget sem @c UDFLoadingScreenWidget: actualiza ProgressBar/Text por nome. */
	TWeakObjectPtr<UProgressBar> FallbackLoadingBar;
	TWeakObjectPtr<UTextBlock> FallbackPctText;
};
