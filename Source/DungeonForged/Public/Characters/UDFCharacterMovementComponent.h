// Source/DungeonForged/Public/Characters/UDFCharacterMovementComponent.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/WeakObjectPtr.h"
#include "UDFCharacterMovementComponent.generated.h"

class ACharacter;
class FNetworkPredictionData_Client;
class FNetworkPredictionData_Client_Character;

DECLARE_MULTICAST_DELEGATE_ThreeParams(
	FOnUDFMovementModeChanged,
	EMovementMode /* NewMovementMode */,
	EMovementMode /* PreviousMovementMode */,
	uint8 /* PreviousCustomMode */);

/**
 * Project CharacterMovement: sprint, stamina drain, dodge root motion, movement-mode anim notify.
 * FSavedMove_DF: network prediction bWantsSprint in FLAG_Custom_0.
 */
UCLASS()
class DUNGEONFORGED_API UDFCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UDFCharacterMovementComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement")
	float WalkSpeed = 400.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement")
	float SprintSpeed = 700.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement")
	float CrouchSpeed = 200.f;

	/** Stamina / second when sprinting (numeric drain). Ignored if bSprintStaminaFromGameplayEffect is true. */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Sprint", meta = (ClampMin = "0.0"))
	float SprintStaminaDrain = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Dodge", meta = (ClampMin = "0.0"))
	float DodgeCooldown = 0.8f;

	/** World-space dodge displacement magnitude (cm) over DodgeDuration. */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Dodge", meta = (ClampMin = "0.0"))
	float DodgeDistance = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Dodge", meta = (ClampMin = "0.0"))
	float DodgeDuration = 0.35f;

	/** i-frames: State.Invulnerable window (may be shorter than DodgeDuration). */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Dodge", meta = (ClampMin = "0.0"))
	float IFrameDuration = 0.25f;

	UPROPERTY(BlueprintReadOnly, Category = "DF|Movement|Sprint")
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly, Category = "DF|Movement|Dodge")
	bool bIsDodging = false;

	/** When true, stamina drain is handled by a periodic GameplayEffect (Sprint ability) instead of TickSprintStamina. */
	UPROPERTY(Transient)
	bool bSprintStaminaFromGameplayEffect = false;

	/** Optional: applied when Stamina runs out while sprinting (CMC or GE drain). */
	UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Sprint")
	TSubclassOf<class UGameplayEffect> SprintExhaustionEffect;

	/** Binds in AnimInstance / BP to react to movement mode changes. */
	FOnUDFMovementModeChanged OnDFMovementModeChanged;

	/** bFromNetworkPrediction: used when unpacking ServerMove compressed flags; does not change behavior beyond sync. */
	void SetSprinting(bool bSprint, bool bFromNetworkPrediction = false);
	void SetSprintStaminaFromGameplayEffect(bool bFromEffect);

	/** If not using periodic GE, drains SprintStaminaDrain * dt from the owner's ASC. At 0 stamina, stops sprint and applies optional exhaustion. */
	void TickSprintStamina(float DeltaTime);

	/** GAS: applies State.Dodging + dodge impulse; State.Invulnerable for IFrameDuration. Respects DodgeCooldown. */
	UFUNCTION(BlueprintCallable, Category = "DF|Movement|Dodge")
	void PerformDodge(const FVector& DirectionWorld);

	/** Last movement input in world; if nearly zero, returns -Actor forward (backward). */
	UFUNCTION(BlueprintCallable, Category = "DF|Movement|Dodge")
	FVector GetDodgeDirection() const;

	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	float TimeLastDodge = -1.f;
	FTimerHandle TimerHandle_EndDodging;
	FTimerHandle TimerHandle_EndIFrame;

	UFUNCTION()
	void EndDodgingState();

	UFUNCTION()
	void EndIFrameState();

	/** Called on drain-path exhaustion. */
	void ApplySprintExhaustionIfAny();

	void RefreshMaxWalkSpeed();
};

struct FSavedMove_DF : public FSavedMove_Character
{
public:
	bool bWantsSprint = false;

	virtual void Clear() override;
	virtual uint8 GetCompressedFlags() const override;
	virtual void SetMoveFor(
		ACharacter* C,
		float InDeltaTime,
		FVector const& NewAccel,
		FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* C) override;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
};

class FNetworkPredictionData_DF : public FNetworkPredictionData_Client_Character
{
public:
	explicit FNetworkPredictionData_DF(const UCharacterMovementComponent& ClientMovement);
	virtual FSavedMovePtr AllocateNewMove() override;
};
