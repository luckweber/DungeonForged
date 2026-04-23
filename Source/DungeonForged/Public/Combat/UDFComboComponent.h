// Source/DungeonForged/Public/Combat/UDFComboComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UDFComboComponent.generated.h"

class UAnimMontage;
class UDFMeleeTraceComponent;
class UAnimInstance;

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFComboComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFComboComponent();

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo")
	int32 CurrentComboStep = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo", meta = (ClampMin = "1"))
	int32 MaxComboSteps = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo", meta = (ClampMin = "0.0"))
	float ComboWindowDuration = 0.6f;

	/** If true, next attack input in the current window is treated as a chain input. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo")
	bool bComboInputBuffered = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo")
	bool bComboWindowActive = false;

	/** Filled in BeginPlay if null; or assign in BP. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo")
	TObjectPtr<UDFMeleeTraceComponent> MeleeTrace;

	/** One montage per step (0 .. MaxComboSteps-1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo")
	TArray<TObjectPtr<UAnimMontage>> ComboMontages;

	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void OnAttackInput();

	/** Start or restart the chain at step 0. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void StartCombo();

	/** Placed on the timeline where the next chain can start (e.g. AnimNotify AN_ComboWindow). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void AdvanceCombo();

	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void ResetCombo();

	/** Exposed for Blueprint; anim delegate calls HandleMontageEndedInternal in C++. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void OnMontageEnded(UAnimMontage* EndedMontage, bool bInterrupted);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnComboWindowTimerExpired();

	void PlayCurrentComboMontage();
	void UnbindMontageEndDelegate();
	void HandleMontageEndedInternal(class UAnimMontage* EndedMontage, bool bInterrupted);
	UAnimInstance* GetAnimInstance() const;
	void TryBindEndDelegateFor(UAnimMontage* Montage);

	FTimerHandle ComboWindowTimer;
	TObjectPtr<UAnimMontage> LastBoundMontageForEnd = nullptr;
	bool bPlayingComboMontage = false;
};
