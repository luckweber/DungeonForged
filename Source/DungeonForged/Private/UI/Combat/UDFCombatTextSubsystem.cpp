// Source/DungeonForged/Private/UI/Combat/UDFCombatTextSubsystem.cpp
#include "UI/Combat/UDFCombatTextSubsystem.h"
#include "Network/UDFNetworkLibrary.h"
#include "UI/Combat/UDFCombatTextWidget.h"
#include "Localization/UDFAccessibilitySubsystem.h"
#include "Engine/GameInstance.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UDFCombatTextSubsystem)

void UDFCombatTextSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UDFCombatTextSubsystem::Deinitialize()
{
	if (UWorld* const W = GetWorld())
	{
		for (TObjectPtr<UDFCombatTextWidget>& Wd : Pooled)
		{
			if (Wd)
			{
				Wd->RemoveFromParent();
			}
		}
		for (TObjectPtr<UDFCombatTextWidget>& Wd : InUse)
		{
			if (Wd)
			{
				Wd->RemoveFromParent();
			}
		}
	}
	Pooled.Empty();
	InUse.Empty();
	bPoolBuilt = false;
	Super::Deinitialize();
}

void UDFCombatTextSubsystem::EnsurePooledWidgets()
{
	if (bPoolBuilt)
	{
		return;
	}
	if (!GetWorld() || !WidgetClass)
	{
		return;
	}
	APlayerController* const PC = UDFNetworkLibrary::GetLocalPlayerController(GetWorld());
	if (!PC)
	{
		return;
	}
	Pooled.Reset();
	for (int32 I = 0; I < PoolSize; ++I)
	{
		UDFCombatTextWidget* const Wd = CreateWidget<UDFCombatTextWidget>(PC, WidgetClass);
		if (Wd)
		{
			Wd->SetVisibility(ESlateVisibility::Collapsed);
			Wd->AddToViewport(60);
			Pooled.Add(Wd);
		}
	}
	bPoolBuilt = true;
}

float UDFCombatTextSubsystem::ResolveDuration(const ECombatTextType Type, const float Custom) const
{
	if (Custom > 0.f)
	{
		return Custom;
	}
	if (Type == ECombatTextType::LevelUp)
	{
		return 2.f;
	}
	return 1.2f;
}

namespace
{
FString AbbreviateDamageNumber(const int32 Value)
{
	const int32 AbsVal = FMath::Abs(Value);
	if (AbsVal >= 1000000)
	{
		return FString::Printf(TEXT("%.1fM"), AbsVal / 1000000.f);
	}
	if (AbsVal >= 1000)
	{
		return FString::Printf(TEXT("%.1fk"), AbsVal / 1000.f);
	}
	return FString::FromInt(AbsVal);
}
} // namespace

FString UDFCombatTextSubsystem::BuildStringForValue(const float Value, const ECombatTextType Type)
{
	switch (Type)
	{
	case ECombatTextType::Damage_Critical:
		return FString::Printf(TEXT("★ %s ★"), *AbbreviateDamageNumber(FMath::RoundToInt(Value)));
	case ECombatTextType::Heal:
		return FString::Printf(TEXT("+%d"), FMath::RoundToInt(FMath::Max(0.f, Value)));
	case ECombatTextType::Damage_Physical:
	case ECombatTextType::Damage_Magic:
	case ECombatTextType::Damage_True:
	case ECombatTextType::Damage_DoT:
		return AbbreviateDamageNumber(FMath::Max(0, FMath::RoundToInt(FMath::Abs(Value))));
	case ECombatTextType::Mana_Restore:
		return FString::Printf(TEXT("+%d"), FMath::RoundToInt(FMath::Max(0.f, Value)));
	case ECombatTextType::XPGain:
		return FString::Printf(TEXT("+%d XP"), FMath::RoundToInt(Value));
	case ECombatTextType::GoldGain:
		return FString::Printf(TEXT("+%d Gold"), FMath::Max(0, FMath::RoundToInt(Value)));
	case ECombatTextType::LevelUp:
		return FString(TEXT("LEVEL UP!"));
	case ECombatTextType::Miss: return TEXT("MISS");
	case ECombatTextType::Dodge: return TEXT("DODGE");
	case ECombatTextType::Block: return TEXT("BLOCK");
	case ECombatTextType::Immune: return TEXT("IMMUNE");
	default:
		return FString::FromInt(FMath::RoundToInt(Value));
	}
}

void UDFCombatTextSubsystem::SpawnText(
	const FVector WorldLocation, const float Value, const ECombatTextType Type, const float CustomDuration)
{
	const FString S = BuildStringForValue(Value, Type);
	SpawnTextString(WorldLocation, S, Type, CustomDuration);
}

namespace
{
bool IsFloatingDamageCombatText(const ECombatTextType Type)
{
	switch (Type)
	{
	case ECombatTextType::Damage_Physical:
	case ECombatTextType::Damage_Magic:
	case ECombatTextType::Damage_True:
	case ECombatTextType::Damage_Critical:
	case ECombatTextType::Damage_DoT:
		return true;
	default:
		return false;
	}
}
} // namespace

void UDFCombatTextSubsystem::SpawnTextString(
	const FVector WorldLocation, const FString& Text, const ECombatTextType Type, const float CustomDuration)
{
	if (!GetWorld() || IsRunningDedicatedServer())
	{
		return;
	}
	if (IsFloatingDamageCombatText(Type))
	{
		if (UGameInstance* const GI = GetWorld()->GetGameInstance())
		{
			if (const UDFAccessibilitySubsystem* const A11y = GI->GetSubsystem<UDFAccessibilitySubsystem>())
			{
				if (!A11y->GetSettings().bShowDamageNumbers)
				{
					return;
				}
			}
		}
	}
	if (!WidgetClass)
	{
		return;
	}
	EnsurePooledWidgets();
	UDFCombatTextWidget* Wd = nullptr;
	if (Pooled.Num() > 0)
	{
		Wd = Pooled[0];
		Pooled.RemoveAt(0, 1, EAllowShrinking::No);
	}
	else
	{
		APlayerController* const PC = UDFNetworkLibrary::GetLocalPlayerController(GetWorld());
		if (PC)
		{
			Wd = CreateWidget<UDFCombatTextWidget>(PC, WidgetClass);
			if (Wd)
			{
				Wd->AddToViewport(60);
			}
		}
	}
	if (!Wd)
	{
		return;
	}
	InUse.Add(Wd);
	const float Dur = ResolveDuration(Type, CustomDuration);
	Wd->InitializeCombatText(Text, Type, WorldLocation, Dur, this);
}

void UDFCombatTextSubsystem::ReturnToPool(UDFCombatTextWidget* const Widget)
{
	if (!Widget)
	{
		return;
	}
	InUse.Remove(Widget);
	if (Pooled.Num() < PoolSize)
	{
		Pooled.Add(Widget);
	}
	else
	{
		Widget->RemoveFromParent();
	}
}
