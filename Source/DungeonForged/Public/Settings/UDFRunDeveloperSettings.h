// Source/DungeonForged/Public/Settings/UDFRunDeveloperSettings.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/DeveloperSettings.h"
#include "UDFRunDeveloperSettings.generated.h"

/**
 * Edit -> Project Settings -> Dungeon Forged | Run.
 * Centraliza data tables e tuning consumidos por @c UDFRunManager (run state, drops,
 * meta-xp). Como @c UGameInstanceSubsystem não tem CDO em Blueprint editável, @c EditDefaultsOnly
 * em @c UPROPERTY deixa esses campos sempre @c nullptr — este DevSettings resolve a lacuna.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Dungeon Forged | Run"))
class DUNGEONFORGED_API UDFRunDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** DT (FDFClassTableRow) consumida por @c UDFRunManager::ClassDataTable e fallback do menu de classes. */
	UPROPERTY(EditAnywhere, Config, Category = "Data", meta = (RowType = "/Script/DungeonForged.FDFClassTableRow"))
	TSoftObjectPtr<UDataTable> ClassDataTable;

	/** DT (FDFAbilityTableRow) consumida por @c UDFRunManager::AbilityDataTable. */
	UPROPERTY(EditAnywhere, Config, Category = "Data", meta = (RowType = "/Script/DungeonForged.FDFAbilityTableRow"))
	TSoftObjectPtr<UDataTable> AbilityDataTable;

	/** DT (FDFItemTableRow) consumida por @c UDFRunManager::ItemDataTable e por componentes de inventário. */
	UPROPERTY(EditAnywhere, Config, Category = "Data", meta = (RowType = "/Script/DungeonForged.FDFItemTableRow"))
	TSoftObjectPtr<UDataTable> ItemDataTable;

	/** DT (FDFNexusLevelRow) usada na barra de Meta XP do menu / @c UDFRunManager::NexusMetaLevelsTable. */
	UPROPERTY(EditAnywhere, Config, Category = "Data|Nexus", meta = (RowType = "/Script/DungeonForged.FDFNexusLevelRow"))
	TSoftObjectPtr<UDataTable> NexusMetaLevelsTable;

	/** Atraso em segundos entre @c OnRunFailed e @c OnShowDeathScreen. */
	UPROPERTY(EditAnywhere, Config, Category = "Presentation", meta = (ClampMin = "0.0", UIMin = 0.0))
	float DeathScreenDelaySeconds = 2.f;
};
