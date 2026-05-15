// Source/DungeonForged/Private/UI/UDFAbilitySlotWidget.cpp
#include "UI/UDFAbilitySlotWidget.h"
#include "UI/DFAbilityBarTypes.h"
#include "UI/UDFAbilityHotbarWidget.h"
#include "AbilitySystemComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Cooldown_Base.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayEffect.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Input/Events.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture2D.h"
#include "TimerManager.h"

namespace
{
static constexpr float CooldownUpdateInterval = 0.05f;
static const FName GPercentParam(TEXT("percent"));

static bool EffectSpecMatchesAbilityTag(const FGameplayEffectSpec& Spec, const FGameplayTag& SlotTag)
{
	if (!SlotTag.IsValid())
	{
		return false;
	}

	FGameplayTagContainer AssetTags;
	Spec.GetAllAssetTags(AssetTags);
	if (AssetTags.HasTag(SlotTag))
	{
		return true;
	}

	FGameplayTagContainer GrantedTags;
	Spec.GetAllGrantedTags(GrantedTags);
	if (GrantedTags.HasTag(SlotTag))
	{
		return true;
	}

	if (Spec.Def)
	{
		if (Spec.Def->GetAssetTags().HasTag(SlotTag))
		{
			return true;
		}
		if (const UGE_Cooldown_Base* const CdDef = Cast<UGE_Cooldown_Base>(Spec.Def))
		{
			if (CdDef->CooldownAssociatedAbilityTag.IsValid()
				&& CdDef->CooldownAssociatedAbilityTag.MatchesTag(SlotTag))
			{
				return true;
			}
		}
	}

	return false;
}

static bool TryGetCooldownFromActiveEffects(
	UAbilitySystemComponent* const ASC,
	const FGameplayTag& SlotTag,
	float& OutRemaining,
	float& OutDuration)
{
	if (!IsValid(ASC) || !SlotTag.IsValid() || !FDFGameplayTags::Ability_Cooldown.IsValid())
	{
		return false;
	}

	FGameplayTagContainer CdOnly;
	CdOnly.AddTag(FDFGameplayTags::Ability_Cooldown);
	const FGameplayEffectQuery CdQuery = FGameplayEffectQuery::MakeQuery_MatchAnyEffectTags(CdOnly);
	const TArray<FActiveGameplayEffectHandle> Handles = ASC->GetActiveEffects(CdQuery);

	const UWorld* const World = ASC->GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.f;

	float BestRemaining = -1.f;
	float BestDuration = 0.f;
	for (const FActiveGameplayEffectHandle& H : Handles)
	{
		const FActiveGameplayEffect* const Active = ASC->GetActiveGameplayEffect(H);
		if (!Active || !EffectSpecMatchesAbilityTag(Active->Spec, SlotTag))
		{
			continue;
		}

		const float Rem = Active->GetTimeRemaining(Now);
		const float Dur = Active->GetDuration();
		if (Rem > BestRemaining)
		{
			BestRemaining = Rem;
			BestDuration = Dur;
		}
	}

	if (BestRemaining < 0.f)
	{
		return false;
	}

	OutRemaining = BestRemaining;
	OutDuration = BestDuration;
	return true;
}
} // namespace

void UDFAbilitySlotWidget::OnCooldownUpdateTimer()
{
	UpdateCooldownVisuals();
}

UDFAbilitySlotWidget::UDFAbilitySlotWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDFAbilitySlotWidget::UnbindCooldownDelegatesIfAny()
{
	if (CooldownSourceASC.IsValid() && OnActiveGEAddedHandle.IsValid())
	{
		CooldownSourceASC->OnActiveGameplayEffectAddedDelegateToSelf.Remove(OnActiveGEAddedHandle);
	}
	OnActiveGEAddedHandle.Reset();
	CooldownSourceASC.Reset();
}

void UDFAbilitySlotWidget::TryBindCooldownDelegates(UAbilitySystemComponent* const ASC)
{
	if (!IsValid(ASC))
	{
		return;
	}
	if (CooldownSourceASC.Get() == ASC && OnActiveGEAddedHandle.IsValid())
	{
		return;
	}

	if (CooldownSourceASC.IsValid() && OnActiveGEAddedHandle.IsValid())
	{
		CooldownSourceASC->OnActiveGameplayEffectAddedDelegateToSelf.Remove(OnActiveGEAddedHandle);
	}
	OnActiveGEAddedHandle.Reset();
	CooldownSourceASC = ASC;
	OnActiveGEAddedHandle = ASC->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(
		this, &UDFAbilitySlotWidget::HandleActiveGameplayEffectAdded);
}

void UDFAbilitySlotWidget::SetAbilitySlotData(
	const FGameplayTag InAbilityTag,
	UTexture2D* const InIcon,
	const FText InDisplayName,
	const FText InInputLabel)
{
	AbilityTag = InAbilityTag;
	AbilityIconTexture = InIcon;
	if (IsValid(AbilityIcon))
	{
		AbilityIcon->SetVisibility(ESlateVisibility::Visible);
		if (InIcon)
		{
			AbilityIcon->SetBrushFromTexture(InIcon, true);
			AbilityIcon->SetColorAndOpacity(FLinearColor::White);
		}
		else
		{
			AbilityIcon->SetBrushFromTexture(nullptr, false);
			AbilityIcon->SetColorAndOpacity(FLinearColor(0.25f, 0.25f, 0.25f, 0.85f));
		}
	}
	if (AbilityNameText)
	{
		AbilityNameText->SetText(InDisplayName);
	}
	if (InputLabelText)
	{
		InputLabelText->SetText(InInputLabel);
	}
	UpdateCooldownVisuals();
}

void UDFAbilitySlotWidget::ClearAbilitySlotData()
{
	AbilityTag = FGameplayTag();
	AbilityIconTexture = nullptr;
	if (IsValid(AbilityIcon))
	{
		AbilityIcon->SetBrushFromTexture(nullptr, false);
		AbilityIcon->SetColorAndOpacity(FLinearColor(0.18f, 0.18f, 0.18f, 0.8f));
	}
	if (AbilityNameText)
	{
		AbilityNameText->SetText(FText::GetEmpty());
	}
	if (InputLabelText)
	{
		InputLabelText->SetText(FText::GetEmpty());
	}
	ClearCooldownUI();
}

void UDFAbilitySlotWidget::SetBarSlotIndex(const int32 InSlotIndex)
{
	BarSlotIndex = InSlotIndex;
}

void UDFAbilitySlotWidget::SetOwningHotbar(UDFAbilityHotbarWidget* const InHotbar)
{
	OwningHotbar = InHotbar;
}

FReply UDFAbilitySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && BarSlotIndex >= 0 && AbilityTag.IsValid())
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UDFAbilitySlotWidget::NativeOnDragDetected(
	const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	UDFAbilityBarDragDropOperation* const Op = NewObject<UDFAbilityBarDragDropOperation>(this);
	Op->SourceSlotIndex = BarSlotIndex;
	Op->Pivot = EDragPivot::CenterCenter;
	OutOperation = Op;
}

bool UDFAbilitySlotWidget::NativeOnDrop(
	const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const UDFAbilityBarDragDropOperation* const Op = Cast<UDFAbilityBarDragDropOperation>(InOperation);
	if (!Op || Op->SourceSlotIndex < 0 || BarSlotIndex < 0 || Op->SourceSlotIndex == BarSlotIndex)
	{
		return false;
	}
	if (UDFAbilityHotbarWidget* const Hotbar = OwningHotbar.Get())
	{
		Hotbar->RequestSwapSlots(Op->SourceSlotIndex, BarSlotIndex);
		return true;
	}
	return false;
}

void UDFAbilitySlotWidget::EnsureCooldownOverlayMID()
{
	if (!IsValid(CooldownOverlay))
	{
		return;
	}
	if (CooldownOverlayMID)
	{
		return;
	}
	if (UMaterialInstanceDynamic* const Existing = CooldownOverlay->GetDynamicMaterial())
	{
		CooldownOverlayMID = Existing;
		return;
	}
	if (!bCreateDynamicMaterialInConstruct)
	{
		return;
	}
	if (UMaterialInterface* const Mat = Cast<UMaterialInterface>(CooldownOverlay->GetBrush().GetResourceObject()))
	{
		if (UMaterialInstanceDynamic* const NewMid = UMaterialInstanceDynamic::Create(Mat, this))
		{
			CooldownOverlayMID = NewMid;
			CooldownOverlay->SetBrushFromMaterial(NewMid);
		}
	}
}

void UDFAbilitySlotWidget::ApplyCooldownMaterialScalars(UMaterialInstanceDynamic* const MID, const float Pct) const
{
	if (!MID)
	{
		return;
	}
	MID->SetScalarParameterValue(CooldownMaterialParameter, Pct);
	if (!CooldownAuxScalarParameter.IsNone() && CooldownAuxScalarParameter != CooldownMaterialParameter)
	{
		MID->SetScalarParameterValue(CooldownAuxScalarParameter, Pct);
	}
	if (CooldownMaterialParameter != GPercentParam && CooldownAuxScalarParameter != GPercentParam)
	{
		MID->SetScalarParameterValue(GPercentParam, Pct);
	}
}

void UDFAbilitySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(AbilityIcon) && IsValid(AbilityIconTexture) && !AbilityIcon->GetBrush().GetResourceObject())
	{
		AbilityIcon->SetBrushFromTexture(AbilityIconTexture);
	}

	EnsureCooldownOverlayMID();

	TryBindCooldownDelegates(GetAbilitySystemComponent());
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
	UnbindCooldownDelegatesIfAny();

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

	EnsureCooldownOverlayMID();

	UAbilitySystemComponent* const ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		UnbindCooldownDelegatesIfAny();
		ClearCooldownUI();
		return;
	}

	TryBindCooldownDelegates(ASC);

	FGameplayTagContainer QueryTags;
	QueryTags.AddTag(FDFGameplayTags::Ability_Cooldown);
	QueryTags.AddTag(AbilityTag);
	FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAllEffectTags(QueryTags);
	TArray<TPair<float, float>> RemainingAndDuration = ASC->GetActiveEffectsTimeRemainingAndDuration(Query);

	// Legacy: cooldown GEs that only tagged the ability on effect tags (no Ability.Cooldown in inheritable tags).
	if (RemainingAndDuration.Num() == 0)
	{
		FGameplayTagContainer LegacyTags;
		LegacyTags.AddTag(AbilityTag);
		const FGameplayEffectQuery LegacyQuery = FGameplayEffectQuery::MakeQuery_MatchAnyEffectTags(LegacyTags);
		RemainingAndDuration = ASC->GetActiveEffectsTimeRemainingAndDuration(LegacyQuery);
	}

	float Remaining = 0.f;
	float Duration = 0.f;
	if (RemainingAndDuration.Num() > 0)
	{
		Remaining = FMath::Max(0.f, RemainingAndDuration[0].Key);
		Duration = FMath::Max(0.f, RemainingAndDuration[0].Value);
	}
	else if (!TryGetCooldownFromActiveEffects(ASC, AbilityTag, Remaining, Duration))
	{
		ClearCooldownUI();
		return;
	}

	const float Pct = Duration > KINDA_SMALL_NUMBER ? FMath::Clamp(Remaining / Duration, 0.f, 1.f) : 0.f;

	UMaterialInstanceDynamic* MID = CooldownOverlayMID.Get();
	if (!MID && IsValid(CooldownOverlay))
	{
		MID = CooldownOverlay->GetDynamicMaterial();
	}
	if (MID)
	{
		ApplyCooldownMaterialScalars(MID, Pct);
		if (IsValid(CooldownOverlay))
		{
			CooldownOverlay->SetRenderOpacity(1.f);
		}
	}
	else if (IsValid(CooldownOverlay))
	{
		// No material on the brush: approximate cooldown fill with image opacity.
		CooldownOverlay->SetRenderOpacity(FMath::Clamp(Pct, 0.f, 1.f) * 0.9f);
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
		ApplyCooldownMaterialScalars(MID, 0.f);
	}
	if (IsValid(CooldownOverlay))
	{
		CooldownOverlay->SetRenderOpacity(MID ? 1.f : 0.f);
		CooldownOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IsValid(CooldownText))
	{
		CooldownText->SetText(FText::GetEmpty());
		CooldownText->SetVisibility(ESlateVisibility::Collapsed);
	}
}
