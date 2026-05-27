// Source/DungeonForged/Public/GAS/Abilities/UDFAbility_AirDash.h
#pragma once

#include "CoreMinimal.h"
#include "Combat/DFDodgeTypes.h"
#include "GAS/UDFGameplayAbility.h"
#include "UDFAbility_AirDash.generated.h"

struct FGameplayAbilityActorInfo;
struct FGameplayEventData;
class ACharacter;
class UAnimMontage;
class UDFCharacterMovementComponent;

UCLASS()
class DUNGEONFORGED_API UDFAbility_AirDash : public UDFGameplayAbility
{
	GENERATED_BODY()

public:
	UDFAbility_AirDash();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|AirDash|Unarmed")
	FDFDodgeAnimSet UnarmedAnimSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|AirDash|Armed")
	FDFDodgeAnimSet ArmedAnimSet;

	/** @deprecated Legacy single montage. Final fallback when both sets fail. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|AirDash|Legacy")
	TObjectPtr<UAnimMontage> AirDashMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|AirDash")
	bool bGrantIFrames = true;

	/** Local-space input speed (cm/s) below which direction snaps to Forward. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|AirDash", meta = (ClampMin = "0.0"))
	float DirectionalInputThreshold = 80.f;

	/** When true, montage supplies pose only — CMC AirDashDrive handles displacement. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|AirDash|RootMotion")
	bool bPreferAnimRootMotion = false;

	/** Zero anim root motion translation while dashing so locomotion RM cannot fight CMC drive. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|AirDash|RootMotion")
	bool bSuppressAnimRootMotionDuringDash = true;

	/** Yaw-align to dash direction before montage (recommended for per-direction air dodge assets). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|AirDash|RootMotion")
	bool bRotateToDashDirection = true;

	/** Strip vertical root-motion drift so dash stays on a flat plane (recommended for combat air dodges). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|AirDash|RootMotion")
	bool bLockAltitudeDuringDash = true;

	/** Must match montage asset slot AND AnimBP slot node. Prefer FullBody over DefaultSlot in air. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|AirDash|Anim")
	FName MontageSlotName = FName(TEXT("FullBody"));

	/** If montage fails to play (wrong slot / AnimBP), apply MoveToForce instead of doing nothing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|AirDash|Anim")
	bool bFallbackToProgrammaticIfMontageFails = true;

	UFUNCTION(BlueprintCallable, Category = "Ability|DF|AirDash")
	EDFDodgeDirection ResolveAirDashDirection() const;

	UFUNCTION(BlueprintCallable, Category = "Ability|DF|AirDash")
	FVector ResolveAirDashDirectionWorld() const;

	UFUNCTION(BlueprintCallable, Category = "Ability|DF|AirDash")
	UAnimMontage* ResolveAirDashMontage(EDFDodgeDirection Direction) const;

protected:
	virtual void PostInitProperties() override;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnAirDashFinished();

	UFUNCTION()
	void OnAirDashMontageCompleted();

	UFUNCTION()
	void OnAirDashMontageInterrupted();

	UFUNCTION()
	void OnAirDashMontageCancelled();

	float GetEffectiveAirDashStaminaCost() const;
	bool IsOwnerArmed() const;

	void FinishAirDash(const TCHAR* Reason);
	float GetActiveMontagePosition() const;
	float GetActiveMontageLength() const;

	void ApplyProgrammaticAirDashDisplacement(ACharacter* Char, UDFCharacterMovementComponent* CMC,
		const FVector& DashDirWorld, float DashDist, float DashDur) const;

	void VerifyMontagePlayback(ACharacter* Char, UAnimMontage* Montage, bool bWantsAnimRootMotion,
		const FVector& DashDirWorld, float DashDist, float DashDur);

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveAirDashMontage = nullptr;

	bool bAirDashFinishRequested = false;
	bool bProgrammaticFallbackApplied = false;
	float SavedAnimRootMotionTranslationScale = 1.f;
	uint8 SavedAnimRootMotionMode = 0;
	bool bRestoredAnimRootMotionMode = false;
};
