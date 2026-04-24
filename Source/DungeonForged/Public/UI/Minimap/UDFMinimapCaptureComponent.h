// Source/DungeonForged/Public/UI/Minimap/UDFMinimapCaptureComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "UDFMinimapCaptureComponent.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

/**
 * World-space top-down scene capture. Place on a level actor, assign a 256^2 UTextureRenderTarget2D,
 * then drive UpdatePosition from game code. Uses a 0.1s recapture (not every frame).
 */
UCLASS(ClassGroup = (DF), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFMinimapCaptureComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UDFMinimapCaptureComponent();

	/** Set world XY from PlayerLocation, keep fixed height above. */
	UFUNCTION(BlueprintCallable, Category = "DF|Minimap|Capture")
	void UpdatePosition(FVector PlayerLocation);

	/** Minimap zoom: smaller ortho width = more zoomed in. */
	UFUNCTION(BlueprintCallable, Category = "DF|Minimap|Capture")
	void SetOrthoWidth(float Width);

	/** Pawn/character to follow; capture moves above and recenters ortho. */
	UFUNCTION(BlueprintCallable, Category = "DF|Minimap|Capture")
	void SetTrackedPawn(AActor* InPawn) { TrackedPawn = InPawn; }

	UFUNCTION(BlueprintPure, Category = "DF|Minimap|Capture")
	USceneCaptureComponent2D* GetCapture() const { return CaptureComp; }

	/** Defaults to 3000. Scaled by 1.0/Zoom in widget when driving from SetOrthoWidth. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Minimap|Capture")
	float BaseOrthoWidth = 3000.f;

	/** Render target (designer: 256x256). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Minimap|Capture")
	TObjectPtr<UTextureRenderTarget2D> MinimapRenderTarget = nullptr;

	/** How often the RT is refreshed, in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Minimap|Capture", meta = (ClampMin = "0.01"))
	float UpdateInterval = 0.1f;

	/** Z offset above the tracked floor position for the capture rig. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Minimap|Capture")
	float HeightAbovePlayer = 2500.f;

	/** If true, assign walls/floors to ShowOnly; otherwise render full scene with cheaper show flags. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Minimap|Capture|Advanced")
	bool bFilterWithShowOnly = false;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleUpdateTimer();

	void ApplyDefaultCaptureSettings();
	void Recapture();
	void StartUpdateTimer();
	void StopUpdateTimer();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Minimap|Capture", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneCaptureComponent2D> CaptureComp = nullptr;

	FTimerHandle UpdateTimer;
	TWeakObjectPtr<AActor> TrackedPawn;
};
