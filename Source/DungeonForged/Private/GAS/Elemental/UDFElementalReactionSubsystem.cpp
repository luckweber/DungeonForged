// Source/DungeonForged/Private/GAS/Elemental/UDFElementalReactionSubsystem.cpp
#include "GAS/Elemental/UDFElementalReactionSubsystem.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Elemental/UDFElementalComponent.h"
#include "GAS/Elemental/UDFElementalLibrary.h"
#include "UI/Combat/DFCombatTextTypes.h"
#include "UI/Combat/UDFCombatTextSubsystem.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UDFElementalReactionSubsystem)

namespace
{
	void TryApplyGe(UAbilitySystemComponent* const TargetAsc, const TSubclassOf<UGameplayEffect> Class, AActor* const Instigator)
	{
		if (!TargetAsc || !Class || !TargetAsc->GetOwner() || !TargetAsc->GetOwner()->HasAuthority())
		{
			return;
		}
		FGameplayEffectContextHandle Ctx = TargetAsc->MakeEffectContext();
		if (Instigator)
		{
			Ctx.AddInstigator(Instigator, Instigator);
		}
		const FGameplayEffectSpecHandle S = TargetAsc->MakeOutgoingSpec(Class, 1.f, Ctx);
		if (S.IsValid() && S.Data.IsValid())
		{
			TargetAsc->ApplyGameplayEffectSpecToSelf(*S.Data);
		}
	}

	void TryDataTableRow(
		AActor* const Target,
		UAbilitySystemComponent* const ASC,
		UDFElementalComponent* const Elem,
		const EDFElementType IncomingElement,
		const EDFElementalRuntimeReaction BuiltIn,
		UDFElementalReactionSubsystem const* Sub)
	{
		if (!Target || !ASC)
		{
			return;
		}
		const FDFElementalAffinityRow* Row = Elem ? &Elem->GetAffinityData() : nullptr;
		if (!Row)
		{
			return;
		}
		const FGameplayTag TableTag = Row->GetReactionForIncoming(IncomingElement);
		if (!TableTag.IsValid())
		{
			return;
		}
		if (BuiltIn != EDFElementalRuntimeReaction::None
			&& (TableTag == FDFGameplayTags::Effect_Reaction_Melt || TableTag == FDFGameplayTags::Effect_Reaction_Steam
				|| TableTag == FDFGameplayTags::Effect_Reaction_Electrocute))
		{
			return;
		}
		ASC->AddLooseGameplayTag(TableTag, 1);
		if (Elem)
		{
			Elem->SetCurrentReactionTag(TableTag);
		}
		if (BuiltIn == EDFElementalRuntimeReaction::None && Sub)
		{
			Sub->TrySpawnReactionVFX(EDFElementalRuntimeReaction::TableDriven, Target);
		}
	}
}

float UDFElementalReactionSubsystem::GetDamageMultiplier(
	const EDFElementType AttackElement,
	const EDFElementType TargetPrimaryElement,
	const FDFElementalAffinityRow& TargetData) const
{
	if (AttackElement == EDFElementType::None)
	{
		return 1.f;
	}
	if (AttackElement == EDFElementType::ElementTrue)
	{
		return 1.f;
	}
	const float Matrix = UDFElementalLibrary::GetAdvantageMultiplier(AttackElement, TargetPrimaryElement);
	if (AttackElement == EDFElementType::Arcane)
	{
		return Matrix;
	}
	return Matrix * TargetData.GetResistance(AttackElement);
}

EDFElementalRuntimeReaction UDFElementalReactionSubsystem::CheckElementalReaction(
	AActor* const Target,
	const EDFElementType IncomingElement,
	AActor* const Instigator)
{
	if (!Target)
	{
		return EDFElementalRuntimeReaction::None;
	}
	UAbilitySystemComponent* const ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!ASC)
	{
		return EDFElementalRuntimeReaction::None;
	}
	FGameplayTagContainer Owned;
	ASC->GetOwnedGameplayTags(Owned);
	UDFElementalComponent* const Elem = Target->FindComponentByClass<UDFElementalComponent>();

	if (IncomingElement == EDFElementType::Fire && Owned.HasTag(FDFGameplayTags::Effect_DoT_Frost))
	{
		{
			FGameplayTagContainer Rm;
			Rm.AddTag(FDFGameplayTags::Effect_DoT_Frost);
			ASC->RemoveActiveEffectsWithAppliedTags(Rm);
		}
		ASC->AddLooseGameplayTag(FDFGameplayTags::Effect_Reaction_Melt, 1);
		TryApplyGe(ASC, GameplayEffect_Melt, Instigator);
		if (Elem)
		{
			Elem->SetCurrentReactionTag(FDFGameplayTags::Effect_Reaction_Melt);
		}
		TrySpawnReactionVFX(EDFElementalRuntimeReaction::Melt, Target);
		TryDataTableRow(Target, ASC, Elem, IncomingElement, EDFElementalRuntimeReaction::Melt, this);
		return EDFElementalRuntimeReaction::Melt;
	}

	if (IncomingElement == EDFElementType::Lightning
		&& (Owned.HasTag(FDFGameplayTags::State_Elemental_Wet) || Owned.HasTag(FDFGameplayTags::Effect_Debuff_Slow)))
	{
		ASC->AddLooseGameplayTag(FDFGameplayTags::Effect_Reaction_Electrocute, 1);
		TryApplyGe(ASC, GameplayEffect_Electrocute, Instigator);
		if (Elem)
		{
			Elem->SetCurrentReactionTag(FDFGameplayTags::Effect_Reaction_Electrocute);
		}
		TrySpawnReactionVFX(EDFElementalRuntimeReaction::Electrocute, Target);
		TryDataTableRow(Target, ASC, Elem, IncomingElement, EDFElementalRuntimeReaction::Electrocute, this);
		return EDFElementalRuntimeReaction::Electrocute;
	}

	if (IncomingElement == EDFElementType::Ice && Owned.HasTag(FDFGameplayTags::Effect_DoT_Fire))
	{
		ASC->AddLooseGameplayTag(FDFGameplayTags::Effect_Reaction_Steam, 1);
		TryApplyGe(ASC, GameplayEffect_Steam, Instigator);
		if (Elem)
		{
			Elem->SetCurrentReactionTag(FDFGameplayTags::Effect_Reaction_Steam);
		}
		TrySpawnReactionVFX(EDFElementalRuntimeReaction::Steam, Target);
		TryDataTableRow(Target, ASC, Elem, IncomingElement, EDFElementalRuntimeReaction::Steam, this);
		return EDFElementalRuntimeReaction::Steam;
	}

	TryDataTableRow(Target, ASC, Elem, IncomingElement, EDFElementalRuntimeReaction::None, this);
	return EDFElementalRuntimeReaction::None;
}

void UDFElementalReactionSubsystem::TrySpawnReactionVFX(const EDFElementalRuntimeReaction R, AActor* const Target) const
{
	if (!GetWorld() || !Target || IsRunningDedicatedServer())
	{
		return;
	}
	const FVector Loc = Target->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	UNiagaraSystem* Sys = nullptr;
	switch (R)
	{
		case EDFElementalRuntimeReaction::Melt: Sys = VFX_ReactionMelt; break;
		case EDFElementalRuntimeReaction::Electrocute: Sys = VFX_ReactionElectrocute; break;
		case EDFElementalRuntimeReaction::Steam: Sys = VFX_ReactionSteam; break;
		case EDFElementalRuntimeReaction::TableDriven: Sys = VFX_ReactionFromTable; break;
		default: break;
	}
	if (Sys)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			const_cast<UDFElementalReactionSubsystem*>(this), Sys, Loc, FRotator::ZeroRotator, FVector(1.f),
			true, true, ENCPoolMethod::AutoRelease, true);
	}
}

void UDFElementalReactionSubsystem::ApplyElementalDamage(
	FGameplayEffectSpecHandle& Spec,
	const EDFElementType Element,
	AActor* const Instigator,
	AActor* const Target,
	const FDFElementalAffinityRow* const OptionalTargetRow)
{
	if (!Target || !Spec.IsValid() || !Spec.Data.IsValid())
	{
		return;
	}
	const FDFElementalAffinityRow* Row = OptionalTargetRow;
	const UDFElementalComponent* ElemComp = Target->FindComponentByClass<UDFElementalComponent>();
	if (Row == nullptr && ElemComp)
	{
		Row = &ElemComp->GetAffinityData();
	}
	EDFElementType DefPrimary = EDFElementType::None;
	if (Row)
	{
		DefPrimary = Row->PrimaryElement;
	}
	else if (ElemComp)
	{
		DefPrimary = ElemComp->GetPrimaryElement();
	}
	const FDFElementalAffinityRow WorkRow = Row ? *Row : FDFElementalAffinityRow();
	const float Mult = GetDamageMultiplier(Element, DefPrimary, WorkRow);

	if (FDFGameplayTags::Data_Damage.IsValid())
	{
		const float Dmg = Spec.Data->GetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, false, 0.f);
		Spec.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Damage, Dmg * Mult);
	}
	const FGameplayTag ElTag = UDFElementalLibrary::GetElementEffectTag(Element);
	if (ElTag.IsValid())
	{
		Spec.Data->DynamicGrantedTags.AddTag(ElTag);
	}
	const EDFElementalRuntimeReaction R = CheckElementalReaction(Target, Element, Instigator);

	if (!IsRunningDedicatedServer() && (FMath::Abs(Mult - 1.f) > KINDA_SMALL_NUMBER || R != EDFElementalRuntimeReaction::None))
	{
		if (UWorld* const W = GetWorld())
		{
			if (UDFCombatTextSubsystem* const Ctx = W->GetSubsystem<UDFCombatTextSubsystem>())
			{
				const FVector Base = Target->GetActorLocation() + FVector(0.f, 0.f, 90.f);
				if (Mult > 1.f + KINDA_SMALL_NUMBER)
				{
					Ctx->SpawnTextString(Base + FVector(0.f, 0.f, 30.f), TEXT("WEAK!"), ECombatTextType::Elemental_Weak);
				}
				else if (Mult < 1.f - KINDA_SMALL_NUMBER)
				{
					Ctx->SpawnTextString(Base + FVector(0.f, 0.f, 30.f), TEXT("RESIST"), ECombatTextType::Elemental_Resist);
				}
				if (R == EDFElementalRuntimeReaction::Melt)
				{
					Ctx->SpawnTextString(
						Base + FVector(0.f, 0.f, 55.f), TEXT("MELT!"), ECombatTextType::Elemental_Reaction, 1.4f);
				}
				else if (R == EDFElementalRuntimeReaction::Electrocute)
				{
					Ctx->SpawnTextString(
						Base + FVector(0.f, 0.f, 55.f), TEXT("ELECTROCUTED!"), ECombatTextType::Elemental_Reaction, 1.4f);
				}
				else if (R == EDFElementalRuntimeReaction::Steam)
				{
					Ctx->SpawnTextString(
						Base + FVector(0.f, 0.f, 55.f), TEXT("STEAM!"), ECombatTextType::Elemental_Reaction, 1.4f);
				}
			}
		}
	}
}
