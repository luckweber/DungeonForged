// Source/DungeonForged/Private/UI/Status/UDFEnemyDebuffStatusBarWidget.cpp
#include "UI/Status/UDFEnemyDebuffStatusBarWidget.h"
#include "Characters/ADFEnemyBase.h"
#include "UI/Status/UDFStatusEffectBarWidget.h"
#include "UI/Status/UDFStatusEffectIconWidget.h"
#include "UI/Status/UDFStatusLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "Characters/ADFPlayerState.h"
#include "Engine/World.h"

void UDFEnemyDebuffStatusBarWidget::SetupObservedEnemy(
	ADFEnemyBase* const InEnemy,
	UDFStatusLibrary* const InLibrary,
	UAbilitySystemComponent* const InLocalFilterAsc)
{
	ObservedEnemy = InEnemy;
	StatusLibrary = InLibrary;
	LocalFilterAsc = InLocalFilterAsc;
	EnemyAsc = InEnemy ? InEnemy->GetAbilitySystemComponent() : nullptr;
}

void UDFEnemyDebuffStatusBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!EnemyAsc.IsValid() && ObservedEnemy.IsValid())
	{
		EnemyAsc = ObservedEnemy->GetAbilitySystemComponent();
	}
	if (!LocalFilterAsc.IsValid())
	{
		if (UWorld* const W = GetWorld())
		{
			if (APlayerController* const PC = W->GetFirstPlayerController())
			{
				if (ADFPlayerState* const PS = PC->GetPlayerState<ADFPlayerState>())
				{
					LocalFilterAsc = PS->GetAbilitySystemComponent();
				}
			}
		}
	}
	BindToEnemyAsc();
	RefreshActive();
}

void UDFEnemyDebuffStatusBarWidget::NativeDestruct()
{
	Unbind();
	Super::NativeDestruct();
}

void UDFEnemyDebuffStatusBarWidget::BindToEnemyAsc()
{
	Unbind();
	if (!EnemyAsc.IsValid())
	{
		return;
	}
	AddedHandle = EnemyAsc->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(
		this,
		&UDFEnemyDebuffStatusBarWidget::OnEffectAdded);
	RemovedHandle = EnemyAsc->OnAnyGameplayEffectRemovedDelegate().AddUObject(
		this,
		&UDFEnemyDebuffStatusBarWidget::OnEffectRemoved);
}

void UDFEnemyDebuffStatusBarWidget::Unbind()
{
	if (UAbilitySystemComponent* const A = EnemyAsc.Get())
	{
		if (AddedHandle.IsValid())
		{
			A->OnActiveGameplayEffectAddedDelegateToSelf.Remove(AddedHandle);
		}
		if (RemovedHandle.IsValid())
		{
			A->OnAnyGameplayEffectRemovedDelegate().Remove(RemovedHandle);
		}
	}
	AddedHandle.Reset();
	RemovedHandle.Reset();
}

bool UDFEnemyDebuffStatusBarWidget::IsFromLocalPlayer(const FGameplayEffectSpec& Spec) const
{
	if (!LocalFilterAsc.IsValid())
	{
		return false;
	}
	const FGameplayEffectContextHandle& Ctx = Spec.GetEffectContext();
	if (UAbilitySystemComponent* const I = Ctx.GetInstigatorAbilitySystemComponent())
	{
		return I == LocalFilterAsc.Get();
	}
	if (UAbilitySystemComponent* const O = Ctx.GetOriginalInstigatorAbilitySystemComponent())
	{
		return O == LocalFilterAsc.Get();
	}
	return false;
}

void UDFEnemyDebuffStatusBarWidget::RefreshActive()
{
	UAbilitySystemComponent* const ASC = EnemyAsc.Get();
	if (!ASC || !StatusLibrary)
	{
		return;
	}
	FGameplayTagContainer Tags;
	UDFStatusLibrary::CollectAllStatusRootTags(StatusLibrary, Tags);
	if (Tags.Num() == 0)
	{
		return;
	}
	const FGameplayEffectQuery Q = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(Tags);
	for (const FActiveGameplayEffectHandle& H : ASC->GetActiveEffects(Q))
	{
		if (const FActiveGameplayEffect* const E = ASC->GetActiveGameplayEffect(H))
		{
			if (IsFromLocalPlayer(E->Spec))
			{
				OnEffectAdded(ASC, E->Spec, H);
			}
		}
	}
}

void UDFEnemyDebuffStatusBarWidget::OnEffectAdded(
	UAbilitySystemComponent* const /*Target*/,
	const FGameplayEffectSpec& Spec,
	const FActiveGameplayEffectHandle Handle)
{
	if (!IsFromLocalPlayer(Spec) || !StatusLibrary || !StatusIconWidgetClass || !EnemyAsc.IsValid())
	{
		return;
	}
	const FGameplayTag Key = UDFStatusEffectBarWidget::ChooseDisplayTag(Spec, StatusLibrary);
	if (!Key.IsValid())
	{
		return;
	}
	const FDFStatusEffectDisplayData* const Row = UDFStatusLibrary::GetStatusData(Key, StatusLibrary);
	if (!Row || !Row->bIsDebuff)
	{
		return;
	}
	if (IconByHandle.Contains(Handle))
	{
		return;
	}
	UDFStatusEffectIconWidget* Icon = nullptr;
	for (UDFStatusEffectIconWidget* P : Pooled)
	{
		if (P && !P->GetDisplayKey().IsValid())
		{
			Icon = P;
			break;
		}
	}
	if (!Icon)
	{
		Icon = CreateWidget<UDFStatusEffectIconWidget>(this, StatusIconWidgetClass);
		if (Icon)
		{
			Pooled.Add(Icon);
		}
	}
	if (!Icon)
	{
		return;
	}
	Icon->InitializeForEnemyBar(Key, EnemyAsc.Get(), Handle, *Row, this, StatusLibrary);
	Icon->SetDesiredIconSize(24.f);
	IconByHandle.Add(Handle, Icon);
	if (IconAddAnim)
	{
		Icon->PlayAnimation(IconAddAnim);
	}
	LayoutTopThree();
}

void UDFEnemyDebuffStatusBarWidget::OnEffectRemoved(const FActiveGameplayEffect& Removed)
{
	if (TObjectPtr<UDFStatusEffectIconWidget>* const Found = IconByHandle.Find(Removed.Handle))
	{
		if (UDFStatusEffectIconWidget* const I = Found->Get())
		{
			I->PlayFadeOutAndRequestReturn();
		}
	}
}

void UDFEnemyDebuffStatusBarWidget::LayoutTopThree()
{
	if (!DebuffRow)
	{
		return;
	}
	struct FEntry
	{
		FActiveGameplayEffectHandle H;
		int32 Sev = 0;
		float Rem = 0.f;
	};
	TArray<FEntry> Items;
	UWorld* const W = GetWorld();
	const float Now = W ? W->GetTimeSeconds() : 0.f;
	for (const auto& P : IconByHandle)
	{
		if (!P.Value.Get())
		{
			continue;
		}
		const FGameplayTag DKey = P.Value->GetDisplayKey();
		const FDFStatusEffectDisplayData* const D = UDFStatusLibrary::GetStatusData(DKey, StatusLibrary);
		FEntry E;
		E.H = P.Key;
		E.Sev = D ? D->DebuffSeverity : 0;
		if (UAbilitySystemComponent* const A = EnemyAsc.Get())
		{
			if (const FActiveGameplayEffect* const Fx = A->GetActiveGameplayEffect(P.Key))
			{
				E.Rem = Fx->GetTimeRemaining(Now);
			}
		}
		Items.Add(E);
	}
	Items.Sort([](const FEntry& A, const FEntry& B)
	{
		if (A.Sev != B.Sev)
		{
			return A.Sev > B.Sev;
		}
		return A.Rem < B.Rem;
	});
	DebuffRow->ClearChildren();
	const FMargin Pad(0.f, 0.f, 3.f, 0.f);
	const int32 MaxIcons = 3;
	for (int32 Ix = 0; Ix < Items.Num() && Ix < MaxIcons; ++Ix)
	{
		if (const TObjectPtr<UDFStatusEffectIconWidget>* Ptr = IconByHandle.Find(Items[Ix].H))
		{
			if (UDFStatusEffectIconWidget* Ic = Ptr->Get())
			{
				if (UHorizontalBoxSlot* const S = DebuffRow->AddChildToHorizontalBox(Ic))
				{
					S->SetPadding(Pad);
				}
			}
		}
	}
}

void UDFEnemyDebuffStatusBarWidget::ReleaseIconToPool(UDFStatusEffectIconWidget* const Icon)
{
	if (!Icon)
	{
		return;
	}
	FActiveGameplayEffectHandle RemoveKey;
	for (const TPair<FActiveGameplayEffectHandle, TObjectPtr<UDFStatusEffectIconWidget>>& P : IconByHandle)
	{
		if (P.Value.Get() == Icon)
		{
			RemoveKey = P.Key;
			break;
		}
	}
	if (RemoveKey.IsValid())
	{
		IconByHandle.Remove(RemoveKey);
	}
	Icon->ResetForPool();
	Icon->SetVisibility(ESlateVisibility::Collapsed);
	Icon->RemoveFromParent();
	LayoutTopThree();
}
