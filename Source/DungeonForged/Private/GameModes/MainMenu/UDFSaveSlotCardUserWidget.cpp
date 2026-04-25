// Source/DungeonForged/Private/GameModes/MainMenu/UDFSaveSlotCardUserWidget.cpp
#include "GameModes/MainMenu/UDFSaveSlotCardUserWidget.h"
#include "Run/DFSaveGame.h"

void UDFSaveSlotCardUserWidget::SetupForSlot(
	int32 const InSlotIndex, EDFSlotScreenMode const InMode, UDFSaveGame* InSaveOrNull, bool const bEmptyOnDisk)
{
	SlotIndex = InSlotIndex;
	Mode = InMode;
	// WBP: read @a bEmptyOnDisk, @a InSaveOrNull e preencha textos, retratos, etc.
	(void)InSaveOrNull;
	(void)bEmptyOnDisk;
}
