// Source/DungeonForged/Private/UI/UDFBossHealthBarWidget.cpp
#include "UI/UDFBossHealthBarWidget.h"
#include "Boss/ADFBossBase.h"
#include "DungeonForgedModule.h"
#include "GAS/DFGameplayTags.h"
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
	BindBossTagEvents();
	Boss->OnBossPhaseChanged.AddDynamic(this, &UDFBossHealthBarWidget::OnPhaseChanged);
	Boss->OnBossEnraged.AddDynamic(this, &UDFBossHealthBarWidget::OnEnraged);
	RefreshHealthFill();
	RefreshEnrageCountdown();
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
	StartHudRefreshTimer();
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

void UDFBossHealthBarWidget::StartHudRefreshTimer()
{
	UWorld* const W = GetWorld();
	if (!W || HudRefreshTimerHandle.IsValid() || !TrackedBoss.IsValid())
	{
		return;
	}
	W->GetTimerManager().SetTimer(
		HudRefreshTimerHandle, this, &UDFBossHealthBarWidget::OnHudRefreshTick, HudRefreshIntervalSec, true);
}

void UDFBossHealthBarWidget::StopHudRefreshTimer()
{
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(HudRefreshTimerHandle);
	}
	HudRefreshTimerHandle.Invalidate();
}

void UDFBossHealthBarWidget::OnHudRefreshTick()
{
	if (!TrackedBoss.IsValid())
	{
		StopHudRefreshTimer();
		return;
	}
	RefreshEnrageCountdown();
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
		BindBossTagEvents();
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

void UDFBossHealthBarWidget::BindBossTagEvents()
{
	if (!TrackedBoss.IsValid() || VulnerableTagDelegateHandle.IsValid() || !FDFGameplayTags::State_BossVulnerable.IsValid())
	{
		return;
	}
	UAbilitySystemComponent* const ASC = TrackedBoss->GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		return;
	}
	RefreshVulnerableCallout(ASC->HasMatchingGameplayTag(FDFGameplayTags::State_BossVulnerable));
	VulnerableTagDelegateHandle = ASC->RegisterGameplayTagEvent(
		FDFGameplayTags::State_BossVulnerable, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UDFBossHealthBarWidget::OnVulnerableTagChanged);
}

void UDFBossHealthBarWidget::OnVulnerableTagChanged(const FGameplayTag Tag, const int32 NewCount)
{
	(void)Tag;
	RefreshVulnerableCallout(NewCount > 0);
}

void UDFBossHealthBarWidget::UnbindBossTagEvents()
{
	if (ADFBossBase* const Boss = TrackedBoss.Get())
	{
		if (UAbilitySystemComponent* const ASC = Boss->GetAbilitySystemComponent())
		{
			if (VulnerableTagDelegateHandle.IsValid() && FDFGameplayTags::State_BossVulnerable.IsValid())
			{
				ASC->UnregisterGameplayTagEvent(VulnerableTagDelegateHandle, FDFGameplayTags::State_BossVulnerable);
			}
		}
	}
	VulnerableTagDelegateHandle.Reset();
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
	RefreshEnrageCountdown();
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

void UDFBossHealthBarWidget::RefreshEnrageCountdown()
{
	if (!EnrageCountdownText)
	{
		return;
	}
	if (!TrackedBoss.IsValid())
	{
		EnrageCountdownText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	const float Remaining = TrackedBoss->GetEnrageSecondsRemaining();
	if (Remaining <= KINDA_SMALL_NUMBER)
	{
		EnrageCountdownText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	const int32 TotalSec = FMath::CeilToInt(Remaining);
	const int32 Minutes = TotalSec / 60;
	const int32 Seconds = TotalSec % 60;
	EnrageCountdownText->SetText(FText::FromString(FString::Printf(TEXT("Enrage %d:%02d"), Minutes, Seconds)));
	EnrageCountdownText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UDFBossHealthBarWidget::RefreshVulnerableCallout(const bool bVisible)
{
	if (!VulnerableCalloutText)
	{
		return;
	}
	if (bVisible)
	{
		VulnerableCalloutText->SetText(NSLOCTEXT("DF", "BossVulnerableCallout", "VULNERABLE!"));
		VulnerableCalloutText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		VulnerableCalloutText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UDFBossHealthBarWidget::ClearBossBindings()
{
	StopRebindTimer();
	StopHudRefreshTimer();
	UnbindBossTagEvents();
	UnbindAllAttributeChanges();
	bBossAttributesBound = false;
	if (ADFBossBase* const B = TrackedBoss.Get())
	{
		B->OnBossPhaseChanged.RemoveDynamic(this, &UDFBossHealthBarWidget::OnPhaseChanged);
		B->OnBossEnraged.RemoveDynamic(this, &UDFBossHealthBarWidget::OnEnraged);
	}
	TrackedBoss = nullptr;
}
