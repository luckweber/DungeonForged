// Source/DungeonForged/Public/Combat/UDFMeleeAimComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "UDFMeleeAimComponent.generated.h"

class AActor;
class APawn;

/**
 * Resolves "best melee target right now" for a character that wants to swing.
 *
 * Priority chain:
 *   1. Hard lock-on (player only, via `UDFLockOnComponent::GetCurrentTarget`).
 *   2. Manual override (set per-attack, e.g. by AI BT or by an ability).
 *   3. Blackboard `TargetActor` (for AI pawns).
 *   4. Forward cone soft-search (sphere overlap + cone scoring) — used when none of the above.
 *
 * Pair with @c UANS_DFMeleeWarp on the attack montage so the windup can rotate/translate
 * toward the chosen target via Motion Warping (and as a safety, snap-rotate the actor's yaw
 * before the trace opens — fixes "enemy attacks with back to the player").
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFMeleeAimComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFMeleeAimComponent();

	/** Max sweep distance for the soft cone search (cm). */
	UPROPERTY(EditAnywhere, Category = "DF|Aim", meta = (ClampMin = "0.0"))
	float SoftSearchDistance = 500.f;

	/** Half-angle (deg) of the soft-search forward cone. */
	UPROPERTY(EditAnywhere, Category = "DF|Aim", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float SoftSearchHalfAngle = 45.f;

	/** Sphere radius for the soft-search overlap (cm). */
	UPROPERTY(EditAnywhere, Category = "DF|Aim", meta = (ClampMin = "10.0"))
	float SoftSearchSphereRadius = 250.f;

	/** Score weighting: 1.0 = pure distance, 0.0 = pure angle alignment with forward. */
	UPROPERTY(EditAnywhere, Category = "DF|Aim", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SoftSearchDistanceWeight = 0.55f;

	/** Collision channel used for the soft-search line-of-sight test (Visibility / Camera, etc.). */
	UPROPERTY(EditAnywhere, Category = "DF|Aim")
	TEnumAsByte<ECollisionChannel> SoftSearchVisChannel = ECC_Visibility;

	/** Optional class filter (e.g. `ADFEnemyBase` on the player; `ADFPlayerCharacter` on enemies). */
	UPROPERTY(EditAnywhere, Category = "DF|Aim")
	TSubclassOf<AActor> TargetClassFilter;

	/** Snap-rotation yaw tolerance (deg). Past this, on-activate snap kicks in. */
	UPROPERTY(EditAnywhere, Category = "DF|Aim|Snap", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float SnapYawTolerance = 15.f;

	/** Max degrees the on-activate snap is allowed to rotate in one frame (clamps huge turns). */
	UPROPERTY(EditAnywhere, Category = "DF|Aim|Snap", meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float SnapMaxYawPerActivation = 180.f;

	/** Optional per-attack override (cleared on End). Set by AI or ability. */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "DF|Aim")
	TWeakObjectPtr<AActor> ManualTarget;

	/** Read player lock-on first (default true). Disable on enemies / NPC if you don't want this path. */
	UPROPERTY(EditAnywhere, Category = "DF|Aim")
	bool bConsiderPlayerLockOn = true;

	/** Read AI Blackboard `TargetActor` if no manual / lock-on (default true on enemies). */
	UPROPERTY(EditAnywhere, Category = "DF|Aim")
	bool bConsiderAIBlackboard = true;

	/** Run the soft cone search as a last resort. */
	UPROPERTY(EditAnywhere, Category = "DF|Aim")
	bool bConsiderSoftCone = true;

	/** Debug draws for the soft cone search. */
	UPROPERTY(EditAnywhere, Category = "DF|Aim|Debug")
	bool bDrawDebug = false;

	/** Resolve the best target *right now*. Never caches; safe to call every frame. */
	UFUNCTION(BlueprintCallable, Category = "DF|Aim")
	AActor* ResolveCurrentTarget() const;

	/**
	 * Rotate the owner's yaw to face @a Target if angle exceeds @c SnapYawTolerance.
	 * Returns the actual yaw delta applied (0 if no snap needed). Server-authoritative if
	 * called on server; on clients, only affects local mesh until replication catches up.
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|Aim|Snap")
	float SnapYawTowardTarget(AActor* Target);

	/**
	 * One-shot helper used on ability activation: resolves target, snaps yaw, sets @c ManualTarget
	 * so subsequent @c UANS_DFMeleeWarp notify states use the same actor. Returns the resolved target.
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|Aim")
	AActor* AcquireAndCommitTarget();

	/** Clear per-attack state (call from EndAbility or montage end). */
	UFUNCTION(BlueprintCallable, Category = "DF|Aim")
	void ReleaseAttackTarget();

	/** True if the owner has line-of-sight to @a Candidate (Visibility channel by default). */
	UFUNCTION(BlueprintPure, Category = "DF|Aim")
	bool HasLineOfSight(AActor* Candidate) const;

	/** True if @a Candidate is valid as a melee target (alive, not self, class filter passes). */
	UFUNCTION(BlueprintPure, Category = "DF|Aim")
	bool IsValidMeleeTarget(AActor* Candidate) const;

protected:
	virtual void BeginPlay() override;

	AActor* TryGetPlayerLockOnTarget() const;
	AActor* TryGetAIBlackboardTarget() const;
	AActor* FindSoftConeTarget() const;

	/** Resolves the owning pawn (or owner cast to APawn). */
	APawn* GetOwningPawn() const;
};
