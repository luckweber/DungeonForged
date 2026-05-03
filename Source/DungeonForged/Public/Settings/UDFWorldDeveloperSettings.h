// Source/DungeonForged/Public/Settings/UDFWorldDeveloperSettings.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/World.h"
#include "UDFWorldDeveloperSettings.generated.h"

/**
 * Edit -> Project Settings -> Dungeon Forged | World.
 * Centraliza os mapas usados por @c UDFWorldTransitionSubsystem (Nexus / Run) e
 * @c UDFGameInstance (MainMenu / HostTravel). Salva em @c DefaultGame.ini para que os
 * caminhos sejam empacotados com builds — preferido a sobrescrever em Blueprints, que
 * tendem a ficar dessincronizados entre BP_DFGameInstance / GameMode etc.
 *
 * Os subsistemas aplicam estes valores no @c Initialize/Init e caem nos defaults C++
 * quando o picker não estiver preenchido (compatibilidade durante a migração).
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Dungeon Forged | World"))
class DUNGEONFORGED_API UDFWorldDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Mapa do menu inicial (`UDFGameInstance::MainMenuMapName` — `OpenLevel` ao sair de uma sessão). */
	UPROPERTY(EditAnywhere, Config, Category = "Maps")
	TSoftObjectPtr<UWorld> MainMenuMap;

	/** Mapa do hub meta (`UDFWorldTransitionSubsystem::NexusMapName`). */
	UPROPERTY(EditAnywhere, Config, Category = "Maps")
	TSoftObjectPtr<UWorld> NexusMap;

	/** Mapa onde a run da masmorra acontece (`UDFWorldTransitionSubsystem::RunMapName`). */
	UPROPERTY(EditAnywhere, Config, Category = "Maps")
	TSoftObjectPtr<UWorld> RunMap;

	/** Mapa para qual o host faz @c ServerTravel após criar a sessão online (vazio = usa @c MainMenuMap). */
	UPROPERTY(EditAnywhere, Config, Category = "Session")
	TSoftObjectPtr<UWorld> HostTravelMap;

	/** Opções anexadas à URL de @c ServerTravel (ex.: `?listen`, `?listen?game=...`). */
	UPROPERTY(EditAnywhere, Config, Category = "Session")
	FString HostTravelOptions = TEXT("?listen");

	/**
	 * Resolve o caminho longo (/Game/...) de um TSoftObjectPtr<UWorld>.
	 * Retorna @a Fallback se o picker não estiver preenchido.
	 */
	static FString ResolveMapPath(const TSoftObjectPtr<UWorld>& Map, const FString& Fallback)
	{
		if (Map.IsNull())
		{
			return Fallback;
		}
		const FString Long = Map.ToSoftObjectPath().GetLongPackageName();
		return Long.IsEmpty() ? Fallback : Long;
	}
};
