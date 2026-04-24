// Source/DungeonForged/Public/UI/Status/DFStatusEffectData.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "DFStatusEffectData.generated.h"

class UTexture2D;

/**
 * Display row for a status / buff / debuff icon. Assign in UDFStatusLibrary or use built-in fallbacks.
 */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFStatusEffectDisplayData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "Effect"))
	FGameplayTag EffectTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> Icon = nullptr;

	/** Buff = gold/green tones, debuff = red, DoT = orange, CC = purple — designer override. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor BorderColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Description;

	/** If false, duration text and bar stay hidden (infinite passives). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bShowDuration = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsDebuff = false;

	/** Higher = more important for enemy debuff cap (max 3) and debuff row ordering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DebuffSeverity = 0;
};
