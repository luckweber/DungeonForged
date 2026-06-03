// Source/DungeonForged/Public/Boss/UDFBossMinionComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UDFBossMinionComponent.generated.h"

class ADFBossBase;
class ADFEnemyBase;
class UNiagaraSystem;
class USoundBase;

UENUM(BlueprintType)
enum class EDFBossMinionRole : uint8
{
	Guard UMETA(DisplayName = "Guard Boss"),
	Exploder UMETA(DisplayName = "Exploder")
};

/** Boss add behavior: intercept threats near the owner or detonate on proximity/death. */
UCLASS(ClassGroup = (DF), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFBossMinionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFBossMinionComponent();

	void InitializeMinion(ADFBossBase* InBoss, EDFBossMinionRole InRole);

	UFUNCTION(BlueprintPure, Category = "DF|Boss|Minion")
	EDFBossMinionRole GetMinionRole() const { return MinionRole; }

	UFUNCTION(BlueprintPure, Category = "DF|Boss|Minion")
	ADFBossBase* GetOwningBoss() const { return OwningBoss.Get(); }

	/** Intercept point between boss and the nearest hostile when the boss is threatened. */
	UFUNCTION(BlueprintPure, Category = "DF|Boss|Minion")
	bool ComputeGuardLocation(FVector& OutLocation) const;

	UFUNCTION(BlueprintPure, Category = "DF|Boss|Minion")
	bool ShouldGuardBoss() const;

	/** Server: proximity detonation for exploder adds. */
	UFUNCTION(BlueprintCallable, Category = "DF|Boss|Minion")
	bool TryDetonateFromProximity();

	/** Server: death burst for exploder adds (called from @c ADFEnemyBase::HandleServerDeath). */
	void HandleOwnerDeath(AActor* Killer);

	UPROPERTY(EditAnywhere, Category = "DF|Boss|Minion|Guard", meta = (ClampMin = "100.0"))
	float BossThreatRadiusCm = 2200.f;

	UPROPERTY(EditAnywhere, Category = "DF|Boss|Minion|Guard", meta = (ClampMin = "50.0"))
	float GuardStandOffDistanceCm = 350.f;

	UPROPERTY(EditAnywhere, Category = "DF|Boss|Minion|Explode", meta = (ClampMin = "50.0"))
	float DetonationRangeCm = 180.f;

	UPROPERTY(EditAnywhere, Category = "DF|Boss|Minion|Explode", meta = (ClampMin = "50.0"))
	float ExplosionRadiusCm = 320.f;

	UPROPERTY(EditAnywhere, Category = "DF|Boss|Minion|Explode", meta = (ClampMin = "0.0"))
	float ExplosionDamage = 28.f;

	UPROPERTY(EditAnywhere, Category = "DF|Boss|Minion|Explode")
	TObjectPtr<UNiagaraSystem> ExplosionVFX = nullptr;

	UPROPERTY(EditAnywhere, Category = "DF|Boss|Minion|Explode")
	TObjectPtr<USoundBase> ExplosionSound = nullptr;

protected:
	void ApplyExplosionDamage();
	void ForceOwnerDeath();
	void ApplyRoleTags();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayExplosionFX();

	UPROPERTY(Transient)
	TWeakObjectPtr<ADFBossBase> OwningBoss;

	UPROPERTY(Transient)
	EDFBossMinionRole MinionRole = EDFBossMinionRole::Guard;

	bool bDetonated = false;
};
