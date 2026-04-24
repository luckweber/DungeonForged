// Source/DungeonForged/Public/GameModes/Nexus/UDFNexusHUDWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UDFNexusHUDWidget.generated.h"

class UTextBlock;
class UProgressBar;
class UOverlay;
class UDFNexusUnlockNotificationWidget;

UCLASS()
class DUNGEONFORGED_API UDFNexusHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** @a XPFill 0-1 for the segment between the current and next nexus level. */
	UFUNCTION(BlueprintCallable, Category = "Nexus|UI")
	void SetMetaInfo(
		int32 MetaLevel,
		int32 TotalRuns,
		int32 TotalWins,
		float XPFill);

	UFUNCTION(BlueprintCallable, Category = "Nexus|UI")
	class UOverlay* GetNotificationOverlay() const { return NotificationOverlay; }

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MetaLevelText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> MetaXPBar = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RunStats = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> NotificationOverlay = nullptr;
};
