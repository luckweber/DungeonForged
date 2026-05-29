// Source/DungeonForged/Private/AI/UDFEnemyArchetypeLibrary.cpp
#include "AI/UDFEnemyArchetypeLibrary.h"
#include "GAS/DFGameplayTags.h"

bool UDFEnemyArchetypeLibrary::PrefersRangedCombat(const EDFEnemyArchetype Archetype)
{
	switch (Archetype)
	{
	case EDFEnemyArchetype::Caster:
	case EDFEnemyArchetype::Sniper:
	case EDFEnemyArchetype::Healer:
		return true;
	default:
		return false;
	}
}

float UDFEnemyArchetypeLibrary::GetFleeEnterHealthFraction(const EDFEnemyArchetype Archetype)
{
	switch (Archetype)
	{
	case EDFEnemyArchetype::Tank:
	case EDFEnemyArchetype::Shielder:
		return 0.12f;
	case EDFEnemyArchetype::Berserker:
	case EDFEnemyArchetype::Grunt:
		return 0.10f;
	case EDFEnemyArchetype::Skirmisher:
	case EDFEnemyArchetype::Bomber:
		return 0.32f;
	case EDFEnemyArchetype::Caster:
	case EDFEnemyArchetype::Sniper:
	case EDFEnemyArchetype::Healer:
	case EDFEnemyArchetype::Spawner:
		return 0.40f;
	default:
		return 0.20f;
	}
}

float UDFEnemyArchetypeLibrary::GetFleeReturnHealthFraction(const EDFEnemyArchetype Archetype)
{
	switch (Archetype)
	{
	case EDFEnemyArchetype::Tank:
	case EDFEnemyArchetype::Shielder:
		return 0.48f;
	case EDFEnemyArchetype::Berserker:
	case EDFEnemyArchetype::Grunt:
		return 0.55f;
	case EDFEnemyArchetype::Skirmisher:
	case EDFEnemyArchetype::Bomber:
		return 0.62f;
	case EDFEnemyArchetype::Caster:
	case EDFEnemyArchetype::Sniper:
	case EDFEnemyArchetype::Healer:
	case EDFEnemyArchetype::Spawner:
		return 0.72f;
	default:
		return 0.60f;
	}
}

float UDFEnemyArchetypeLibrary::GetPreferredInRangeDistance(
	const EDFEnemyArchetype Archetype,
	const float MeleeRange,
	const float RangedRange)
{
	if (PrefersRangedCombat(Archetype))
	{
		return FMath::Max(100.f, RangedRange);
	}
	switch (Archetype)
	{
	case EDFEnemyArchetype::Skirmisher:
		return FMath::Max(100.f, MeleeRange * 1.15f);
	case EDFEnemyArchetype::Bomber:
		return FMath::Max(100.f, MeleeRange * 1.25f);
	default:
		return FMath::Max(100.f, MeleeRange);
	}
}

int32 UDFEnemyArchetypeLibrary::GetMeleeAttackTokenPriority(const EDFEnemyArchetype Archetype)
{
	switch (Archetype)
	{
	case EDFEnemyArchetype::Tank: return 100;
	case EDFEnemyArchetype::Shielder: return 95;
	case EDFEnemyArchetype::Berserker: return 85;
	case EDFEnemyArchetype::Grunt: return 65;
	case EDFEnemyArchetype::Skirmisher: return 60;
	case EDFEnemyArchetype::Bomber: return 55;
	case EDFEnemyArchetype::Caster: return 35;
	case EDFEnemyArchetype::Sniper: return 30;
	case EDFEnemyArchetype::Healer: return 25;
	case EDFEnemyArchetype::Spawner: return 20;
	default: return 50;
	}
}

int32 UDFEnemyArchetypeLibrary::GetRangedCastTokenPriority(const EDFEnemyArchetype Archetype)
{
	switch (Archetype)
	{
	case EDFEnemyArchetype::Sniper: return 100;
	case EDFEnemyArchetype::Caster: return 90;
	case EDFEnemyArchetype::Healer: return 75;
	case EDFEnemyArchetype::Bomber: return 60;
	case EDFEnemyArchetype::Spawner: return 50;
	default: return 25;
	}
}

int32 UDFEnemyArchetypeLibrary::GetTelegraphPriority(const EDFEnemyArchetype Archetype, const EEnemyTier Tier)
{
	int32 Priority = 40;
	switch (Archetype)
	{
	case EDFEnemyArchetype::Tank:
	case EDFEnemyArchetype::Shielder:
		Priority = 70;
		break;
	case EDFEnemyArchetype::Berserker:
		Priority = 80;
		break;
	case EDFEnemyArchetype::Caster:
	case EDFEnemyArchetype::Sniper:
		Priority = 85;
		break;
	case EDFEnemyArchetype::Bomber:
		Priority = 75;
		break;
	default:
		Priority = 55;
		break;
	}
	if (Tier == EEnemyTier::Elite)
	{
		Priority += 15;
	}
	else if (Tier == EEnemyTier::Boss)
	{
		Priority += 40;
	}
	return Priority;
}

float UDFEnemyArchetypeLibrary::GetMovementSpeedScale(const EDFEnemyArchetype Archetype)
{
	switch (Archetype)
	{
	case EDFEnemyArchetype::Skirmisher: return 1.15f;
	case EDFEnemyArchetype::Berserker: return 1.08f;
	case EDFEnemyArchetype::Tank:
	case EDFEnemyArchetype::Shielder:
		return 0.88f;
	case EDFEnemyArchetype::Caster:
	case EDFEnemyArchetype::Healer:
		return 0.92f;
	default:
		return 1.f;
	}
}

float UDFEnemyArchetypeLibrary::GetFleeSampleDistance(const EDFEnemyArchetype Archetype)
{
	switch (Archetype)
	{
	case EDFEnemyArchetype::Skirmisher:
	case EDFEnemyArchetype::Bomber:
		return 1200.f;
	case EDFEnemyArchetype::Caster:
	case EDFEnemyArchetype::Sniper:
	case EDFEnemyArchetype::Healer:
	case EDFEnemyArchetype::Spawner:
		return 1600.f;
	case EDFEnemyArchetype::Tank:
	case EDFEnemyArchetype::Shielder:
		return 650.f;
	case EDFEnemyArchetype::Berserker:
		return 900.f;
	default:
		return 800.f;
	}
}

FGameplayTagContainer UDFEnemyArchetypeLibrary::GetDefaultGrantedAbilityTags(const EDFEnemyArchetype Archetype)
{
	FGameplayTagContainer Tags;
	if (PrefersRangedCombat(Archetype))
	{
		if (FDFGameplayTags::Ability_Attack_Ranged.IsValid())
		{
			Tags.AddTag(FDFGameplayTags::Ability_Attack_Ranged);
		}
	}
	else if (FDFGameplayTags::Ability_Attack_Melee.IsValid())
	{
		Tags.AddTag(FDFGameplayTags::Ability_Attack_Melee);
	}
	return Tags;
}

void UDFEnemyArchetypeLibrary::ApplyArchetypeRangeDefaults(
	const EDFEnemyArchetype Archetype,
	float& InOutMeleeRange,
	float& InOutRangedRange,
	float& InOutAttackRange)
{
	if (InOutMeleeRange <= KINDA_SMALL_NUMBER)
	{
		switch (Archetype)
		{
		case EDFEnemyArchetype::Tank:
		case EDFEnemyArchetype::Shielder:
			InOutMeleeRange = 240.f;
			break;
		case EDFEnemyArchetype::Skirmisher:
			InOutMeleeRange = 180.f;
			break;
		default:
			InOutMeleeRange = 200.f;
			break;
		}
	}
	if (InOutRangedRange <= KINDA_SMALL_NUMBER)
	{
		switch (Archetype)
		{
		case EDFEnemyArchetype::Sniper:
			InOutRangedRange = 3200.f;
			break;
		case EDFEnemyArchetype::Caster:
		case EDFEnemyArchetype::Healer:
			InOutRangedRange = 2600.f;
			break;
		default:
			InOutRangedRange = 2000.f;
			break;
		}
	}
	if (InOutAttackRange <= KINDA_SMALL_NUMBER)
	{
		InOutAttackRange = PrefersRangedCombat(Archetype)
			? InOutRangedRange * 0.9f
			: FMath::Max(InOutMeleeRange * 2.5f, 600.f);
	}
}
