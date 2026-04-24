// Source/DungeonForged/Public/Performance/UDFRoomCullingComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "UDFRoomCullingComponent.generated.h"

UCLASS(ClassGroup = (DF), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFRoomCullingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFRoomCullingComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Culling")
	TArray<TObjectPtr<AActor>> RoomActors;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Culling")
	bool bIsVisible = false;

	/** Rooms farther than `CullDistance * 1.5` from the player are culled (see UDFPerformanceSubsystem). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Culling", meta = (ClampMin = "100.0"))
	float CullDistance = 3000.f;

	UFUNCTION(BlueprintCallable, Category = "DF|Culling")
	void SetRoomVisible(bool bVisible);

	/** Distance check from room owner to player; applies show/hide band: visible if `Distance <= CullDistance * 1.5`. */
	void UpdateVisibilityForPlayer(const FVector& PlayerLocation);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	FVector GetCullOrigin() const;
};
