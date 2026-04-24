// Source/DungeonForged/Public/Dungeon/Traps/UDFTrapDetectionComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UDFTrapDetectionComponent.generated.h"

class ADFTrapBase;
class UUserWidget;

UCLASS(ClassGroup = (DF), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFTrapDetectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFTrapDetectionComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Sense", meta = (ClampMin = "1.0"))
	float DetectionRadius = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Traps|Sense")
	bool bTrapHighlightEnabled = true;

	/** Optional screen-space “!”; add a small widget with screen-position tick. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Traps|UI")
	TSubclassOf<UUserWidget> TrapIndicatorClass;

	UFUNCTION(BlueprintCallable, Category = "DF|Traps|Sense")
	void SetTrapHighlightEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "DF|Traps|Sense")
	void HideTrapHighlight(ADFTrapBase* Trap);

protected:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void TickDetection();

	FTimerHandle DetectionTimer;
	TMap<TWeakObjectPtr<ADFTrapBase>, TObjectPtr<UUserWidget>> IndicatorByTrap;
};
