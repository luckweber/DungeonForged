// Source/DungeonForged/Public/Settings/UDFClassSelectionDeveloperSettings.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/Character.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DeveloperSettings.h"
#include "GameModes/MainMenu/DFMainMenuTypes.h"
#include "UDFClassSelectionDeveloperSettings.generated.h"

/**
 * Edit -> Project Settings -> Dungeon Forged | Class Selection.
 * Valores aqui sobrescrevem DefaultGame.ini em runtime depois de LoadConfig()
 * em @c UDFClassSelectionSubsystem::Initialize (pre-visualizacao + widget BP).
 */
UCLASS(Config = Editor, DefaultConfig, meta = (DisplayName = "Dungeon Forged | Class Selection"))
class DUNGEONFORGED_API UDFClassSelectionDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Se definido, o subsistema usa esta @c UDataTable no menu / preview (útil quando @c DFRunManager não tem @c ClassDataTable). @c Config não admite @c TObjectPtr; usa soft ref. */
	UPROPERTY(EditAnywhere, Config, Category = "Data", meta = (RowType = "/Script/DungeonForged.FDFClassTableRow"))
	TSoftObjectPtr<UDataTable> ClassDataTable;

	/** Se definido, substitui @c [/Script/DungeonForged.DFClassSelectionSubsystem] PreviewPawnClass. */
	UPROPERTY(EditAnywhere, Config, Category = "Preview")
	TSubclassOf<ACharacter> PreviewPawnClass;

	/** Sobrescreve @c [/Script/DungeonForged.DFClassSelectionSubsystem] PreviewDisplayMode em @c Initialize. */
	UPROPERTY(EditAnywhere, Config, Category = "Preview")
	EDFClassPreviewDisplayMode PreviewDisplayMode = EDFClassPreviewDisplayMode::WorldWithPlayerCamera;

	/** Se definido, substitui ClassSelectionWidgetClass do subsistema / Game.ini. */
	UPROPERTY(EditAnywhere, Config, Category = "UI")
	TSubclassOf<UUserWidget> ClassSelectionWidgetClass;

	/** Se valido, substitui ClassSelectionWidgetSoftPath (vazio = deixa o valor de LoadConfig / C++ do subsistema). */
	UPROPERTY(EditAnywhere, Config, Category = "UI")
	FSoftClassPath ClassSelectionWidgetSoftPath;

	/** Se true, usa fill light na preview. */
	UPROPERTY(EditAnywhere, Config, Category = "UI")
    bool bPreviewUseFillLight = true;
};
