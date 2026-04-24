// Source/DungeonForged/Public/Dungeon/Traps/ADFTrapBase.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemComponent.h"
#include "ADFTrapBase.generated.h"

class UGameplayEffect;
class UNiagaraComponent;
class UNiagaraSystem;
class IAbilitySystemInterface;

UCLASS(Abstract, Blueprintable)
class DUNGEONFORGED_API ADFTrapBase : public AActor
{
	GENERATED_BODY()

public:
	ADFTrapBase();

	/** Whether the trap is active and can fire. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "DF|Traps|State")
	bool bIsArmed = true;

	/** If true, the trap rearms (when Rearm is allowed). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|State")
	bool bIsRepeating = true;

	/** After trigger conditions are met, wait this long before the damage/telegraph end phase (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Timing", meta = (ClampMin = "0.0"))
	float TriggerDelay = 0.f;

	/** How long to wait after completion before the trap is armed again. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Timing", meta = (ClampMin = "0.0"))
	float RearmDelay = 2.f;

	/** Used with SetByCaller (Data.Damage) when no explicit GEs are set on child traps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Damage", meta = (ClampMin = "0.0"))
	float DamageAmount = 30.f;

	/** Optional primary GE; subclasses often use UGE_Damage_* directly instead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|GAS")
	TSubclassOf<UGameplayEffect> TrapEffect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Traps|GAS")
	TObjectPtr<UAbilitySystemComponent> TrapAbilitySystem = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Traps|VFX", meta = (DisplayName = "State Niagara"))
	TObjectPtr<UNiagaraComponent> StateNiagaraComponent = nullptr;

	/** If set, StateNiagaraComponent uses this when armed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|VFX")
	TObjectPtr<UNiagaraSystem> ActiveVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|VFX")
	TObjectPtr<UNiagaraSystem> TriggerVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|VFX")
	TObjectPtr<UNiagaraSystem> DisabledVFX = nullptr;

	/** If true, detection highlight may apply (see UDFTrapDetectionComponent). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|UI")
	bool bIsHidden = false;

	/** Local-only outline stencil when highlighted (detection / awareness). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|UI", meta = (ClampMin = "0", ClampMax = "255"))
	int32 CustomDepthStencilValue = 252;

	UFUNCTION(BlueprintCallable, Category = "DF|Traps")
	virtual void Disarm();

	UFUNCTION(BlueprintCallable, Category = "DF|Traps")
	virtual void Rearm();

	/** Environmental / perception: hidden traps (e.g. flush spikes) return false from far. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DF|Traps")
	bool CanBeSeen() const;
	virtual bool CanBeSeen_Implementation() const;

	/** Play warning: VFX variant, SFX, etc. (called TriggerDelay s before the hit unless delay is 0). */
	UFUNCTION(BlueprintNativeEvent, Category = "DF|Traps")
	void TelegraphActivation(AActor* InstigatorActor);
	virtual void TelegraphActivation_Implementation(AActor* InstigatorActor);

	/**
	 * Core trap behaviour. Implement in subclasses. Base does not call this from BeginPlay; overlap/timers do.
	 * @param InstigatorActor Who fired the trigger (e.g. overlapping pawn); may be null for timed/fixed.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "DF|Traps")
	void OnTrapTriggered(AActor* InstigatorActor);
	virtual void OnTrapTriggered_Implementation(AActor* InstigatorActor);

	/** Primitives to render with CustomDepth for trap-highlight (override in subclasses with multiple meshes). */
	UFUNCTION(BlueprintNativeEvent, Category = "DF|Traps|UI")
	TArray<UPrimitiveComponent*> GetHighlightPrimitives() const;
	virtual TArray<UPrimitiveComponent*> GetHighlightPrimitives_Implementation() const;

	UFUNCTION(BlueprintCallable, Category = "DF|Traps|UI")
	void SetTrapHighlight(bool bEnabled);

	/** Apply TrapEffect to target, or you can use ApplyDamageGE on concrete classes. */
	UFUNCTION(BlueprintCallable, Category = "DF|Traps|GAS")
	void TryApplyTrapEffect(AActor* Target, AActor* InstigatorPawn) const;

	/**
	 * Call when the trap has finished all damage/FX (sync or at end of async work).
	 * Plays trigger burst VFX, optional TrapEffect on Instigator, disarms or starts rear m timer.
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|Traps")
	void CompleteTrap(AActor* InstigatorActor);

	/** If TriggerDelay>0, runs telegraph then OnTrap on a timer. Otherwise fires immediately. */
	UFUNCTION(BlueprintCallable, Category = "DF|Traps")
	void StartTriggerSequence(AActor* InstigatorActor);

protected:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ScheduleRearm();
	void OnRearmTimerFired();
	void OnTriggerDelayElapsed(AActor* InstigatorActor);
	void OnTelegraphForTrigger(AActor* InstigatorActor);

	/** Spawn one-shot at trap location. */
	void SpawnNiagaraAtTrap(UNiagaraSystem* FX) const;
	void UpdateStateNiagaraFromArmed() const;
	void CreateTrapContext(FGameplayEffectContextHandle& Ctx, AActor* InstigatorPawn) const;

public:
	/** Apply a damage GE with SetByCaller Data.Damage = DamageAmount, using the trap as source. */
	UFUNCTION(BlueprintCallable, Category = "DF|Traps|GAS")
	void ApplyDamageGE(
		AActor* Target,
		AActor* InstigatorPawn,
		TSubclassOf<UGameplayEffect> EffectClass) const;

	/**
	 * Applies a GE; sets SetByCaller Data.Damage and optionally Data.Duration.
	 * Pass OptionalDuration < 0 to skip duration.
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|Traps|GAS")
	void ApplyEffectWithMagnitude(
		AActor* Target,
		AActor* InstigatorPawn,
		TSubclassOf<UGameplayEffect> EffectClass,
		float DamageMagnitude,
		float OptionalDuration) const;

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayTriggerVFX();

	FTimerHandle RearmTimer;
	FTimerHandle TriggerDelayTimer;
	FTimerHandle TelegraphTimer;
};
