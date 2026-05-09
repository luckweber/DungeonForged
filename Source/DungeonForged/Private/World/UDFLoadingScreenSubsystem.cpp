// Source/DungeonForged/Private/World/UDFLoadingScreenSubsystem.cpp
#include "World/UDFLoadingScreenSubsystem.h"
#include "World/UDFLoadingScreenWidget.h"
#include "World/UDFWorldTransitionSubsystem.h"
#include "Settings/UDFLoadingScreenDeveloperSettings.h"
#include "DungeonForgedModule.h"
#include "Blueprint/UserWidget.h"
#include "Containers/Ticker.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "HAL/PlatformTime.h"

namespace
{

struct FCollectedLoadingTipPresentation
{
	FText Label = FText::GetEmpty();
	FText Body = FText::GetEmpty();
	TSoftObjectPtr<UTexture2D> BgPreferred;
};

static TArray<FCollectedLoadingTipPresentation> GatherLoadingTipChoices(
	const UDataTable* const Dt, const ETravelReason Reason)
{
	TArray<FCollectedLoadingTipPresentation> Pool;
	if (!Dt)
	{
		return Pool;
	}
	Dt->ForeachRow<FDFLoadingScreenTipsRow>(TEXT("GatherLoadingTips"),
		[&Pool, Reason](const FName& /*RowName*/, const FDFLoadingScreenTipsRow& R)
		{
			const bool bMatchReason = R.bAlwaysIncludeInPool || R.ForReason == Reason;

			auto PushBgOnly = [&Pool](const FDFLoadingScreenTipsRow& RowRef)
				{
					if (RowRef.RowBackgroundTexture.IsNull())
					{
						return;
					}
					FCollectedLoadingTipPresentation BgOnly;
					BgOnly.BgPreferred = RowRef.RowBackgroundTexture;
					Pool.Add(MoveTemp(BgOnly));
				};

			if (!bMatchReason)
			{
				return;
			}
			bool bAddedFromTips = false;
			for (const FDFLoadingScreenTipPair& Tip : R.Tips)
			{
				if (Tip.TipBody.IsEmpty())
				{
					continue;
				}
				FCollectedLoadingTipPresentation C;
				C.Label = Tip.TipLabel;
				C.Body = Tip.TipBody;
				C.BgPreferred =
					Tip.BackgroundOverride.IsNull() ? R.RowBackgroundTexture : Tip.BackgroundOverride;
				Pool.Add(MoveTemp(C));
				bAddedFromTips = true;
			}
			if (!bAddedFromTips)
			{
				PushBgOnly(R);
			}
		});
	return Pool;
}

static bool PickRandomPresentationFromTable(const UDataTable* const Dt, const ETravelReason Reason,
	FText& OutLabel, FText& OutBody, UTexture2D*& OutBg)
{
	OutBg = nullptr;
	const TArray<FCollectedLoadingTipPresentation> Pool = GatherLoadingTipChoices(Dt, Reason);
	const int32 N = Pool.Num();
	if (N <= 0)
	{
		return false;
	}
	const int32 Idx = FMath::RandHelper(N);
	const FCollectedLoadingTipPresentation& W = Pool[Idx];
	const FText DefaultLbl = NSLOCTEXT("DF", "LoadTipLabel", "Dica");
	static const auto DefaultTipBody =
		NSLOCTEXT("DF", "LoadTipBody", "Use o terreno. Quebrar a linha de visao nega o encanto inimigo.");
	OutLabel = W.Label.IsEmpty() ? DefaultLbl : W.Label;
	OutBody = W.Body.IsEmpty() ? DefaultTipBody : W.Body;
	if (!W.BgPreferred.IsNull())
	{
		OutBg = Cast<UTexture2D>(W.BgPreferred.LoadSynchronous());
	}
	return true;
}

template <typename TWidget>
static TWidget* ResolveWidgetByAliases(UUserWidget* const Root,
	std::initializer_list<const TCHAR*> const Aliases)
{
	if (!Root)
	{
		return nullptr;
	}
	for (const TCHAR* const Name : Aliases)
	{
		if (UWidget* const Candidate = Root->GetWidgetFromName(FName(Name)))
		{
			return Cast<TWidget>(Candidate);
		}
	}
	return nullptr;
}

} // namespace

void UDFLoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ApplyDeveloperLoadingDefaults();
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UDFLoadingScreenSubsystem::HandlePostLoadMapWithWorld);
}

void UDFLoadingScreenSubsystem::ApplyDeveloperLoadingDefaults()
{
	CachedTipsTable = nullptr;
	UDFLoadingScreenDeveloperSettings* const DevCfg = GetMutableDefault<UDFLoadingScreenDeveloperSettings>();
	if (!DevCfg)
	{
		return;
	}
	// Garantir que DefaultGame.ini (secao [/Script/DungeonForged.DFLoadingScreenDeveloperSettings]) entre no CDO.
	DevCfg->LoadConfig(DevCfg->GetClass());

	if (DevCfg->LoadingScreenWidgetClass)
	{
		LoadingScreenClass = DevCfg->LoadingScreenWidgetClass;
	}
	if (DevCfg->LoadingScreenWidgetSoftPath.IsValid())
	{
		if (UClass* const Loaded = DevCfg->LoadingScreenWidgetSoftPath.TryLoadClass<UUserWidget>())
		{
			LoadingScreenClass = Loaded;
		}
		else
		{
			DF_LOG(Warning,
				"[DF|Loading] LoadingScreenWidgetSoftPath nao encontrado: %s",
				*DevCfg->LoadingScreenWidgetSoftPath.ToString());
		}
	}

	if (!LoadingScreenClass)
	{
		DF_LOG(Warning,
			"[DF|Loading] Sem LoadingScreenWidgetClass apos ler Project Settings / DefaultGame.ini. "
			"Use formato /Game/.../BP_X.BP_X_C ou preencha Soft Path.");
	}

	MinLoadingTime = DevCfg->MinLoadingTime;
	CachedTipsTable = DevCfg->TipsTable.LoadSynchronous();
	if (!DevCfg->TipsTable.IsNull() && !CachedTipsTable)
	{
		DF_LOG(Warning,
			"[DF|Loading] TipsTable configurada mas nao carregavel: %s",
			*DevCfg->TipsTable.ToString());
	}
}

void UDFLoadingScreenSubsystem::Deinitialize()
{
	ClearFallbackBindings();
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}
	CancelLoadingProgressTicker();
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UWorld* const W = GI->GetWorld())
		{
			W->GetTimerManager().ClearTimer(MinTimeTimer);
			W->GetTimerManager().ClearTimer(FadeTimer);
		}
	}
	CachedTipsTable = nullptr;
	Super::Deinitialize();
}

void UDFLoadingScreenSubsystem::ShowLoadingScreen(
	const ETravelReason Reason, const int32 NextFloorNumber, const int32 MaxFloors)
{
	UGameInstance* const GI = GetGameInstance();
	if (!GI)
	{
		return;
	}
	if (!LoadingScreenClass)
	{
		if (UDFWorldTransitionSubsystem* const T = GI->GetSubsystem<UDFWorldTransitionSubsystem>())
		{
			T->NotifyLoadingFinished();
		}
		return;
	}
	ShownReason = Reason;
	LoadingStartTime = FPlatformTime::Seconds();
	APlayerController* PC = nullptr;
	if (UWorld* const W = GI->GetWorld())
	{
		PC = W->GetFirstPlayerController();
	}
	if (!PC)
	{
		PC = UGameplayStatics::GetPlayerController(GI, 0);
	}
	if (!PC)
	{
		DF_LOG(Warning,
			"[DF|Loading] Sem PlayerController ao mostrar overlay (ex.: servidor dedicado). Transicao deve continuar sem UI.");
		if (UDFWorldTransitionSubsystem* const T = GI->GetSubsystem<UDFWorldTransitionSubsystem>())
		{
			T->NotifyLoadingFinished();
		}
		return;
	}
	ActiveLoadingScreen = CreateWidget<UUserWidget>(PC, LoadingScreenClass);
	ClearFallbackBindings();
	ActiveLoadingCxx = Cast<UDFLoadingScreenWidget>(ActiveLoadingScreen);
	if (ActiveLoadingScreen && !ActiveLoadingCxx)
	{
		DF_LOG(Warning,
			"[DF|Loading] O widget %s deve herdar de UDFLoadingScreenWidget para titulo/barra pela API C++. Tentando Fallback por nome (LoadingBar, FloorNumber)...",
			*LoadingScreenClass->GetPathName());
		TryDiscoverFallbackControls(ActiveLoadingScreen);
		if (FallbackLoadingBar.IsValid())
		{
			FallbackLoadingBar->SetPercent(0.f);
		}
		if (FallbackPctText.IsValid())
		{
			FallbackPctText->SetText(
				FText::Format(NSLOCTEXT("DF", "LoadingPctFmt", "{0}%"), FText::AsNumber(0)));
		}
		if (!FallbackLoadingBar.IsValid())
		{
			DF_LOG(Warning,
				"[DF|Loading] Sem ProgressBar encontrado nos aliases (ex.: LoadingBar). Barra pode ficar fixa.");
		}
	}
	if (ActiveLoadingCxx)
	{
		ActiveLoadingCxx->ResetForTravel();
		switch (Reason)
		{
		case ETravelReason::NewRun:
			ActiveLoadingCxx->SetLoadingTitleText(
				NSLOCTEXT("DF", "LoadTitleNewRun", "Gerando Dungeon..."));
			ActiveLoadingCxx->SetFlavorText(
				NSLOCTEXT("DF", "LoadFlavorNewRun", "A masmorra desperta; cada passo forja a lenda."));
			break;
		case ETravelReason::NextFloor:
			ActiveLoadingCxx->SetLoadingTitleText(
				FText::Format(
					NSLOCTEXT("DF", "LoadTitleFloor", "Andar {0}..."),
					FText::AsNumber(NextFloorNumber)));
			ActiveLoadingCxx->SetFloorCopy(NextFloorNumber, MaxFloors, FText::GetEmpty());
			ActiveLoadingCxx->SetRunProgress(
				static_cast<float>(NextFloorNumber) / static_cast<float>(FMath::Max(1, MaxFloors)));
			ActiveLoadingCxx->SetFlavorText(
				NSLOCTEXT("DF", "LoadFlavorFloor", "Subindo — os corredores mudam, mas a procura nao cessa."));
			break;
		case ETravelReason::Victory:
			ActiveLoadingCxx->SetLoadingTitleText(
				NSLOCTEXT("DF", "LoadTitleVictory", "Retornando ao Nexus..."));
			ActiveLoadingCxx->SetFlavorText(
				NSLOCTEXT("DF", "LoadFlavorVictory", "O portal brilha; a forja do hub aguarda teus trofeus."));
			break;
		case ETravelReason::Defeat:
		case ETravelReason::AbandonRun:
			ActiveLoadingCxx->SetLoadingTitleText(
				NSLOCTEXT("DF", "LoadTitleReturn", "Retornando ao Nexus..."));
			ActiveLoadingCxx->SetFlavorText(
				NSLOCTEXT("DF", "LoadFlavorDefeat", "Toda queda e runa; levanta e forja de novo."));
			break;
		case ETravelReason::FirstLaunch:
		default:
			ActiveLoadingCxx->SetLoadingTitleText(
				NSLOCTEXT("DF", "LoadTitleDefault", "Carregando..."));
			ActiveLoadingCxx->SetFlavorText(FText::GetEmpty());
			break;
		}
		FText TipLabelChosen;
		FText TipBodyChosen;
		UTexture2D* TipBgChosen = nullptr;
		if (!PickRandomPresentationFromTable(CachedTipsTable.Get(), Reason, TipLabelChosen, TipBodyChosen, TipBgChosen))
		{
			TipLabelChosen = NSLOCTEXT("DF", "LoadTipLabel", "Dica");
			TipBodyChosen = NSLOCTEXT(
				"DF", "LoadTipBody", "Use o terreno. Quebrar a linha de visao nega o encanto inimigo.");
			TipBgChosen = nullptr;
		}
		ActiveLoadingCxx->SetTip(TipLabelChosen, TipBodyChosen);
		if (TipBgChosen)
		{
			ActiveLoadingCxx->SetOptionalBackgroundFromTexture(TipBgChosen);
		}
	}
	const bool bCxxPath = ActiveLoadingCxx != nullptr;
	DF_LOG(Display,
		"[DF|Loading] Overlay: Razao=%d Classe=%s C++widget=%s FallbackBar=%s FallbackPctTxt=%s",
		static_cast<int32>(Reason),
		LoadingScreenClass ? *LoadingScreenClass->GetPathName() : TEXT("<null>"),
		bCxxPath ? TEXT("sim") : TEXT("nao"),
		FallbackLoadingBar.IsValid() ? TEXT("sim") : TEXT("nao"),
		FallbackPctText.IsValid() ? TEXT("sim") : TEXT("nao"));
	ActiveLoadingScreen->AddToViewport(100);
	FInputModeUIOnly M;
	PC->SetInputMode(M);
	PC->SetShowMouseCursor(false);

	CancelLoadingProgressTicker();
	LerpStepIndex = 0;
	bMapLoaded = false;
	CurrentFilledPct = 0.f;
	ProgressTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UDFLoadingScreenSubsystem::OnLoadingProgressTicker), 0.05f);

	if (!ProgressTickerHandle.IsValid())
	{
		DF_LOG(Warning, "[DF|Loading] Falha ao registar ticker de progresso CoreTicker.");
	}
}

void UDFLoadingScreenSubsystem::CancelLoadingProgressTicker()
{
	if (!ProgressTickerHandle.IsValid())
	{
		return;
	}
	FTSTicker::GetCoreTicker().RemoveTicker(ProgressTickerHandle);
	ProgressTickerHandle.Reset();
}

bool UDFLoadingScreenSubsystem::OnLoadingProgressTicker(float /*DeltaTime*/)
{
	UGameInstance* const GI = GetGameInstance();
	if (!GI || !ActiveLoadingScreen)
	{
		return false;
	}

	if (!ActiveLoadingCxx && !FallbackLoadingBar.IsValid())
	{
		return false;
	}

	static constexpr float kStepSecs = 0.05f;
	static constexpr float kFakeTop = 0.9f;
	static constexpr float kFakeSpeed = 2.5f;
	static constexpr float kFillSpeed = 6.f;

	auto ApplyShown = [this](const float Pct)
		{
			if (ActiveLoadingCxx)
			{
				ActiveLoadingCxx->SetLoadingProgress(Pct, false);
			}
			else if (FallbackLoadingBar.IsValid())
			{
				FallbackSetLoadingProgress(Pct, false);
			}
		};

	if (!bMapLoaded)
	{
		if (LerpStepIndex < LerpStepCount)
		{
			++LerpStepIndex;
		}
		const float FakeTarget =
			FMath::Clamp(static_cast<float>(LerpStepIndex) / static_cast<float>(LerpStepCount), 0.f, 1.f) * kFakeTop;
		CurrentFilledPct = FMath::FInterpTo(CurrentFilledPct, FakeTarget, kStepSecs, kFakeSpeed);
		ApplyShown(FMath::Clamp(CurrentFilledPct, 0.f, 1.f));
		return true;
	}

	CurrentFilledPct = FMath::FInterpTo(CurrentFilledPct, 1.f, kStepSecs, kFillSpeed);
	ApplyShown(FMath::Clamp(CurrentFilledPct, 0.f, 1.f));

	if (CurrentFilledPct >= 0.999f)
	{
		CurrentFilledPct = 1.f;
		ApplyShown(1.f);
		HideLoadingScreen();
		return false;
	}

	return true;
}

void UDFLoadingScreenSubsystem::HandlePostLoadMapWithWorld(UWorld* const LoadedWorld)
{
	if (!ActiveLoadingScreen)
	{
		return;
	}
	if (!LoadedWorld || !GetGameInstance() || GetGameInstance()->GetWorld() != LoadedWorld)
	{
		return;
	}
	// Nao cancelar o CoreTicker: fase 2 anima ate 100% antes de HideLoadingScreen.
	bMapLoaded = true;
}

void UDFLoadingScreenSubsystem::HideLoadingScreen()
{
	CancelLoadingProgressTicker();
	UGameInstance* const GI = GetGameInstance();
	if (!GI)
	{
		return;
	}
	const double Elapsed = FPlatformTime::Seconds() - LoadingStartTime;
	const float Rem = FMath::Max(0.f, static_cast<float>(MinLoadingTime) - static_cast<float>(Elapsed));
	if (UWorld* const W = GI->GetWorld())
	{
		if (Rem > 0.01f)
		{
			W->GetTimerManager().SetTimer(
				MinTimeTimer,
				FTimerDelegate::CreateUObject(this, &UDFLoadingScreenSubsystem::FinishHideAfterMinTime),
				Rem,
				false);
			return;
		}
	}
	FinishHideAfterMinTime();
}

void UDFLoadingScreenSubsystem::FinishHideAfterMinTime()
{
	UGameInstance* const GI = GetGameInstance();
	if (!GI)
	{
		return;
	}
	if (!ActiveLoadingScreen)
	{
		if (UDFWorldTransitionSubsystem* const T = GI->GetSubsystem<UDFWorldTransitionSubsystem>())
		{
			T->NotifyLoadingFinished();
		}
		return;
	}
	// 0.5s fade: animation length in asset; playback at normal speed
	if (UDFLoadingScreenWidget* const Cxx = ActiveLoadingCxx)
	{
		Cxx->PlayFadeOutAnimation();
	}
	if (UWorld* const W = GI->GetWorld())
	{
		W->GetTimerManager().SetTimer(
			FadeTimer,
			FTimerDelegate::CreateUObject(this, &UDFLoadingScreenSubsystem::OnFadeOutRemoveWidget),
			0.5f,
			false);
	}
}

void UDFLoadingScreenSubsystem::OnFadeOutRemoveWidget()
{
	if (ActiveLoadingScreen)
	{
		ActiveLoadingScreen->RemoveFromParent();
	}
	ActiveLoadingScreen = nullptr;
	ActiveLoadingCxx = nullptr;
	ClearFallbackBindings();
	bMapLoaded = false;
	CurrentFilledPct = 0.f;
	if (UGameInstance* const G = GetGameInstance())
	{
		if (UDFWorldTransitionSubsystem* const T = G->GetSubsystem<UDFWorldTransitionSubsystem>())
		{
			T->NotifyLoadingFinished();
		}
	}
}

void UDFLoadingScreenSubsystem::ClearFallbackBindings()
{
	FallbackLoadingBar.Reset();
	FallbackPctText.Reset();
}

void UDFLoadingScreenSubsystem::TryDiscoverFallbackControls(UUserWidget* const ScreenRoot)
{
	if (!ScreenRoot)
	{
		return;
	}

	UProgressBar* const Bar = ResolveWidgetByAliases<UProgressBar>(ScreenRoot,
		{
			TEXT("LoadingBar"),
			TEXT("PB_Loading"),
			TEXT("Progress_LoadingBar"),
			TEXT("ProgressBar_Loading"),
			TEXT("ProgressBar"),
		});

	UTextBlock* const PctTxt = ResolveWidgetByAliases<UTextBlock>(ScreenRoot,
		{
			TEXT("FloorNumber"),
			TEXT("LoadingPct"),
			TEXT("PercentLabel"),
			TEXT("Txt_Percent"),
			TEXT("Txt_Percentage"),
			TEXT("Text_Pct"),
			TEXT("LoadingPercent"),
		});

	if (Bar)
	{
		FallbackLoadingBar = Bar;
	}
	if (PctTxt)
	{
		FallbackPctText = PctTxt;
	}
}

void UDFLoadingScreenSubsystem::FallbackSetLoadingProgress(const float Pct, const bool bSnapComplete)
{
	const float ShownPct = bSnapComplete ? 1.f : FMath::Clamp(Pct, 0.f, 1.f);
	if (FallbackLoadingBar.IsValid())
	{
		FallbackLoadingBar->SetPercent(ShownPct);
	}
	if (FallbackPctText.IsValid())
	{
		FallbackPctText->SetText(FText::Format(
			NSLOCTEXT("DF", "LoadingPctFmt", "{0}%"),
			FText::AsNumber(FMath::Clamp(FMath::RoundToInt(ShownPct * 100.f), 0, 100))));
	}
}
