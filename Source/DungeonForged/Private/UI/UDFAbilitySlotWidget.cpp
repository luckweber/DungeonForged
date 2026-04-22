// Source/DungeonForged/Private/UI/UDFAbilitySlotWidget.cpp
#include "UI/UDFAbilitySlotWidget.h"
#include "AbilitySystemComponent.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "TimerManager.h"

namespace
{
static constexpr float CooldownUpdateInterval = 0.05f;
}

void UDFAbilitySlotWidget::OnCooldownUpdateTimer()
{
	UpdateCooldownVisuals();
}

UDFAbilitySlotWidget::UDFAbilitySlotWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDFAbilitySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(AbilityIcon) && IsValid(AbilityIconTexture) && !AbilityIcon->GetBrush().GetResourceObject())
	{
		AbilityIcon->SetBrushFromTexture(AbilityIconTexture);
	}

	if (IsValid(CooldownOverlay) && bCreateDynamicMaterialInConstruct)
	{
		if (UObject* R = CooldownOverlay->GetBrush().GetResourceObject())
		{
			if (UMaterialInterface* const Mat = Cast<UMaterialInterface>(R))
			{
				CooldownOverlayMID = CooldownOverlay->GetDynamicMaterial();
				if (!CooldownOverlayMID)
				{
					UMaterialInstanceDynamic* const NewMid = UMaterialInstanceDynamic::Create(Mat, this);
					CooldownOverlayMID = NewMid;
					if (NewMid)
					{
						CooldownOverlay->SetBrushFromMaterial(NewMid);
					}
				}
			}
		}
	}

	if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponent())
	{
		CooldownSourceASC = ASC;
		OnActiveGEAddedHandle = ASC->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(
			this, &UDFAbilitySlotWidget::HandleActiveGameplayEffectAdded);
	}
	UpdateCooldownVisuals();

	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			CooldownUpdateTimerHandle, this, &UDFAbilitySlotWidget::OnCooldownUpdateTimer, CooldownUpdateInterval, true);
	}
}

void UDFAbilitySlotWidget::NativeDestruct()
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CooldownUpdateTimerHandle);
	}
	if (CooldownSourceASC.IsValid() && OnActiveGEAddedHandle.IsValid())
	{
		CooldownSourceASC->OnActiveGameplayEffectAddedDelegateToSelf.Remove(OnActiveGEAddedHandle);
	}
	CooldownSourceASC.Reset();
	OnActiveGEAddedHandle.Reset();

	Super::NativeDestruct();
}

void UDFAbilitySlotWidget::HandleActiveGameplayEffectAdded(
	UAbilitySystemComponent* Target, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle)
{
	(void)Target;
	(void)Spec;
	(void)Handle;
	UpdateCooldownVisuals();
}

void UDFAbilitySlotWidget::UpdateCooldownVisuals()
{
	if (!AbilityTag.IsValid() || !IsValid(CooldownOverlay))
	{
		ClearCooldownUI();
		return;
	}
	UAbilitySystemComponent* const ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		ClearCooldownUI();
		return;
	}

	FGameplayTagContainer QueryTags;
	QueryTags.AddTag(AbilityTag);
	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyEffectTags(QueryTags);
	const TArray<TPair<float, float>> RemainingAndDuration = ASC->GetActiveEffectsTimeRemainingAndDuration(Query);

	if (RemainingAndDuration.Num() == 0)
	{
		ClearCooldownUI();
		return;
	}

	const float Remaining = FMath::Max(0.f, RemainingAndDuration[0].Key);
	const float Duration = FMath::Max(0.f, RemainingAndDuration[0].Value);
	const float Pct = Duration > KINDA_SMALL_NUMBER ? FMath::Clamp(Remaining / Duration, 0.f, 1.f) : 0.f;

	UMaterialInstanceDynamic* MID = CooldownOverlayMID.Get();
	if (!MID && IsValid(CooldownOverlay))
	{
		MID = CooldownOverlay->GetDynamicMaterial();
	}
	if (MID)
	{
		MID->SetScalarParameterValue(CooldownMaterialParameter, Pct);
	}
	CooldownOverlay->SetVisibility(ESlateVisibility::Visible);

	if (IsValid(CooldownText))
	{
		if (Remaining > 0.05f)
		{
			const int32 Sec = FMath::CeilToInt(Remaining);
			CooldownText->SetText(FText::AsNumber(Sec));
			CooldownText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			CooldownText->SetText(FText::GetEmpty());
			CooldownText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
}

void UDFAbilitySlotWidget::ClearCooldownUI()
{
	UMaterialInstanceDynamic* MID = CooldownOverlayMID.Get();
	if (!MID && IsValid(CooldownOverlay))
	{
		MID = CooldownOverlay->GetDynamicMaterial();
	}
	if (MID)
	{
		MID->SetScalarParameterValue(CooldownMaterialParameter, 0.f);
	}
	if (IsValid(CooldownOverlay))
	{
		CooldownOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (IsValid(CooldownText))
	{
		CooldownText->SetText(FText::GetEmpty());
	}
}
