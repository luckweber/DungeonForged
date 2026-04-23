// Source/DungeonForged/Public/Combat/DFBlizzardZone.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DFBlizzardZone.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UDecalComponent;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFBlizzardZone : public AActor
{
	GENERATED_BODY()

public:
	ADFBlizzardZone();

	UPROPERTY(EditAnywhere, Category = "DF|GAS")
	TSubclassOf<UGameplayEffect> MagicDamageEffect;

	UPROPERTY(EditAnywhere, Category = "DF|GAS")
	TSubclassOf<UGameplayEffect> DoTFrostEffect;

	UPROPERTY(EditAnywhere, Category = "DF|Blizzard")
	float ZoneDuration = 8.f;

	UPROPERTY(EditAnywhere, Category = "DF|Blizzard")
	float TickPeriod = 0.5f;

	UPROPERTY(EditAnywhere, Category = "DF|Blizzard")
	float OverlapRadius = 400.f;

	UPROPERTY(EditAnywhere, Category = "DF|VFX")
	TObjectPtr<class UNiagaraSystem> TickNiagara = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "DF|Components")
	TObjectPtr<UDecalComponent> GroundDecal = nullptr;

protected:
	virtual void BeginPlay() override;
	void OnBlizzardTick();
	void OnBlizzardZoneExpire();


	FTimerHandle BlizzardTimer;
	FTimerHandle EndTimer;
};
