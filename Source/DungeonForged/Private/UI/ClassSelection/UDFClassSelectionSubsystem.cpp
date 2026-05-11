// Source/DungeonForged/Private/UI/ClassSelection/UDFClassSelectionSubsystem.cpp
#include "UI/ClassSelection/UDFClassSelectionSubsystem.h"
#include "Settings/UDFClassSelectionDeveloperSettings.h"
#include "GameModes/MainMenu/ADFMainMenuHUD.h"
#include "GameModes/Nexus/ADFNexusHUD.h"
#include "DungeonForgedModule.h"
#include "UI/ClassSelection/UDFClassSelectionWidget.h"
#include "UI/ClassSelection/UDFClassPreviewRotatorComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/Character.h"
#include "Characters/ADFPlayerState.h"
#include "Data/DFDataTableStructs.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameModes/Nexus/ADFNexusPlayerController.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Run/DFRunManager.h"
#include "Run/DFSaveGame.h"
#include "Run/UDFSaveSlotManagerSubsystem.h"
#include "World/UDFWorldTransitionSubsystem.h"
#include "World/DFWorldTypes.h"
#include "Blueprint/UserWidget.h"

static bool DfIsUnlockedByRules(FName ClassName, const UDFSaveGame* Save);
static FText DfGetUnlockText(FName ClassName, const UDFSaveGame* Save);

static const TCHAR* DfPreviewDisplayModeLabel(const EDFClassPreviewDisplayMode M)
{
	switch (M)
	{
	case EDFClassPreviewDisplayMode::SceneCaptureUMG:
		return TEXT("SceneCaptureUMG");
	case EDFClassPreviewDisplayMode::WorldWithPlayerCamera:
		return TEXT("WorldWithPlayerCamera");
	case EDFClassPreviewDisplayMode::WorldShowcaseCamera:
		return TEXT("WorldShowcaseCamera");
	default:
		return TEXT("?");
	}
}

/** Prefer actor with this tag that has a UCameraComponent (Cine incl.). */
static AActor* DfFindShowcaseCameraViewTarget(UWorld* const W, const FName ActorTag)
{
	if (!W || ActorTag.IsNone())
	{
		return nullptr;
	}
	TArray<AActor*> Tagged;
	UGameplayStatics::GetAllActorsWithTag(W, ActorTag, Tagged);
	for (AActor* const A : Tagged)
	{
		if (A && A->FindComponentByClass<UCameraComponent>())
		{
			return A;
		}
	}
	return nullptr;
}

void UDFClassSelectionSubsystem::ClearShowcaseFadeChain()
{
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(ShowcaseFadeChainTimerHandle);
	}
	ShowcaseFadeEnterViewTarget.Reset();
	ShowcaseFadeRestoreOrchestrationPC.Reset();
	ShowcaseFadeRestoreViewTargetPrior.Reset();
}

void UDFClassSelectionSubsystem::OnShowcaseEnterFadeOutElapsed()
{
	APlayerController* const PC = ShowcaseOwningPlayerController.Get();
	AActor* const VT = ShowcaseFadeEnterViewTarget.Get();
	ShowcaseFadeEnterViewTarget.Reset();
	if (!PC || !IsValid(VT))
	{
		return;
	}
	PC->SetViewTargetWithBlend(
		VT, 0.f, EViewTargetBlendFunction::VTBlend_Linear, 0.f, false);
	if (APlayerCameraManager* const PCM = PC->PlayerCameraManager)
	{
		const float FadeIn = FMath::Max(ShowcaseFadeInSeconds, KINDA_SMALL_NUMBER);
		PCM->StartCameraFade(1.f, 0.f, FadeIn, FLinearColor::Black, false, true);
	}
}

void UDFClassSelectionSubsystem::OnShowcaseRestoreFadeOutElapsed()
{
	APlayerController* const PC = ShowcaseFadeRestoreOrchestrationPC.Get();
	AActor* const R = ShowcaseFadeRestoreViewTargetPrior.Get();
	ShowcaseFadeRestoreOrchestrationPC.Reset();
	ShowcaseFadeRestoreViewTargetPrior.Reset();
	if (!PC || !IsValid(R))
	{
		return;
	}
	PC->SetViewTargetWithBlend(
		R, 0.f, EViewTargetBlendFunction::VTBlend_Linear, 0.f, false);
	if (APlayerCameraManager* const PCM = PC->PlayerCameraManager)
	{
		const float FadeIn = FMath::Max(ShowcaseFadeInSeconds, KINDA_SMALL_NUMBER);
		PCM->StartCameraFade(1.f, 0.f, FadeIn, FLinearColor::Black, false, true);
	}
}

void UDFClassSelectionSubsystem::ApplyShowcaseCameraIfNeeded(APlayerController* const PC)
{
	UWorld* const W = GetWorld();
	if (!PC || !W || PreviewDisplayMode != EDFClassPreviewDisplayMode::WorldShowcaseCamera)
	{
		return;
	}
	if (ShowcaseCameraActorTag.IsNone())
	{
		DF_LOG(Warning,
			"[DF|ClassSelection] ShowcaseCamera: ShowcaseCameraActorTag vazio — defina uma tag ou use um CameraActor marcado.");
		return;
	}

	AActor* ViewTarget = DfFindShowcaseCameraViewTarget(W, ShowcaseCameraActorTag);
	FName ViewTargetSourceTag = ShowcaseCameraActorTag;
	if (!ViewTarget)
	{
		static const FName PreviewAnchorTag(TEXT("ClassSelectionPreview"));
		ViewTarget = DfFindShowcaseCameraViewTarget(W, PreviewAnchorTag);
		ViewTargetSourceTag = PreviewAnchorTag;
	}

	if (!ViewTarget)
	{
		DF_LOG(Warning,
			"[DF|ClassSelection] ShowcaseCamera: sem view target com CameraComponent — tag primária '%s' ou fallback 'ClassSelectionPreview'. "
			"Marca o actor com tag 'ClassSelectionPreviewCamera' (mesmo BP que a âncora) e adiciona Cine Camera ou Camera no actor com 'ClassSelectionPreview'.",
			*ShowcaseCameraActorTag.ToString());
		return;
	}

	DF_LOG(Log, "[DF|ClassSelection] ShowcaseCamera: ViewTarget=%s (tag usada na procura: %s)",
		*ViewTarget->GetName(), *ViewTargetSourceTag.ToString());

	RestoreShowcaseCameraIfNeeded();

	ShowcaseViewTargetPrior = PC->GetViewTarget();
	ShowcaseOwningPlayerController = PC;
	bShowcaseCameraActive = true;

	APlayerCameraManager* const PCM = PC->PlayerCameraManager;
	const bool bFadeCut = bShowcaseUseFadeCutTransition && ShowcaseFadeOutSeconds > 0.f && PCM;
	if (PCM)
	{
		PCM->StopCameraFade();
	}
	if (bFadeCut)
	{
		ShowcaseFadeEnterViewTarget = ViewTarget;
		PCM->StartCameraFade(0.f, 1.f, ShowcaseFadeOutSeconds, FLinearColor::Black, false, true);
		W->GetTimerManager().SetTimer(
			ShowcaseFadeChainTimerHandle,
			this,
			&UDFClassSelectionSubsystem::OnShowcaseEnterFadeOutElapsed,
			ShowcaseFadeOutSeconds,
			false);
		return;
	}

	const float EnterBlendSecs = bShowcaseUseFadeCutTransition ? 0.f : ShowcaseCameraBlendInSeconds;
	PC->SetViewTargetWithBlend(
		ViewTarget,
		EnterBlendSecs,
		EViewTargetBlendFunction::VTBlend_Cubic,
		0.f,
		false);
}

void UDFClassSelectionSubsystem::RestoreShowcaseCameraIfNeeded()
{
	ClearShowcaseFadeChain();

	if (!bShowcaseCameraActive)
	{
		return;
	}

	APlayerController* const PC = ShowcaseOwningPlayerController.Get();
	bShowcaseCameraActive = false;
	ShowcaseOwningPlayerController.Reset();

	TWeakObjectPtr<AActor> PriorWeak = ShowcaseViewTargetPrior;
	ShowcaseViewTargetPrior.Reset();

	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StopCameraFade();
	}

	if (!PC)
	{
		return;
	}

	AActor* RestoreTo = PriorWeak.IsValid() ? PriorWeak.Get() : nullptr;
	if (!IsValid(RestoreTo))
	{
		RestoreTo = PC->GetPawn();
	}
	if (!IsValid(RestoreTo))
	{
		return;
	}

	UWorld* const W = GetWorld();
	APlayerCameraManager* const PCM = PC->PlayerCameraManager;

	const bool bFadeRestore =
		W && PreviewDisplayMode == EDFClassPreviewDisplayMode::WorldShowcaseCamera && bShowcaseUseFadeCutTransition
		&& ShowcaseFadeOutSeconds > 0.f && PCM;

	if (bFadeRestore)
	{
		ShowcaseFadeRestoreOrchestrationPC = PC;
		ShowcaseFadeRestoreViewTargetPrior = RestoreTo;
		PCM->StartCameraFade(0.f, 1.f, ShowcaseFadeOutSeconds, FLinearColor::Black, false, true);
		W->GetTimerManager().SetTimer(
			ShowcaseFadeChainTimerHandle,
			this,
			&UDFClassSelectionSubsystem::OnShowcaseRestoreFadeOutElapsed,
			ShowcaseFadeOutSeconds,
			false);
		return;
	}

	const float RestoreBlendSecs =
		(PreviewDisplayMode == EDFClassPreviewDisplayMode::WorldShowcaseCamera && bShowcaseUseFadeCutTransition)
		 ? 0.f : ShowcaseCameraRestoreSeconds;
	PC->SetViewTargetWithBlend(
		RestoreTo,
		RestoreBlendSecs,
		EViewTargetBlendFunction::VTBlend_Cubic,
		0.f,
		false);
}

void UDFClassSelectionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// WorldSubsystem nao mescla Config/DefaultGame.ini automaticamente como alguns actors —
	// sem LoadConfig(), ClassSelectionWidgetClass / PreviewPawnClass ficam None mesmo com .ini correto.
	LoadConfig(GetClass());

	if (const UDFClassSelectionDeveloperSettings* const Dev = GetDefault<UDFClassSelectionDeveloperSettings>())
	{
		if (Dev->PreviewPawnClass)
		{
			PreviewPawnClass = Dev->PreviewPawnClass;
		}
		if (Dev->ClassSelectionWidgetClass)
		{
			ClassSelectionWidgetClass = Dev->ClassSelectionWidgetClass;
		}
		if (Dev->ClassSelectionWidgetSoftPath.IsValid())
		{
			ClassSelectionWidgetSoftPath = Dev->ClassSelectionWidgetSoftPath;
		}
		if (!Dev->ClassDataTable.IsNull())
		{
			ClassTable = Dev->ClassDataTable.LoadSynchronous();
		}
		bPreviewUseFillLight = Dev->bPreviewUseFillLight;
		PreviewDisplayMode = Dev->PreviewDisplayMode;
	}

	DF_LOG(Log,
		"[DF|ClassSelection] Initialize: PreviewPawnClass=%s DisplayMode=%s ClassSelectionWidgetClass=%s SoftPath=%s ClassTable=%s",
		PreviewPawnClass ? *PreviewPawnClass->GetPathName() : TEXT("(none)"),
		DfPreviewDisplayModeLabel(PreviewDisplayMode),
		ClassSelectionWidgetClass ? *ClassSelectionWidgetClass->GetPathName() : TEXT("(none)"),
		*ClassSelectionWidgetSoftPath.ToString(),
		ClassTable ? *ClassTable->GetPathName() : TEXT("(none)"));
}

void UDFClassSelectionSubsystem::Deinitialize()
{
	RestoreShowcaseCameraIfNeeded();
	if (bMainMenuLayersSuppressedForWorldPreview)
	{
		if (UWorld* const W = GetWorld())
		{
			if (APlayerController* const PC = UGameplayStatics::GetPlayerController(W, 0))
			{
				if (ADFMainMenuHUD* const HUD = Cast<ADFMainMenuHUD>(PC->GetHUD()))
				{
					HUD->SuppressUnderlyingMenuForClassSelectionWorldPreview(false);
				}
			}
		}
		bMainMenuLayersSuppressedForWorldPreview = false;
	}
	if (bNexusHudSuppressedForClassSelection)
	{
		if (UWorld* const Wd = GetWorld())
		{
			if (APlayerController* const PC = UGameplayStatics::GetPlayerController(Wd, 0))
			{
				if (ADFNexusHUD* const NH = Cast<ADFNexusHUD>(PC->GetHUD()))
				{
					NH->SetRootWidgetHiddenForClassSelection(false);
				}
			}
		}
		bNexusHudSuppressedForClassSelection = false;
	}
	if (UWorld* const W = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(W, 1.f);
	}
	DestroyPreviewPawn();
	SaveRef = nullptr;
	ClassTable = nullptr;
	ActiveRenderTarget = nullptr;
	ClassSelectionWidgetInstance = nullptr;
	MainMenuClassDestination = EDFMainMenuClassPickDestination::None;
	Super::Deinitialize();
}

UDataTable* UDFClassSelectionSubsystem::GetClassTable() const
{
	UDFClassSelectionSubsystem* const S = const_cast<UDFClassSelectionSubsystem*>(this);
	S->EnsureClassTable();
	if (S->ClassTable)
	{
		return S->ClassTable;
	}
	UGameInstance* const GI = S->GetWorld() ? S->GetWorld()->GetGameInstance() : nullptr;
	UDFRunManager* const RM = GI ? GI->GetSubsystem<UDFRunManager>() : nullptr;
	return RM ? RM->ClassDataTable : nullptr;
}

void UDFClassSelectionSubsystem::EnsureClassTable()
{
	if (ClassTable)
	{
		return;
	}
	UGameInstance* const GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UDFRunManager* const RM = GI ? GI->GetSubsystem<UDFRunManager>() : nullptr;
	if (RM && RM->ClassDataTable)
	{
		ClassTable = RM->ClassDataTable;
	}
}

void UDFClassSelectionSubsystem::EnsureWidgetClassResolved()
{
	if (ClassSelectionWidgetClass)
	{
		return;
	}
	if (ClassSelectionWidgetSoftPath.IsValid())
	{
		// LoadSynchronous garante a classe disponivel ja na primeira abertura,
		// evitando que o subsystem caia no UDFClassSelectionWidget vazio (sem layout).
		if (UClass* const Loaded = ClassSelectionWidgetSoftPath.TryLoadClass<UUserWidget>())
		{
			ClassSelectionWidgetClass = Loaded;
			DF_LOG(Log, "[DF|ClassSelection] EnsureWidgetClassResolved: ClassSelectionWidgetClass resolvida via SoftPath -> %s",
				*Loaded->GetName());
			return;
		}
		DF_LOG(Warning, "[DF|ClassSelection] EnsureWidgetClassResolved: SoftPath '%s' nao encontrado",
			*ClassSelectionWidgetSoftPath.ToString());
	}
}

void UDFClassSelectionSubsystem::EnsureSaveLoaded()
{
	if (SaveRef)
	{
		return;
	}
	UGameInstance* const GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	if (UDFSaveSlotManagerSubsystem* const S = GI ? GI->GetSubsystem<UDFSaveSlotManagerSubsystem>() : nullptr)
	{
		SaveRef = S->GetActiveOrLegacyMetaSave();
	}
	if (!SaveRef)
	{
		SaveRef = UDFSaveGame::Load();
	}
}

const FDFClassTableRow* UDFClassSelectionSubsystem::GetClassData(const FName ClassName) const
{
	const UDataTable* const T = GetClassTable();
	if (!T)
	{
		return nullptr;
	}
	return T->FindRow<FDFClassTableRow>(ClassName, TEXT("GetClassData"));
}

bool UDFClassSelectionSubsystem::IsClassUnlocked(const FName ClassName) const
{
	return DfIsUnlockedByRules(ClassName, SaveRef);
}

FText UDFClassSelectionSubsystem::GetUnlockConditionText(const FName ClassName) const
{
	return DfGetUnlockText(ClassName, SaveRef);
}

void UDFClassSelectionSubsystem::GetStatBarScalesForClass(
	const FName ClassName, float& OutStr, float& OutInt, float& OutAgi, float& OutDef, float& OutMaxHp) const
{
	OutStr = OutInt = OutAgi = OutDef = OutMaxHp = 0.f;
	const UDataTable* const T = GetClassTable();
	if (!T)
	{
		return;
	}
	const FGameplayAttribute SAttr = UDFAttributeSet::GetStrengthAttribute();
	const FGameplayAttribute IAttr = UDFAttributeSet::GetIntelligenceAttribute();
	const FGameplayAttribute AAttr = UDFAttributeSet::GetAgilityAttribute();
	const FGameplayAttribute ArAttr = UDFAttributeSet::GetArmorAttribute();
	const FGameplayAttribute MrAttr = UDFAttributeSet::GetMagicResistAttribute();
	const FGameplayAttribute MhAttr = UDFAttributeSet::GetMaxHealthAttribute();

	float MaxS = 1.f, MaxI = 1.f, MaxA = 1.f, MaxD = 1.f, MaxH = 1.f;
	T->ForeachRow<FDFClassTableRow>(
		TEXT("StatMaxes"),
		[SAttr, IAttr, AAttr, ArAttr, MrAttr, MhAttr, &MaxS, &MaxI, &MaxA, &MaxD, &MaxH](const FName&,
		const FDFClassTableRow& R) {
			for (const TPair<FGameplayAttribute, float>& P : R.BaseAttributeValues)
			{
				if (P.Key == SAttr) MaxS = FMath::Max(MaxS, P.Value);
				if (P.Key == IAttr) MaxI = FMath::Max(MaxI, P.Value);
				if (P.Key == AAttr) MaxA = FMath::Max(MaxA, P.Value);
				if (P.Key == MhAttr) MaxH = FMath::Max(MaxH, P.Value);
			}
			const float* const Ar = R.BaseAttributeValues.Find(ArAttr);
			const float* const Mr = R.BaseAttributeValues.Find(MrAttr);
			const float D = 0.5f * (Ar ? *Ar : 0.f) + 0.5f * (Mr ? *Mr : 0.f);
			MaxD = FMath::Max(MaxD, D);
		});

	const FDFClassTableRow* const Row = T->FindRow<FDFClassTableRow>(ClassName, TEXT("StatBars"));
	if (!Row)
	{
		return;
	}
	OutStr = MaxS > 0.f ? Row->BaseAttributeValues.FindRef(SAttr) / MaxS : 0.f;
	OutInt = MaxI > 0.f ? Row->BaseAttributeValues.FindRef(IAttr) / MaxI : 0.f;
	OutAgi = MaxA > 0.f ? Row->BaseAttributeValues.FindRef(AAttr) / MaxA : 0.f;
	{
		const float* const Ar = Row->BaseAttributeValues.Find(ArAttr);
		const float* const Mr = Row->BaseAttributeValues.Find(MrAttr);
		const float D = 0.5f * (Ar ? *Ar : 0.f) + 0.5f * (Mr ? *Mr : 0.f);
		OutDef = MaxD > 0.f ? D / MaxD : 0.f;
	}
	OutMaxHp = MaxH > 0.f ? Row->BaseAttributeValues.FindRef(MhAttr) / MaxH : 0.f;
}

void UDFClassSelectionSubsystem::ApplyUiTag(APlayerController* const PC, const bool bAdd)
{
	if (!PC)
	{
		return;
	}
	ADFPlayerState* const DPS = PC->GetPlayerState<ADFPlayerState>();
	if (!DPS || !DPS->AbilitySystemComponent)
	{
		DF_LOG(Log, "[DF|ClassSelection] ApplyUiTag: sem PlayerState/ASC (menu principal?) bAdd=%s - tag UI_ClassSelectionOpen ignorada",
			bAdd ? TEXT("sim") : TEXT("nao"));
		return;
	}
	if (bAdd)
	{
		DPS->AbilitySystemComponent->AddLooseGameplayTag(FDFGameplayTags::UI_ClassSelectionOpen);
	}
	else
	{
		DPS->AbilitySystemComponent->RemoveLooseGameplayTag(FDFGameplayTags::UI_ClassSelectionOpen);
	}
}

void UDFClassSelectionSubsystem::OpenClassSelection()
{
	UWorld* const W = GetWorld();
	if (!W)
	{
		DF_LOG(Warning, "[DF|ClassSelection] OpenClassSelection: World nulo");
		return;
	}
	if (ClassSelectionWidgetInstance)
	{
		// Fluxos distintos (ex.: Play/Nova run -> depois Criar perfil) chamam OpenClassSelection de novo.
		// Antigo comportamento ignorava a segunda chamada e bloqueava WBP_ClassSelection apos uma abertura stale.
		DF_LOG(Log, "[DF|ClassSelection] OpenClassSelection: instancia anterior (%s) — fechando antes de reabrir",
			*ClassSelectionWidgetInstance->GetClass()->GetName());
		CloseClassSelection(false);
	}
	UGameInstance* const GI = W->GetGameInstance();
	if (!GI)
	{
		DF_LOG(Warning, "[DF|ClassSelection] OpenClassSelection: GameInstance nulo");
		return;
	}
	EnsureClassTable();
	EnsureSaveLoaded();
	EnsureWidgetClassResolved();

	if (APlayerController* const PC = UGameplayStatics::GetPlayerController(W, 0))
	{
		ApplyUiTag(PC, true);
		if (ADFNexusHUD* const NH = Cast<ADFNexusHUD>(PC->GetHUD()))
		{
			NH->SetRootWidgetHiddenForClassSelection(true);
			bNexusHudSuppressedForClassSelection = true;
		}
	}

	PreviousGlobalTimeDilation = UGameplayStatics::GetGlobalTimeDilation(W);
	UGameplayStatics::SetGlobalTimeDilation(W, PreviewTimeDilation);

	ActiveWorldPreviewDistance = WorldPreviewDistanceFromCamera;
	ActiveRenderTarget = nullptr;
	if (PreviewDisplayMode == EDFClassPreviewDisplayMode::SceneCaptureUMG)
	{
		if (UTextureRenderTarget2D* const RT = PreviewRenderTarget)
		{
			ActiveRenderTarget = RT;
		}
		else
		{
			ActiveRenderTarget = NewObject<UTextureRenderTarget2D>(this);
			ActiveRenderTarget->InitAutoFormat(RenderTargetWidth, RenderTargetHeight);
		}
	}

	SpawnPreviewPawn();
	if (SelectedClass.IsNone())
	{
		const FName FirstUnlocked = FindFirstUnlockedClassName();
		if (!FirstUnlocked.IsNone())
		{
			SelectedClass = FirstUnlocked;
		}
		else
		{
			DF_LOG(Warning,
				"[DF|ClassSelection] OpenClassSelection: nenhuma classe desbloqueada na DT (%s); preview fica malha por defeito do BP.",
				ClassTable ? *ClassTable->GetName() : TEXT("(sem tabela)"));
		}
	}
	W->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(
		this, &UDFClassSelectionSubsystem::ReapplySelectedClassPreviewAfterActorInit));

	if (APlayerController* const PCShowcase = UGameplayStatics::GetPlayerController(W, 0))
	{
		ApplyShowcaseCameraIfNeeded(PCShowcase);
	}

	if (PreviewDisplayMode != EDFClassPreviewDisplayMode::SceneCaptureUMG)
	{
		if (APlayerController* const PC = UGameplayStatics::GetPlayerController(W, 0))
		{
			if (ADFMainMenuHUD* const HUD = Cast<ADFMainMenuHUD>(PC->GetHUD()))
			{
				bMainMenuLayersSuppressedForWorldPreview =
					HUD->SuppressUnderlyingMenuForClassSelectionWorldPreview(true);
			}
		}
	}

	if (APlayerController* const PC = UGameplayStatics::GetPlayerController(W, 0))
	{
		const bool bUsingFallback = ClassSelectionWidgetClass == nullptr;
		const TSubclassOf<UUserWidget> WgtClass = ClassSelectionWidgetClass
			 ? ClassSelectionWidgetClass
			 : TSubclassOf<UUserWidget>(UDFClassSelectionWidget::StaticClass());
		if (bUsingFallback)
		{
			DF_LOG(Error,
				"[DF|ClassSelection] OpenClassSelection: ClassSelectionWidgetClass NAO atribuido (e SoftPath '%s' nao resolveu). "
				"Usando UDFClassSelectionWidget puro (sem layout). Configure WBP_ClassSelection em DefaultGame.ini "
				"sob [/Script/DungeonForged.DFClassSelectionSubsystem] ou no Content Browser (parent C++: UDFClassSelectionWidget).",
				*ClassSelectionWidgetSoftPath.ToString());
		}
		if (WgtClass)
		{
			ClassSelectionWidgetInstance = CreateWidget<UUserWidget>(PC, WgtClass);
		}
		if (!ClassSelectionWidgetInstance)
		{
			DF_LOG(Error, "[DF|ClassSelection] OpenClassSelection: CreateWidget falhou (classe=%s)",
				WgtClass ? *WgtClass->GetName() : TEXT("null"));
		}
		else
		{
			ClassSelectionWidgetInstance->AddToViewport(DFMainMenuUI::ViewportZ_ClassSelection);
			DF_LOG(Log, "[DF|ClassSelection] OpenClassSelection: OK AddToViewport Z=%d WBP=%s MainMenuDest=%u PreviewPawn=%s fallbackCpp=%s",
				DFMainMenuUI::ViewportZ_ClassSelection,
				*ClassSelectionWidgetInstance->GetClass()->GetName(),
				static_cast<uint32>(MainMenuClassDestination),
				SpawnedPreviewPawn ? TEXT("sim") : TEXT("nao"),
				bUsingFallback ? TEXT("sim") : TEXT("nao"));
		}
		FInputModeUIOnly In;
		if (ClassSelectionWidgetInstance)
		{
			DFPrepareWidgetForUIModeFocus(ClassSelectionWidgetInstance);
			In.SetWidgetToFocus(ClassSelectionWidgetInstance->TakeWidget());
		}
		PC->SetInputMode(In);
		PC->SetShowMouseCursor(true);
	}
	else
	{
		DF_LOG(Warning, "[DF|ClassSelection] OpenClassSelection: PlayerController indice 0 nulo");
	}
}

void UDFClassSelectionSubsystem::CloseClassSelection(const bool bConfirm)
{
	DF_LOG(Log, "[DF|ClassSelection] CloseClassSelection: bConfirm=%s SelectedClass=%s MainMenuDest=%u",
		bConfirm ? TEXT("sim") : TEXT("nao"),
		SelectedClass.IsNone() ? TEXT("(nenhuma)") : *SelectedClass.ToString(),
		static_cast<uint32>(MainMenuClassDestination));
	UWorld* const W = GetWorld();

	RestoreShowcaseCameraIfNeeded();

	const bool bTravelingFromMainMenuClassPick =
		bConfirm
		&& !SelectedClass.IsNone()
		&& MainMenuClassDestination != EDFMainMenuClassPickDestination::None;
	if (bMainMenuLayersSuppressedForWorldPreview && !bTravelingFromMainMenuClassPick)
	{
		if (W)
		{
			if (APlayerController* const PC = UGameplayStatics::GetPlayerController(W, 0))
			{
				if (ADFMainMenuHUD* const HUD = Cast<ADFMainMenuHUD>(PC->GetHUD()))
				{
					HUD->SuppressUnderlyingMenuForClassSelectionWorldPreview(false);
					HUD->RestoreFocusAfterClassSelectionWorldPreview();
				}
			}
		}
	}
	bMainMenuLayersSuppressedForWorldPreview = false;

	if (bNexusHudSuppressedForClassSelection && W)
	{
		if (APlayerController* const PC = UGameplayStatics::GetPlayerController(W, 0))
		{
			if (ADFNexusHUD* const NH = Cast<ADFNexusHUD>(PC->GetHUD()))
			{
				NH->SetRootWidgetHiddenForClassSelection(false);
			}
		}
		bNexusHudSuppressedForClassSelection = false;
	}
	if (W)
	{
		UGameplayStatics::SetGlobalTimeDilation(W, PreviousGlobalTimeDilation > 0.f ? PreviousGlobalTimeDilation : 1.f);
	}
	if (APlayerController* const PC = W ? UGameplayStatics::GetPlayerController(W, 0) : nullptr)
	{
		ApplyUiTag(PC, false);
	}
	if (bConfirm && SelectedClass.IsNone() == false)
	{
		const FName ConfirmedClass = SelectedClass;
		UGameInstance* const GI = W ? W->GetGameInstance() : nullptr;
		if (MainMenuClassDestination != EDFMainMenuClassPickDestination::None)
		{
			if (UDFSaveSlotManagerSubsystem* const Slots = GI ? GI->GetSubsystem<UDFSaveSlotManagerSubsystem>() : nullptr)
			{
				if (UDFSaveGame* const Save = Slots->GetActiveSave())
				{
					Save->LastRunClass = ConfirmedClass;
					Save->bHasActiveRun = true;
					Save->bIsFirstLaunch = false;
					(void)Slots->SaveActiveSlot();
				}
			}
			if (UDFWorldTransitionSubsystem* const WT = GI ? GI->GetSubsystem<UDFWorldTransitionSubsystem>() : nullptr)
			{
				if (MainMenuClassDestination == EDFMainMenuClassPickDestination::NexusFirstLaunch)
				{
					WT->TravelToNexus(ETravelReason::FirstLaunch);
				}
				else if (MainMenuClassDestination == EDFMainMenuClassPickDestination::RunDungeon)
				{
					WT->TravelToRun(ConfirmedClass);
				}
			}
			MainMenuClassDestination = EDFMainMenuClassPickDestination::None;
		}
		else if (APlayerController* const PC = W ? UGameplayStatics::GetPlayerController(W, 0) : nullptr)
		{
			if (ADFNexusPlayerController* const N = Cast<ADFNexusPlayerController>(PC))
			{
				N->Server_BeginRunWithClass(ConfirmedClass);
			}
		}
	}
	if (ClassSelectionWidgetInstance)
	{
		ClassSelectionWidgetInstance->RemoveFromParent();
		ClassSelectionWidgetInstance = nullptr;
	}
	DestroyPreviewPawn();
	SelectedClass = NAME_None;
	if (APlayerController* const PC = W ? UGameplayStatics::GetPlayerController(W, 0) : nullptr)
	{
		if (PC->GetPawn<ACharacter>())
		{
			// Return to action controls after UI (GameAndUI + cursor was fighting camera look).
			PC->SetShowMouseCursor(false);
			PC->SetInputMode(FInputModeGameOnly());
		}
	}
}

void UDFClassSelectionSubsystem::SpawnPreviewPawn()
{
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	DestroyPreviewPawn();
	bPreviewSpawnUsedTaggedAnchor = false;
	if (!PreviewPawnClass)
	{
		DF_LOG(Warning, "[DF|ClassSelection] SpawnPreviewPawn: PreviewPawnClass nulo (configure no subsistema / Class Defaults)");
		return;
	}
	FVector SpawnLoc = PreviewSpawnLocation;
	FRotator SpawnRot = PreviewSpawnRotation;
	{
		TArray<AActor*> Tagged;
		UGameplayStatics::GetAllActorsWithTag(W, FName("ClassSelectionPreview"), Tagged);
		if (Tagged.Num() > 0 && Tagged[0])
		{
			SpawnLoc = Tagged[0]->GetActorLocation();
			SpawnRot = Tagged[0]->GetActorRotation();
			bPreviewSpawnUsedTaggedAnchor = true;
		}
	}
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnedPreviewPawn = W->SpawnActor<ACharacter>(PreviewPawnClass, SpawnLoc, SpawnRot, P);
	if (!SpawnedPreviewPawn)
	{
		DF_LOG(Error, "[DF|ClassSelection] SpawnPreviewPawn: SpawnActor falhou (classe=%s)",
			*PreviewPawnClass->GetName());
		return;
	}
	DF_LOG(Log, "[DF|ClassSelection] SpawnPreviewPawn: OK %s em %s modo=%s tagAnchor=%s",
		*SpawnedPreviewPawn->GetName(),
		*SpawnLoc.ToString(),
		DfPreviewDisplayModeLabel(PreviewDisplayMode),
		bPreviewSpawnUsedTaggedAnchor ? TEXT("sim") : TEXT("nao"));
	SpawnedPreviewPawn->SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnedPreviewPawn->SetActorEnableCollision(false);
	SpawnedPreviewPawn->DisableInput(nullptr);

	if (bPreviewUseFillLight)
	{
		if (UPointLightComponent* const Fill = NewObject<UPointLightComponent>(SpawnedPreviewPawn, TEXT("ClassPreviewFill")))
		{
			Fill->SetupAttachment(SpawnedPreviewPawn->GetRootComponent());
			Fill->SetRelativeLocation(FVector(140.f, -120.f, 160.f));
			Fill->SetIntensityUnits(ELightUnits::Lumens);
			Fill->SetIntensity(PreviewFillLightLumens);
			Fill->SetLightColor(FLinearColor(1.f, 0.98f, 0.92f));
			Fill->SetAttenuationRadius(2800.f);
			Fill->SetCastShadows(false);
			Fill->RegisterComponent();
		}
	}

	if (PreviewDisplayMode == EDFClassPreviewDisplayMode::SceneCaptureUMG)
	{
		PreviewSpringArm = NewObject<USpringArmComponent>(SpawnedPreviewPawn, TEXT("ClassPreviewArm"));
		PreviewSpringArm->bUsePawnControlRotation = false;
		PreviewSpringArm->bDoCollisionTest = false;
		PreviewSpringArm->bEnableCameraLag = false;
		PreviewSpringArm->TargetArmLength = 400.f;
		PreviewSpringArm->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
		PreviewSpringArm->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
		PreviewSpringArm->SetupAttachment(SpawnedPreviewPawn->GetRootComponent());
		PreviewSpringArm->RegisterComponent();

		PreviewSceneCapture = NewObject<USceneCaptureComponent2D>(SpawnedPreviewPawn, TEXT("ClassPreviewSceneCapture"));
		PreviewSceneCapture->bCaptureEveryFrame = true;
		PreviewSceneCapture->CaptureSource = SCS_FinalColorLDR;
		PreviewSceneCapture->SetRelativeRotation(FRotator::ZeroRotator);
		PreviewSceneCapture->SetupAttachment(PreviewSpringArm, USpringArmComponent::SocketName);
		PreviewSceneCapture->FOVAngle = 38.f;
		PreviewSceneCapture->RegisterComponent();

		if (ActiveRenderTarget)
		{
			ActiveRenderTarget->InitAutoFormat(RenderTargetWidth, RenderTargetHeight);
			PreviewSceneCapture->TextureTarget = ActiveRenderTarget;
		}
	}
	else
	{
		PreviewSpringArm = nullptr;
		PreviewSceneCapture = nullptr;
		const bool bFollowPlayerPlacement =
			PreviewDisplayMode == EDFClassPreviewDisplayMode::WorldWithPlayerCamera
			&& !bPreviewSpawnUsedTaggedAnchor;
		if (bFollowPlayerPlacement)
		{
			if (APlayerController* const PC = UGameplayStatics::GetPlayerController(W, 0))
			{
				PositionPreviewForDirectWorldView(PC);
			}
		}
	}

	Rotator = NewObject<UDFClassPreviewRotatorComponent>(SpawnedPreviewPawn, TEXT("ClassPreviewRotator"));
	Rotator->SetSpringArm(PreviewSpringArm);
	Rotator->RegisterComponent();
	Rotator->SetComponentTickEnabled(true);
	if (Rotator)
	{
		Rotator->SyncYawFromOwner();
	}
}

void UDFClassSelectionSubsystem::PositionPreviewForDirectWorldView(APlayerController* const PC)
{
	if (!SpawnedPreviewPawn || !PC || PreviewDisplayMode != EDFClassPreviewDisplayMode::WorldWithPlayerCamera)
	{
		return;
	}
	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);
	const FVector Forward = CamRot.Vector();
	const FVector TargetLoc = CamLoc + Forward * ActiveWorldPreviewDistance;
	SpawnedPreviewPawn->SetActorLocation(TargetLoc);
	const FVector ToCam = (CamLoc - TargetLoc).GetSafeNormal();
	const float Yaw = ToCam.Rotation().Yaw;
	SpawnedPreviewPawn->SetActorRotation(FRotator(0.f, Yaw, 0.f));
}

FName UDFClassSelectionSubsystem::FindFirstUnlockedClassName() const
{
	const UDataTable* const DT = GetClassTable();
	if (!DT)
	{
		return NAME_None;
	}
	TArray<FName> RowNames;
	DT->GetRowMap().GetKeys(RowNames);
	RowNames.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
	for (const FName& N : RowNames)
	{
		if (IsClassUnlocked(N))
		{
			return N;
		}
	}
	return NAME_None;
}

void UDFClassSelectionSubsystem::ReapplySelectedClassPreviewAfterActorInit()
{
	if (!SpawnedPreviewPawn || SelectedClass.IsNone())
	{
		return;
	}
	UpdatePreviewForClass(SelectedClass);
}

void UDFClassSelectionSubsystem::DestroyPreviewPawn()
{
	Rotator = nullptr;
	PreviewSpringArm = nullptr;
	PreviewSceneCapture = nullptr;
	if (SpawnedPreviewPawn)
	{
		SpawnedPreviewPawn->Destroy();
		SpawnedPreviewPawn = nullptr;
	}
}

void UDFClassSelectionSubsystem::UpdatePreviewForClass(const FName ClassName)
{
	SelectedClass = ClassName;
	const FDFClassTableRow* const Row = GetClassData(ClassName);
	if (!Row || !SpawnedPreviewPawn)
	{
		return;
	}
	if (!Row->CharacterMesh)
	{
		DF_LOG(Warning,
			"[DF|ClassSelection] UpdatePreviewForClass: linha '%s' sem CharacterMesh — DT_Class / FDFClassTableRow.CharacterMesh deve apontar "
			"para malha compatível com o skeleton de PreviewPawnClass.",
			*ClassName.ToString());
	}
	if (USkeletalMeshComponent* const Mesh = SpawnedPreviewPawn->GetMesh())
	{
		if (Row->CharacterMesh)
		{
			Mesh->SetSkeletalMesh(Row->CharacterMesh);
		}
		ApplyClassTintToPreview(Row->PreviewCosmeticTint);
		if (ADFPlayerCharacter* const Hero = Cast<ADFPlayerCharacter>(SpawnedPreviewPawn))
		{
			Hero->RefreshWeaponAndOffHandSocketAttachments();
		}
	}
	PlayClassIdleAnimation(*Row);
	SpawnClassChangeVfx(*Row);
}

void UDFClassSelectionSubsystem::ApplyClassTintToPreview(const FLinearColor& Tint) const
{
	if (!SpawnedPreviewPawn)
	{
		return;
	}
	if (USkeletalMeshComponent* const Mesh = SpawnedPreviewPawn->GetMesh())
	{
		UMaterialInterface* const M0 = Mesh->GetMaterial(0);
		UMaterialInstanceDynamic* const MID = Mesh->CreateDynamicMaterialInstance(0, M0);
		if (MID)
		{
			MID->SetVectorParameterValue(FName("TintColor"), FVector(Tint));
		}
	}
}

void UDFClassSelectionSubsystem::PlayClassIdleAnimation(const FDFClassTableRow& Row) const
{
	if (!SpawnedPreviewPawn)
	{
		return;
	}
	UAnimSequence* Idle = Row.ClassPreviewIdle.IsNull() ? nullptr : Row.ClassPreviewIdle.LoadSynchronous();
	if (USkeletalMeshComponent* const Mesh = SpawnedPreviewPawn->GetMesh())
	{
		if (UAnimInstance* const AI = Mesh->GetAnimInstance())
		{
			AI->StopAllMontages(0.15f);
			if (Idle)
			{
				AI->PlaySlotAnimationAsDynamicMontage(Idle, FName("DefaultSlot"), 0.2f, 0.2f, 1.f, 0);
			}
		}
	}
}

void UDFClassSelectionSubsystem::SpawnClassChangeVfx(const FDFClassTableRow& Row) const
{
	if (!SpawnedPreviewPawn)
	{
		return;
	}
	UNiagaraSystem* const NS = Row.ClassChangeVFX.IsNull() ? nullptr : Row.ClassChangeVFX.LoadSynchronous();
	if (NS)
	{
		const FVector L = SpawnedPreviewPawn->GetMesh()
			 ? SpawnedPreviewPawn->GetMesh()->GetComponentLocation() + FVector(0.f, 0.f, 40.f)
			 : SpawnedPreviewPawn->GetActorLocation();
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			SpawnedPreviewPawn->GetWorld(), NS, L, FRotator::ZeroRotator, FVector(1.f), true, true, ENCPoolMethod::None, true);
	}
}

void UDFClassSelectionSubsystem::RotatePreview(const float YawDelta)
{
	if (Rotator)
	{
		Rotator->AddYawInput(YawDelta);
	}
}

void UDFClassSelectionSubsystem::ZoomPreview(const float Delta)
{
	if (PreviewDisplayMode == EDFClassPreviewDisplayMode::WorldShowcaseCamera)
	{
		return;
	}
	if (PreviewDisplayMode == EDFClassPreviewDisplayMode::SceneCaptureUMG)
	{
		if (Rotator)
		{
			Rotator->AddZoomInput(Delta);
		}
		return;
	}
	UWorld* const W = GetWorld();
	if (!W || !SpawnedPreviewPawn)
	{
		return;
	}
	if (bPreviewSpawnUsedTaggedAnchor)
	{
		return;
	}
	APlayerController* const PC = UGameplayStatics::GetPlayerController(W, 0);
	if (!PC)
	{
		return;
	}
	ActiveWorldPreviewDistance = FMath::Clamp(ActiveWorldPreviewDistance + Delta * 40.f, 120.f, 900.f);
	PositionPreviewForDirectWorldView(PC);
	if (Rotator)
	{
		Rotator->SyncYawFromOwner();
	}
}

static bool DfNameIsAny(
	const FName N,
	const TCHAR* A1,
	const TCHAR* A2 = nullptr,
	const TCHAR* A3 = nullptr,
	const TCHAR* A4 = nullptr)
{
	if (N == A1) return true;
	if (A2 && N == A2) return true;
	if (A3 && N == A3) return true;
	if (A4 && N == A4) return true;
	return false;
}

static bool DfIsUnlockedByRules(const FName ClassName, const UDFSaveGame* const Save)
{
	if (!ClassName.IsNone() && Save)
	{
		if (Save->UnlockedClasses.Contains(ClassName))
		{
			return true;
		}
	}
	// Guerreiro / Mago: always
	if (DfNameIsAny(ClassName, TEXT("Guerreiro"), TEXT("Warrior"), TEXT("Mago"), TEXT("Mage")))
	{
		return true;
	}
	if (!Save)
	{
		return DfNameIsAny(ClassName, TEXT("Guerreiro"), TEXT("Mago"), TEXT("Warrior"), TEXT("Mage"));
	}
	// Assassino: 2+ runs
	if (DfNameIsAny(ClassName, TEXT("Assassino"), TEXT("Rogue"), TEXT("Assassin")))
	{
		return Save->TotalRuns >= 2;
	}
	// Paladino: 1+ win
	if (DfNameIsAny(ClassName, TEXT("Paladino"), TEXT("Paladin")))
	{
		return Save->TotalWins >= 1;
	}
	// Necromante: Meta >= 5
	if (DfNameIsAny(ClassName, TEXT("Necromante"), TEXT("Necromancer")))
	{
		return Save->MetaLevel >= 5;
	}
	return false;
}

static FText DfGetUnlockText(const FName ClassName, const UDFSaveGame* const Save)
{
	if (DfIsUnlockedByRules(ClassName, Save))
	{
		return FText::GetEmpty();
	}
	if (DfNameIsAny(ClassName, TEXT("Assassino"), TEXT("Rogue"), TEXT("Assassin")))
	{
		return NSLOCTEXT("DF", "UnlockTwoRuns", "Conclua 2 runs (qualquer desfecho).");
	}
	if (DfNameIsAny(ClassName, TEXT("Paladino"), TEXT("Paladin")))
	{
		return NSLOCTEXT("DF", "UnlockOneWin", "Vença 1 run (qualquer classe).");
	}
	if (DfNameIsAny(ClassName, TEXT("Necromante"), TEXT("Necromancer")))
	{
		return NSLOCTEXT("DF", "UnlockMeta5", "Atinga Meta n\u00edvel 5 no Nexus.");
	}
	if (Save && Save->UnlockedClasses.Contains(ClassName) == false)
	{
		return NSLOCTEXT("DF", "UnlockGeneric", "Cumpra o requisito de desbloqueio no Nexus.");
	}
	return FText::GetEmpty();
}
