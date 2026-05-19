// Source/DungeonForged/Private/UI/UDFBossHealthBarWidget.cpp
#include "UI/UDFBossHealthBarWidget.h"
#include "Boss/ADFBossBase.h"
#include "DungeonForgedModule.h"
#include "GAS/UDFAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameplayEffectTypes.h"
#include "TimerManager.h"

void UDFBossHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UDFBossHealthBarWidget::ShowForBoss(ADFBossBase* const Boss, const FText& DisplayName)
{
	ClearBossBindings();
	TrackedBoss = Boss;
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (BossNameText)
	{
		BossNameText->SetText(DisplayName);
	}
	else
	{
		DF_LOG(Warning,
			"UDFBossHealthBarWidget: BossNameText not bound — WBP must expose a TextBlock named BossNameText.");
	}
	if (!BossHealthBar)
	{
		DF_LOG(Warning,
			"UDFBossHealthBarWidget: BossHealthBar not bound — WBP must expose a ProgressBar named BossHealthBar.");
	}
	if (!Boss)
	{
		return;
	}
	bBossAttributesBound = false;
	TryBindBossAttributes();
	Boss->OnBossPhaseChanged.AddDynamic(this, &UDFBossHealthBarWidget::OnPhaseChanged);
	Boss->OnBossEnraged.AddDynamic(this, &UDFBossHealthBarWidget::OnEnraged);
	RefreshHealthFill();
	OnPhaseChanged(0, Boss->CurrentPhase, Boss);
	OnEnraged(Boss, Boss->bIsEnraged);

	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UDFBossHealthBarWidget::RefreshHealthFill));
	}
	if (!bBossAttributesBound)
	{
		StartRebindTimer();
	}
}

void UDFBossHealthBarWidget::HideBossBar()
{
	ClearBossBindings();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UDFBossHealthBarWidget::StartRebindTimer()
{
	if (bBossAttributesBound || !TrackedBoss.IsValid())
	{
		return;
	}
	UWorld* const W = GetWorld();
	if (!W || RebindTimerHandle.IsValid())
	{
		return;
	}
	W->GetTimerManager().SetTimer(
		RebindTimerHandle, this, &UDFBossHealthBarWidget::OnRebindTimerTick, RebindIntervalSec, true);
}

void UDFBossHealthBarWidget::StopRebindTimer()
{
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(RebindTimerHandle);
	}
	RebindTimerHandle.Invalidate();
}

void UDFBossHealthBarWidget::OnRebindTimerTick()
{
	if (!TrackedBoss.IsValid())
	{
		StopRebindTimer();
		return;
	}
	if (!bBossAttributesBound)
	{
		TryBindBossAttributes();
	}
	RefreshHealthFill();
	if (BossNameText)
	{
		BossNameText->SetText(TrackedBoss->GetBossDisplayName());
	}
	if (bBossAttributesBound)
	{
		StopRebindTimer();
	}
}

void UDFBossHealthBarWidget::TryBindBossAttributes()
{
	if (bBossAttributesBound || !TrackedBoss.IsValid())
	{
		return;
	}
	UAbilitySystemComponent* const ASC = TrackedBoss->GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		return;
	}
	const float MaxH = ASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute());
	if (MaxH <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	const TDelegate<void(const FOnAttributeChangeData&)> H = TDelegate<void(const FOnAttributeChangeData&)>::CreateUObject(
		this, &UDFBossHealthBarWidget::OnHealthAttrChanged);
	const TDelegate<void(const FOnAttributeChangeData&)> HM = TDelegate<void(const FOnAttributeChangeData&)>::CreateUObject(
		this, &UDFBossHealthBarWidget::OnMaxHealthAttrChanged);
	BindToAttributeChanges(ASC, UDFAttributeSet::GetHealthAttribute(), H);
	BindToAttributeChanges(ASC, UDFAttributeSet::GetMaxHealthAttribute(), HM);
	bBossAttributesBound = true;
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
	StopRebindTimer();
	UnbindAllAttributeChanges();
	bBossAttributesBound = false;
	if (ADFBossBase* const B = TrackedBoss.Get())
	{
		B->OnBossPhaseChanged.RemoveDynamic(this, &UDFBossHealthBarWidget::OnPhaseChanged);
		B->OnBossEnraged.RemoveDynamic(this, &UDFBossHealthBarWidget::OnEnraged);
	}
	TrackedBoss = nullptr;
}
