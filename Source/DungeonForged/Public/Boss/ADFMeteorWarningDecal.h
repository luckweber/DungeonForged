// Source/DungeonForged/Public/Boss/ADFMeteorWarningDecal.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ADFMeteorWarningDecal.generated.h"

class UDecalComponent;
class USoundBase;

/**
 * Red ground decal + optional rumble SFX, pulses over lifetime (e.g. 2s telegraph).
 */
UCLASS()
class DUNGEONFORGED_API ADFMeteorWarningDecal : public AActor
{
	GENERATED_BODY()
public:
	ADFMeteorWarningDecal();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Boss|Meteor")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Boss|Meteor")
	TObjectPtr<UDecalComponent> GroundDecal;

	/** Cylinder/XY radius in cm for designers (decal size scales with this on spawn). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Boss|Meteor", meta = (ClampMin = "1"))
	float DecalRadius = 400.f;

	UPROPERTY(EditAnywhere, Category = "DF|Boss|Meteor")
	TObjectPtr<USoundBase> RumbleLoop;

	/** Pulsing intensity: sin wave over lifetime. */
	UPROPERTY(EditAnywhere, Category = "DF|Boss|Meteor", meta = (ClampMin = "0.1"))
	float PulseRate = 2.f;

	/** If set, set scalar parameter "Pulse" on the decal MID. */
	UPROPERTY(EditAnywhere, Category = "DF|Boss|Meteor")
	FName PulseMaterialParamName = FName(TEXT("Pulse"));

private:
	TObjectPtr<class UMaterialInstanceDynamic> DecalMid;
	float Elapsed = 0.f;
};
