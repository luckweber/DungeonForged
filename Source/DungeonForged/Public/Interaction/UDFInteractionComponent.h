// Source/DungeonForged/Public/Interaction/UDFInteractionComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UDFInteractionComponent.generated.h"

class ACharacter;
class APlayerCameraManager;

UCLASS(ClassGroup = (DF), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFInteractionComponent();

	UFUNCTION(BlueprintCallable, Category = "DF|Interaction")
	void TryInteract();

	UFUNCTION(BlueprintCallable, Category = "DF|Interaction")
	void RegisterInteractable(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "DF|Interaction")
	void UnregisterInteractable(AActor* Actor);

	AActor* GetCurrentFocused() const { return CurrentFocusedActor.Get(); }

	/** Icon passed to the prompt (optional). */
	UPROPERTY(EditAnywhere, Category = "DF|Interaction|UI")
	TObjectPtr<UTexture2D> InteractKeyIcon = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(Server, Reliable)
	void Server_Interact(AActor* Target);

	UPROPERTY(EditAnywhere, Category = "DF|Interaction", meta = (ClampMin = "0.0"))
	float InteractTraceRange = 300.f;

	/** How wide the forward cone is when picking a nearby focus (0.85 ≈ 30°). */
	UPROPERTY(EditAnywhere, Category = "DF|Interaction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FocusDirectionDot = 0.7f;

	TWeakObjectPtr<AActor> CurrentFocusedActor;
	TArray<TWeakObjectPtr<AActor>> NearbyInteractables;

	void RecomputeCurrentFocus();
	bool TraceInteractedActor(AActor*& OutActor) const;
	AActor* PickBestFromNearby() const;
};
