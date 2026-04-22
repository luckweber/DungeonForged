// Source/DungeonForged/Public/UI/ADFHUDBase.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ADFHUDBase.generated.h"

class UUserWidget;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFHUDBase : public AHUD
{
	GENERATED_BODY()

public:
	ADFHUDBase();

	/** UMG class that becomes the in-game HUD (e.g. WBP_DFMainHUD). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|UI")
	TSubclassOf<UUserWidget> MainHUDWidgetClass;

	/** Live instance, filled in local BeginPlay. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|UI")
	TObjectPtr<UUserWidget> MainHUDWidget;

	virtual void ShowHUD() override;
	void HideHUD();

protected:
	virtual void BeginPlay() override;
};
