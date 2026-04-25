// Source/DungeonForged/Public/GameModes/MainMenu/UDFSaveSlotCardUserWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameModes/MainMenu/DFMainMenuTypes.h"
#include "UDFSaveSlotCardUserWidget.generated.h"

class UTextBlock;
class UImage;
class UButton;
class UProgressBar;
class UDFSaveGame;

UCLASS(Blueprintable)
class DUNGEONFORGED_API UDFSaveSlotCardUserWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	/** Index 0, 1, 2. */
	UFUNCTION(BlueprintCallable, Category = "DF|MainMenu|Slots")
	void SetupForSlot(
		int32 InSlotIndex, EDFSlotScreenMode InMode, UDFSaveGame* InSaveOrNull, bool bEmptyOnDisk);
protected:
	int32 SlotIndex = 0;
	EDFSlotScreenMode Mode = EDFSlotScreenMode::SelectToPlay;
};
