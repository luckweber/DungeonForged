// Source/DungeonForged/Private/UI/Status/UDFStatusEffectBarWidget.cpp
#include "UI/Status/UDFStatusEffectBarWidget.h"
#include "UI/Status/UDFStatusEffectIconWidget.h"
#include "UI/Status/UDFStatusLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"

FGameplayTag UDFStatusEffectBarWidget::ChooseDisplayTag(
	const FGameplayEffectSpec& Spec,
	const UDFStatusLibrary* Lib)
{
	FGameplayTagContainer Granted;
	Spec.GetAllGrantedTags(Granted);
	for (const FGameplayTag& T : Granted)
	{
		if (UDFStatusLibrary::GetStatusData(T, Lib))
		{
			return T;
		}
	}
	FGameplayTagContainer Asset;
	Spec.GetAllAssetTags(Asset);
	for (const FGameplayTag& T : Asset)
	{
		if (UDFStatusLibrary::GetStatusData(T, Lib))
		{
			return T;
		}
	}
	for (const FGameplayTag& T : Granted)
	{
		return T;
	}
	return FGameplayTag::EmptyTag;
}

void UDFStatusEffectBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponent())
	{
		BindToAsc(ASC);
		RefreshExistingActiveEffects();
	}
}

void UDFStatusEffectBarWidget::NativeDestruct()
{
	UnbindFromAsc();
	Super::NativeDestruct();
}

void UDFStatusEffectBarWidget::BindToAsc(UAbilitySystemComponent* const ASC)
{
	UnbindFromAsc();
	if (!ASC)
	{
		return;
	}
	BoundAsc = ASC;
	AddedHandle = ASC->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(
		this,
		&UDFStatusEffectBarWidget::OnEffectAdded);
	RemovedHandle = ASC->OnAnyGameplayEffectRemovedDelegate().AddUObject(
		this,
		&UDFStatusEffectBarWidget::OnEffectRemoved);
}

void UDFStatusEffectBarWidget::UnbindFromAsc()
{
	if (UAbilitySystemComponent* const A = BoundAsc.Get())
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
	BoundAsc = nullptr;
}

void UDFStatusEffectBarWidget::RefreshExistingActiveEffects()
{
	UAbilitySystemComponent* const ASC = BoundAsc.Get();
	if (!ASC)
	{
		return;
	}
	FGameplayTagContainer QueryTags;
	UDFStatusLibrary::CollectAllStatusRootTags(StatusLibrary, QueryTags);
	if (QueryTags.Num() == 0)
	{
		return;
	}
	const FGameplayEffectQuery Q = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(QueryTags);
	for (const FActiveGameplayEffectHandle& H : ASC->GetActiveEffects(Q))
	{
		if (const FActiveGameplayEffect* const E = ASC->GetActiveGameplayEffect(H))
		{
			OnEffectAdded(ASC, E->Spec, H);
		}
	}
}

void UDFStatusEffectBarWidget::OnEffectAdded(
	UAbilitySystemComponent* const /*Target*/,
	const FGameplayEffectSpec& Spec,
	const FActiveGameplayEffectHandle Handle)
{
	if (!StatusLibrary || !BoundAsc.IsValid() || !StatusIconWidgetClass)
	{
		return;
	}
	const FGameplayTag Key = ChooseDisplayTag(Spec, StatusLibrary);
	if (!Key.IsValid())
	{
		return;
	}
	const FDFStatusEffectDisplayData* const Row = UDFStatusLibrary::GetStatusData(Key, StatusLibrary);
	if (!Row)
	{
		return;
	}
	if (IconByHandle.Contains(Handle))
	{
		return;
	}
	UDFStatusEffectIconWidget* const Icon = AcquireIcon();
	if (!Icon)
	{
		return;
	}
	Icon->InitializeForPlayerBar(Key, BoundAsc.Get(), Handle, *Row, this, StatusLibrary);
	Icon->SetDesiredIconSize(32.f);
	IconByHandle.Add(Handle, Icon);
	ActiveIcons.Add(Icon);
	ReparentIconToRow(Icon, Row->bIsDebuff);
	if (IconAddBounceAnim)
	{
		Icon->PlayAnimation(IconAddBounceAnim);
	}
	SortAndLayoutIcons();
}

void UDFStatusEffectBarWidget::OnEffectRemoved(const FActiveGameplayEffect& Removed)
{
	RemoveIconForHandle(Removed.Handle);
}

void UDFStatusEffectBarWidget::RemoveIconForHandle(const FActiveGameplayEffectHandle Handle)
{
	if (TObjectPtr<UDFStatusEffectIconWidget>* const Found = IconByHandle.Find(Handle))
	{
		if (UDFStatusEffectIconWidget* const I = Found->Get())
		{
			I->PlayFadeOutAndRequestReturn();
		}
	}
}

void UDFStatusEffectBarWidget::SortAndLayoutIcons()
{
	if (!BuffRow || !DebuffRow)
	{
		return;
	}
	struct FEntry
	{
		FActiveGameplayEffectHandle H;
		float Rem = 0.f;
		int32 Sev = 0;
		bool bDebuff = false;
	};
	TArray<FEntry> Buffs;
	TArray<FEntry> Debuffs;
	UAbilitySystemComponent* const ASC = BoundAsc.Get();
	UWorld* const W = GetWorld();
	const float Now = W ? W->GetTimeSeconds() : 0.f;
	for (const TPair<FActiveGameplayEffectHandle, TObjectPtr<UDFStatusEffectIconWidget>>& P : IconByHandle)
	{
		UDFStatusEffectIconWidget* const Icon = P.Value.Get();
		if (!Icon)
		{
			continue;
		}
		const FGameplayTag DKey = Icon->GetDisplayKey();
		const FDFStatusEffectDisplayData* const D = UDFStatusLibrary::GetStatusData(DKey, StatusLibrary);
		FEntry E;
		E.H = P.Key;
		E.bDebuff = D && D->bIsDebuff;
		E.Sev = D ? D->DebuffSeverity : 0;
		if (ASC)
		{
			if (const FActiveGameplayEffect* const A = ASC->GetActiveGameplayEffect(P.Key))
			{
				E.Rem = A->GetTimeRemaining(Now);
			}
		}
		if (E.bDebuff)
		{
			Debuffs.Add(E);
		}
		else
		{
			Buffs.Add(E);
		}
	}
	Buffs.Sort([](const FEntry& A, const FEntry& B) { return A.Rem > B.Rem; });
	Debuffs.Sort([](const FEntry& A, const FEntry& B)
	{
		if (A.Sev != B.Sev)
		{
			return A.Sev > B.Sev;
		}
		return A.Rem > B.Rem;
	});
	const FMargin Pad(0.f, 0.f, 4.f, 0.f);
	auto Rebuild = [&](UHorizontalBox* Row, const TArray<FEntry>& Order)
	{
		Row->ClearChildren();
		for (const FEntry& E : Order)
		{
			if (const TObjectPtr<UDFStatusEffectIconWidget>* const Ptr = IconByHandle.Find(E.H))
			{
				if (UDFStatusEffectIconWidget* const I = Ptr->Get())
				{
					if (UHorizontalBoxSlot* const S = Row->AddChildToHorizontalBox(I))
					{
						S->SetPadding(Pad);
					}
				}
			}
		}
	};
	Rebuild(BuffRow, Buffs);
	Rebuild(DebuffRow, Debuffs);
}

void UDFStatusEffectBarWidget::ReparentIconToRow(UDFStatusEffectIconWidget* const /*Icon*/, const bool /*bDebuffRow*/)
{
	SortAndLayoutIcons();
}

UDFStatusEffectIconWidget* UDFStatusEffectBarWidget::AcquireIcon()
{
	for (UDFStatusEffectIconWidget* Pooled : IconPool)
	{
		if (Pooled && !Pooled->GetDisplayKey().IsValid())
		{
			Pooled->SetVisibility(ESlateVisibility::Visible);
			return Pooled;
		}
	}
	UDFStatusEffectIconWidget* const NewIcon = CreateWidget<UDFStatusEffectIconWidget>(this, StatusIconWidgetClass);
	if (NewIcon)
	{
		IconPool.Add(NewIcon);
	}
	return NewIcon;
}

void UDFStatusEffectBarWidget::ReleaseIconToPool(UDFStatusEffectIconWidget* const Icon)
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
	ActiveIcons.Remove(Icon);
	Icon->ResetForPool();
	Icon->SetVisibility(ESlateVisibility::Collapsed);
	Icon->RemoveFromParent();
	SortAndLayoutIcons();
}
