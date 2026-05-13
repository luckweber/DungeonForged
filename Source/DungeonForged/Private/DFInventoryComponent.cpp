// Source/DungeonForged/Private/DFInventoryComponent.cpp

#include "DFInventoryComponent.h"
#include "Engine/Engine.h"
#include "Data/DFDataTableStructs.h"
#include "UI/Combat/UDFCombatTextSubsystem.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "DFLootDrop.h"
#include "DungeonForgedModule.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

namespace
{
void TryGoldCombatText(AActor* const Owner, const FDFItemTableRow& Row, const int32 Gained)
{
	if (Gained < 1 || Row.ItemType != EItemType::Currency)
	{
		return;
	}
	if (IsRunningDedicatedServer() || !Owner)
	{
		return;
	}
	if (UWorld* const W = Owner->GetWorld())
	{
		if (UDFCombatTextSubsystem* const Ctx = W->GetSubsystem<UDFCombatTextSubsystem>())
		{
			const FVector L = Owner->GetActorLocation() + FVector(0.f, 0.f, 90.f);
			Ctx->SpawnText(L, static_cast<float>(Gained), ECombatTextType::GoldGain);
		}
	}
}
} // namespace

UDFInventoryComponent::UDFInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UDFInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureAuthorityBagGridSized();
}

void UDFInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UDFInventoryComponent, MaxSlots, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UDFInventoryComponent, MaxCarryWeight, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UDFInventoryComponent, ItemDataTable, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UDFInventoryComponent, Items, COND_OwnerOnly);
}

void UDFInventoryComponent::OnRep_Items()
{
	OnInventoryChanged.Broadcast();
}

UAbilitySystemComponent* UDFInventoryComponent::ResolveOwnerASC() const
{
	if (const IAbilitySystemInterface* const I = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		return I->GetAbilitySystemComponent();
	}
	if (const AActor* const O = GetOwner())
	{
		if (const APawn* const P = Cast<APawn>(O))
		{
			if (APlayerState* const PS = P->GetPlayerState())
			{
				if (IAbilitySystemInterface* const I2 = Cast<IAbilitySystemInterface>(PS))
				{
					return I2->GetAbilitySystemComponent();
				}
			}
		}
	}
	return nullptr;
}

bool UDFInventoryComponent::IsAuthority() const
{
	return GetOwner() && GetOwner()->HasAuthority();
}

const FDFItemTableRow* UDFInventoryComponent::GetItemData(const FName RowName) const
{
	if (!ItemDataTable || RowName.IsNone())
	{
		return nullptr;
	}
	return ItemDataTable->FindRow<FDFItemTableRow>(RowName, TEXT("UDFInventory|GetItemData"));
}

float UDFInventoryComponent::ComputeStackContributionWeight(const FDFInventorySlot& Slot) const
{
	if (Slot.RowName.IsNone() || Slot.Quantity < 1)
	{
		return 0.f;
	}
	if (const FDFItemTableRow* const Meta = GetItemData(Slot.RowName))
	{
		return Meta->ItemWeight * static_cast<float>(Slot.Quantity);
	}
	return 0.f;
}

float UDFInventoryComponent::GetCurrentCarriedWeight() const
{
	float Total = 0.f;
	for (const FDFInventorySlot& S : Items)
	{
		Total += ComputeStackContributionWeight(S);
	}
	return Total;
}

float UDFInventoryComponent::GetRemainingCarryCapacity() const
{
	if (!IsCarryWeightLimited())
	{
		return 1.e12f;
	}
	return FMath::Max(0.f, MaxCarryWeight - GetCurrentCarriedWeight());
}

bool UDFInventoryComponent::WouldRejectAddDueToWeight(const FName RowName, const int32 Quantity) const
{
	if (!IsCarryWeightLimited() || RowName.IsNone() || Quantity < 1)
	{
		return false;
	}
	if (const FDFItemTableRow* const Meta = GetItemData(RowName))
	{
		return !CanAffordCarryWeightDelta(Meta->ItemWeight * static_cast<float>(Quantity));
	}
	return false;
}

bool UDFInventoryComponent::CanAffordCarryWeightDelta(const float DeltaWeight) const
{
	if (!IsCarryWeightLimited())
	{
		return true;
	}
	return GetCurrentCarriedWeight() + DeltaWeight <= MaxCarryWeight + KINDA_SMALL_NUMBER;
}

void UDFInventoryComponent::EnsureAuthorityBagGridSized()
{
	if (!IsAuthority() || MaxSlots < 1)
	{
		return;
	}
	while (Items.Num() < MaxSlots)
	{
		FDFInventorySlot E;
		E.RowName     = NAME_None;
		E.Quantity    = 0;
		E.bIsEquipped = false;
		Items.Add(E);
	}
}

bool UDFInventoryComponent::ReceiveUnequippedItemAtBagIndex(
	const int32 BagIndex,
	const FName RowName,
	const int32 Quantity)
{
	if (!IsAuthority() || RowName.IsNone() || Quantity < 1)
	{
		DF_LOG(Verbose,
			"[DF|Bag|UnequipCell] SKIP not authority/bad qty owner=%s bag=%d row=%s q=%d",
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
			BagIndex,
			*RowName.ToString(),
			Quantity);
		return false;
	}
	EnsureAuthorityBagGridSized();
	if (!Items.IsValidIndex(BagIndex))
	{
		DF_LOG(Verbose,
			"[DF|Bag|UnequipCell] SKIP bad index owner=%s bagIdx=%d (items=%d max=%d) row=%s",
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
			BagIndex,
			Items.Num(),
			MaxSlots,
			*RowName.ToString());
		return false;
	}

	const FDFItemTableRow* const RowMeta = GetItemData(RowName);
	if (!RowMeta)
	{
		DF_LOG(Verbose,
			"[DF|Bag|UnequipCell] SKIP unknown DT row owner=%s bag=%d row=%s",
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
			BagIndex,
			*RowName.ToString());
		return false;
	}

	const int32 MaxStack = FMath::Max(1, RowMeta->MaxStack);
	FDFInventorySlot& Cell = Items[BagIndex];

	if (Cell.RowName.IsNone() || Cell.Quantity < 1)
	{
		if (WouldRejectAddDueToWeight(RowName, Quantity))
		{
			DF_LOG(Verbose,
				"[DF|Bag|UnequipCell] SKIP encumbrance owner=%s bag=%d %s x%d (current=%.1f max=%.1f)",
				GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
				BagIndex,
				*RowName.ToString(),
				Quantity,
				GetCurrentCarriedWeight(),
				MaxCarryWeight);
			return false;
		}
		Cell.RowName     = RowName;
		Cell.Quantity    = Quantity;
		Cell.bIsEquipped = false;
		OnInventoryChanged.Broadcast();
		DF_LOG(Verbose,
			"[DF|Bag|UnequipCell] EMPTY->place owner=%s bag=%d %s x%d",
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
			BagIndex,
			*RowName.ToString(),
			Quantity);
		return true;
	}

	if (Cell.RowName == RowName)
	{
		const int32 Room = MaxStack - Cell.Quantity;
		if (Room >= Quantity)
		{
			if (WouldRejectAddDueToWeight(RowName, Quantity))
			{
				DF_LOG(Verbose,
					"[DF|Bag|UnequipCell] SKIP encumbrance merge owner=%s bag=%d %s +%d (current=%.1f max=%.1f)",
					GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
					BagIndex,
					*RowName.ToString(),
					Quantity,
					GetCurrentCarriedWeight(),
					MaxCarryWeight);
				return false;
			}
			const int32 PrevQty = Cell.Quantity;
			Cell.Quantity += Quantity;
			Cell.bIsEquipped = false;
			OnInventoryChanged.Broadcast();
			DF_LOG(Verbose,
				"[DF|Bag|UnequipCell] MERGE owner=%s bag=%d %s qty %d + %d (maxstack=%d)",
				GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
				BagIndex,
				*RowName.ToString(),
				PrevQty,
				Quantity,
				MaxStack);
			return true;
		}
	}

	// Displace: snapshot before UnequipItem — it clears GAS / bIsEquipped in-place; we need the
	// original equipped flag to restore if re-homing the displaced stack fails.
	const FDFInventorySlot DisplacedSnapshot = Items[BagIndex];
	const float WeightDeltaForReplace =
		RowMeta->ItemWeight * static_cast<float>(Quantity) -
		ComputeStackContributionWeight(DisplacedSnapshot);
	if (IsCarryWeightLimited() && !CanAffordCarryWeightDelta(WeightDeltaForReplace))
	{
		DF_LOG(Verbose,
			"[DF|Bag|UnequipCell] SKIP encumbrance displace owner=%s bag=%d incoming %s x%d (Δw=%.2f cur=%.1f max=%.1f)",
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
			BagIndex,
			*RowName.ToString(),
			Quantity,
			WeightDeltaForReplace,
			GetCurrentCarriedWeight(),
			MaxCarryWeight);
		return false;
	}

	if (Cell.bIsEquipped)
	{
		UnequipItem(BagIndex);
	}

	Items[BagIndex].RowName     = RowName;
	Items[BagIndex].Quantity    = Quantity;
	Items[BagIndex].bIsEquipped = false;
	EquipHandles.Remove(BagIndex);

	if (DisplacedSnapshot.RowName.IsNone() || DisplacedSnapshot.Quantity < 1)
	{
		OnInventoryChanged.Broadcast();
		DF_LOG(Verbose,
			"[DF|Bag|UnequipCell] DISPLACE(no displaced) owner=%s bag=%d %s x%d",
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
			BagIndex,
			*RowName.ToString(),
			Quantity);
		return true;
	}

	if (!AddItem(DisplacedSnapshot.RowName, DisplacedSnapshot.Quantity))
	{
		Items[BagIndex] = DisplacedSnapshot;
		EquipHandles.Remove(BagIndex);
		if (DisplacedSnapshot.bIsEquipped)
		{
			FDFInventorySlot& R = Items[BagIndex];
			if (!R.RowName.IsNone() && R.Quantity >= 1 && GetItemData(R.RowName))
			{
				R.bIsEquipped = false;
				EquipItem(BagIndex);
			}
		}
		OnInventoryChanged.Broadcast();
		DF_LOG(Verbose,
			"[DF|Bag|UnequipCell] DISPLACE REVERT owner=%s bag=%d new=%s (could not AddItem displaced %s x%d)",
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
			BagIndex,
			*RowName.ToString(),
			*DisplacedSnapshot.RowName.ToString(),
			DisplacedSnapshot.Quantity);
		return false;
	}

	OnInventoryChanged.Broadcast();
	DF_LOG(Verbose,
		"[DF|Bag|UnequipCell] DISPLACE+REHOME OK owner=%s bag=%d placed %s x%d | AddItem rehome %s x%d",
		GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
		BagIndex,
		*RowName.ToString(),
		Quantity,
		*DisplacedSnapshot.RowName.ToString(),
		DisplacedSnapshot.Quantity);
	return true;
}

namespace
{
float PredictInventoryGridWeight(
	const TArray<FDFInventorySlot>& Grid,
	const UDFInventoryComponent* Inv)
{
	float Total = 0.f;
	for (const FDFInventorySlot& Slot : Grid)
	{
		if (!Inv || Slot.RowName.IsNone() || Slot.Quantity < 1)
		{
			continue;
		}
		if (const FDFItemTableRow* Meta = Inv->GetItemData(Slot.RowName))
		{
			Total += Meta->ItemWeight * static_cast<float>(Slot.Quantity);
		}
	}
	return Total;
}

bool SimulateBagAddOntoGridCopy(
	const UDFInventoryComponent* Inv,
	TArray<FDFInventorySlot>& Grid,
	FName RowName,
	int32 QtyRemaining)
{
	if (!Inv || RowName.IsNone() || QtyRemaining < 1)
	{
		return false;
	}
	const FDFItemTableRow* const RowMeta = Inv->GetItemData(RowName);
	if (!RowMeta)
	{
		return false;
	}
	const float UnitW                   = RowMeta->ItemWeight;
	const int32 MaxStack               = FMath::Max(1, RowMeta->MaxStack);
	const float EffectiveMaxWeight     = Inv->GetEffectiveMaxCarryWeight();

	auto EncumbranceAllowsDelta = [&](const float DeltaKg) -> bool
	{
		if (!Inv->IsCarryWeightLimited())
		{
			return true;
		}
		return PredictInventoryGridWeight(Grid, Inv) + DeltaKg <= EffectiveMaxWeight + KINDA_SMALL_NUMBER;
	};

	for (FDFInventorySlot& Slot : Grid)
	{
		if (Slot.RowName != RowName || Slot.Quantity < 1)
		{
			continue;
		}
		const int32 Room = MaxStack - Slot.Quantity;
		if (Room <= 0)
		{
			continue;
		}
		const int32 ToAdd = FMath::Min(Room, QtyRemaining);
		if (!EncumbranceAllowsDelta(UnitW * static_cast<float>(ToAdd)))
		{
			return false;
		}
		Slot.Quantity += ToAdd;
		QtyRemaining  -= ToAdd;
		if (QtyRemaining <= 0)
		{
			break;
		}
	}

	const int32 Capacity = FMath::Max(Grid.Num(), Inv->MaxSlots);
	while (QtyRemaining > 0)
	{
		while (Grid.Num() < Capacity)
		{
			FDFInventorySlot E;
			E.RowName     = NAME_None;
			E.Quantity    = 0;
			E.bIsEquipped = false;
			Grid.Add(E);
		}
		const int32 EmptyIdx = Grid.IndexOfByPredicate([&](const FDFInventorySlot& Slot)
		{
			return Slot.RowName.IsNone() || Slot.Quantity < 1;
		});
		if (!Grid.IsValidIndex(EmptyIdx))
		{
			return false;
		}
		const int32 ToAdd = FMath::Min(MaxStack, QtyRemaining);
		if (!EncumbranceAllowsDelta(UnitW * static_cast<float>(ToAdd)))
		{
			return false;
		}
		FDFInventorySlot& Cell = Grid[EmptyIdx];
		Cell.RowName     = RowName;
		Cell.Quantity    = ToAdd;
		Cell.bIsEquipped = false;
		QtyRemaining    -= ToAdd;
	}

	return QtyRemaining <= 0;
}
} // namespace

bool UDFInventoryComponent::PredictCanReceiveUnequippedStackAtBagIndex(
	const int32 BagIndex,
	const FName RowName,
	const int32 Quantity) const
{
	if (!ItemDataTable || RowName.IsNone() || Quantity < 1)
	{
		return false;
	}
	const FDFItemTableRow* const RowMeta = GetItemData(RowName);
	if (!RowMeta)
	{
		return false;
	}

	TArray<FDFInventorySlot> Work = Items;
	const int32 Cap               = FMath::Max(Items.Num(), MaxSlots);
	while (Work.Num() < Cap)
	{
		FDFInventorySlot E;
		E.RowName     = NAME_None;
		E.Quantity    = 0;
		E.bIsEquipped = false;
		Work.Add(E);
	}
	if (!Work.IsValidIndex(BagIndex))
	{
		return false;
	}

	const int32 MaxStack        = FMath::Max(1, RowMeta->MaxStack);
	const FDFInventorySlot Cell = Items[BagIndex];

	if (Cell.RowName.IsNone() || Cell.Quantity < 1)
	{
		return !WouldRejectAddDueToWeight(RowName, Quantity);
	}

	if (Cell.RowName == RowName)
	{
		const int32 Room = MaxStack - Cell.Quantity;
		return Room >= Quantity && !WouldRejectAddDueToWeight(RowName, Quantity);
	}

	const float WeightDeltaIncomingVsDisplaced =
		RowMeta->ItemWeight * static_cast<float>(Quantity) - ComputeStackContributionWeight(Cell);
	if (IsCarryWeightLimited() && !CanAffordCarryWeightDelta(WeightDeltaIncomingVsDisplaced))
	{
		return false;
	}

	FDFInventorySlot DisplacedSnapshot = Items[BagIndex];
	Work[BagIndex].RowName              = RowName;
	Work[BagIndex].Quantity             = Quantity;
	Work[BagIndex].bIsEquipped         = false;

	if (DisplacedSnapshot.RowName.IsNone() || DisplacedSnapshot.Quantity < 1)
	{
		return true;
	}

	return SimulateBagAddOntoGridCopy(this, Work, DisplacedSnapshot.RowName, DisplacedSnapshot.Quantity);
}

bool UDFInventoryComponent::AddItem(const FName RowName, const int32 Quantity)
{
	if (!IsAuthority() || !ItemDataTable || RowName.IsNone() || Quantity < 1)
	{
		return false;
	}
	EnsureAuthorityBagGridSized();

	const FDFItemTableRow* const Row = GetItemData(RowName);
	if (!Row)
	{
		return false;
	}

	if (IsCarryWeightLimited())
	{
		const float AddedW = Row->ItemWeight * static_cast<float>(Quantity);
		if (!CanAffordCarryWeightDelta(AddedW))
		{
			DF_LOG(Verbose,
				"[DF|Bag|AddItem] SKIP encumbrance owner=%s %s x%d (w=%.2f cur=%.1f max=%.1f)",
				GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
				*RowName.ToString(),
				Quantity,
				AddedW,
				GetCurrentCarriedWeight(),
				MaxCarryWeight);
			return false;
		}
	}

	int32 QtyLeft = Quantity;
	int32 Gained  = 0;
	const int32 MaxStack = FMath::Max(1, Row->MaxStack);

	// ── Fill existing stacks first ────────────────────────────────────────────
	for (FDFInventorySlot& S : Items)
	{
		if (S.RowName != RowName)
		{
			continue;
		}
		const int32 Room  = MaxStack - S.Quantity;
		if (Room <= 0)
		{
			continue;
		}
		const int32 ToAdd = FMath::Min(Room, QtyLeft);
		S.Quantity += ToAdd;
		Gained     += ToAdd;
		QtyLeft    -= ToAdd;
		if (QtyLeft <= 0)
		{
			break;
		}
	}

	// ── Place remainder into empty grid cells (fixed MaxSlots-sized bag) ──────
	while (QtyLeft > 0)
	{
		EnsureAuthorityBagGridSized();
		const int32 TargetIdx =
			Items.IndexOfByPredicate([&](const FDFInventorySlot& S)
			{
				return S.RowName.IsNone() || S.Quantity < 1;
			});

		if (!Items.IsValidIndex(TargetIdx))
		{
			// Inventory full – broadcast whatever was gained before stopping.
			if (Gained > 0)
			{
				TryGoldCombatText(GetOwner(), *Row, Gained);
				OnInventoryChanged.Broadcast();
			}
			return Gained > 0;
		}

		const int32 ToAdd = FMath::Min(MaxStack, QtyLeft);
		FDFInventorySlot& Cell = Items[TargetIdx];
		Cell.RowName     = RowName;
		Cell.Quantity    = ToAdd;
		Cell.bIsEquipped = false;
		Gained  += ToAdd;
		QtyLeft -= ToAdd;
	}

	TryGoldCombatText(GetOwner(), *Row, Gained);
	OnInventoryChanged.Broadcast();
	return true;
}

void UDFInventoryComponent::RemoveItem(
	const FName RowName, const int32 Quantity, const int32 PreferredSlotIndex)
{
	if (!IsAuthority() || RowName.IsNone() || Quantity < 1)
	{
		return;
	}
	EnsureAuthorityBagGridSized();
	int32 Remaining = Quantity;

	auto ConsumeFromSlot = [&](const int32 Idx)
	{
		if (!Items.IsValidIndex(Idx) || Remaining < 1)
		{
			return;
		}
		FDFInventorySlot& S = Items[Idx];
		if (S.RowName != RowName || S.Quantity < 1)
		{
			return;
		}
		if (S.Quantity <= Remaining)
		{
			Remaining -= S.Quantity;
			if (S.bIsEquipped)
			{
				UnequipItem(Idx);
			}
			S.RowName     = NAME_None;
			S.Quantity    = 0;
			S.bIsEquipped = false;
		}
		else
		{
			S.Quantity -= Remaining;
			Remaining   = 0;
		}
	};

	if (PreferredSlotIndex != INDEX_NONE)
	{
		ConsumeFromSlot(PreferredSlotIndex);
	}

	for (int32 I = 0; I < Items.Num() && Remaining > 0; ++I)
	{
		ConsumeFromSlot(I);
	}

	if (Remaining < Quantity)
	{
		OnInventoryChanged.Broadcast();
	}
}

void UDFInventoryComponent::UnequipItem(const int32 SlotIndex)
{
	if (!IsAuthority())
	{
		return;
	}
	EnsureAuthorityBagGridSized();
	if (!Items.IsValidIndex(SlotIndex))
	{
		return;
	}
	FDFInventorySlot& S = Items[SlotIndex];
	if (!S.bIsEquipped)
	{
		return;
	}

	// BUG FIX: `Row` was fetched but never used; removed the dead assignment.
	if (UAbilitySystemComponent* const ASC = ResolveOwnerASC())
	{
		if (const FActiveGameplayEffectHandle* const H = EquipHandles.Find(SlotIndex))
		{
			if (ASC->GetAvatarActor())
			{
				ASC->RemoveActiveGameplayEffect(*H);
			}
		}
	}
	EquipHandles.Remove(SlotIndex);
	S.bIsEquipped = false;
	OnInventoryChanged.Broadcast();
}

void UDFInventoryComponent::UnequipOthersOfType(const EItemType Type, const int32 ExceptSlot)
{
	if (!ItemDataTable)
	{
		return;
	}
	for (int32 I = 0; I < Items.Num(); ++I)
	{
		if (I == ExceptSlot || !Items[I].bIsEquipped)
		{
			continue;
		}
		if (const FDFItemTableRow* const R = GetItemData(Items[I].RowName))
		{
			if (R->ItemType == Type)
			{
				UnequipItem(I);
			}
		}
	}
}

void UDFInventoryComponent::EquipItem(const int32 SlotIndex)
{
	if (!IsAuthority())
	{
		return;
	}
	EnsureAuthorityBagGridSized();
	if (!Items.IsValidIndex(SlotIndex))
	{
		return;
	}
	FDFInventorySlot& S = Items[SlotIndex];
	if (S.bIsEquipped || S.RowName.IsNone() || S.Quantity < 1)
	{
		return;
	}
	const FDFItemTableRow* const Row = GetItemData(S.RowName);
	if (!Row)
	{
		return;
	}

	// BUG FIX: `UnequipOthersOfType` must run even when there is no GAS effect,
	// otherwise multiple items of the same type could be marked equipped simultaneously.
	UnequipOthersOfType(Row->ItemType, SlotIndex);

	if (!Row->OnEquipEffect)
	{
		S.bIsEquipped = true;
		OnInventoryChanged.Broadcast();
		return;
	}

	UAbilitySystemComponent* const ASC = ResolveOwnerASC();
	if (!ASC || !ASC->GetAvatarActor())
	{
		// ApplyGameplayEffectToSelf can assert if InitAbilityActorInfo never ran.
		return;
	}

	const UGameplayEffect* const EquipGE = Row->OnEquipEffect.GetDefaultObject();
	if (!EquipGE)
	{
		return;
	}

	FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
	if (AActor* const OwnerActor = GetOwner())
	{
		Ctx.AddInstigator(OwnerActor, OwnerActor);
	}

	const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectToSelf(EquipGE, 1.f, Ctx);
	if (Handle.IsValid())
	{
		EquipHandles.Add(SlotIndex, Handle);
		S.bIsEquipped = true;
		OnInventoryChanged.Broadcast();
	}
}

void UDFInventoryComponent::ServerPickUpFromLoot_Implementation(ADFLootDrop* Source)
{
	if (!IsAuthority() || !IsValid(Source))
	{
		return;
	}
	if (Source->GetItemRowName().IsNone())
	{
		return;
	}

	// BUG FIX: original code skipped the distance check when OwnerActor was null
	// and continued to AddItem; now we always require a valid owner.
	AActor* const OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}
	if (FVector::Dist(OwnerActor->GetActorLocation(), Source->GetActorLocation()) > 500.f)
	{
		return;
	}

	if (AddItem(Source->GetItemRowName(), 1))
	{
		AActor* Pawn = Cast<APawn>(OwnerActor);
		if (!Pawn)
		{
			if (APlayerState* const PS = Cast<APlayerState>(OwnerActor))
			{
				Pawn = PS->GetPawn();
			}
		}
		Source->OnPickedUp.Broadcast(Pawn, Source->GetItemRowName());
		Source->Multicast_PlayPickupVFX(Pawn, Source->GetActorLocation());
		Source->Destroy();
	}
}

bool UDFInventoryComponent::ServerPickUpFromLoot_Validate(ADFLootDrop* Source)
{
	return IsValid(Source);
}

namespace
{
void SwapEquipHandlesForSlots(
	TMap<int32, FActiveGameplayEffectHandle>& Map,
	const int32 A,
	const int32 B)
{
	if (A == B)
	{
		return;
	}
	FActiveGameplayEffectHandle HA, HB;
	const bool bRemovedA = Map.RemoveAndCopyValue(A, HA);
	const bool bRemovedB = Map.RemoveAndCopyValue(B, HB);
	if (bRemovedA)
	{
		Map.Add(B, HA);
	}
	if (bRemovedB)
	{
		Map.Add(A, HB);
	}
}
} // namespace

void UDFInventoryComponent::RequestMoveBagSlot(
	const int32 SourceSlotIndex, const int32 TargetSlotIndex)
{
	if (!GetOwner())
	{
		return;
	}
	if (IsAuthority())
	{
		MoveBagSlotInternal(SourceSlotIndex, TargetSlotIndex);
	}
	else
	{
		ServerRequestMoveBagSlot(SourceSlotIndex, TargetSlotIndex);
	}
}

void UDFInventoryComponent::ServerRequestMoveBagSlot_Implementation(
	const int32 SourceSlotIndex, const int32 TargetSlotIndex)
{
	DF_LOG(Verbose,
		"[DF|Bag|Swap] RPC owner=%s origem_idx=%d destino_idx=%d",
		GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
		SourceSlotIndex,
		TargetSlotIndex);
	MoveBagSlotInternal(SourceSlotIndex, TargetSlotIndex);
}

bool UDFInventoryComponent::ServerRequestMoveBagSlot_Validate(
	const int32 SourceSlotIndex, const int32 TargetSlotIndex)
{
	return Items.IsValidIndex(SourceSlotIndex) && Items.IsValidIndex(TargetSlotIndex) &&
	       SourceSlotIndex != TargetSlotIndex;
}

void UDFInventoryComponent::MoveBagSlotInternal(const int32 SourceSlotIndex, const int32 TargetSlotIndex)
{
	if (!IsAuthority() || !Items.IsValidIndex(SourceSlotIndex) || !Items.IsValidIndex(TargetSlotIndex) ||
		SourceSlotIndex == TargetSlotIndex)
	{
		return;
	}
	EnsureAuthorityBagGridSized();

	auto RowLabel = [](const FDFInventorySlot& S) -> FString
	{
		return S.RowName.IsNone()
			? FString(TEXT("(empty)"))
			: S.RowName.ToString();
	};

	const FString FromRow = RowLabel(Items[SourceSlotIndex]);
	const FString ToRow = RowLabel(Items[TargetSlotIndex]);
	const int32 FromQ = Items[SourceSlotIndex].Quantity;
	const int32 ToQ = Items[TargetSlotIndex].Quantity;
	DF_LOG(Verbose,
		"[DF|Bag|Swap] commit owner=%s origem_idx=%d \"%s\" x%d "
		"<-> destino_idx=%d \"%s\" x%d",
		GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
		SourceSlotIndex,
		*FromRow,
		FromQ,
		TargetSlotIndex,
		*ToRow,
		ToQ);

	Items.Swap(SourceSlotIndex, TargetSlotIndex);
	SwapEquipHandlesForSlots(EquipHandles, SourceSlotIndex, TargetSlotIndex);
	OnInventoryChanged.Broadcast();
}