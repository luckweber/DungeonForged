// Source/DungeonForged/Public/Boss/ADFBossTriggerVolume.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ADFBossTriggerVolume.generated.h"

class UBoxComponent;
class ULevelSequence;
class ADFBossBase;
class UDFBossHealthBarWidget;

UCLASS()
class DUNGEONFORGED_API ADFBossTriggerVolume : public AActor
{
	GENERATED_BODY()

public:
	ADFBossTriggerVolume();

protected:
	UPROPERTY(VisibleAnywhere, Category = "DF|Boss|Trigger")
	TObjectPtr<UBoxComponent> Box;

	/** Cinematic; optional. If null, only UI / GAS + timer still run. */
	UPROPERTY(EditAnywhere, Category = "DF|Boss|Trigger")
	TObjectPtr<ULevelSequence> IntroLevelSequence;

	UPROPERTY(EditAnywhere, Category = "DF|Boss|Trigger", meta = (ClampMin = "0.0"))
	float IntroEndDelay = 5.f;

	UPROPERTY(EditAnywhere, Category = "DF|Boss|Trigger")
	TObjectPtr<ADFBossBase> TargetBoss;

	UPROPERTY(EditAnywhere, Category = "DF|Boss|UI")
	TSubclassOf<UDFBossHealthBarWidget> BossBarWidgetClass;

	/** Receivers of Event.Boss.DoorLock (if they have an ASC). */
	UPROPERTY(EditAnywhere, Category = "DF|Boss|Trigger")
	TArray<TObjectPtr<AActor>> DoorLockTargets;

	UFUNCTION()
	void OnBoxBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BeginIntroCinematic();

	UFUNCTION()
	void OnIntroEnd_Server();

	UFUNCTION()
	void OnLocalSequenceFinished();

	FTimerHandle IntroServerTimer;
	bool bTriggered = false;
};
