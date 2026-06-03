// Source/DungeonForged/Private/GAS/Elemental/UDFElementalLibrary.cpp
#include "GAS/Elemental/UDFElementalLibrary.h"
#include "GAS/DFGameplayTags.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

EDFElementType UDFElementalLibrary::GetElementFromEffectTag(const FGameplayTag Tag)
{
	if (!Tag.IsValid())
	{
		return EDFElementType::None;
	}
	if (Tag == FDFGameplayTags::Effect_Element_Fire) return EDFElementType::Fire;
	if (Tag == FDFGameplayTags::Effect_Element_Ice) return EDFElementType::Ice;
	if (Tag == FDFGameplayTags::Effect_Element_Water) return EDFElementType::Water;
	if (Tag == FDFGameplayTags::Effect_Element_Lightning) return EDFElementType::Lightning;
	if (Tag == FDFGameplayTags::Effect_Element_Earth) return EDFElementType::Earth;
	if (Tag == FDFGameplayTags::Effect_Element_Arcane) return EDFElementType::Arcane;
	if (Tag == FDFGameplayTags::Effect_Element_Physical) return EDFElementType::Physical;
	if (Tag == FDFGameplayTags::Effect_Element_True) return EDFElementType::ElementTrue;
	return EDFElementType::None;
}

namespace
{
	bool TagContainerHasElementTag(const FGameplayTagContainer& Tags, EDFElementType& OutElement)
	{
		static const FGameplayTag ElementTags[] = {
			FDFGameplayTags::Effect_Element_Fire,
			FDFGameplayTags::Effect_Element_Ice,
			FDFGameplayTags::Effect_Element_Water,
			FDFGameplayTags::Effect_Element_Lightning,
			FDFGameplayTags::Effect_Element_Earth,
			FDFGameplayTags::Effect_Element_Arcane,
			FDFGameplayTags::Effect_Element_Physical,
			FDFGameplayTags::Effect_Element_True,
		};
		for (const FGameplayTag& ElTag : ElementTags)
		{
			if (ElTag.IsValid() && Tags.HasTag(ElTag))
			{
				OutElement = UDFElementalLibrary::GetElementFromEffectTag(ElTag);
				return OutElement != EDFElementType::None;
			}
		}
		return false;
	}

	void CollectInferenceTags(const FGameplayEffectSpec& Spec, FGameplayTagContainer& OutTags)
	{
		OutTags.Reset();
		OutTags.AppendTags(Spec.GetDynamicAssetTags());
		if (Spec.Def)
		{
			OutTags.AppendTags(Spec.Def->GetAssetTags());
		}
		if (const FGameplayEffectContext* const Ctx = Spec.GetContext().Get())
		{
		if (const UGameplayAbility* const Ability = Ctx->GetAbility())
		{
			OutTags.AppendTags(Ability->AbilityTags);
		}
		}
	}
}

EDFElementType UDFElementalLibrary::InferElementFromGameplayTags(const FGameplayTagContainer& Tags)
{
	EDFElementType Direct = EDFElementType::None;
	if (TagContainerHasElementTag(Tags, Direct))
	{
		return Direct;
	}

	auto MatchesParent = [&Tags](const FGameplayTag& Parent) -> bool
	{
		if (!Parent.IsValid())
		{
			return false;
		}
		for (const FGameplayTag& Tag : Tags)
		{
			if (Tag.MatchesTag(Parent))
			{
				return true;
			}
		}
		return false;
	};

	if (MatchesParent(FDFGameplayTags::Ability_Fire))
	{
		return EDFElementType::Fire;
	}
	if (MatchesParent(FDFGameplayTags::Ability_Ice))
	{
		return EDFElementType::Ice;
	}
	if (MatchesParent(FDFGameplayTags::Ability_Universal_CallLightning))
	{
		return EDFElementType::Lightning;
	}
	if (Tags.HasTag(FDFGameplayTags::Ability_Mage_FrostBolt)
		|| Tags.HasTag(FDFGameplayTags::Ability_Mage_BlizzardStorm)
		|| Tags.HasTag(FDFGameplayTags::Ability_Ice_FrostBolt)
		|| Tags.HasTag(FDFGameplayTags::Ability_Ice_Blizzard))
	{
		return EDFElementType::Ice;
	}
	if (Tags.HasTag(FDFGameplayTags::Ability_Mage_ArcaneBarrage)
		|| Tags.HasTag(FDFGameplayTags::Ability_Universal_Siphon)
		|| Tags.HasTag(FDFGameplayTags::Ability_Passive_Mage_ArcaneMastery))
	{
		return EDFElementType::Arcane;
	}

	return EDFElementType::None;
}

EDFElementType UDFElementalLibrary::ResolveElementFromEffectSpec(const FGameplayEffectSpec& Spec)
{
	static const FGameplayTag ElementTags[] = {
		FDFGameplayTags::Effect_Element_Fire,
		FDFGameplayTags::Effect_Element_Ice,
		FDFGameplayTags::Effect_Element_Water,
		FDFGameplayTags::Effect_Element_Lightning,
		FDFGameplayTags::Effect_Element_Earth,
		FDFGameplayTags::Effect_Element_Arcane,
		FDFGameplayTags::Effect_Element_Physical,
		FDFGameplayTags::Effect_Element_True,
	};

	FGameplayTagContainer Combined = Spec.GetDynamicAssetTags();
	if (Spec.Def)
	{
		Combined.AppendTags(Spec.Def->GetAssetTags());
	}
	for (const FGameplayTag& ElTag : ElementTags)
	{
		if (ElTag.IsValid() && Combined.HasTag(ElTag))
		{
			return GetElementFromEffectTag(ElTag);
		}
	}

	FGameplayTagContainer InferenceTags;
	CollectInferenceTags(Spec, InferenceTags);
	if (const EDFElementType FromAbility = InferElementFromGameplayTags(InferenceTags);
		FromAbility != EDFElementType::None)
	{
		return FromAbility;
	}

	if (Combined.HasTag(FDFGameplayTags::Effect_Damage_True))
	{
		return EDFElementType::ElementTrue;
	}
	if (Combined.HasTag(FDFGameplayTags::Effect_Damage_Physical))
	{
		return EDFElementType::Physical;
	}
	if (Combined.HasTag(FDFGameplayTags::Effect_Damage_Magic))
	{
		return EDFElementType::Arcane;
	}
	return EDFElementType::None;
}

void UDFElementalLibrary::StampDefaultElementOnDamageSpec(
	FGameplayEffectSpec& Spec,
	const EDFElementType ExplicitElement /*= EDFElementType::None*/)
{
	for (const FGameplayTag& Tag : Spec.GetDynamicAssetTags())
	{
		if (GetElementFromEffectTag(Tag) != EDFElementType::None)
		{
			return;
		}
	}

	const EDFElementType Element = ExplicitElement != EDFElementType::None
		? ExplicitElement
		: ResolveElementFromEffectSpec(Spec);
	if (Element == EDFElementType::None)
	{
		return;
	}

	const FGameplayTag ElTag = GetElementEffectTag(Element);
	if (ElTag.IsValid())
	{
		Spec.AddDynamicAssetTag(ElTag);
	}
}

FActiveGameplayEffectHandle UDFElementalLibrary::ApplyOutgoingDamageSpecToTarget(
	UAbilitySystemComponent* const SourceASC,
	UAbilitySystemComponent* const TargetASC,
	FGameplayEffectSpec& Spec)
{
	if (!SourceASC || !TargetASC)
	{
		return FActiveGameplayEffectHandle();
	}
	StampDefaultElementOnDamageSpec(Spec, EDFElementType::None);
	return SourceASC->ApplyGameplayEffectSpecToTarget(Spec, TargetASC);
}

FGameplayTag UDFElementalLibrary::GetElementEffectTag(EDFElementType Element)
{
	switch (Element)
	{
		case EDFElementType::Fire: return FDFGameplayTags::Effect_Element_Fire;
		case EDFElementType::Ice: return FDFGameplayTags::Effect_Element_Ice;
		case EDFElementType::Water: return FDFGameplayTags::Effect_Element_Water;
		case EDFElementType::Lightning: return FDFGameplayTags::Effect_Element_Lightning;
		case EDFElementType::Earth: return FDFGameplayTags::Effect_Element_Earth;
		case EDFElementType::Arcane: return FDFGameplayTags::Effect_Element_Arcane;
		case EDFElementType::Physical: return FDFGameplayTags::Effect_Element_Physical;
		case EDFElementType::ElementTrue: return FDFGameplayTags::Effect_Element_True;
		default: return FGameplayTag();
	}
}

namespace
{
	constexpr float Strong = 1.5f;
	constexpr float Weak = 0.7f;
	constexpr float Neut = 1.f;

	bool Index(const EDFElementType A, const EDFElementType D, int32& OutA, int32& OutD)
	{
		const int32 IA = static_cast<int32>(A);
		const int32 ID = static_cast<int32>(D);
		if (IA <= 0 || ID <= 0 || IA >= static_cast<int32>(EDFElementType::MAX) || ID >= static_cast<int32>(EDFElementType::MAX))
		{
			return false;
		}
		OutA = IA;
		OutD = ID;
		return true;
	}
}

float UDFElementalLibrary::GetAdvantageMultiplier(const EDFElementType AttackElement, const EDFElementType DefenderPrimaryElement)
{
	int32 A = 0, D = 0;
	if (!Index(AttackElement, DefenderPrimaryElement, A, D))
	{
		return Neut;
	}
	// [Attack][Defender] = multiplier. Order of enum: None(0) unused in matrix rows 1..8
	static float M[9][9];
	static bool bInit = false;
	if (!bInit)
	{
		bInit = true;
		for (int32 I = 0; I < 9; ++I)
		{
			for (int32 J = 0; J < 9; ++J)
			{
				M[I][J] = Neut;
			}
		}
		const int32 F = static_cast<int32>(EDFElementType::Fire);
		const int32 Ic = static_cast<int32>(EDFElementType::Ice);
		const int32 W = static_cast<int32>(EDFElementType::Water);
		const int32 L = static_cast<int32>(EDFElementType::Lightning);
		const int32 E = static_cast<int32>(EDFElementType::Earth);

		// Fire
		M[F][Ic] = Strong;
		M[F][W] = Weak;
		M[F][E] = Weak;
		// Ice
		M[Ic][E] = Strong;
		M[Ic][F] = Weak;
		// Lightning
		M[L][W] = Strong;
		M[L][E] = Weak;
		// Earth
		M[E][L] = Strong;
		M[E][Ic] = Weak;
	}
	return M[A][D];
}

FText UDFElementalLibrary::GetElementGlyph(const EDFElementType Element)
{
	// Kept as text so UMG can render; swap for Texture2D in WBP.
	switch (Element)
	{
		case EDFElementType::Fire: return FText::FromString(TEXT("\U0001F525"));
		case EDFElementType::Ice: return FText::FromString(TEXT("\U00002744\U0000FE0F"));
		case EDFElementType::Water: return FText::FromString(TEXT("\U0001F4A7"));
		case EDFElementType::Lightning: return FText::FromString(TEXT("\U000026A1"));
		case EDFElementType::Earth: return FText::FromString(TEXT("\U0001FAA8"));
		case EDFElementType::Arcane: return FText::FromString(TEXT("\u2728"));
		case EDFElementType::Physical: return FText::FromString(TEXT("\U0001F4A5"));
		case EDFElementType::ElementTrue: return FText::FromString(TEXT("\u2605"));
		default: return FText::GetEmpty();
	}
}

FLinearColor UDFElementalLibrary::GetElementColor(const EDFElementType Element)
{
	switch (Element)
	{
		case EDFElementType::Fire: return FLinearColor(1.f, 0.35f, 0.1f, 1.f);
		case EDFElementType::Ice: return FLinearColor(0.4f, 0.8f, 1.f, 1.f);
		case EDFElementType::Water: return FLinearColor(0.2f, 0.5f, 1.f, 1.f);
		case EDFElementType::Lightning: return FLinearColor(1.f, 1.f, 0.2f, 1.f);
		case EDFElementType::Earth: return FLinearColor(0.6f, 0.4f, 0.2f, 1.f);
		case EDFElementType::Arcane: return FLinearColor(0.7f, 0.2f, 1.f, 1.f);
		case EDFElementType::Physical: return FLinearColor(0.9f, 0.9f, 0.9f, 1.f);
		case EDFElementType::ElementTrue: return FLinearColor(1.f, 0.2f, 0.2f, 1.f);
		default: return FLinearColor::White;
	}
}
