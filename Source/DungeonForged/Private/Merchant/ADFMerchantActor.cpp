// Source/DungeonForged/Private/Merchant/ADFMerchantActor.cpp
#include "Merchant/ADFMerchantActor.h"
#include "Merchant/DFMerchantData.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Data/DFDataTableStructs.h"
#include "DFInventoryComponent.h"
#include "Interaction/DFInteractable.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Run/DFRunManager.h"

ADFMerchantActor::ADFMerchantActor()
{
	bSingleUse = false;
	SetReplicateMovement(false);
	if (Mesh)
	{
		Mesh->SetVisibility(false, false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	MerchantMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MerchantMesh"));
	MerchantMesh->SetupAttachment(GetRootComponent());
	MerchantMesh->SetCanEverAffectNavigation(false);
}

void ADFMerchantActor::BeginPlay()
{
	Super::BeginPlay();
	if (MerchantMesh && IdleAnimation)
	{
		if (UAnimInstance* const AI = MerchantMesh->GetAnimInstance())
		{
			AI->Montage_Play(IdleAnimation, 1.f);
		}
	}
	if (!HasAuthority())
	{
		return;
	}
	if (UDFRunManager* const RM = ResolveRunManager())
	{
		RM->OnRunFloorChanged.AddDynamic(this, &ADFMerchantActor::HandleRunFloorChanged);
	}
	GenerateStock();
}

void ADFMerchantActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		if (UWorld* const W = GetWorld())
		{
			if (UGameInstance* const GI = W->GetGameInstance())
			{
				if (UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>())
				{
					RM->OnRunFloorChanged.RemoveDynamic(this, &ADFMerchantActor::HandleRunFloorChanged);
				}
			}
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ADFMerchantActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADFMerchantActor, CurrentStock);
	DOREPLIFETIME(ADFMerchantActor, RerollCount);
	DOREPLIFETIME(ADFMerchantActor, RerollBaseCost);
}

FText ADFMerchantActor::GetInteractionText_Implementation() const
{
	return MerchantDisplayName.IsEmpty()
		? NSLOCTEXT("DF", "MerchantInteract", "Open shop")
		: FText::Format(NSLOCTEXT("DF", "MerchantInteractFmt", "{0}"), MerchantDisplayName);
}

void ADFMerchantActor::Interact_Implementation(ACharacter* Interactor)
{
	if (!HasAuthority() || !Interactor || !bIsInteractable)
	{
		return;
	}
	if (!IDFInteractable::Execute_CanInteract(this, Interactor))
	{
		return;
	}
	PlayInteractEffects(Interactor);
	ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(Interactor);
	if (PC && PC->GetController())
	{
		PC->ClientOpenMerchantShop(this);
	}
}

UDFRunManager* ADFMerchantActor::ResolveRunManager() const
{
	UWorld* const W = GetWorld();
	if (!W)
	{
		return nullptr;
	}
	UGameInstance* const GI = W->GetGameInstance();
	return GI ? GI->GetSubsystem<UDFRunManager>() : nullptr;
}

UDataTable* ADFMerchantActor::ResolveItemTable() const
{
	if (ItemDataTableOverride)
	{
		return ItemDataTableOverride;
	}
	if (UDFRunManager* const RM = ResolveRunManager())
	{
		return RM->ItemDataTable.Get();
	}
	return nullptr;
}

bool ADFMerchantActor::AuthoritySpendGold(ADFPlayerCharacter* /*Buyer*/, int32 Amount)
{
	UDFRunManager* const RM = ResolveRunManager();
	if (!RM)
	{
		return false;
	}
	return RM->SpendGold(Amount);
}

bool ADFMerchantActor::IsBuyerInInteractionRange(ADFPlayerCharacter* Buyer) const
{
	if (!Buyer)
	{
		return false;
	}
	const float Range = IDFInteractable::Execute_GetInteractionRange(this) + 50.f;
	return FVector::Dist(GetActorLocation(), Buyer->GetActorLocation()) <= Range;
}

int32 ADFMerchantActor::RarityToPickWeightIndex(const EItemRarity R)
{
	return static_cast<int32>(R);
}

void ADFMerchantActor::GenerateStock()
{
	if (!HasAuthority())
	{
		return;
	}
	CurrentStock.Reset();
	RestockTemplateQuantities.Reset();
	UDataTable* const MerchTable = MerchantStockTable;
	UDataTable* const Items = ResolveItemTable();
	if (!MerchTable || !Items)
	{
		OnStockChanged.Broadcast();
		return;
	}
	const UDFRunManager* const RM = ResolveRunManager();
	const int32 CurrentFloor = RM ? FMath::Max(1, RM->GetRunStateCopy().CurrentFloor) : 1;

	struct FCandidate
	{
		FName TableRowKey;
		FDFMerchantStockRow Row;
	};
	TArray<FCandidate> Pool;
	MerchTable->ForeachRow<FDFMerchantStockRow>(
		TEXT("ADFMerchantActor::GenerateStock"),
		[&](const FName& Key, const FDFMerchantStockRow& R) {
			if (R.ItemRowName.IsNone() || CurrentFloor < R.MinFloorAvailable)
			{
				return;
			}
			if (const FDFItemTableRow* const I = Items->FindRow<FDFItemTableRow>(R.ItemRowName, TEXT(""), false))
			{
				if (static_cast<uint8>(I->Rarity) < static_cast<uint8>(R.MinRarity))
				{
					return;
				}
			}
			FCandidate C;
			C.TableRowKey = Key;
			C.Row = R;
			Pool.Add(C);
		});
	const int32 Want = FMath::Max(0, StockSlots);
	int32 Picked = 0;
	while (Picked < Want && Pool.Num() > 0)
	{
		float TotalW = 0.f;
		for (const FCandidate& C : Pool)
		{
			if (const FDFItemTableRow* const I = Items->FindRow<FDFItemTableRow>(C.Row.ItemRowName, TEXT(""), false))
			{
				const int32 RIdx = RarityToPickWeightIndex(I->Rarity);
				TotalW += 1.f / (1.f + FMath::Max(0, RIdx));
			}
		}
		if (TotalW <= KINDA_SMALL_NUMBER)
		{
			break;
		}
		const float Rnd = FMath::FRandRange(0.f, TotalW);
		float Acc = 0.f;
		int32 Chosen = 0;
		for (int32 i = 0; i < Pool.Num(); ++i)
		{
			if (const FDFItemTableRow* const I = Items->FindRow<FDFItemTableRow>(Pool[i].Row.ItemRowName, TEXT(""), false))
			{
				const int32 RIdx = RarityToPickWeightIndex(I->Rarity);
				Acc += 1.f / (1.f + FMath::Max(0, RIdx));
			}
			Chosen = i;
			if (Rnd < Acc)
			{
				break;
			}
		}
		const FCandidate Win = Pool[Chosen];
		Pool.RemoveAt(Chosen);
		++Picked;
		const FDFItemTableRow* const IRow = Items->FindRow<FDFItemTableRow>(Win.Row.ItemRowName, TEXT(""), false);
		float Mult;
		if (Win.Row.PriceVariance > KINDA_SMALL_NUMBER)
		{
			const float V = FMath::Clamp(Win.Row.PriceVariance, 0.01f, 0.99f);
			Mult = FMath::FRandRange(1.f - V, 1.f + V);
		}
		else
		{
			Mult = FMath::FRandRange(0.8f, 1.2f);
		}
		const int32 UnitPrice = FMath::Max(0, FMath::RoundToInt(Win.Row.BasePrice * Mult));
		FDFMerchantStockEntry E;
		E.MerchantStockRowKey = Win.TableRowKey;
		E.ItemRowName = Win.Row.ItemRowName;
		E.UnitPrice = UnitPrice;
		E.bIsConsumable = Win.Row.bIsConsumable;
		if (Win.Row.StockQuantity < 0)
		{
			E.Quantity = -1;
			E.bUnlimited = true;
		}
		else
		{
			E.Quantity = FMath::Max(0, Win.Row.StockQuantity);
			E.bUnlimited = false;
		}
		CurrentStock.Add(E);
		RestockTemplateQuantities.FindOrAdd(Win.TableRowKey) = Win.Row.StockQuantity;
	}
	OnStockChanged.Broadcast();
}

int32 ADFMerchantActor::GetNextRerollCost() const
{
	if (RerollBaseCost < 1)
	{
		return 0;
	}
	const int32 Exp = FMath::Clamp(RerollCount, 0, 24);
	const int64 Power = (int64(1) << Exp);
	const int64 Cost64 = (int64)RerollBaseCost * Power;
	return int32(FMath::Min(Cost64, (int64)0x7fffffffLL));
}

bool ADFMerchantActor::RerollStock(ADFPlayerCharacter* Buyer)
{
	if (!HasAuthority() || !Buyer)
	{
		return false;
	}
	if (!IsBuyerInInteractionRange(Buyer))
	{
		return false;
	}
	const int32 Cost = GetNextRerollCost();
	if (Cost < 1)
	{
		return false;
	}
	if (!AuthoritySpendGold(Buyer, Cost))
	{
		return false;
	}
	++RerollCount;
	GenerateStock();
	return true;
}

bool ADFMerchantActor::PurchaseItem(int32 SlotIndex, ADFPlayerCharacter* Buyer)
{
	if (!HasAuthority() || !Buyer)
	{
		return false;
	}
	if (!CurrentStock.IsValidIndex(SlotIndex))
	{
		return false;
	}
	if (!IsBuyerInInteractionRange(Buyer))
	{
		return false;
	}
	FDFMerchantStockEntry& E = CurrentStock[SlotIndex];
	if (!E.bUnlimited && E.Quantity < 1)
	{
		return false;
	}
	UDFInventoryComponent* const Inv = Buyer->FindComponentByClass<UDFInventoryComponent>();
	if (!Inv)
	{
		return false;
	}
	const int32 Price = E.UnitPrice;
	if (Price < 0 || !AuthoritySpendGold(Buyer, Price))
	{
		return false;
	}
	if (!Inv->AddItem(E.ItemRowName, 1))
	{
		if (UDFRunManager* const RM = ResolveRunManager())
		{
			RM->AddRunGold(Price);
		}
		return false;
	}
	const FName BoughtName = E.ItemRowName;
	if (!E.bUnlimited)
	{
		--E.Quantity;
		if (E.Quantity < 1)
		{
			CurrentStock.RemoveAt(SlotIndex);
		}
	}
	OnPurchaseComplete.Broadcast(Buyer, BoughtName, SlotIndex);
	Buyer->ClientNotifyMerchantPurchase(SlotIndex);
	OnStockChanged.Broadcast();
	return true;
}

void ADFMerchantActor::RestockConsumablesFromTable()
{
	if (!HasAuthority())
	{
		return;
	}
	for (FDFMerchantStockEntry& E : CurrentStock)
	{
		if (!E.bIsConsumable)
		{
			continue;
		}
		if (E.bUnlimited)
		{
			continue;
		}
		if (int32* const T = RestockTemplateQuantities.Find(E.MerchantStockRowKey))
		{
			if (*T >= 0)
			{
				E.Quantity = *T;
			}
		}
	}
	OnStockChanged.Broadcast();
}

void ADFMerchantActor::OnRep_CurrentStock()
{
	OnStockChanged.Broadcast();
}

void ADFMerchantActor::HandleRunFloorChanged(int32 /*NewFloor*/)
{
	RestockConsumablesFromTable();
}
