// Source/DungeonForged/Private/Combat/UDFStaminaExhaustionComponent.cpp
#include "Combat/UDFStaminaExhaustionComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Data/UDFCombatTuningData.h"
#include "DFAssetManager.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "Engine/World.h"
#include "TimerManager.h"

UDFStaminaExhaustionComponent::UDFStaminaExhaustionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDFStaminaExhaustionComponent::BeginPlay()
{
	Super::BeginPlay();
	if (IAbilitySystemInterface* const ASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		if (UAbilitySystemComponent* const ASC = ASI->GetAbilitySystemComponent())
		{
			StaminaDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(UDFAttributeSet::GetStaminaAttribute())
				.AddUObject(this, &UDFStaminaExhaustionComponent::OnStaminaChanged);
		}
	}
}

void UDFStaminaExhaustionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IAbilitySystemInterface* const ASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		if (UAbilitySystemComponent* const ASC = ASI->GetAbilitySystemComponent())
		{
			if (StaminaDelegateHandle.IsValid())
			{
				ASC->GetGameplayAttributeValueChangeDelegate(UDFAttributeSet::GetStaminaAttribute())
					.Remove(StaminaDelegateHandle);
				StaminaDelegateHandle.Reset();
			}
		}
	}
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ExhaustTimerHandle);
	}
	ClearExhausted();
	Super::EndPlay(EndPlayReason);
}

void UDFStaminaExhaustionComponent::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
	const UDFCombatTuningData* const Tuning = UDFAssetManager::Get().GetCombatTuningData();
	const float Threshold = Tuning ? Tuning->ExhaustedStaminaThreshold : 10.f;
	if (Data.NewValue >= Threshold)
	{
		return;
	}
	IAbilitySystemInterface* const ASI = Cast<IAbilitySystemInterface>(GetOwner());
	UAbilitySystemComponent* const ASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
	if (!ASC || !FDFGameplayTags::State_Exhausted.IsValid())
	{
		return;
	}
	if (ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Exhausted))
	{
		return;
	}
	ASC->AddLooseGameplayTag(FDFGameplayTags::State_Exhausted, 1);
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}
	const float Duration = Tuning ? Tuning->ExhaustedLockDuration : 0.5f;
	World->GetTimerManager().ClearTimer(ExhaustTimerHandle);
	World->GetTimerManager().SetTimer(
		ExhaustTimerHandle, this, &UDFStaminaExhaustionComponent::ClearExhausted, Duration, false);
}

void UDFStaminaExhaustionComponent::ClearExhausted()
{
	if (IAbilitySystemInterface* const ASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		if (UAbilitySystemComponent* const ASC = ASI->GetAbilitySystemComponent())
		{
			if (FDFGameplayTags::State_Exhausted.IsValid())
			{
				ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Exhausted, 1);
			}
		}
	}
}
