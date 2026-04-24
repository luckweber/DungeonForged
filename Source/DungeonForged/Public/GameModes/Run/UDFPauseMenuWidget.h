// Source/DungeonForged/Public/GameModes/Run/UDFPauseMenuWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UDFPauseMenuWidget.generated.h"

class UTextBlock;
class UButton;
class UMaterialInterface;

UCLASS(Blueprintable, Abstract)
class DUNGEONFORGED_API UDFPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	/** Optional blur / frosted glass background. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|UI")
	TObjectPtr<UMaterialInterface> BlurBackgroundMaterial;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Resume;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Options;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> AbandonRun;

	/** Show andar / kills / gold / tempo — update from @ref ADFRunGameState in @c NativeConstruct or tick. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RunStatsText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|UI")
	TSubclassOf<UUserWidget> OptionsScreenClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Run|UI")
	TObjectPtr<UUserWidget> OptionsScreenInstance;

	UFUNCTION()
	void HandleResume();
	UFUNCTION()
	void HandleOptions();
	UFUNCTION()
	void HandleAbandonRun();
};
