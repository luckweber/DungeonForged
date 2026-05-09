// Source/DungeonForged/Public/Settings/UDFLoadingScreenDeveloperSettings.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DeveloperSettings.h"

#include "UDFLoadingScreenDeveloperSettings.generated.h"

/**
 * Edit -> Project Settings -> Dungeon Forged | Loading Screen.
 * Centraliza classe UMG soft + DataTable de dicas / fundos opcionais, tal como @c UDFClassSelectionDeveloperSettings e
 * @c UDFRunDeveloperSettings fazem para outros subsistemas.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Dungeon Forged | Loading Screen"))
class DUNGEONFORGED_API UDFLoadingScreenDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Se definido, substitui @c UDFLoadingScreenSubsystem::LoadingScreenClass no Initialize. Parent C++ esperado @c UDFLoadingScreenWidget. */
	UPROPERTY(EditAnywhere, Config, Category = "UI")
	TSubclassOf<UUserWidget> LoadingScreenWidgetClass;

	/** Se válido e @a LoadingScreenWidgetClass for nulo, carrega classe UMG antes do primeiro @c ShowLoadingScreen. */
	UPROPERTY(EditAnywhere, Config, Category = "UI")
	FSoftClassPath LoadingScreenWidgetSoftPath;

	/** Substitui tempo mínimo de exibição (segundos) antes do fade quando o nível já carregou. */
	UPROPERTY(EditAnywhere, Config, Category = "Timing", meta = (ClampMin = "0.0", UIMin = 0))
	float MinLoadingTime = 2.f;

	/** Linhas @c FDFLoadingScreenTipsRow; vazio mantém texto de dica em código (@c ShowLoadingScreen). */
	UPROPERTY(EditAnywhere, Config, Category = "Data", meta = (RowType = "/Script/DungeonForged.FDFLoadingScreenTipsRow"))
	TSoftObjectPtr<UDataTable> TipsTable;
};
