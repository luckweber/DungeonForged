// Source/DungeonForged/Public/Dungeon/Traps/ADFTrap_FireJet.h
#pragma once

#include "CoreMinimal.h"
#include "Dungeon/Traps/ADFTrapBase.h"
#include "ADFTrap_FireJet.generated.h"

class UCapsuleComponent;
class UNiagaraComponent;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFireJetVignetteDelegate, float, VignetteStrength);

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFTrap_FireJet : public ADFTrapBase
{
	GENERATED_BODY()

public:
	ADFTrap_FireJet();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Traps|Fire")
	TObjectPtr<UCapsuleComponent> DamageVolume = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Traps|Fire|VFX")
	TObjectPtr<UNiagaraComponent> FireJetNiagara = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Fire|Timing", meta = (ClampMin = "0.1"))
	float ActiveDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Fire|Timing", meta = (ClampMin = "0.1"))
	float InactiveDuration = 2.0f;

	/** Ticks of damage + DoT every this many seconds while active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Fire|Timing", meta = (ClampMin = "0.1"))
	float DamageTickInterval = 0.3f;

	/** Forwards to Blueprint / post-process: 0-1 (client local pawn only). */
	UPROPERTY(BlueprintAssignable, Category = "DF|Traps|Fire|UI")
	FFireJetVignetteDelegate OnLocalVignetteStrength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Fire|Audio")
	TObjectPtr<USoundBase> PreIgniteSound = nullptr;

	/** Disarm is ignored: trap always runs its cycle. */
	virtual void Disarm() override;

protected:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	void OnCycleTurnActive();
	void OnCyclePreIgnite();
	void OnCycleTurnInactive();
	void ApplyFireDamageTick();
	void UpdateVignetteForLocalPawn() const;
	void ResetCycleTimers();

	FTimerHandle ActivePhaseTimer;
	FTimerHandle InactivePhaseTimer;
	FTimerHandle PreIgniteTimer;
	FTimerHandle DamageTickTimer;
	bool bFireActive = false;
};
