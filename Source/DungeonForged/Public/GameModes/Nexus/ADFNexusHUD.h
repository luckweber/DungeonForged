// Source/DungeonForged/Public/GameModes/Nexus/ADFNexusHUD.h
#pragma once

#include "CoreMinimal.h"
#include "Types/SlateEnums.h"
#include "GameFramework/HUD.h"
#include "GameModes/Nexus/DFNexusTypes.h"
#include "TimerManager.h"
#include "UI/Combat/UDFCombatTextWidget.h"
#include "ADFNexusHUD.generated.h"

class UUserWidget;
class UDFNexusHUDWidget;
class UDFNexusUnlockNotificationWidget;
class UDFInteractionPromptWidget;
class APlayerController;

UCLASS()
class DUNGEONFORGED_API ADFNexusHUD : public AHUD
{
	GENERATED_BODY()

public:
	ADFNexusHUD();

	/** e.g. WBP_NexusHUD parent = @a UDFNexusHUDWidget */
	UPROPERTY(EditDefaultsOnly, Category = "Nexus|UI")
	TSubclassOf<UDFNexusHUDWidget> NexusRootWidgetClass;

	/** "Prompt 24" — optional full-screen or overlay prompt from interactables. */
	UPROPERTY(EditDefaultsOnly, Category = "Nexus|UI")
	TSubclassOf<UDFInteractionPromptWidget> InteractionPromptClass;

	/** One-at-a-time slide notifications. */
	UPROPERTY(EditDefaultsOnly, Category = "Nexus|UI")
	TSubclassOf<UDFNexusUnlockNotificationWidget> UnlockNotificationClass;

	virtual void BeginPlay() override;

	/** Called from @a ADFNexusGameMode on pending unlocks. */
	UFUNCTION(BlueprintCallable, Category = "Nexus|UI")
	void QueueUnlockNotificationForEntry(const FDFPendingUnlockEntry& Entry);

	/** Push next notification in queue. */
	UFUNCTION(BlueprintCallable, Category = "Nexus|UI")
	void DequeueAndShowNextNotification();

	/** Esconde o root (ex. barra Meta XP) durante @c UDFClassSelectionSubsystem::OpenClassSelection. */
	UFUNCTION(BlueprintCallable, Category = "Nexus|UI")
	void SetRootWidgetHiddenForClassSelection(bool InHidden);

protected:
	UPROPERTY(Transient)
	TObjectPtr<UDFNexusHUDWidget> RootWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UDFInteractionPromptWidget> InteractionLayer = nullptr;

	TArray<FDFPendingUnlockEntry> NotificationQueue;
	bool bNotificationShowing = false;
	FTimerHandle NotificationChainTimer;

	bool bRootHiddenForClassSelection = false;
	ESlateVisibility CachedRootVisibilityBeforeClassSelection = ESlateVisibility::SelfHitTestInvisible;

	UFUNCTION()
	void OnNotificationChainStep();
};
