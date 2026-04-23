// Source/DungeonForged/Private/UI/UDFBossHealthBarWidget.cpp
#include "UI/UDFBossHealthBarWidget.h"
#include "Boss/ADFBossBase.h"
#include "GAS/UDFAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameplayEffectTypes.h"

void UDFBossHealthBarWidget::ShowForBoss(ADFBossBase* const Boss, const FText& DisplayName)
{
	ClearBossBindings();
	TrackedBoss = Boss;
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (BossNameText)
	{
		BossNameText->SetText(DisplayName);
	}
	if (!Boss)
	{
		return;
	}
	if (UAbilitySystemComponent* const ASC = Boss->GetAbilitySystemComponent())
	{
		const TDelegate<void(const FOnAttributeChangeData&)> H = TDelegate<void(const FOnAttributeChangeData&)>::CreateUObject(
			this, &UDFBossHealthBarWidget::OnHealthAttrChanged);
		const TDelegate<void(const FOnAttributeChangeData&)> HM = TDelegate<void(const FOnAttributeChangeData&)>::CreateUObject(
			this, &UDFBossHealthBarWidget::OnMaxHealthAttrChanged);
		BindToAttributeChanges(ASC, UDFAttributeSet::GetHealthAttribute(), H);
		BindToAttributeChanges(ASC, UDFAttributeSet::GetMaxHealthAttribute(), HM);
	}
	Boss->OnBossPhaseChanged.AddDynamic(this, &UDFBossHealthBarWidget::OnPhaseChanged);
	Boss->OnBossEnraged.AddDynamic(this, &UDFBossHealthBarWidget::OnEnraged);
	RefreshHealthFill();
	OnPhaseChanged(0, Boss->CurrentPhase, Boss);
	OnEnraged(Boss, Boss->bIsEnraged);
}

void UDFBossHealthBarWidget::HideBossBar()
{
	ClearBossBindings();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UDFBossHealthBarWidget::NativeDestruct()
{
	ClearBossBindings();
	Super::NativeDestruct();
}

void UDFBossHealthBarWidget::OnHealthAttrChanged(const FOnAttributeChangeData& Data)
{
	(void)Data;
	RefreshHealthFill();
}

void UDFBossHealthBarWidget::OnMaxHealthAttrChanged(const FOnAttributeChangeData& Data)
{
	(void)Data;
	RefreshHealthFill();
}

void UDFBossHealthBarWidget::OnPhaseChanged(const int32 OldPhase, const int32 NewPhase, AActor* const Boss)
{
	(void)OldPhase;
	(void)Boss;
	if (PhaseText)
	{
		PhaseText->SetText(FText::Format(NSLOCTEXT("DF", "BossPhaseFmt", "Phase {0}"), FText::AsNumber(NewPhase)));
	}
}

void UDFBossHealthBarWidget::OnEnraged(AActor* const Boss, const bool bEnraged)
{
	(void)Boss;
	if (EnrageIcon)
	{
		EnrageIcon->SetVisibility(bEnraged ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UDFBossHealthBarWidget::RefreshHealthFill()
{
	if (!BossHealthBar || !TrackedBoss.IsValid())
	{
		return;
	}
	UAbilitySystemComponent* const ASC = TrackedBoss->GetAbilitySystemComponent();
	if (!ASC)
	{
		BossHealthBar->SetPercent(0.f);
		return;
	}
	const float H = ASC->GetNumericAttribute(UDFAttributeSet::GetHealthAttribute());
	const float M = ASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute());
	const float Pct = M > KINDA_SMALL_NUMBER ? FMath::Clamp(H / M, 0.f, 1.f) : 0.f;
	BossHealthBar->SetPercent(Pct);
}

void UDFBossHealthBarWidget::ClearBossBindings()
{
	UnbindAllAttributeChanges();
	if (ADFBossBase* const B = TrackedBoss.Get())
	{
		B->OnBossPhaseChanged.RemoveDynamic(this, &UDFBossHealthBarWidget::OnPhaseChanged);
		B->OnBossEnraged.RemoveDynamic(this, &UDFBossHealthBarWidget::OnEnraged);
	}
	TrackedBoss = nullptr;
}
