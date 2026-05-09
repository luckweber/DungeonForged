// Source/DungeonForged/Public/World/DFWorldTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "GameModes/Run/DFRunTypes.h"
#include "DFWorldTypes.generated.h"

/** Why a level change was requested (Nexus, dungeon run, floor, or return). @see UDFWorldTransitionSubsystem */
UENUM(BlueprintType)
enum class ETravelReason : uint8
{
	NewRun = 0,
	NextFloor = 1,
	Victory = 2,
	Defeat = 3,
	AbandonRun = 4,
	FirstLaunch = 5,
	/** No pending world travel (internal). */
	None = 6
};

/** When the last @ref FDFRunState snapshot in @ref UDFSaveGame::LastCheckpoint was taken. */
UENUM(BlueprintType)
enum class ECheckpointType : uint8
{
	RunStart = 0,
	FloorComplete = 1,
	RunEnd = 2
};

namespace DFWorldTransition
{
	/** @c ERunNexusTravelReason values are a subset of travel reasons. */
	FORCEINLINE ETravelReason NexusReasonToTravel(const ERunNexusTravelReason R)
	{
		switch (R)
		{
		case ERunNexusTravelReason::Victory: return ETravelReason::Victory;
		case ERunNexusTravelReason::Defeat: return ETravelReason::Defeat;
		case ERunNexusTravelReason::Abandon: return ETravelReason::AbandonRun;
		case ERunNexusTravelReason::FirstLaunch: return ETravelReason::FirstLaunch;
		case ERunNexusTravelReason::MenuEntry: return ETravelReason::FirstLaunch;
		default: return ETravelReason::FirstLaunch;
		}
	}

	FORCEINLINE ERunNexusTravelReason TravelToNexusArrival(const ETravelReason T)
	{
		switch (T)
		{
		case ETravelReason::Victory: return ERunNexusTravelReason::Victory;
		case ETravelReason::Defeat: return ERunNexusTravelReason::Defeat;
		case ETravelReason::AbandonRun: return ERunNexusTravelReason::Abandon;
		case ETravelReason::FirstLaunch: return ERunNexusTravelReason::FirstLaunch;
		case ETravelReason::NewRun:
		case ETravelReason::NextFloor: return ERunNexusTravelReason::MenuEntry; // not used for Nexus
		default: return ERunNexusTravelReason::FirstLaunch;
		}
	}
} // namespace DFWorldTransition

/** Uma entrada de texto para o painel de dicas do @ref UDFLoadingScreenSubsystem (DataTable opcional em Project Settings). */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFLoadingScreenTipPair
{
	GENERATED_BODY()

	/** Opcional; vazio ⇒ rótulo padrão "Dica". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loading")
	FText TipLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loading")
	FText TipBody;

	/** Sobrescreve @c RowBackgroundTexture da linha se resolvível. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loading")
	TSoftObjectPtr<UTexture2D> BackgroundOverride;
};

/**
 * Linha de DT (Project Settings → Dungeon Forged | Loading Screen).
 * Mesmo @a ForReason pode repetir entre linhas; todas entram no sorteio.
 */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFLoadingScreenTipsRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Entra no pool de dicas independentemente do @c ETravelReason atual. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loading")
	bool bAlwaysIncludeInPool = false;

	/** Matching pela viagem apenas quando @c bAlwaysIncludeInPool é false. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loading")
	ETravelReason ForReason = ETravelReason::FirstLaunch;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loading")
	TArray<FDFLoadingScreenTipPair> Tips;

	/** Fallback de fundo @c BackgroundArt quando a entrada não tem @c BackgroundOverride. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loading")
	TSoftObjectPtr<UTexture2D> RowBackgroundTexture;
};
