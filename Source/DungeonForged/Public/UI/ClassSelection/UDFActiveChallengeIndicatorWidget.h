// Source/DungeonForged/Public/UI/ClassSelection/UDFActiveChallengeIndicatorWidget.h
#pragma once

#include "CoreMinimal.h"
#include "UI/UDFUserWidgetBase.h"
#include "UDFActiveChallengeIndicatorWidget.generated.h"

class UImage;
class UTextBlock;
class UButton;

/** Prompt 52: challenge strip; designer wires content; C++ keeps layout slots. */
UCLASS()
class DUNGEONFORGED_API UDFActiveChallengeIndicatorWidget : public UDFUserWidgetBase
{
	GENERATED_BODY()
protected:
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UImage> ChallengeIcon;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ChallengeName;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ModifierSummary;
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	TObjectPtr<UButton> RemoveChallenge;
};
