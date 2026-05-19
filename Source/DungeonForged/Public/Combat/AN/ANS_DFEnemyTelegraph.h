// Source/DungeonForged/Public/Combat/AN/ANS_DFEnemyTelegraph.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_DFEnemyTelegraph.generated.h"

class UAnimSequenceBase;
class UNiagaraComponent;
class UNiagaraSystem;
class USkeletalMeshComponent;
class USoundBase;
struct FAnimNotifyEventReference;

/**
 * Enemy attack telegraph (windup ground warning).
 *
 * Drop on the **windup** section of an enemy attack montage. It:
 *   1. Resolves the target via `UDFMeleeAimComponent::ResolveCurrentTarget()`.
 *   2. Spawns @c GroundWarningVFX at the target's feet (re-positioned every tick if the target moves).
 *   3. Plays @c WindupSound once at begin (one-shot at attacker's location).
 *   4. Adds loose tag `State.Combat.Telegraph.Active` to the attacker's ASC (UI / boss listeners hook this).
 *   5. Fires `Event.Combat.Telegraph.Begin / End` so abilities / HUD can react.
 *
 * Tracks per-mesh state in a TMap so two enemies playing the same notify simultaneously don't clobber each other.
 *
 * **Net note:** AnimNotifyState fires on every machine that plays the montage (server + clients), so the FX
 * appears for everyone without a manual multicast. Loose tags are added locally — that's fine for UI / FX.
 */
UCLASS(Blueprintable, meta = (DisplayName = "DF Enemy Telegraph"))
class DUNGEONFORGED_API UANS_DFEnemyTelegraph : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UANS_DFEnemyTelegraph();

	/** Niagara FX spawned attached-in-world at the target's feet for the windup duration. */
	UPROPERTY(EditAnywhere, Category = "DF|Telegraph|FX")
	TObjectPtr<UNiagaraSystem> GroundWarningVFX = nullptr;

	/** Optional Niagara FX spawned on the attacker's weapon socket (glow / energy build-up). */
	UPROPERTY(EditAnywhere, Category = "DF|Telegraph|FX")
	TObjectPtr<UNiagaraSystem> WeaponChargeVFX = nullptr;

	/** Socket name on the attacker mesh for @c WeaponChargeVFX (e.g. weapon_r tip). */
	UPROPERTY(EditAnywhere, Category = "DF|Telegraph|FX")
	FName WeaponChargeSocketName = TEXT("weapon_r");

	/** One-shot windup sound at the attacker's location on begin. */
	UPROPERTY(EditAnywhere, Category = "DF|Telegraph|FX")
	TObjectPtr<USoundBase> WindupSound = nullptr;

	/** Offset applied to the resolved target location (e.g. lift the decal slightly above the floor). */
	UPROPERTY(EditAnywhere, Category = "DF|Telegraph")
	FVector LocationOffset = FVector(0.f, 0.f, 5.f);

	/** If true, re-position the FX every tick to follow a moving target. */
	UPROPERTY(EditAnywhere, Category = "DF|Telegraph")
	bool bUpdateTargetEveryTick = true;

	/** If true, the warning is dropped on the SPOT where the attacker is aiming (not where the target currently is).
	 *  Combine with the swing motion warp to telegraph where the swing will land.
	 */
	UPROPERTY(EditAnywhere, Category = "DF|Telegraph")
	bool bUseAttackerForwardInsteadOfTarget = false;

	/** Forward distance in front of attacker when @c bUseAttackerForwardInsteadOfTarget is true. */
	UPROPERTY(EditAnywhere, Category = "DF|Telegraph", meta = (EditCondition = "bUseAttackerForwardInsteadOfTarget", ClampMin = "0.0"))
	float ForwardOffsetCm = 180.f;

	/** Add the `State.Combat.Telegraph.Active` loose tag to the attacker for the windup duration. */
	UPROPERTY(EditAnywhere, Category = "DF|Telegraph|GAS")
	bool bAddStateTag = true;

	/** Fire `Event.Combat.Telegraph.Begin` / End on the attacker's ASC. */
	UPROPERTY(EditAnywhere, Category = "DF|Telegraph|GAS")
	bool bSendGameplayEvents = true;

	UPROPERTY(EditAnywhere, Category = "DF|Telegraph|Debug")
	bool bDrawDebug = false;

	virtual FString GetNotifyName_Implementation() const override;
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

private:
	/** Per-mesh active Niagara — keyed by mesh so two enemies don't share state. */
	UPROPERTY(Transient)
	TMap<TObjectPtr<USkeletalMeshComponent>, TWeakObjectPtr<UNiagaraComponent>> GroundFXByMesh;

	UPROPERTY(Transient)
	TMap<TObjectPtr<USkeletalMeshComponent>, TWeakObjectPtr<UNiagaraComponent>> WeaponFXByMesh;

	FVector ResolveWarningLocation(USkeletalMeshComponent* MeshComp, AActor* Target) const;
};
