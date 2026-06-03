// Source/DungeonForged/Public/Combat/UDFLauncherComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UDFLauncherComponent.generated.h"

class ACharacter;

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFLauncherComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DF|Launcher")
	void ApplyLaunch(AActor* Target, FVector LaunchVel, float TargetGravity, float Hangtime);

	UFUNCTION(BlueprintCallable, Category = "DF|Launcher")
	void ApplySelfLaunch(FVector SelfVel);

	/** Max aerial launches per victim before further launches are ignored (0 = unlimited). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Launcher|Juggle", meta = (ClampMin = "0"))
	int32 MaxJuggleHitsPerTarget = 3;

	/** Seconds after a launch before that victim's juggle count resets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Launcher|Juggle", meta = (ClampMin = "0.1"))
	float JuggleCountResetSeconds = 2.5f;

protected:
	void RestoreTargetGravity(TWeakObjectPtr<ACharacter> TargetChar, float PriorGravity);
	void ScheduleJuggleCountReset(AActor* Target);

	TMap<TWeakObjectPtr<AActor>, int32> JuggleHitCounts;
};
