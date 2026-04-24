// Source/DungeonForged/Public/Debug/UDFDebugComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UDFDebugComponent.generated.h"

class AAIController;

UCLASS(Blueprintable, ClassGroup = (DF), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFDebugComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UDFDebugComponent();

#if !UE_BUILD_SHIPPING
	void DrawAttributeDebug() const;
	void DrawAbilityDebug() const;
	/** Spheres and labels for nearby enemies' AI blackboard. */
	void DrawAIDebug(float Radius) const;
	void LogGASEvent(const FString& Event) const;
#endif
};
