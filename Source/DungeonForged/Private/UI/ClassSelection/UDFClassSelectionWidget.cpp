// Source/DungeonForged/Private/UI/ClassSelection/UDFClassSelectionWidget.cpp
#include "UI/ClassSelection/UDFClassSelectionWidget.h"
#include "UI/ClassSelection/UDFClassSelectionSubsystem.h"
#include "UI/ClassSelection/UDFClassListEntryWidget.h"
#include "UI/ClassSelection/UDFClassStatBarWidget.h"
#include "UI/ClassSelection/UDFAbilityPreviewIconWidget.h"
#include "UI/ClassSelection/UDFClassAbilityTooltipWidget.h"
#include "UI/ClassSelection/UDFActiveChallengeIndicatorWidget.h"
#include "UI/ClassSelection/UDFPartnerClassPreviewWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/WrapBox.h"
#include "Components/PanelWidget.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameModes/Nexus/ADFNexusGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Run/DFRunManager.h"
#include "Run/DFSaveGame.h"
#include "GAS/UDFAttributeSet.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameModes/Nexus/DFNexusTypes.h"
#include "Input/Events.h"
#include "Layout/Geometry.h"
#include "Engine/DataTable.h"
#include "Data/DFDataTableStructs.h"

static int32 DfFindRunHistoryForClass(const UDFSaveGame* Save, const FName ClassName, int32& OutBestFloor, int32& OutWins)
{
	OutBestFloor = 0;
	OutWins = 0;
	int32 Runs = 0;
	if (!Save)
	{
		return 0;
	}
	for (const FDFRunHistoryEntry& E : Save->RunHistory)
	{
		if (E.ClassName != ClassName)
		{
			continue;
		}
		++Runs;
		OutBestFloor = FMath::Max(OutBestFloor, E.FloorReached);
		if (E.Outcome == EDFRunRecordOutcome::Victory)
		{
			++OutWins;
		}
	}
	return Runs;
}

void UDFClassSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (BackButton) BackButton->OnClicked.AddDynamic(this, &UDFClassSelectionWidget::OnBackClicked);
	if (ConfirmButton) ConfirmButton->OnClicked.AddDynamic(this, &UDFClassSelectionWidget::OnConfirmClicked);
	if (GetWorld())
	{
		if (UDFClassSelectionSubsystem* const Sub = GetWorld()->GetSubsystem<UDFClassSelectionSubsystem>())
		{
			if (UTextureRenderTarget2D* const RT = Sub->GetRenderTarget())
			{
				if (PreviewRenderTarget) PreviewRenderTarget->SetBrushResourceObject(RT);
			}
		}
	}
	if (AbilityTooltip == nullptr)
	{
		AbilityTooltip = CreateWidget<UDFClassAbilityTooltipWidget>(this, UDFClassAbilityTooltipWidget::StaticClass());
		if (AbilityTooltip)
		{
			AbilityTooltip->AddToViewport(20);
			AbilityTooltip->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	BuildClassList();
	RefreshAll();
}

void UDFClassSelectionWidget::NativeDestruct()
{
	if (AbilityTooltip)
	{
		AbilityTooltip->RemoveFromParent();
		AbilityTooltip = nullptr;
	}
	Super::NativeDestruct();
}

void UDFClassSelectionWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateCoopUi();
}

void UDFClassSelectionWidget::SelectClassByRow(const FName ClassRow)
{
	if (UWorld* const W = GetWorld())
	{
		if (UDFClassSelectionSubsystem* const Sub = W->GetSubsystem<UDFClassSelectionSubsystem>())
		{
			Sub->SetSelectedClass(ClassRow);
			Sub->UpdatePreviewForClass(ClassRow);
		}
	}
	RefreshAll();
}

void UDFClassSelectionWidget::RefreshAll()
{
	if (UWorld* const W = GetWorld())
	{
		if (UDFClassSelectionSubsystem* const Sub = W->GetSubsystem<UDFClassSelectionSubsystem>())
		{
			if (UTextureRenderTarget2D* const RT = Sub->GetRenderTarget())
			{
				if (PreviewRenderTarget) PreviewRenderTarget->SetBrushResourceObject(RT);
			}
		}
	}
	BuildClassList();
	FillDetailsPanel();
	UpdateConfirmButton();
}

void UDFClassSelectionWidget::ClearChildPanel(UPanelWidget* const Box)
{
	if (!Box)
	{
		return;
	}
	Box->ClearChildren();
}

void UDFClassSelectionWidget::BuildClassList()
{
	if (!ClassListScroll || !ClassEntryWidgetClass)
	{
		return;
	}
	ClassListScroll->ClearChildren();
	UWorld* const W = GetWorld();
	UDFClassSelectionSubsystem* const Sub = W ? W->GetSubsystem<UDFClassSelectionSubsystem>() : nullptr;
	UDataTable* const DT = Sub ? Sub->GetClassTable() : nullptr;
	if (!Sub || !DT)
	{
		return;
	}
	TArray<FName> RowNames;
	DT->GetRowMap().GetKeys(RowNames);
	RowNames.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
	const FName Selected = Sub->GetSelectedClass();
	for (const FName& N : RowNames)
	{
		if (UDFClassListEntryWidget* const Entry = CreateWidget<UDFClassListEntryWidget>(this, ClassEntryWidgetClass))
		{
			const bool bUn = Sub->IsClassUnlocked(N);
			Entry->InitializeEntry(N, this, bUn, N == Selected);
			ClassListScroll->AddChild(Entry);
		}
	}
}

void UDFClassSelectionWidget::FillDetailsPanel()
{
	UWorld* const W = GetWorld();
	UDFClassSelectionSubsystem* const Sub = W ? W->GetSubsystem<UDFClassSelectionSubsystem>() : nullptr;
	UGameInstance* const GI = W ? W->GetGameInstance() : nullptr;
	UDFRunManager* const RM = GI ? GI->GetSubsystem<UDFRunManager>() : nullptr;
	if (!Sub)
	{
		return;
	}
	const FName Sel = Sub->GetSelectedClass();
	const FDFClassTableRow* Row = Sub->GetClassData(Sel);
	if (ClassTitle) ClassTitle->SetText(Row ? Row->ClassName : FText::GetEmpty());
	if (ClassDescription) ClassDescription->SetText(Row ? Row->ClassDescription : FText::GetEmpty());
	if (PlaystyleTag) PlaystyleTag->SetText(Row ? Row->PlaystyleTag : FText::GetEmpty());
	if (SelectedClassName) SelectedClassName->SetText(Row ? Row->ClassName : FText::GetEmpty());
	if (SelectedClassName && Row) SelectedClassName->SetColorAndOpacity(Row->ClassColor);
	if (ClassTitle && Row) ClassTitle->SetColorAndOpacity(FSlateColor(Row->ClassColor));
	if (PreviewBorderImage && Row)
	{
		PreviewBorderImage->SetColorAndOpacity(Row->ClassColor);
	}
	if (StatBarsBox && StatBarWidgetClass)
	{
		ClearChildPanel(StatBarsBox);
		if (Row)
		{
			float Ss, Is, As, Ds, Hs;
			Sub->GetStatBarScalesForClass(Sel, Ss, Is, As, Ds, Hs);
			const FGameplayAttribute SAt = UDFAttributeSet::GetStrengthAttribute();
			const FGameplayAttribute IAt = UDFAttributeSet::GetIntelligenceAttribute();
			const FGameplayAttribute AAt = UDFAttributeSet::GetAgilityAttribute();
			const FGameplayAttribute ArAt = UDFAttributeSet::GetArmorAttribute();
			const FGameplayAttribute MrAt = UDFAttributeSet::GetMagicResistAttribute();
			const FGameplayAttribute MhAt = UDFAttributeSet::GetMaxHealthAttribute();
			const int32 Vs = FMath::RoundToInt(Row->BaseAttributeValues.FindRef(SAt));
			const int32 Vi = FMath::RoundToInt(Row->BaseAttributeValues.FindRef(IAt));
			const int32 Va = FMath::RoundToInt(Row->BaseAttributeValues.FindRef(AAt));
			const float* const ArP = Row->BaseAttributeValues.Find(ArAt);
			const float* const MrP = Row->BaseAttributeValues.Find(MrAt);
			const float ArV = ArP ? *ArP : 0.f;
			const float MrV = MrP ? *MrP : 0.f;
			const int32 Vd = FMath::RoundToInt(0.5f * ArV + 0.5f * MrV);
			const int32 Vh = FMath::RoundToInt(Row->BaseAttributeValues.FindRef(MhAt));
			auto AddBar = [&](const FText& Label, float F, int32 V, const FLinearColor C) {
				if (UDFClassStatBarWidget* const B = CreateWidget<UDFClassStatBarWidget>(this, StatBarWidgetClass))
				{
					B->SetData(Label, F, V, C);
					StatBarsBox->AddChild(B);
				}
			};
			AddBar(NSLOCTEXT("DF", "AttrStr", "For\u00e7a"), Ss, Vs, FLinearColor(0.85f, 0.2f, 0.15f));
			AddBar(NSLOCTEXT("DF", "AttrInt", "Intelig\u00eancia"), Is, Vi, FLinearColor(0.35f, 0.45f, 1.f));
			AddBar(NSLOCTEXT("DF", "AttrAgi", "Agilidade"), As, Va, FLinearColor(0.3f, 0.85f, 0.35f));
			AddBar(NSLOCTEXT("DF", "AttrDef", "Defesa"), Ds, Vd, FLinearColor(0.6f, 0.6f, 0.65f));
			AddBar(NSLOCTEXT("DF", "AttrHp", "Vida M\u00e1xima"), Hs, Vh, FLinearColor(0.9f, 0.25f, 0.25f));
		}
	}
	if (StartingAbilitiesScroll && AbilityIconWidgetClass && RM && RM->AbilityDataTable)
	{
		ClearChildPanel(StartingAbilitiesScroll);
		if (Row)
		{
			for (const FName& AName : Row->StartingAbilities)
			{
				if (const FDFAbilityTableRow* AR = RM->AbilityDataTable->FindRow<FDFAbilityTableRow>(AName, TEXT("AbilityPreview")))
				{
					if (UDFAbilityPreviewIconWidget* const Ic = CreateWidget<UDFAbilityPreviewIconWidget>(this, AbilityIconWidgetClass))
					{
						Ic->SetAbilityRow(*AR, AName, AbilityTooltip);
						StartingAbilitiesScroll->AddChild(Ic);
					}
				}
			}
		}
	}
	UDFSaveGame* const Save = Sub->SaveRef;
	int32 BestFloor = 0, Wins = 0;
	const int32 Runs = DfFindRunHistoryForClass(Save, Sel, BestFloor, Wins);
	if (RunCountText)
	{
		RunCountText->SetText(Runs > 0 ? FText::Format(NSLOCTEXT("DF", "RunCountFmt", "{0} runs"), FText::AsNumber(Runs))
										: NSLOCTEXT("DF", "NoRunsClass", "Nunca jogada \u2014 seja o primeiro!"));
	}
	if (BestFloorText)
	{
		BestFloorText->SetText(
			Runs > 0 ? FText::Format(NSLOCTEXT("DF", "BestFloorFmt", "Melhor: Andar {0}"), FText::AsNumber(BestFloor))
					 : FText::GetEmpty());
	}
	if (WinCountText)
	{
		WinCountText->SetText(
			Runs > 0 ? FText::Format(NSLOCTEXT("DF", "WinCountFmt", "{0} vit\u00f3rias"), FText::AsNumber(Wins))
					 : FText::GetEmpty());
	}
	if (NoHistoryText)
	{
		NoHistoryText->SetVisibility(Runs > 0 ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (MetaUpgradesWrap)
	{
		MetaUpgradesWrap->ClearChildren();
	}
	if (MetaUpgradesHeader && Save)
	{
		MetaUpgradesHeader->SetText(
			Save->CompletedUpgrades.Num() > 0
				? NSLOCTEXT("DF", "MetaUpHdr", "Melhorias Permanentes Ativas")
				: NSLOCTEXT("DF", "MetaUpNone", "Nenhuma melhoria permanente ainda. Visite o S\u00e1bio no Nexus."));
	}
}

void UDFClassSelectionWidget::UpdateCoopUi()
{
	if (CoopStatusText)
	{
		CoopStatusText->SetVisibility(bIsCoopSession ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bIsCoopSession)
		{
			CoopStatusText->SetText(FText::Format(
				NSLOCTEXT("DF", "CoopStatus", "Parceiro: {0} \u2014 Selecionando..."),
				CoopPartnerDisplayName));
		}
	}
	if (UWorld* const W = GetWorld())
	{
		if (ADFNexusGameState* const GS = W->GetGameState<ADFNexusGameState>())
		{
			if (PartnerClassPreview && GS->CoopPartnerHoveredClass.IsNone() == false)
			{
				if (UDFClassSelectionSubsystem* const Sub = W->GetSubsystem<UDFClassSelectionSubsystem>())
				{
					if (const FDFClassTableRow* R = Sub->GetClassData(GS->CoopPartnerHoveredClass))
					{
						PartnerClassPreview->SetVisibility(ESlateVisibility::Visible);
					}
				}
			}
		}
	}
}

void UDFClassSelectionWidget::UpdateConfirmButton()
{
	UWorld* const W = GetWorld();
	UDFClassSelectionSubsystem* const Sub = W ? W->GetSubsystem<UDFClassSelectionSubsystem>() : nullptr;
	if (!ConfirmButton || !Sub)
	{
		return;
	}
	const FName Sel = Sub->GetSelectedClass();
	const bool bClassOk = Sel.IsNone() == false && Sub->IsClassUnlocked(Sel);
	bool bCoopOk = true;
	if (bIsCoopSession)
	{
		bCoopOk = bLocalPlayerLockedIn && bPartnerLockedIn;
	}
	ConfirmButton->SetIsEnabled(bClassOk && (!bIsCoopSession || bCoopOk));
}

void UDFClassSelectionWidget::OnBackClicked()
{
	if (UWorld* const W = GetWorld())
	{
		if (UDFClassSelectionSubsystem* const Sub = W->GetSubsystem<UDFClassSelectionSubsystem>())
		{
			Sub->CloseClassSelection(false);
		}
	}
}

void UDFClassSelectionWidget::OnConfirmClicked()
{
	UWorld* const W = GetWorld();
	UDFClassSelectionSubsystem* const Sub = W ? W->GetSubsystem<UDFClassSelectionSubsystem>() : nullptr;
	if (!Sub)
	{
		return;
	}
	const FName Sel = Sub->GetSelectedClass();
	if (Sel.IsNone() || !Sub->IsClassUnlocked(Sel))
	{
		return;
	}
	if (bHasActiveChallenge)
	{
	}
	if (bIsCoopSession && !(bLocalPlayerLockedIn && bPartnerLockedIn))
	{
		return;
	}
	Sub->CloseClassSelection(true);
}

FReply UDFClassSelectionWidget::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && PreviewRenderTarget)
	{
		if (PreviewRenderTarget->IsHovered())
		{
			bDraggingPreview = true;
			LastMouseScreen = InMouseEvent.GetScreenSpacePosition();
			return FReply::Handled().CaptureMouse(TakeWidget());
		}
	}
	return Super::NativeOnMouseButtonDown(MyGeometry, InMouseEvent);
}

FReply UDFClassSelectionWidget::NativeOnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bDraggingPreview)
	{
		bDraggingPreview = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(MyGeometry, InMouseEvent);
}

FReply UDFClassSelectionWidget::NativeOnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDraggingPreview)
	{
		const FVector2D Now = InMouseEvent.GetScreenSpacePosition();
		const float Dx = Now.X - LastMouseScreen.X;
		LastMouseScreen = Now;
		if (UWorld* const W = GetWorld())
		{
			if (UDFClassSelectionSubsystem* const Sub = W->GetSubsystem<UDFClassSelectionSubsystem>())
			{
				Sub->RotatePreview(Dx);
			}
		}
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(MyGeometry, InMouseEvent);
}

FReply UDFClassSelectionWidget::NativeOnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& InMouseEvent)
{
	if (PreviewRenderTarget && PreviewRenderTarget->IsHovered())
	{
		if (UWorld* const W = GetWorld())
		{
			if (UDFClassSelectionSubsystem* const Sub = W->GetSubsystem<UDFClassSelectionSubsystem>())
			{
				Sub->ZoomPreview(InMouseEvent.GetWheelDelta());
			}
		}
		return FReply::Handled();
	}
	return Super::NativeOnMouseWheel(MyGeometry, InMouseEvent);
}
