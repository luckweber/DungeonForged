// Source/DungeonForged/Public/Characters/ADFPlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ADFPlayerController.generated.h"

class UDFGASDebugOverlayWidget;

UCLASS()
class DUNGEONFORGED_API ADFPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ADFPlayerController();

	/** Toggles the GAS debug overlay. No-op in shipping. */
	UFUNCTION(BlueprintCallable, Category = "DF|Debug")
	void ToggleGASDebugOverlay();

	/** GAS / perf overlay; only populated in non-shipping. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|Debug")
	TObjectPtr<UDFGASDebugOverlayWidget> GASDebugOverlay;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|Debug")
	bool bGASDebugOverlayVisible = false;
};
