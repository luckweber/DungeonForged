// Source/DungeonForged/Private/UI/UDFEnemyHealthBarWidget.cpp
#include "UI/UDFEnemyHealthBarWidget.h"
#include "Characters/ADFEnemyBase.h"
#include "DungeonForgedModule.h"
#include "GAS/UDFAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameplayEffectTypes.h"
#include "TimerManager.h"

void UDFEnemyHealthBarWidget::ResolveWidgetBindings()
{
	if (!EnemyHealthBar)
	{
		EnemyHealthBar = Cast<UProgressBar>(GetWidgetFromName(TEXT("EnemyHealthBar")));
	}
	if (!EnemyHealthBar)
	{
		EnemyHealthBar = Cast<UProgressBar>(GetWidgetFromName(TEXT("HealthBar")));
	}
	if (!EnemyNameText)
	{
		EnemyNameText = Cast<UTextBlock>(GetWidgetFromName(TEXT("EnemyNameText")));
	}
	if (!EnemyNameText)
	{
		EnemyNameText = Cast<UTextBlock>(GetWidgetFromName(TEXT("BossNameText")));
	}
	if (!HealthValueText)
	{
		HealthValueText = Cast<UTextBlock>(GetWidgetFromName(TEXT("HealthValueText")));
	}
}

void UDFEnemyHealthBarWidget::SetupObservedEnemy(ADFEnemyBase* const InEnemy, const FText& InDisplayName)
{
	ObservedEnemy = InEnemy;
	EnemyAsc = InEnemy ? InEnemy->GetAbilitySystemComponent() : nullptr;
	if (!InDisplayName.IsEmpty())
	{
		CachedDisplayName = InDisplayName;
	}
	else if (InEnemy)
	{
		CachedDisplayName = InEnemy->GetEnemyDisplayName();
	}
	bAttributesBound = false;
	UnbindAllAttributeChanges();
	if (BoundAttributeSet.IsValid() && HealthChangedDelegateHandle.IsValid())
	{
		BoundAttributeSet->OnHealthChanged.Remove(HealthChangedDelegateHandle);
		HealthChangedDelegateHandle.Reset();
	}
	BoundAttributeSet = nullptr;
	ResolveWidgetBindings();
	TryBindEnemyAttributes();
	ApplyDisplayName();
	RefreshHealthFill();
	StartRebindTimer();
}

void UDFEnemyHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ResolveWidgetBindings();

	if (!EnemyHealthBar)
	{
		DF_LOG(Warning,
			"UDFEnemyHealthBarWidget: bind a ProgressBar named EnemyHealthBar (or HealthBar) in the WBP parent class.");
	}

	if (!EnemyAsc.IsValid() && ObservedEnemy.IsValid())
	{
		EnemyAsc = ObservedEnemy->GetAbilitySystemComponent();
	}

	ApplyDisplayName();
	TryBindEnemyAttributes();
	RefreshHealthFill();

	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UDFEnemyHealthBarWidget::RefreshHealthFill));
	}
	StartRebindTimer();
}

void UDFEnemyHealthBarWidget::NativeDestruct()
{
	StopRebindTimer();
	ClearEnemyBindings();
	Super::NativeDestruct();
}

void UDFEnemyHealthBarWidget::StartRebindTimer()
{
	if (!ObservedEnemy.IsValid())
	{
		return;
	}
	UWorld* const W = GetWorld();
	if (!W || RebindTimerHandle.IsValid())
	{
		return;
	}
	W->GetTimerManager().SetTimer(
		RebindTimerHandle, this, &UDFEnemyHealthBarWidget::OnRebindTimerTick, RebindIntervalSec, true);
}

void UDFEnemyHealthBarWidget::StopRebindTimer()
{
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(RebindTimerHandle);
	}
	RebindTimerHandle.Invalidate();
}

void UDFEnemyHealthBarWidget::OnRebindTimerTick()
{
	if (!ObservedEnemy.IsValid())
	{
		StopRebindTimer();
		return;
	}
	if (!EnemyAsc.IsValid())
	{
		EnemyAsc = ObservedEnemy->GetAbilitySystemComponent();
	}
	if (!bAttributesBound)
	{
		TryBindEnemyAttributes();
	}
	RefreshHealthFill();
	ApplyDisplayName();
}

void UDFEnemyHealthBarWidget::TryBindEnemyAttributes()
{
	if (bAttributesBound || !ObservedEnemy.IsValid())
	{
		return;
	}
	if (!EnemyAsc.IsValid())
	{
		EnemyAsc = ObservedEnemy->GetAbilitySystemComponent();
	}
	UAbilitySystemComponent* const ASC = EnemyAsc.Get();
	UDFAttributeSet* const AttrSet = ObservedEnemy->AttributeSet;
	if (!IsValid(ASC) || !AttrSet)
	{
		return;
	}
	const float MaxH = ASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute());
	if (MaxH <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// ASC value-change delegates often do not fire on clients with Minimal replication.
	BoundAttributeSet = AttrSet;
	if (!HealthChangedDelegateHandle.IsValid())
	{
		HealthChangedDelegateHandle = AttrSet->OnHealthChanged.AddUObject(
			this, &UDFEnemyHealthBarWidget::OnEnemyHealthChanged);
	}

	const TDelegate<void(const FOnAttributeChangeData&)> HealthCb = TDelegate<void(const FOnAttributeChangeData&)>::CreateUObject(
		this, &UDFEnemyHealthBarWidget::OnHealthAttrChanged);
	const TDelegate<void(const FOnAttributeChangeData&)> MaxHealthCb = TDelegate<void(const FOnAttributeChangeData&)>::CreateUObject(
		this, &UDFEnemyHealthBarWidget::OnMaxHealthAttrChanged);
	BindToAttributeChanges(ASC, UDFAttributeSet::GetHealthAttribute(), HealthCb);
	BindToAttributeChanges(ASC, UDFAttributeSet::GetMaxHealthAttribute(), MaxHealthCb);
	bAttributesBound = true;
	const float CurH = ASC->GetNumericAttribute(UDFAttributeSet::GetHealthAttribute());
	UE_LOG(LogDungeonForged, Verbose, TEXT("UDFEnemyHealthBarWidget: bound to %s (HP %.1f/%.1f)."),
		*GetNameSafe(ObservedEnemy.Get()), CurH, MaxH);
}

void UDFEnemyHealthBarWidget::OnEnemyHealthChanged(const float CurrentHealth, const float MaxHealth)
{
	(void)CurrentHealth;
	(void)MaxHealth;
	RefreshHealthFill();
}

void UDFEnemyHealthBarWidget::OnHealthAttrChanged(const FOnAttributeChangeData& Data)
{
	(void)Data;
	RefreshHealthFill();
}

void UDFEnemyHealthBarWidget::OnMaxHealthAttrChanged(const FOnAttributeChangeData& Data)
{
	(void)Data;
	RefreshHealthFill();
}

void UDFEnemyHealthBarWidget::RefreshHealthFill()
{
	if (!ObservedEnemy.IsValid())
	{
		return;
	}
	if (!EnemyHealthBar)
	{
		ResolveWidgetBindings();
	}
	if (!EnemyHealthBar)
	{
		return;
	}
	if (!EnemyAsc.IsValid())
	{
		EnemyAsc = ObservedEnemy->GetAbilitySystemComponent();
	}
	UAbilitySystemComponent* const ASC = EnemyAsc.Get();
	if (!IsValid(ASC))
	{
		EnemyHealthBar->SetPercent(0.f);
		if (HealthValueText)
		{
			HealthValueText->SetText(FText::GetEmpty());
		}
		return;
	}
	const float H = ASC->GetNumericAttribute(UDFAttributeSet::GetHealthAttribute());
	const float M = ASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute());
	const float Pct = M > KINDA_SMALL_NUMBER ? FMath::Clamp(H / M, 0.f, 1.f) : 0.f;
	if (!FMath::IsNearlyEqual(EnemyHealthBar->GetPercent(), Pct, 0.001f))
	{
		EnemyHealthBar->SetPercent(Pct);
	}
	if (HealthValueText && bShowHealthNumbers)
	{
		const int32 Cur = FMath::RoundToInt(H);
		const int32 MaxVal = FMath::Max(1, FMath::RoundToInt(M));
		HealthValueText->SetText(FText::Format(
			NSLOCTEXT("DF", "EnemyHealthValue", "{0} / {1}"),
			FText::AsNumber(Cur),
			FText::AsNumber(MaxVal)));
		HealthValueText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else if (HealthValueText)
	{
		HealthValueText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UDFEnemyHealthBarWidget::ApplyDisplayName()
{
	if (!EnemyNameText)
	{
		ResolveWidgetBindings();
	}
	if (!EnemyNameText)
	{
		return;
	}
	if (CachedDisplayName.IsEmpty() && ObservedEnemy.IsValid())
	{
		CachedDisplayName = ObservedEnemy->GetEnemyDisplayName();
	}
	if (CachedDisplayName.IsEmpty())
	{
		EnemyNameText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	EnemyNameText->SetText(CachedDisplayName);
	EnemyNameText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UDFEnemyHealthBarWidget::ClearEnemyBindings()
{
	StopRebindTimer();
	UnbindAllAttributeChanges();
	if (BoundAttributeSet.IsValid() && HealthChangedDelegateHandle.IsValid())
	{
		BoundAttributeSet->OnHealthChanged.Remove(HealthChangedDelegateHandle);
	}
	HealthChangedDelegateHandle.Reset();
	BoundAttributeSet = nullptr;
	bAttributesBound = false;
	ObservedEnemy = nullptr;
	EnemyAsc = nullptr;
}
