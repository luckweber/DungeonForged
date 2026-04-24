// Source/DungeonForged/Private/Debug/UDFDebugComponent.cpp

#include "Debug/UDFDebugComponent.h"
#include "DungeonForgedModule.h"

#if !UE_BUILD_SHIPPING
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Characters/ADFEnemyBase.h"
#include "Characters/ADFPlayerState.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GAS/UDFAttributeSet.h"
#include "Engine/Engine.h"

UDFDebugComponent::UDFDebugComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDFDebugComponent::DrawAttributeDebug() const
{
	APawn* const P = Cast<APawn>(GetOwner());
	if (!P || !GEngine)
	{
		return;
	}
	ADFPlayerState* const DPS = P->GetPlayerState<ADFPlayerState>();
	if (!DPS || !DPS->AbilitySystemComponent)
	{
		return;
	}
	UAbilitySystemComponent* const ASC = DPS->AbilitySystemComponent;
	int32 R = 20;
	GEngine->AddOnScreenDebugMessage(R++, 3.f, FColor::Cyan, TEXT("— Attributes —"));
	GEngine->AddOnScreenDebugMessage(
		R++, 3.f, FColor::White,
		*FString::Printf(TEXT("Health %.1f / %.1f"), ASC->GetNumericAttribute(UDFAttributeSet::GetHealthAttribute()),
			ASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute())));
	GEngine->AddOnScreenDebugMessage(
		R++, 3.f, FColor::White,
		*FString::Printf(TEXT("Mana %.1f / %.1f"), ASC->GetNumericAttribute(UDFAttributeSet::GetManaAttribute()),
			ASC->GetNumericAttribute(UDFAttributeSet::GetMaxManaAttribute())));
	GEngine->AddOnScreenDebugMessage(
		R++, 3.f, FColor::White,
		*FString::Printf(TEXT("Stamina %.1f / %.1f"), ASC->GetNumericAttribute(UDFAttributeSet::GetStaminaAttribute()),
			ASC->GetNumericAttribute(UDFAttributeSet::GetMaxStaminaAttribute())));
	GEngine->AddOnScreenDebugMessage(
		R++, 3.f, FColor::White,
		*FString::Printf(TEXT("Str %.1f  Int %.1f  Agi %.1f"), ASC->GetNumericAttribute(UDFAttributeSet::GetStrengthAttribute()),
			ASC->GetNumericAttribute(UDFAttributeSet::GetIntelligenceAttribute()),
			ASC->GetNumericAttribute(UDFAttributeSet::GetAgilityAttribute())));
	GEngine->AddOnScreenDebugMessage(
		R++, 3.f, FColor::White,
		*FString::Printf(TEXT("Armor %.1f  MR %.1f"), ASC->GetNumericAttribute(UDFAttributeSet::GetArmorAttribute()),
			ASC->GetNumericAttribute(UDFAttributeSet::GetMagicResistAttribute())));
	GEngine->AddOnScreenDebugMessage(
		R++, 3.f, FColor::White,
		*FString::Printf(TEXT("Crit %.2f  CDR %.2f"), ASC->GetNumericAttribute(UDFAttributeSet::GetCritChanceAttribute()),
			ASC->GetNumericAttribute(UDFAttributeSet::GetCooldownReductionAttribute())));
}

void UDFDebugComponent::DrawAbilityDebug() const
{
	APawn* const P = Cast<APawn>(GetOwner());
	if (!P || !GEngine)
	{
		return;
	}
	ADFPlayerState* const DPS = P->GetPlayerState<ADFPlayerState>();
	if (!DPS || !DPS->AbilitySystemComponent)
	{
		return;
	}
	UAbilitySystemComponent* const ASC = DPS->AbilitySystemComponent;
	int32 R = 40;
	GEngine->AddOnScreenDebugMessage(R++, 3.f, FColor::Green, TEXT("— Activatable abilities —"));
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		const FString Nm = Spec.Ability ? Spec.Ability->GetClass()->GetName() : TEXT("(null)");
		GEngine->AddOnScreenDebugMessage(
			R++, 3.f, FColor::Yellow, FString::Printf(TEXT("%s  Lv %d"), *Nm, Spec.Level));
	}
}

void UDFDebugComponent::DrawAIDebug(const float Radius) const
{
	UWorld* const W = GetWorld();
	AActor* const O = GetOwner();
	if (!W || !O)
	{
		return;
	}
	const FVector C = O->GetActorLocation();
	for (TActorIterator<ADFEnemyBase> It(W); It; ++It)
	{
		ADFEnemyBase* const E = *It;
		if (!E || FVector::Dist(C, E->GetActorLocation()) > Radius)
		{
			continue;
		}
		const FVector L = E->GetActorLocation();
		DrawDebugSphere(W, L, 48.f, 12, FColor::Magenta, false, 0.12f, 0, 2.f);
		if (AAIController* const AI = Cast<AAIController>(E->GetController()))
		{
			FString Line = AI->GetClass()->GetName();
			if (UBlackboardComponent* const B = AI->GetBlackboardComponent())
			{
				if (const UBlackboardData* const D = B->GetBlackboardAsset())
				{
					Line += TEXT(" | ");
					for (FBlackboardEntry const& Ent : D->Keys)
					{
						if (B->GetKeyID(Ent.EntryName) == FBlackboard::InvalidKey)
						{
							continue;
						}
						Line += Ent.EntryName.ToString();
						Line += TEXT("=");
						if (AActor* const Tgt = Cast<AActor>(B->GetValueAsObject(Ent.EntryName)))
						{
							Line += Tgt->GetName();
						}
						else
						{
							Line += TEXT("...");
						}
						Line += TEXT(" ");
					}
				}
			}
			DrawDebugString(W, L + FVector(0, 0, 90.f), Line, nullptr, FColor::White, 0.12f, true, 1.f);
		}
	}
}

void UDFDebugComponent::LogGASEvent(const FString& Event) const
{
	DF_LOG(Log, "GAS | %s", *Event);
}

#else

UDFDebugComponent::UDFDebugComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

#endif
