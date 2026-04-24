// Source/DungeonForged/Public/Localization/UDFUILanguageListEntry.h
#pragma once

#include "CoreMinimal.h"
#include "Localization/DFLocalizationTypes.h"
#include "UObject/NoExportTypes.h"

#include "UDFUILanguageListEntry.generated.h"

class UTexture2D;

/**
 * UObject payload for UTileView / UListView language rows. Create one per EDFLanguage in WBP or C++.
 */
UCLASS(Blueprintable, BlueprintType)
class DUNGEONFORGED_API UDFUILanguageListEntry : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "DF|Localization|UI")
	EDFLanguage Language = EDFLanguage::PortugueseBrazil;

	/** e.g. from UDFLocalizationSubsystem::GetAvailableLanguageDisplayNames. */
	UPROPERTY(BlueprintReadWrite, Category = "DF|Localization|UI")
	FText DisplayName;

	UPROPERTY(BlueprintReadWrite, Category = "DF|Localization|UI")
	TSoftObjectPtr<UTexture2D> FlagIcon;

	/** Shown in the Language tab for confirmation before SetLanguage. */
	UPROPERTY(BlueprintReadWrite, Category = "DF|Localization|UI")
	FText PreviewSample;
};
