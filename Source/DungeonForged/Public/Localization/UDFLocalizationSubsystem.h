// Source/DungeonForged/Public/Localization/UDFLocalizationSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Localization/DFLocalizationTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "UDFLocalizationSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDFLanguageChanged, EDFLanguage, NewLanguage);

UCLASS()
class DUNGEONFORGED_API UDFLocalizationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "DF|Localization")
	void SetLanguage(EDFLanguage Language);

	UFUNCTION(BlueprintPure, Category = "DF|Localization")
	EDFLanguage GetCurrentLanguage() const { return CurrentLanguage; }

	UFUNCTION(BlueprintPure, Category = "DF|Localization")
	FString GetCurrentCultureCode() const { return CurrentCultureCode; }

	/** Localized end-user names of available languages (order matches enum). */
	UFUNCTION(BlueprintCallable, Category = "DF|Localization")
	TArray<FText> GetAvailableLanguageDisplayNames() const;

	UPROPERTY(BlueprintAssignable, Category = "DF|Localization|Events")
	FOnDFLanguageChanged OnLanguageChanged;

protected:
	static FString LanguageToCultureCode(EDFLanguage InLanguage);
	static EDFLanguage CultureCodeToLanguage(const FString& Code);
	void ApplyCulture(const FString& CultureCode, EDFLanguage InLanguage, bool bSave);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Localization")
	EDFLanguage CurrentLanguage = EDFLanguage::PortugueseBrazil;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Localization")
	FString CurrentCultureCode = TEXT("pt-BR");
};
