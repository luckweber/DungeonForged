// Source/DungeonForged/Private/Localization/UDFLocalizationSubsystem.cpp
#include "Localization/UDFLocalizationSubsystem.h"

#include "Engine/World.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Run/DFSaveGame.h"
#include "Run/UDFSaveLibrary.h"

void UDFLocalizationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (UDFSaveGame* const S = UDFSaveLibrary::ResolveMutableMetaSave(this))
	{
		ApplyCulture(S->PreferredCultureCode, S->PreferredLanguage, false);
	}
	else
	{
		CurrentLanguage = EDFLanguage::English;
		CurrentCultureCode = TEXT("en");
	}
}

FString UDFLocalizationSubsystem::LanguageToCultureCode(const EDFLanguage InLanguage)
{
	switch (InLanguage)
	{
	case EDFLanguage::English: return TEXT("en");
	case EDFLanguage::Spanish: return TEXT("es");
	case EDFLanguage::French: return TEXT("fr");
	case EDFLanguage::PortugueseBrazil: return TEXT("pt-BR");
	default: return TEXT("en");
	}
}

EDFLanguage UDFLocalizationSubsystem::CultureCodeToLanguage(const FString& Code)
{
	const FString Lower = Code.ToLower();
	if (Lower.StartsWith(TEXT("pt")))
	{
		return EDFLanguage::PortugueseBrazil;
	}
	if (Lower.StartsWith(TEXT("en")))
	{
		return EDFLanguage::English;
	}
	if (Lower.StartsWith(TEXT("es")))
	{
		return EDFLanguage::Spanish;
	}
	if (Lower.StartsWith(TEXT("fr")))
	{
		return EDFLanguage::French;
	}
	return EDFLanguage::English;
}

void UDFLocalizationSubsystem::SetLanguage(const EDFLanguage Language)
{
	const FString Code = LanguageToCultureCode(Language);
	ApplyCulture(Code, Language, true);
}

void UDFLocalizationSubsystem::ApplyCulture(const FString& CultureCode, const EDFLanguage InLanguage, const bool bSave)
{
	CurrentLanguage = InLanguage;
	CurrentCultureCode = CultureCode;

	// Mutating engine FInternationalization in Play-In-Editor rewrites Editor chrome (same OS process).
#if WITH_EDITOR
	bool bSkipGlobalCulture = false;
	if (const UWorld* World = GetWorld())
	{
		bSkipGlobalCulture = World->WorldType == EWorldType::PIE;
	}
	if (!bSkipGlobalCulture)
#endif
	{
		if (FInternationalization::Get().SetCurrentCulture(CultureCode))
		{
			FInternationalization::Get().SetCurrentLanguage(CultureCode);
			FInternationalization::Get().SetCurrentLocale(CultureCode);
		}
	}

	if (bSave)
	{
		if (UDFSaveGame* S = UDFSaveLibrary::ResolveMutableMetaSave(this))
		{
			S->PreferredLanguage = InLanguage;
			S->PreferredCultureCode = CultureCode;
			UDFSaveLibrary::SaveMetaSave(this, S);
		}
	}
	OnLanguageChanged.Broadcast(InLanguage);
}

TArray<FText> UDFLocalizationSubsystem::GetAvailableLanguageDisplayNames() const
{
	TArray<FText> Out;
	Out.Reserve(4);
	const TArray<FString> Codes = {TEXT("pt-BR"), TEXT("en"), TEXT("es"), TEXT("fr")};
	for (const FString& C : Codes)
	{
		const FCulturePtr Ptr = FInternationalization::Get().GetCulture(C);
		if (Ptr.IsValid())
		{
			Out.Add(FText::FromString(Ptr->GetNativeName()));
		}
		else
		{
			Out.Add(FText::FromString(C));
		}
	}
	return Out;
}
