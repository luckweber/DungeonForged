// Source/DungeonForged/Public/Settings/UDFIconStudioWorkflowSettings.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/World.h"
#include "UDFIconStudioWorkflowSettings.generated.h"

/** Edit → Project Settings → Dungeon Forged | Icon Studio Workflow. Apenas útil ao abrir mapas no Editor (não Packaging). */
UCLASS(Config = Editor, DefaultConfig, meta = (DisplayName = "Dungeon Forged | Icon Studio Workflow"))
class DUNGEONFORGED_API UDFIconStudioWorkflowSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Se true, ao abrir exatamente o mapa @c IconStudioPersistentMap persistente (.umap) no Editor, spawna o EUW indicado. */
	UPROPERTY(EditAnywhere, Config, Category = "Auto Open EUW")
	bool bAutoOpenIconBakeEUWWhenMapOpens = false;

	/** Mapa persistente (asset @c UWorld); escolher no picker (ex. @c L_IconStudio). */
	UPROPERTY(EditAnywhere, Config, Category = "Auto Open EUW", meta = (AllowedClasses = "/Script/Engine.World"))
	TSoftObjectPtr<UWorld> IconStudioPersistentMap;

	/** Asset @c Editor Utility Widget Blueprint a abrir (soft path no Content Browser). */
	UPROPERTY(EditAnywhere, Config, Category = "Auto Open EUW")
	FSoftObjectPath IconBakeEditorUtilityWidget;
};
