// Source/DungeonForged/Private/UI/UDFAttributeBarWidget.cpp
#include "UI/UDFAttributeBarWidget.h"
#include "AbilitySystemComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameplayEffectTypes.h"

UDFAttributeBarWidget::UDFAttributeBarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDFAttributeBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponent())
	{
		const TDelegate<void(const FOnAttributeChangeData&)> CB = TDelegate<void(const FOnAttributeChangeData&)>::CreateUObject(
			this, &UDFAttributeBarWidget::OnAttributeChanged);
		if (TrackedAttribute.IsValid())
		{
			BindToAttributeChanges(ASC, TrackedAttribute, CB);
		}
		if (MaxAttribute.IsValid())
		{
			BindToAttributeChanges(ASC, MaxAttribute, CB);
		}
	}
	RefreshFromASC();
}

void UDFAttributeBarWidget::OnAttributeChanged(const FOnAttributeChangeData& Data)
{
	(void)Data;
	RefreshFromASC();
}

void UDFAttributeBarWidget::RefreshFromASC()
{
	if (!IsValid(AttributeBar))
	{
		return;
	}
	UAbilitySystemComponent* const ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC) || !TrackedAttribute.IsValid())
	{
		AttributeBar->SetPercent(0.f);
		if (ValueText)
		{
			ValueText->SetText(FText::GetEmpty());
		}
		return;
	}
	const float Current = ASC->GetNumericAttribute(TrackedAttribute);
	const float MaxVal = MaxAttribute.IsValid() ? ASC->GetNumericAttribute(MaxAttribute) : 1.f;
	const float Pct = MaxVal > KINDA_SMALL_NUMBER ? FMath::Clamp(Current / MaxVal, 0.f, 1.f) : 0.f;
	AttributeBar->SetPercent(Pct);
	if (ValueText)
	{
		const int32 RoundedCurrent = FMath::RoundToInt(Current);
		if (MaxAttribute.IsValid())
		{
			const int32 RoundedMax = FMath::Max(1, FMath::RoundToInt(MaxVal));
			ValueText->SetText(FText::Format(
				NSLOCTEXT("DF", "AttrBarValue", "{0} / {1}"),
				FText::AsNumber(RoundedCurrent),
				FText::AsNumber(RoundedMax)));
		}
		else
		{
			ValueText->SetText(FText::AsNumber(RoundedCurrent));
		}
	}
}
