// Source/DungeonForged/Private/UI/Status/UDFStatusLibrary.cpp
#include "UI/Status/UDFStatusLibrary.h"
#include "GAS/DFGameplayTags.h"

namespace DFStatusDisplay
{
	bool GBuiltInsReady = false;
	TMap<FGameplayTag, FDFStatusEffectDisplayData> GBuiltIns;

	void AddBuiltIn(
		const FGameplayTag& Tag,
		const FLinearColor& Border,
		const FText& Name,
		const FText& Desc,
		bool bDebuff,
		bool bShowDuration,
		int32 Severity)
	{
		FDFStatusEffectDisplayData Row;
		Row.EffectTag = Tag;
		Row.BorderColor = Border;
		Row.DisplayName = Name;
		Row.Description = Desc;
		Row.bIsDebuff = bDebuff;
		Row.bShowDuration = bShowDuration;
		Row.DebuffSeverity = Severity;
		GBuiltIns.Add(Tag, Row);
	}

	void BuildBuiltInsIfNeeded()
	{
		if (GBuiltInsReady)
		{
			return;
		}
		GBuiltInsReady = true;
		FDFGameplayTags::RegisterGameplayTags();

		const FText DescNone = FText::GetEmpty();

		// Buffs (gold/green/orange borders per design doc)
		AddBuiltIn(
			FDFGameplayTags::Effect_Buff_Speed,
			FLinearColor(0.15f, 0.85f, 0.25f),
			NSLOCTEXT("DFStatus", "BuffSpeed", "Haste"),
			NSLOCTEXT("DFStatus", "BuffSpeedDesc", "Increased movement or attack speed."),
			false,
			true,
			20);
		AddBuiltIn(
			FDFGameplayTags::Effect_Buff_DamageUp,
			FLinearColor(1.f, 0.55f, 0.1f),
			NSLOCTEXT("DFStatus", "BuffDmg", "Damage Up"),
			NSLOCTEXT("DFStatus", "BuffDmgDesc", "Increased damage dealt."),
			false,
			true,
			25);
		AddBuiltIn(
			FDFGameplayTags::Effect_Buff_Shield,
			FLinearColor(0.3f, 0.65f, 1.f),
			NSLOCTEXT("DFStatus", "BuffShield", "Shield"),
			NSLOCTEXT("DFStatus", "BuffShieldDesc", "Absorption shield."),
			false,
			true,
			30);
		AddBuiltIn(
			FDFGameplayTags::Effect_Buff_BattleHymn,
			FLinearColor(0.95f, 0.8f, 0.2f),
			NSLOCTEXT("DFStatus", "BuffBattleHymn", "Battle Hymn"),
			NSLOCTEXT("DFStatus", "BuffBattleHymnDesc", "Combat empowerment window."),
			false,
			true,
			28);
		AddBuiltIn(
			FDFGameplayTags::Effect_Buff_LevelStatScaling,
			FLinearColor(0.6f, 0.85f, 0.4f),
			NSLOCTEXT("DFStatus", "BuffLevelScaling", "Level Scaling"),
			NSLOCTEXT("DFStatus", "BuffLevelScalingDesc", "Passive level-based stat growth."),
			false,
			false,
			5);
		AddBuiltIn(
			FDFGameplayTags::Buff_Mage_TimeWarpHaste,
			FLinearColor(0.5f, 0.35f, 1.f),
			NSLOCTEXT("DFStatus", "BuffTimeWarp", "Time Warp"),
			NSLOCTEXT("DFStatus", "BuffTimeWarpDesc", "Haste from Time Warp."),
			false,
			true,
			40);
		AddBuiltIn(
			FDFGameplayTags::Buff_Rogue_Ambush,
			FLinearColor(0.8f, 0.3f, 0.9f),
			NSLOCTEXT("DFStatus", "BuffAmbush", "Ambush"),
			NSLOCTEXT("DFStatus", "BuffAmbushDesc", "Rogue ambush window."),
			false,
			true,
			22);
		AddBuiltIn(
			FDFGameplayTags::Buff_Rogue_KillingSpree,
			FLinearColor(1.f, 0.35f, 0.15f),
			NSLOCTEXT("DFStatus", "BuffKillingSpree", "Killing Spree"),
			NSLOCTEXT("DFStatus", "BuffKillingSpreeDesc", "Escalating damage on kills."),
			false,
			true,
			35);

		// Debuffs
		AddBuiltIn(
			FDFGameplayTags::Effect_Debuff_Slow,
			FLinearColor(0.25f, 0.45f, 1.f),
			NSLOCTEXT("DFStatus", "DebuffSlow", "Slowed"),
			NSLOCTEXT("DFStatus", "DebuffSlowDesc", "Reduced movement speed."),
			true,
			true,
			55);
		AddBuiltIn(
			FDFGameplayTags::Effect_Debuff_Weaken,
			FLinearColor(0.85f, 0.35f, 0.35f),
			NSLOCTEXT("DFStatus", "DebuffWeaken", "Weakened"),
			NSLOCTEXT("DFStatus", "DebuffWeakenDesc", "Reduced outgoing damage or stats."),
			true,
			true,
			60);
		AddBuiltIn(
			FDFGameplayTags::Effect_Debuff_ArmorBreak,
			FLinearColor(0.9f, 0.5f, 0.2f),
			NSLOCTEXT("DFStatus", "DebuffArmorBreak", "Armor Broken"),
			NSLOCTEXT("DFStatus", "DebuffArmorBreakDesc", "Reduced mitigation."),
			true,
			true,
			65);
		AddBuiltIn(
			FDFGameplayTags::Effect_Debuff_Blinded,
			FLinearColor(0.55f, 0.55f, 0.55f),
			NSLOCTEXT("DFStatus", "DebuffBlinded", "Blinded"),
			NSLOCTEXT("DFStatus", "DebuffBlindedDesc", "Severely reduced accuracy."),
			true,
			true,
			70);
		AddBuiltIn(
			FDFGameplayTags::Effect_Debuff_Terrified,
			FLinearColor(0.45f, 0.15f, 0.65f),
			NSLOCTEXT("DFStatus", "DebuffTerrified", "Terrified"),
			NSLOCTEXT("DFStatus", "DebuffTerrifiedDesc", "Fear: reduced control."),
			true,
			true,
			80);

		// States / CC
		AddBuiltIn(
			FDFGameplayTags::State_Stunned,
			FLinearColor(1.f, 0.92f, 0.2f),
			NSLOCTEXT("DFStatus", "StateStun", "Stunned"),
			NSLOCTEXT("DFStatus", "StateStunDesc", "Cannot act."),
			true,
			true,
			100);
		AddBuiltIn(
			FDFGameplayTags::State_Berserk,
			FLinearColor(0.95f, 0.15f, 0.15f),
			NSLOCTEXT("DFStatus", "StateBerserk", "Berserk"),
			NSLOCTEXT("DFStatus", "StateBerserkDesc", "High risk / high damage stance."),
			false,
			true,
			45);
		AddBuiltIn(
			FDFGameplayTags::State_ManaShieldActive,
			FLinearColor(0.25f, 0.55f, 1.f),
			NSLOCTEXT("DFStatus", "StateManaShield", "Mana Shield"),
			NSLOCTEXT("DFStatus", "StateManaShieldDesc", "Shield absorbing damage using mana."),
			false,
			true,
			50);
		AddBuiltIn(
			FDFGameplayTags::State_Invisible,
			FLinearColor(0.55f, 0.55f, 0.6f),
			NSLOCTEXT("DFStatus", "StateInvisible", "Invisible"),
			NSLOCTEXT("DFStatus", "StateInvisibleDesc", "Harder to detect."),
			false,
			true,
			30);
		AddBuiltIn(
			FDFGameplayTags::State_Stealthed,
			FLinearColor(0.45f, 0.45f, 0.55f),
			NSLOCTEXT("DFStatus", "StateStealth", "Stealth"),
			NSLOCTEXT("DFStatus", "StateStealthDesc", "Stealthed — reduced threat."),
			false,
			true,
			32);
		AddBuiltIn(
			FDFGameplayTags::State_Rooted,
			FLinearColor(0.7f, 0.45f, 1.f),
			NSLOCTEXT("DFStatus", "StateRoot", "Rooted"),
			NSLOCTEXT("DFStatus", "StateRootDesc", "Cannot move."),
			true,
			true,
			95);
		AddBuiltIn(
			FDFGameplayTags::State_Silenced,
			FLinearColor(0.55f, 0.3f, 0.85f),
			NSLOCTEXT("DFStatus", "StateSilence", "Silenced"),
			NSLOCTEXT("DFStatus", "StateSilenceDesc", "Cannot use abilities."),
			true,
			true,
			90);

		// DoTs
		AddBuiltIn(
			FDFGameplayTags::Effect_DoT_Fire,
			FLinearColor(1.f, 0.45f, 0.1f),
			NSLOCTEXT("DFStatus", "DoTFire", "Burning"),
			NSLOCTEXT("DFStatus", "DoTFireDesc", "Fire damage over time."),
			true,
			true,
			75);
		AddBuiltIn(
			FDFGameplayTags::Effect_DoT_Poison,
			FLinearColor(0.2f, 0.75f, 0.25f),
			NSLOCTEXT("DFStatus", "DoTPoison", "Poisoned"),
			NSLOCTEXT("DFStatus", "DoTPoisonDesc", "Poison damage over time."),
			true,
			true,
			72);
		AddBuiltIn(
			FDFGameplayTags::Effect_DoT_Bleed,
			FLinearColor(0.85f, 0.1f, 0.1f),
			NSLOCTEXT("DFStatus", "DoTBleed", "Bleeding"),
			NSLOCTEXT("DFStatus", "DoTBleedDesc", "Physical DoT."),
			true,
			true,
			78);
		AddBuiltIn(
			FDFGameplayTags::Effect_DoT_Frost,
			FLinearColor(0.4f, 0.85f, 1.f),
			NSLOCTEXT("DFStatus", "DoTFrost", "Frostbitten"),
			NSLOCTEXT("DFStatus", "DoTFrostDesc", "Frost DoT or shatter mark."),
			true,
			true,
			68);
		(void)DescNone;
	}

	void EnsureBuiltInTable()
	{
		BuildBuiltInsIfNeeded();
	}
} // namespace DFStatusDisplay

const FDFStatusEffectDisplayData* UDFStatusLibrary::GetStatusData(
	const FGameplayTag Tag,
	const UDFStatusLibrary* const OptionalLibrary)
{
	if (OptionalLibrary)
	{
		if (const FDFStatusEffectDisplayData* const Row = OptionalLibrary->StatusByTag.Find(Tag))
		{
			return Row;
		}
	}
	DFStatusDisplay::BuildBuiltInsIfNeeded();
	if (FDFStatusEffectDisplayData* const Found = DFStatusDisplay::GBuiltIns.Find(Tag))
	{
		return Found;
	}
	return nullptr;
}

FDFStatusEffectDisplayData UDFStatusLibrary::GetStatusDataValue(const FGameplayTag Tag) const
{
	if (const FDFStatusEffectDisplayData* const P = GetStatusData(Tag, this))
	{
		return *P;
	}
	return FDFStatusEffectDisplayData();
}

void UDFStatusLibrary::CollectAllStatusRootTags(
	const UDFStatusLibrary* const OptionalLibrary,
	FGameplayTagContainer& OutTags)
{
	DFStatusDisplay::BuildBuiltInsIfNeeded();
	for (const TPair<FGameplayTag, FDFStatusEffectDisplayData>& P : DFStatusDisplay::GBuiltIns)
	{
		if (P.Key.IsValid())
		{
			OutTags.AddTag(P.Key);
		}
	}
	if (OptionalLibrary)
	{
		for (const TPair<FGameplayTag, FDFStatusEffectDisplayData>& P : OptionalLibrary->StatusByTag)
		{
			if (P.Key.IsValid())
			{
				OutTags.AddTag(P.Key);
			}
		}
	}
}
