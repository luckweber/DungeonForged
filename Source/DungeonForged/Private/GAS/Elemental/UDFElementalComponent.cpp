// Source/DungeonForged/Private/GAS/Elemental/UDFElementalComponent.cpp
#include "GAS/Elemental/UDFElementalComponent.h"
#include "GAS/Elemental/UDFElementalReactionSubsystem.h"
#include "Engine/World.h"
#include "GameplayEffect.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UDFElementalComponent)

UDFElementalComponent::UDFElementalComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDFElementalComponent::InitFromTable(UDataTable* const Table, const FName RowName)
{
	if (!Table || RowName.IsNone())
	{
		return;
	}
	if (const FDFElementalAffinityRow* R = Table->FindRow<FDFElementalAffinityRow>(RowName, TEXT("UDFElementalComponent::InitFromTable"), false))
	{
		AffinityData = *R;
	}
}

float UDFElementalComponent::GetResistance(const EDFElementType VsElement) const
{
	return AffinityData.GetResistance(VsElement);
}

void UDFElementalComponent::OnElementalHit(FGameplayEffectSpecHandle& Spec, const EDFElementType IncomingElement, AActor* Instigator)
{
	if (!GetWorld() || !GetOwner())
	{
		return;
	}
	if (UDFElementalReactionSubsystem* const Sub = GetWorld()->GetSubsystem<UDFElementalReactionSubsystem>())
	{
		Sub->ApplyElementalDamage(Spec, IncomingElement, Instigator, GetOwner(), &AffinityData);
	}
}

void UDFElementalComponent::SetCurrentReactionTag(const FGameplayTag Tag)
{
	CurrentReactionTag = Tag;
}
