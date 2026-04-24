// Source/DungeonForged/Private/Debug/UDFGASDebugOverlayWidget.cpp

#include "Debug/UDFGASDebugOverlayWidget.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/ADFPlayerState.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"

namespace
{
static FGameplayTagContainer BuildBroadActiveEffectQueryTags()
{
	FGameplayTagContainer C;
	// Add many parent/sibling tags to catch most GEs; misses untagged or exotic effects.
	struct F
	{
		FGameplayTag T;
	};
	const FGameplayTag* const Tags[] = {
		&FDFGameplayTags::Effect_Damage_Physical, &FDFGameplayTags::Effect_Damage_Magic, &FDFGameplayTags::Effect_Damage_True, &FDFGameplayTags::Effect_Critical, &FDFGameplayTags::Effect_DoT_Fire, &FDFGameplayTags::Effect_DoT_Frost, &FDFGameplayTags::Effect_DoT_Poison, &FDFGameplayTags::Effect_DoT_Bleed, &FDFGameplayTags::Effect_Debuff_Slow, &FDFGameplayTags::Effect_Debuff_Weaken, &FDFGameplayTags::Effect_Debuff_ArmorBreak, &FDFGameplayTags::Effect_Buff_Speed, &FDFGameplayTags::Effect_Buff_DamageUp, &FDFGameplayTags::Effect_Buff_Shield, &FDFGameplayTags::Buff_Mage_TimeWarpHaste, &FDFGameplayTags::State_Dead, &FDFGameplayTags::State_Stunned, &FDFGameplayTags::State_Sprinting, &FDFGameplayTags::State_Invulnerable, &FDFGameplayTags::State_Casting, &FDFGameplayTags::State_Dodging, &FDFGameplayTags::State_Berserk, &FDFGameplayTags::State_ManaShieldActive, &FDFGameplayTags::Effect_Buff_LevelStatScaling, &FDFGameplayTags::Ability_Cooldown
	};
	for (const FGameplayTag* P : Tags)
	{
		if (P && P->IsValid())
		{
			C.AddTag(*P);
		}
	}
	{
		const FGameplayTag E = FGameplayTag::RequestGameplayTag(FName("Effect"), false);
		if (E.IsValid()) { C.AddTag(E); }
	}
	{
		const FGameplayTag S = FGameplayTag::RequestGameplayTag(FName("State"), false);
		if (S.IsValid()) { C.AddTag(S); }
	}
	{
		const FGameplayTag A = FGameplayTag::RequestGameplayTag(FName("Ability"), false);
		if (A.IsValid()) { C.AddTag(A); }
	}
	return C;
}
} // namespace

void UDFGASDebugOverlayWidget::NativeConstruct()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* const Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), FName("DFGASDebugRoot"));
		WidgetTree->RootWidget = Root;
		Root->SetRenderOpacity(0.92f);

		UScrollBox* const MainScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), FName("MainScroll"));
		TagsScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), FName("TagsScroll"));
		EffectsScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), FName("EffectsScroll"));
		AttributesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName("AttrBox"));
		AbilitiesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName("AbBox"));
		PerfText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName("Perf"));
		PerfText->SetJustification(ETextJustify::Right);
		PerfText->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 1.f, 0.4f, 1.f)));
		UTextBlock* const Titles = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName("Title"));
		Titles->SetText(FText::FromString(TEXT("=== GAS Debug (0.2s) ===")));
		UTextBlock* const TAttr = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName("AttrHdr"));
		TAttr->SetText(FText::FromString(TEXT("Attributes (red vitals < 20% of max)")));
		UTextBlock* const TEff = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName("EffHdr"));
		TEff->SetText(FText::FromString(TEXT("Active GEs (broad match)")));
		UTextBlock* const TAb = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName("AbHdr"));
		TAb->SetText(FText::FromString(TEXT("Ability slots")));

		UVerticalBox* const V = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName("VCol"));
		V->AddChildToVerticalBox(Titles);
		V->AddChildToVerticalBox(TagsScroll);
		UTextBlock* const TTagHdr = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName("TagHdr"));
		TTagHdr->SetText(FText::FromString(TEXT("Gameplay tags")));
		V->AddChildToVerticalBox(TTagHdr);
		V->AddChildToVerticalBox(TAttr);
		V->AddChildToVerticalBox(AttributesBox);
		V->AddChildToVerticalBox(TEff);
		V->AddChildToVerticalBox(EffectsScroll);
		V->AddChildToVerticalBox(TAb);
		V->AddChildToVerticalBox(AbilitiesBox);
		MainScroll->AddChild(V);

		if (UCanvasPanelSlot* const S0 = Root->AddChildToCanvas(MainScroll))
		{
			S0->SetZOrder(0);
			S0->SetAnchors(FAnchors(0.f, 0.f, 0.7f, 1.f));
			S0->SetAutoSize(false);
			S0->SetAlignment(FVector2D(0.f, 0.f));
			S0->SetOffsets(FMargin(8.f, 8.f, 8.f, 8.f));
		}
		if (UCanvasPanelSlot* const S1 = Root->AddChildToCanvas(PerfText))
		{
			S1->SetZOrder(100);
			S1->SetAutoSize(true);
			S1->SetAlignment(FVector2D(1.f, 0.f));
			S1->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f));
			S1->SetPosition(FVector2D(-8.f, 8.f));
		}
	}
	Super::NativeConstruct();
}

void UDFGASDebugOverlayWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (GetVisibility() == ESlateVisibility::Collapsed)
	{
		return;
	}
	const UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	const float D = W->GetDeltaSeconds() > 1e-4f ? W->GetDeltaSeconds() : 0.016f;
	if (IsValid(PerfText))
	{
		PerfText->SetText(FText::FromString(FString::Printf(
			TEXT("FPS: %.0f  |  %.2f ms\nRHI draw / particles: n/a (stat unit)"), 1.f / D, D * 1000.f)));
	}
	TimeSinceRefresh += InDeltaTime;
	if (TimeSinceRefresh < RefreshInterval)
	{
		return;
	}
	TimeSinceRefresh = 0.f;
	RefreshContent();
}

FLinearColor UDFGASDebugOverlayWidget::ColorForTag(const FGameplayTag& T)
{
	if (!T.IsValid())
	{
		return FLinearColor::White;
	}
	if (FGameplayTag::RequestGameplayTag(FName("State"), false).IsValid() && T.MatchesTag(FGameplayTag::RequestGameplayTag(FName("State"), false)))
	{
		return FLinearColor(1.f, 1.f, 0.15f, 1.f);
	}
	if (FGameplayTag::RequestGameplayTag(FName("Ability"), false).IsValid() && T.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Ability"), false)))
	{
		return FLinearColor(0.2f, 0.5f, 1.f, 1.f);
	}
	if (FGameplayTag::RequestGameplayTag(FName("Effect.Buff"), false).IsValid() && T.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Effect.Buff"), false)))
	{
		return FLinearColor(0.2f, 0.85f, 0.2f, 1.f);
	}
	if (FGameplayTag::RequestGameplayTag(FName("Effect.Debuff"), false).IsValid() && T.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Effect.Debuff"), false)))
	{
		return FLinearColor(1.f, 0.25f, 0.25f, 1.f);
	}
	return FLinearColor(0.85f, 0.85f, 0.85f, 1.f);
}

void UDFGASDebugOverlayWidget::EnsureTextPool(
	int32 const MinCount, TArray<TObjectPtr<UTextBlock>>& Pool, UPanelWidget* const Parent, const FName BaseName)
{
	(void)BaseName;
	if (!Parent)
	{
		return;
	}
	while (Pool.Num() < MinCount)
	{
		UTextBlock* const T = WidgetTree
			? WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass())
			: nullptr;
		if (T)
		{
			Pool.Add(T);
			Parent->AddChild(T);
		}
		else
		{
			break;
		}
	}
	for (int32 i = MinCount; i < Pool.Num(); ++i)
	{
		if (Pool[i])
		{
			Pool[i]->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UDFGASDebugOverlayWidget::RefreshContent()
{
	APlayerController* const PC = GetOwningPlayer();
	ADFPlayerState* const PS = PC ? PC->GetPlayerState<ADFPlayerState>() : nullptr;
	UAbilitySystemComponent* const ASC = PS ? PS->GetAbilitySystemComponent() : nullptr;
	if (!TagsScroll || !EffectsScroll || !AttributesBox || !AbilitiesBox || !ASC)
	{
		return;
	}

	//~ Tags
	FGameplayTagContainer Owned;
	ASC->GetOwnedGameplayTags(Owned);
	TArray<FGameplayTag> TagList;
	Owned.GetGameplayTagArray(TagList);
	TagList.Sort([](FGameplayTag const& A, FGameplayTag const& B) { return A.ToString() < B.ToString(); });
	EnsureTextPool(FMath::Max(1, TagList.Num()), TagLinePool, TagsScroll, TEXT("T"));
	for (int32 i = 0; i < TagLinePool.Num(); ++i)
	{
		if (UTextBlock* const Tb = TagLinePool[i].Get())
		{
			if (i < TagList.Num())
			{
				Tb->SetVisibility(ESlateVisibility::Visible);
				const FGameplayTag& Tg = TagList[i];
				Tb->SetText(FText::FromString(Tg.ToString()));
				Tb->SetColorAndOpacity(FSlateColor(ColorForTag(Tg)));
			}
			else
			{
				Tb->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
	if (TagList.Num() == 0 && TagLinePool.Num() > 0)
	{
		if (UTextBlock* const Tb = TagLinePool[0].Get())
		{
			Tb->SetVisibility(ESlateVisibility::Visible);
			Tb->SetText(FText::FromString(TEXT("(no tags)")));
		}
	}

	//~ Clear attribute rows: rebuild
	AttributesBox->ClearChildren();
	struct FOneAttr
	{
		FText Lbl;
		float Cur = 0.f;
		float Max = 0.f;
		bool bHasMax = false;
		bool bVital = false;
	};
	TArray<FOneAttr> Rows;
	auto PushVital = [&Rows](FText const& L, float const c, float const m)
	{ Rows.Add(FOneAttr{L, c, m, true, true}); };
	auto PushStat = [&Rows](FText const& L, float const c)
	{ Rows.Add(FOneAttr{L, c, 0.f, false, false}); };
	PushVital(FText::FromString(TEXT("Health")), ASC->GetNumericAttribute(UDFAttributeSet::GetHealthAttribute()),
		ASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute()));
	PushVital(FText::FromString(TEXT("Mana")), ASC->GetNumericAttribute(UDFAttributeSet::GetManaAttribute()),
		ASC->GetNumericAttribute(UDFAttributeSet::GetMaxManaAttribute()));
	PushVital(FText::FromString(TEXT("Stamina")), ASC->GetNumericAttribute(UDFAttributeSet::GetStaminaAttribute()),
		ASC->GetNumericAttribute(UDFAttributeSet::GetMaxStaminaAttribute()));
	PushStat(FText::FromString(TEXT("Strength")), ASC->GetNumericAttribute(UDFAttributeSet::GetStrengthAttribute()));
	PushStat(FText::FromString(TEXT("Intelligence")), ASC->GetNumericAttribute(UDFAttributeSet::GetIntelligenceAttribute()));
	PushStat(FText::FromString(TEXT("Agility")), ASC->GetNumericAttribute(UDFAttributeSet::GetAgilityAttribute()));
	PushStat(FText::FromString(TEXT("Armor")), ASC->GetNumericAttribute(UDFAttributeSet::GetArmorAttribute()));
	PushStat(FText::FromString(TEXT("MagicResist")), ASC->GetNumericAttribute(UDFAttributeSet::GetMagicResistAttribute()));
	PushStat(FText::FromString(TEXT("CritChance")), ASC->GetNumericAttribute(UDFAttributeSet::GetCritChanceAttribute()));
	PushStat(FText::FromString(TEXT("CDR")), ASC->GetNumericAttribute(UDFAttributeSet::GetCooldownReductionAttribute()));
	for (FOneAttr const& R : Rows)
	{
		UTextBlock* const T = WidgetTree
			? WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), NAME_None)
			: nullptr;
		if (!T)
		{
			continue;
		}
		AttributesBox->AddChildToVerticalBox(T);
		if (R.bVital)
		{
			const bool bLow = R.Max > KINDA_SMALL_NUMBER && (R.Cur / R.Max) < 0.2f;
			T->SetText(FText::FromString(
				FString::Printf(TEXT("%s | %.1f / %.1f"), *R.Lbl.ToString(), R.Cur, R.Max)));
			T->SetColorAndOpacity(
				bLow ? FSlateColor(FLinearColor(1.f, 0.2f, 0.2f)) : FSlateColor(FLinearColor::White));
		}
		else
		{
			T->SetText(
				FText::FromString(FString::Printf(TEXT("%s | %.2f  /  n/a"), *R.Lbl.ToString(), R.Cur)));
			T->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		}
	}

	//~ Active GEs: broad OR-query
	const FGameplayTagContainer QAny = BuildBroadActiveEffectQueryTags();
	const FGameplayEffectQuery Q = FGameplayEffectQuery::MakeQuery_MatchAnyEffectTags(QAny);
	const TArray<FActiveGameplayEffectHandle> Handles = ASC->GetActiveEffects(Q);
	int32 N = 0;
	for (const FActiveGameplayEffectHandle& H : Handles)
	{
		if (ASC->GetActiveGameplayEffect(H) != nullptr)
		{
			++N;
		}
	}
	EnsureTextPool(FMath::Max(1, N), EffectLinePool, EffectsScroll, TEXT("E"));
	int32 Ix = 0;
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	for (const FActiveGameplayEffectHandle& H : Handles)
	{
		if (const FActiveGameplayEffect* const E = ASC->GetActiveGameplayEffect(H))
		{
			UTextBlock* const Tb = EffectLinePool.IsValidIndex(Ix) ? EffectLinePool[Ix].Get() : nullptr;
			if (Tb)
			{
				const UGameplayEffect* const Def = E->Spec.Def;
				const FString Name = Def ? Def->GetName() : TEXT("(unknown)");
				const float Rem = E->GetTimeRemaining(Now);
				const int32 St = E->Spec.GetStackCount();
				AActor* Src = Cast<AActor>(E->Spec.GetEffectContext().GetSourceObject());
				const FString Line = FString::Printf(
					TEXT("%s | %.1fs | x%d | %s"), *Name, Rem, St, Src ? *Src->GetName() : TEXT("-"));
				Tb->SetText(FText::FromString(Line));
				bool bBuff = false;
				bool bDebuff = false;
				if (Def)
				{
					const FGameplayTagContainer& AssetTags = Def->GetAssetTags();
					bBuff = AssetTags.HasTag(FGameplayTag::RequestGameplayTag(FName("Effect.Buff"), false));
					bDebuff = AssetTags.HasTag(FGameplayTag::RequestGameplayTag(FName("Effect.Debuff"), false));
				}
				Tb->SetColorAndOpacity(FSlateColor(
					bBuff ? FLinearColor(0.2f, 0.9f, 0.2f)
						  : bDebuff ? FLinearColor(1.f, 0.2f, 0.2f) : FLinearColor::White));
				Tb->SetVisibility(ESlateVisibility::Visible);
			}
			++Ix;
		}
	}
	for (/**/; Ix < EffectLinePool.Num(); ++Ix)
	{
		if (UTextBlock* const Tb = EffectLinePool[Ix].Get())
		{
			Tb->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (N == 0 && EffectLinePool.Num() > 0)
	{
		if (UTextBlock* const Tb = EffectLinePool[0].Get())
		{
			Tb->SetVisibility(ESlateVisibility::Visible);
			Tb->SetText(FText::FromString(TEXT("(no effects matched)")));
		}
	}

	//~ Ability slots
	const FGameplayTag Slots[] = {
		FDFGameplayTags::Ability_Slot_1, FDFGameplayTags::Ability_Slot_2, FDFGameplayTags::Ability_Slot_3, FDFGameplayTags::Ability_Slot_4
	};
	EnsureTextPool(4, AbilityLinePool, AbilitiesBox, TEXT("A"));
	for (int32 s = 0; s < 4; ++s)
	{
		if (UTextBlock* const Tb = AbilityLinePool[s].Get())
		{
			if (!Slots[s].IsValid())
			{
				Tb->SetText(FText::FromString(FString::Printf(TEXT("Slot %d: (invalid tag)"), s + 1)));
				continue;
			}
			FGameplayTagContainer One;
			One.AddTag(Slots[s]);
			const FGameplayEffectQuery Cq = FGameplayEffectQuery::MakeQuery_MatchAnyEffectTags(One);
			const TArray<TPair<float, float>> Tm = ASC->GetActiveEffectsTimeRemainingAndDuration(Cq);
			const float CD = (Tm.Num() > 0) ? Tm[0].Key : 0.f;
			const int32 Charges = 0;
			Tb->SetText(FText::FromString(FString::Printf(
				TEXT("%s | CD %.1fs | Charges %d"), *Slots[s].ToString(), CD, Charges)));
		}
	}
}

void UDFGASDebugOverlayWidget::EnsureWidgetsBuilt() {}
