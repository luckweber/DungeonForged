// Source/DungeonForged/Public/FX/UDFCameraShakeFunctionLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UDFCameraShakeFunctionLibrary.generated.h"

class UCameraShakeBase;
class APlayerController;
class AActor;
class APlayerCameraManager;

UCLASS()
class DUNGEONFORGED_API UDFCameraShakeFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Rotational / light. */
	UFUNCTION(BlueprintCallable, Category = "DF|FX|Camera", meta = (WorldContext = "WorldContextObject"))
	static void PlayLightHitOnOwner(const UObject* WorldContextObject, APlayerController* PC);

	UFUNCTION(BlueprintCallable, Category = "DF|FX|Camera", meta = (WorldContext = "WorldContextObject"))
	static void PlayHeavyHitOnOwner(const UObject* WorldContextObject, APlayerController* PC);

	UFUNCTION(BlueprintCallable, Category = "DF|FX|Camera", meta = (WorldContext = "WorldContextObject"))
	static void PlayBossSlamOnOwner(const UObject* WorldContextObject, APlayerController* PC);

	/**
	 * Radial falloff: scale matches UGameplayStatics::PlayWorldCameraShake (inner/outer in world units).
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|FX|Camera", meta = (WorldContext = "WorldContextObject"))
	static void PlayExplosionShakeAt(
		const UObject* WorldContextObject,
		const FVector& Origin,
		float InnerRadius,
		float OuterRadius,
		float Scale = 1.f);
};
