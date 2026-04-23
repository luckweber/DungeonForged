// Source/DungeonForged/Public/Interaction/ADFDoor.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Interaction/ADFInteractableBase.h"
#include "Interaction/DFInteractionTypes.h"
#include "ADFDoor.generated.h"

class UCurveFloat;
class UTimelineComponent;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFDoor : public ADFInteractableBase
{
	GENERATED_BODY()

public:
	ADFDoor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual FText GetInteractionText_Implementation() const override;
	virtual void Interact_Implementation(ACharacter* Interactor) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "DF|Interaction")
	void LockDoor();

	UFUNCTION(BlueprintCallable, Category = "DF|Interaction")
	void UnlockDoor();

	/** Local mesh yaw when closed (default uses actor rotation on spawn). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "DF|Door")
	float ClosedMeshYaw = 0.f;

	/** Degrees to lerp to when open. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Door", meta = (ClampMin = "-120.0", ClampMax = "120.0"))
	float OpenMeshYaw = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Door")
	EDFDoorType DoorType = EDFDoorType::ExitDoor;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "DF|Door")
	bool bIsLocked = false;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "DF|Door")
	bool bIsOpen = false;

	/** If set, drives alpha 0-1; otherwise uses linear. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Door|Timeline")
	TObjectPtr<UCurveFloat> OpenCurve = nullptr;

	/** If > 0, used when no curve. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Door|Timeline", meta = (ClampMin = "0.0"))
	float OpenDuration = 1.2f;

	/** Called when `UDFInteractionEventBus` dispatches a matching tag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Door|Events", meta = (Categories = "Event.Door"))
	FGameplayTag RemoteOpenEventTag;

	UFUNCTION(BlueprintCallable, Category = "DF|Interaction")
	void OpenDoor();

	UFUNCTION()
	void OnWorldGameplayEvent(FGameplayTag EventTag, AActor* EventSource);

protected:
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayDoorOpen();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Door")
	TObjectPtr<UTimelineComponent> OpenTimeline = nullptr;

	UFUNCTION()
	void OnOpenTimelineUpdate(float Alpha);

	UFUNCTION()
	void OnOpenTimelineFinished();
};
