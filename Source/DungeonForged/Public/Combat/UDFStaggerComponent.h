// Source/DungeonForged/Public/Combat/UDFStaggerComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "UDFStaggerComponent.generated.h"

class UAbilitySystemComponent;
class UAnimMontage;
class UGameplayEffect;
struct FOnAttributeChangeData;

/**
 * Damage-accumulation stagger ("Poise" in Soulslike / "Posture" in Sekiro).
 *
 * Listens for Health attribute drops, sums damage in a sliding window. When the sum exceeds
 * @c Poise within @c StaggerWindow seconds, the actor is staggered:
 *   1. @c StaggerGameplayEffect is applied (typically @c UGE_Debuff_Stun with a duration SetByCaller).
 *   2. `Event.Combat.Stagger.Triggered` is sent on the avatar's ASC (UI / AI listeners react).
 *   3. The window resets and a cooldown blocks re-stagger for @c StaggerCooldown seconds.
 *
 * Tunable per data table — designers set Poise based on enemy archetype (Grunt = 30, Brute = 80, Boss = 200).
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFStaggerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFStaggerComponent();

	/** Damage in a sliding window summed against this; designed to be set from data table. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Stagger", meta = (ClampMin = "1.0"))
	float Poise = 60.f;

	/** Seconds the damage accumulator looks back. Lower = harder to stagger. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Stagger", meta = (ClampMin = "0.1"))
	float StaggerWindow = 3.f;

	/** Seconds between staggers (avoid stun-lock loop). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Stagger", meta = (ClampMin = "0.0"))
	float StaggerCooldown = 4.5f;

	/** GE applied on stagger trigger (typically @c UGE_Debuff_Stun). */
	UPROPERTY(EditAnywhere, Category = "DF|Stagger|GAS")
	TSubclassOf<UGameplayEffect> StaggerGameplayEffect;

	/** SetByCaller key on @c StaggerGameplayEffect (e.g. `Data.Duration`). */
	UPROPERTY(EditAnywhere, Category = "DF|Stagger|GAS", meta = (Categories = "Data"))
	FGameplayTag StaggerSetByCallerTag;

	/** Value sent for @c StaggerSetByCallerTag (e.g. stagger duration in seconds). */
	UPROPERTY(EditAnywhere, Category = "DF|Stagger|GAS", meta = (ClampMin = "0.0"))
	float StaggerSetByCallerMagnitude = 1.25f;

	/** If set, this montage is played multicast on stagger trigger; otherwise just the GE + tag drives feedback. */
	UPROPERTY(EditAnywhere, Category = "DF|Stagger|FX")
	TObjectPtr<UAnimMontage> StaggerMontage = nullptr;

	/** Only the authority records damage and applies the stagger GE; clients see via replication. */
	UPROPERTY(EditAnywhere, Category = "DF|Stagger|Net")
	bool bServerAuthoritative = true;

	UPROPERTY(EditAnywhere, Category = "DF|Stagger|Debug")
	bool bLogVerbose = false;

	/** True while the cooldown after the last stagger is still active. */
	UFUNCTION(BlueprintPure, Category = "DF|Stagger")
	bool IsOnStaggerCooldown() const;

	/** Manually clear the recent damage accumulator (used by tests / bossfight phase transitions). */
	UFUNCTION(BlueprintCallable, Category = "DF|Stagger")
	void ResetAccumulator();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void BindToAbilitySystem();
	void HandleHealthChange(const FOnAttributeChangeData& Data);
	void TriggerStagger(float Overshoot);
	void PruneOldEntries();

private:
	struct FStaggerHit
	{
		float Damage;
		double TimeSeconds;
	};

	TArray<FStaggerHit> RecentHits;
	double LastStaggerTime = -1.0;
	FDelegateHandle HealthChangeHandle;
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
};
