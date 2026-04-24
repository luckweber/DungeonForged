// Source/DungeonForged/Public/UI/Minimap/UDFMinimapFogComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "UDFMinimapFogComponent.generated.h"

class ADFMinimapRoom;
class USphereComponent;
class UPrimitiveComponent;

/**
 * On the player: overlaps `ADFMinimapRoom` box triggers — reveal on enter, mark visited on exit,
 * and updates `UDFDungeonManager::SetPlayerCurrentMinimapRoom` for the pulsing “current room” icon.
 */
UCLASS(ClassGroup = (DF), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFMinimapFogComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UDFMinimapFogComponent();

	/** Collision sphere (attached to this component) used only for overlap with room boxes. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Minimap|Fog", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> OverlapSphere = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Minimap|Fog", meta = (ClampMin = "1.0"))
	float SphereRadius = 120.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnRoomBoundaryOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnRoomBoundaryEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	/** Avoid CreateDefaultSubobject on a nested sphere — Blueprint subclasses (e.g. BP_HeroCharacter) hit template mismatch. */
	void EnsureOverlapSphere();
};
