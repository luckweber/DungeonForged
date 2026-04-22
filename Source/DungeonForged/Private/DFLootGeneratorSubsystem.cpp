// Source/DungeonForged/Private/DFLootGeneratorSubsystem.cpp

#include "DFLootGeneratorSubsystem.h"
#include "Data/DFDataTableStructs.h"
#include "DFLootDrop.h"
#include "Engine/World.h"

namespace DungeonForgedLoot
{
	static constexpr float TwoPI = 6.28318530718f;
}

FName UDFLootGeneratorSubsystem::PickOneWeightedRow(
	const UDataTable* InItemDataTable, const TArray<FName>& InRowNames)
{
	if (!InItemDataTable || InRowNames.Num() == 0)
	{
		return NAME_None;
	}
	float Total = 0.f;
	TArray<float> Weights;
	Weights.Reserve(InRowNames.Num());
	for (const FName& N : InRowNames)
	{
		if (const FDFItemTableRow* R = InItemDataTable->FindRow<FDFItemTableRow>(N, TEXT("DFLoot|PickOne")))
		{
			float W = 1.f;
			switch (R->Rarity)
			{
			case EItemRarity::Uncommon: W = 0.7f; break;
			case EItemRarity::Rare: W = 0.45f; break;
			case EItemRarity::Epic: W = 0.25f; break;
			case EItemRarity::Legendary: W = 0.1f; break;
			case EItemRarity::Common:
			default: W = 1.f; break;
			}
			Total += W;
			Weights.Add(W);
		}
		else
		{
			Weights.Add(0.f);
		}
	}
	if (Total <= KINDA_SMALL_NUMBER)
	{
		return InRowNames[FMath::RandRange(0, InRowNames.Num() - 1)];
	}
	const float P = FMath::FRand() * Total;
	float Acc = 0.f;
	for (int32 I = 0; I < InRowNames.Num(); ++I)
	{
		Acc += Weights[I];
		if (P <= Acc)
		{
			return InRowNames[I];
		}
	}
	return InRowNames[InRowNames.Num() - 1];
}

float UDFLootGeneratorSubsystem::GetRarityMult(EItemRarity R) const
{
	switch (R)
	{
	case EItemRarity::Uncommon: return UncommonMultiplier;
	case EItemRarity::Rare: return RareMultiplier;
	case EItemRarity::Epic: return EpicMultiplier;
	case EItemRarity::Legendary: return LegendaryMultiplier;
	case EItemRarity::Common:
	default: return CommonMultiplier;
	}
}

void UDFLootGeneratorSubsystem::RollLoot(const FDFEnemyTableRow& EnemyData, FVector SpawnLocation, FVector Impulse)
{
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}
	if (World->GetNetMode() == NM_Client)
	{
		return;
	}
	if (!ItemDataTable)
	{
		return;
	}
	const TSubclassOf<ADFLootDrop> LClass = LootDropClass
		? LootDropClass
		: TSubclassOf<ADFLootDrop>(ADFLootDrop::StaticClass());
	int32 N = 0;
	for (const FName& RowName : EnemyData.LootTableRows)
	{
		const FDFItemTableRow* const Row = ItemDataTable->FindRow<FDFItemTableRow>(RowName, TEXT("DFLoot|Roll"));
		if (!Row)
		{
			continue;
		}
		const float RarityMult = GetRarityMult(Row->Rarity);
		const float P = FMath::Clamp(BaseDropChance * RarityMult, 0.f, 1.f);
		if (FMath::FRand() > P)
		{
			continue;
		}
		const float Ang = DungeonForgedLoot::TwoPI * (static_cast<float>(N) / FMath::Max(1, EnemyData.LootTableRows.Num() + 1));
		const float Rad = FMath::FRandRange(0.f, LootScatterRadius);
		const FVector Offset = FVector(FMath::Cos(Ang) * Rad, FMath::Sin(Ang) * Rad, 8.f);
		const FTransform T(FRotator::ZeroRotator, SpawnLocation + Offset);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		ADFLootDrop* const Drop = World->SpawnActor<ADFLootDrop>(LClass, T, Params);
		if (!Drop)
		{
			++N;
			continue;
		}
		const FVector J = Impulse.IsNearlyZero() ? FVector::UpVector * 200.f : Impulse;
		Drop->InitLoot(ItemDataTable, RowName, J, true);
		++N;
	}
}
