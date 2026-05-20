// Source/DungeonForged/Public/GAS/Cues/UDFGameplayCueNotify_EnemyDeath.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "UDFGameplayCueNotify_EnemyDeath.generated.h"

/** Replicated cosmetic: enemy death montage + corpse movement (see ADFEnemyBase). */
UCLASS(DisplayName = "DF Enemy Death")
class DUNGEONFORGED_API UDFGameplayCueNotify_EnemyDeath : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UDFGameplayCueNotify_EnemyDeath();

	UPROPERTY(EditAnywhere, Category = "Death|Cue")
	TObjectPtr<class UNiagaraSystem> DeathBurstNiagara = nullptr;

	UPROPERTY(EditAnywhere, Category = "Death|Cue")
	TObjectPtr<class USoundBase> DeathImpactSound = nullptr;

	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const override;
};
