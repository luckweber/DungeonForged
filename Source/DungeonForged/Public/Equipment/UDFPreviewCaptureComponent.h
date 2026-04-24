// Source/DungeonForged/Public/Equipment/UDFPreviewCaptureComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "UDFPreviewCaptureComponent.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class AActor;

/**
 * Offsets a SceneCapture2D in front of the character for paper-doll UIs. Enable only when the
 * character screen is open to avoid per-frame work.
 */
UCLASS(ClassGroup = (DF), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFPreviewCaptureComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UDFPreviewCaptureComponent();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Preview")
	TObjectPtr<USceneCaptureComponent2D> SceneCapture = nullptr;

	/** If set in editor or at runtime, assigned to the capture. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Preview")
	TObjectPtr<UTextureRenderTarget2D> RenderTarget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Preview", meta = (ClampMin = "10.0"))
	float OrthoWidth = 400.f;

	/** Extra yaw on the root (e.g. mouse drag rotation). */
	UPROPERTY(BlueprintReadWrite, Transient, Category = "DF|Preview")
	float OrbitYawDegrees = 0.f;

	/** Wire this actor into ShowOnly; capture updates only when bCaptureEveryFrame is on. */
	UFUNCTION(BlueprintCallable, Category = "DF|Preview")
	void SetPreviewTarget(AActor* OnlyShow);

	/** Turn capture on while the character panel is open. */
	UFUNCTION(BlueprintCallable, Category = "DF|Preview")
	void SetPreviewActive(bool bActive);

	UFUNCTION(BlueprintCallable, Category = "DF|Preview")
	void AddOrbitDeltaYaw(float DeltaDegrees);

	/** Assigns RT to the scene capture (call when opening the character screen). */
	UFUNCTION(BlueprintCallable, Category = "DF|Preview")
	void SetRenderTargetForCapture(UTextureRenderTarget2D* RT);

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;

	/** For Blueprint child classes: default-subobject + nested attach can CDO/instance mismatch. Create capture only for live actors. */
	void EnsureRuntimeSceneCapture();

	void ApplyOrbit();
};
