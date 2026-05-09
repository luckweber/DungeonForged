// Source/DungeonForged/Private/GameModes/Nexus/UDFNexusClassDetailPanelWidget.cpp
#include "GameModes/Nexus/UDFNexusClassDetailPanelWidget.h"
#include "UI/ClassSelection/UDFClassSelectionSubsystem.h"
#include "Data/DFDataTableStructs.h"
#include "DungeonForgedModule.h"
#include "Engine/DataTable.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

namespace
{

static void SetTB(UTextBlock* const Tb, const FText& In)
{
	if (Tb)
	{
		Tb->SetText(In);
	}
}

static void SetBar(UProgressBar* const Bar, const float P)
{
	if (Bar)
	{
		Bar->SetPercent(FMath::Clamp(P, 0.f, 1.f));
	}
}

} // namespace

UTextBlock* UDFNexusClassDetailPanelWidget::ResolveClassTitleTextBlock() const
{
	if (DetailClassName)
	{
		return DetailClassName;
	}
	if (UWidget* const W = GetWidgetFromName(FName(TEXT("DetailClassName"))))
	{
		return Cast<UTextBlock>(W);
	}
	if (UWidget* const W = GetWidgetFromName(FName(TEXT("ClassName"))))
	{
		return Cast<UTextBlock>(W);
	}
	static const TCHAR* ExtraTitleNames[] = {
		TEXT("TXT_ClassName"),
		TEXT("Text_ClassName"),
		TEXT("HeroName"),
		TEXT("ClassTitle"),
		TEXT("DetailClassTitle"),
	};
	for (const TCHAR* N : ExtraTitleNames)
	{
		if (UWidget* const W = GetWidgetFromName(FName(N)))
		{
			if (UTextBlock* const TB = Cast<UTextBlock>(W))
			{
				return TB;
			}
		}
	}
	return nullptr;
}

void UDFNexusClassDetailPanelWidget::ClearOptionalWidgets()
{
	SetTB(ResolveClassTitleTextBlock(), FText::GetEmpty());
	SetTB(DetailDescription, FText::GetEmpty());
	SetTB(DetailArchetype, FText::GetEmpty());
	SetTB(DetailPlaystyle, FText::GetEmpty());
	SetTB(DetailDifficulty, FText::GetEmpty());
	SetBar(BarStrength, 0.f);
	SetBar(BarInt, 0.f);
	SetBar(BarAgi, 0.f);
	SetBar(BarDefense, 0.f);
}

void UDFNexusClassDetailPanelWidget::RefreshForClass(const FName ClassRow)
{
	ClearOptionalWidgets();

	if (ClassRow.IsNone())
	{
		return;
	}

	UWorld* const W = GetWorld();
	UDFClassSelectionSubsystem* const Sub = W ? W->GetSubsystem<UDFClassSelectionSubsystem>() : nullptr;
	UDataTable* const DT = Sub ? Sub->GetClassTable() : nullptr;

	if (!Sub || !DT)
	{
		return;
	}

	const FDFClassTableRow* const Row =
		DT->FindRow<FDFClassTableRow>(ClassRow, TEXT("UDFNexusClassDetailPanelWidget::RefreshForClass"));

	if (!Row)
	{
		return;
	}

	if (UTextBlock* const Title = ResolveClassTitleTextBlock())
	{
		Title->SetText(Row->ClassName);
	}
	else
	{
		DF_LOG(Warning,
			"[DF|Nexus|ClassDetailPanel] TextBlock do titulo nao encontrado — no WBP do painel, renomeie o TextBlock dinamico para DetailClassName (BindWidget opcional "
			"com o mesmo nome) ou ClassName / TXT_ClassName / HeroName; is_variable ligado ajuda.");
	}
	SetTB(DetailDescription, Row->ClassDescription);
	SetTB(DetailArchetype, Row->ClassArchetype);
	SetTB(DetailPlaystyle, Row->PlaystyleTag);
	SetTB(
		DetailDifficulty,
		FText::Format(
			NSLOCTEXT("DFNexus", "DifficultyPips", "Dificuldade: {0} / 5"),
			FText::AsNumber(Row->DifficultyPips)));

	float OutStr = 0.f, OutIntel = 0.f, OutAgi = 0.f, OutDef = 0.f;
	float OutHp = 0.f;
	Sub->GetStatBarScalesForClass(ClassRow, OutStr, OutIntel, OutAgi, OutDef, OutHp);

	SetBar(BarStrength, OutStr);
	SetBar(BarInt, OutIntel);
	SetBar(BarAgi, OutAgi);
	SetBar(BarDefense, OutDef);
}
