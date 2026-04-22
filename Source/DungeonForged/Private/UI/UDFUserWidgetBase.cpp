// Source/DungeonForged/Private/UI/UDFUserWidgetBase.cpp
#include "UI/UDFUserWidgetBase.h"
#include "AbilitySystemComponent.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Characters/ADFPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

ADFPlayerCharacter* UDFUserWidgetBase::GetDFPlayerCharacter() const
{
	return GetOwningPlayerPawn<ADFPlayerCharacter>();
}

ADFPlayerState* UDFUserWidgetBase::GetDFPlayerState() const
{
	if (APlayerController* const PC = GetOwningPlayer())
	{
		return PC->GetPlayerState<ADFPlayerState>();
	}
	return nullptr;
}

UAbilitySystemComponent* UDFUserWidgetBase::GetAbilitySystemComponent() const
{
	if (const ADFPlayerState* const PS = GetDFPlayerState())
	{
		return PS->GetAbilitySystemComponent();
	}
	// Fall back: cached ASC on character if PS not ready
	if (const ADFPlayerCharacter* const Ch = GetDFPlayerCharacter())
	{
		return Ch->GetAbilitySystemComponent();
	}
	return nullptr;
}

FDelegateHandle UDFUserWidgetBase::BindToAttributeChanges(
	UAbilitySystemComponent* InASC,
	const FGameplayAttribute& InAttribute,
	TDelegate<void(const FOnAttributeChangeData&)> InCallback)
{
	if (!IsValid(InASC) || !InAttribute.IsValid() || !InCallback.IsBound())
	{
		return FDelegateHandle();
	}

	const FDelegateHandle H =
		InASC->GetGameplayAttributeValueChangeDelegate(InAttribute).Add(InCallback);
	AttributeChangeBindings.Add(FDFAttributeChangeBinding{InASC, InAttribute, H});
	return H;
}

void UDFUserWidgetBase::UnbindAllAttributeChanges()
{
	for (FDFAttributeChangeBinding& B : AttributeChangeBindings)
	{
		if (B.ASC.IsValid() && B.Handle.IsValid())
		{
			B.ASC->GetGameplayAttributeValueChangeDelegate(B.Attribute).Remove(B.Handle);
		}
	}
	AttributeChangeBindings.Empty();
}

void UDFUserWidgetBase::NativeDestruct()
{
	UnbindAllAttributeChanges();
	Super::NativeDestruct();
}
