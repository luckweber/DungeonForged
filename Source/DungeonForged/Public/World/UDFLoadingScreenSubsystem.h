// Source/DungeonForged/Public/World/UDFLoadingScreenSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "World/DFWorldTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UDFLoadingScreenSubsystem.generated.h"

class UUserWidget;
class UDFLoadingScreenWidget;

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

	/** Enforced in @ref HideLoadingScreen. */
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
	void LerpProgressStep();
	void FinishHideAfterMinTime();
	void OnFadeOutRemoveWidget();

	UPROPERTY(Transient)
	TObjectPtr<UDFLoadingScreenWidget> ActiveLoadingCxx = nullptr;

	FTimerHandle ProgressTimer;
	FTimerHandle MinTimeTimer;
	FTimerHandle FadeTimer;
	FTimerDelegate ProgressTickDelegate;
	FDelegateHandle PostLoadMapHandle;

	int32 LerpStepIndex = 0;
	static constexpr int32 LerpStepCount = 30; // ~1.5s at 0.05
	ETravelReason ShownReason = ETravelReason::FirstLaunch;
};
