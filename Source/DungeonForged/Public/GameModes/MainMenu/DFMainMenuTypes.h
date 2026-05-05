// Source/DungeonForged/Public/GameModes/MainMenu/DFMainMenuTypes.h
#pragma once

#include "CoreMinimal.h"
#include "DFMainMenuTypes.generated.h"

UENUM(BlueprintType)
enum class EDFSlotScreenMode : uint8
{
	/** Player picks a profile to play; may continue or start new. */
	SelectToPlay  UMETA(DisplayName = "Select To Play"),
	/**
	 * “Gerenciar perfis” no menu: só apagamento / revisão, sem @c Travel.
	 * (Design doc: pode aparecer como @c SelectToDelete ou “ManageProfiles”.)
	 */
	SelectToDelete UMETA(DisplayName = "Select To Delete / Manage")
};

/** Where to travel after @c WBP_ClassSelection confirm when opened from the main menu. */
UENUM(BlueprintType)
enum class EDFMainMenuClassPickDestination : uint8
{
	None,
	/** First-time nexus entry or empty profile creation. */
	NexusFirstLaunch,
	/** Start dungeon run with selected class. */
	RunDungeon
};

/** Como mostrar o personagem 3D durante @c OpenClassSelection. */
UENUM(BlueprintType)
enum class EDFClassPreviewDisplayMode : uint8
{
	SceneCaptureUMG UMETA(DisplayName = "Scene capture → UMG (Render Target na Image)"),

	WorldWithPlayerCamera UMETA(DisplayName = "Mundo — pawn à frente da câmera do jogador (UI centro transparente)"),

	WorldShowcaseCamera UMETA(DisplayName =
		"Palco mundo + câmera cinematográfica (estilo Genshin/showcase na cena)"),
};

/** Z-order de AddToViewport no menu principal (maior = desenha por cima). */
namespace DFMainMenuUI
{
	constexpr int32 ViewportZ_Splash = 0;
	constexpr int32 ViewportZ_Main = 5;
	constexpr int32 ViewportZ_Credits = 15;
	constexpr int32 ViewportZ_SaveSlot = 20;
	/** WBP_ClassSelection — acima dos slots para capturar input; em modo mundo direto os widgets abaixo são ocultos. */
	constexpr int32 ViewportZ_ClassSelection = 25;
	constexpr int32 ViewportZ_ConfirmDialog = 100;
}
