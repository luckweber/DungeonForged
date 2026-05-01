// Source/DungeonForged/Public/UI/ClassSelection/UDFClassPreviewRotatorComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UDFClassPreviewRotatorComponent.generated.h"

UCLASS(ClassGroup = (DF), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFClassPreviewRotatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFClassPreviewRotatorComponent();

	/** Drag delta; combines with lerp in Tick. */
	UFUNCTION(BlueprintCallable, Category = "DF|ClassSelection|Preview")
	void AddYawInput(float Delta);

	/** Mouse wheel: zoom spring arm 200–600 if bound. */
	UFUNCTION(BlueprintCallable, Category = "DF|ClassSelection|Preview")
	void AddZoomInput(float Delta);

	UFUNCTION(BlueprintCallable, Category = "DF|ClassSelection|Preview")
	void SetSpringArm(class USpringArmComponent* InArm) { SpringArm = InArm; }

	/** Depois de teleportar o pawn (preview mundo direto), alinha yaw interno ao actor. */
	void SyncYawFromOwner();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		enum ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** Lerp current toward target. */
	UPROPERTY(EditAnywhere, Category = "DF|ClassSelection|Preview", meta = (ClampMin = "0.0"))
	float YawLerpSpeed = 10.f;

	/** When not dragging, advance target yaw. */
	UPROPERTY(EditAnywhere, Category = "DF|ClassSelection|Preview")
	float AutoRotateSpeed = 15.f;

	UPROPERTY(EditAnywhere, Category = "DF|ClassSelection|Preview")
	float IdleThreshold = 3.f;

	UPROPERTY(EditAnywhere, Category = "DF|ClassSelection|Preview", meta = (ClampMin = "0.0"))
	float MinArmLength = 200.f;

	UPROPERTY(EditAnywhere, Category = "DF|ClassSelection|Preview", meta = (ClampMin = "0.0"))
	float MaxArmLength = 600.f;

	float CurrentYaw = 0.f;
	float TargetYaw = 0.f;
	float IdleTimer = 0.f;
	bool bUserInteracting = false;

	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> SpringArm = nullptr;
};
