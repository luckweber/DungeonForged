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

protected:
	void RestoreTargetGravity(TWeakObjectPtr<ACharacter> TargetChar, float PriorGravity);
};
