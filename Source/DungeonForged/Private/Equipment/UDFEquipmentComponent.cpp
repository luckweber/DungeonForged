// Source/DungeonForged/Private/Equipment/UDFEquipmentComponent.cpp
#include "Equipment/UDFEquipmentComponent.h"
#include "DFInventoryComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/DFDataTableStructs.h"
#include "Engine/DataTable.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

static bool IsEquippableItemType(const EItemType T)
{
	switch (T)
	{
		case EItemType::Weapon:
		case EItemType::OffHand:
		case EItemType::Armor:
		case EItemType::Helmet:
		case EItemType::Chest:
		case EItemType::Legs:
		case EItemType::Boots:
		case EItemType::Gloves:
		case EItemType::Ring:
		case EItemType::Amulet:
			return true;
		default: return false;
	}
}

EEquipmentSlot UDFEquipmentComponent::ResolveItemEquipmentSlot(const FDFItemTableRow& Row)
{
	if (Row.TargetEquipmentSlot != EEquipmentSlot::None)
	{
		return Row.TargetEquipmentSlot;
	}
	switch (Row.ItemType)
	{
		case EItemType::Weapon: return EEquipmentSlot::Weapon;
		case EItemType::OffHand: return EEquipmentSlot::OffHand;
		case EItemType::Helmet: return EEquipmentSlot::Helmet;
		case EItemType::Chest: return EEquipmentSlot::Chest;
		case EItemType::Legs: return EEquipmentSlot::Legs;
		case EItemType::Boots: return EEquipmentSlot::Boots;
		case EItemType::Gloves: return EEquipmentSlot::Gloves;
		case EItemType::Ring: return EEquipmentSlot::Ring1;
		case EItemType::Amulet: return EEquipmentSlot::Amulet;
		case EItemType::Armor: return EEquipmentSlot::Chest;
		default: return EEquipmentSlot::None;
	}
}

bool UDFEquipmentComponent::DoesItemMatchEquipmentSlot(
	const FDFItemTableRow& Row, const EEquipmentSlot RequestedSlot, FString* const OutError)
{
	if (RequestedSlot == EEquipmentSlot::None)
	{
		if (OutError)
		{
			*OutError = TEXT("Invalid equipment slot");
		}
		return false;
	}
	if (!IsEquippableItemType(Row.ItemType))
	{
		if (OutError)
		{
			*OutError = TEXT("Item is not equippable");
		}
		return false;
	}
	if (Row.ItemType == EItemType::Ring)
	{
		if (Row.TargetEquipmentSlot == EEquipmentSlot::None)
		{
			return (RequestedSlot == EEquipmentSlot::Ring1 || RequestedSlot == EEquipmentSlot::Ring2);
		}
		if (Row.TargetEquipmentSlot == RequestedSlot)
		{
			return true;
		}
		if (OutError)
		{
			*OutError = TEXT("Ring slot mismatch with data");
		}
		return false;
	}
	if (Row.TargetEquipmentSlot != EEquipmentSlot::None)
	{
		if (Row.TargetEquipmentSlot != RequestedSlot)
		{
			if (OutError)
			{
				*OutError = TEXT("Data table target slot != requested");
			}
			return false;
		}
		return true;
	}
	const EEquipmentSlot Required = ResolveItemEquipmentSlot(Row);
	if (Required == EEquipmentSlot::None)
	{
		if (OutError)
		{
			*OutError = TEXT("Could not resolve item slot");
		}
		return false;
	}
	if (Required != RequestedSlot)
	{
		if (OutError)
		{
			*OutError = TEXT("Item does not match this slot");
		}
		return false;
	}
	return true;
}

UDFEquipmentComponent::UDFEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UDFEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UDFEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UDFEquipmentComponent, ReplicatedLoadout);
}

UAbilitySystemComponent* UDFEquipmentComponent::ResolveOwnerASC() const
{
	if (const IAbilitySystemInterface* const I = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		return I->GetAbilitySystemComponent();
	}
	if (const AActor* O = GetOwner())
	{
		if (const APawn* P = Cast<APawn>(O))
		{
			if (APlayerState* PS = P->GetPlayerState())
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

UDFInventoryComponent* UDFEquipmentComponent::ResolveInventory() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UDFInventoryComponent>() : nullptr;
}

const FDFItemTableRow* UDFEquipmentComponent::GetItemData(const FName RowName) const
{
	const UDataTable* Table = ItemDataTable;
	if (!Table)
	{
		if (const UDFInventoryComponent* const Inv = ResolveInventory())
		{
			Table = Inv->ItemDataTable;
		}
	}
	if (!Table || RowName.IsNone())
	{
		return nullptr;
	}
	return Table->FindRow<FDFItemTableRow>(RowName, TEXT("UDFEquipment|GetItemData"));
}

const FDFItemTableRow* UDFEquipmentComponent::GetEquippedItemDataRaw(const EEquipmentSlot Slot) const
{
	const FName N = EquippedItems.FindRef(Slot);
	if (N.IsNone())
	{
		return nullptr;
	}
	return GetItemData(N);
}

bool UDFEquipmentComponent::TryGetEquippedItemData(
	const EEquipmentSlot Slot, FDFItemTableRow& OutRow) const
{
	if (const FDFItemTableRow* const P = GetEquippedItemDataRaw(Slot))
	{
		OutRow = *P;
		return true;
	}
	return false;
}

bool UDFEquipmentComponent::IsSlotEmpty(const EEquipmentSlot Slot) const
{
	if (const FName* P = EquippedItems.Find(Slot))
	{
		return P->IsNone();
	}
	return true;
}

void UDFEquipmentComponent::RegisterSlotMesh(const EEquipmentSlot Slot, USkeletalMeshComponent* const Mesh)
{
	if (Mesh)
	{
		SlotMeshComponents.Add(Slot, Mesh);
	}
}

USkeletalMeshComponent* UDFEquipmentComponent::GetSlotMesh(const EEquipmentSlot Slot) const
{
	if (USkeletalMeshComponent* const* P = SlotMeshComponents.Find(Slot))
	{
		return *P;
	}
	return nullptr;
}

void UDFEquipmentComponent::SwapSlotMesh(
	const EEquipmentSlot Slot, USkeletalMesh* const NewMesh, USkeletalMeshComponent* const BaseMesh)
{
	USkeletalMeshComponent* const Comp = GetSlotMesh(Slot);
	if (!Comp)
	{
		return;
	}
	if (NewMesh)
	{
		Comp->SetSkeletalMesh(NewMesh);
		if (BaseMesh)
		{
			Comp->SetLeaderPoseComponent(BaseMesh, true);
		}
	}
	else
	{
		Comp->SetSkeletalMesh(nullptr);
		Comp->SetLeaderPoseComponent(nullptr);
	}
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->bReceivesDecals = true;
	Comp->SetCastShadow(true);
	Comp->SetComponentTickEnabled(false);
}

void UDFEquipmentComponent::OnRep_Loadout()
{
	RebuildMapFromReplicated();
	RecalculateAllVisuals();
}

void UDFEquipmentComponent::SyncReplicatedArrayFromMap()
{
	ReplicatedLoadout.Reset();
	for (const TPair<EEquipmentSlot, FName>& P : EquippedItems)
	{
		if (P.Value.IsNone())
		{
			continue;
		}
		FDFEquippedItemRep R;
		R.Slot = P.Key;
		R.ItemRow = P.Value;
		ReplicatedLoadout.Add(R);
	}
}

void UDFEquipmentComponent::RebuildMapFromReplicated()
{
	EquippedItems.Empty();
	for (const FDFEquippedItemRep& R : ReplicatedLoadout)
	{
		if (R.Slot != EEquipmentSlot::None && !R.ItemRow.IsNone())
		{
			EquippedItems.Add(R.Slot, R.ItemRow);
		}
	}
}

void UDFEquipmentComponent::RecalculateAllVisuals()
{
	for (uint8 S = (uint8)EEquipmentSlot::Weapon; S <= (uint8)EEquipmentSlot::Amulet; ++S)
	{
		RecalculateVisualsForSlot((EEquipmentSlot)S);
	}
}

void UDFEquipmentComponent::RecalculateVisualsForSlot(const EEquipmentSlot Slot)
{
	USkeletalMeshComponent* const Comp = GetSlotMesh(Slot);
	if (!Comp)
	{
		return;
	}
	const FName N = EquippedItems.FindRef(Slot);
	USkeletalMeshComponent* const Leader = BaseBodyMesh ? BaseBodyMesh.Get() : nullptr;
	if (N.IsNone())
	{
		if (USkeletalMesh* const D = DefaultNakedMeshes.FindRef(Slot).Get())
		{
			SwapSlotMesh(Slot, D, Leader);
		}
		else
		{
			SwapSlotMesh(Slot, nullptr, nullptr);
		}
		return;
	}
	if (const FDFItemTableRow* const R = GetItemData(N))
	{
		if (R->ItemSkeletalMesh)
		{
			SwapSlotMesh(Slot, R->ItemSkeletalMesh, Leader);
		}
		else if (USkeletalMesh* D = DefaultNakedMeshes.FindRef(Slot).Get())
		{
			SwapSlotMesh(Slot, D, Leader);
		}
		else
		{
			SwapSlotMesh(Slot, nullptr, nullptr);
		}
	}
	else
	{
		if (USkeletalMesh* const D = DefaultNakedMeshes.FindRef(Slot).Get())
		{
			SwapSlotMesh(Slot, D, Leader);
		}
		else
		{
			SwapSlotMesh(Slot, nullptr, nullptr);
		}
	}
}

float UDFEquipmentComponent::GetTotalStatBonus(const FGameplayAttribute Attribute) const
{
	if (!Attribute.IsValid())
	{
		return 0.f;
	}
	float Sum = 0.f;
	for (uint8 S = (uint8)EEquipmentSlot::Weapon; S <= (uint8)EEquipmentSlot::Amulet; ++S)
	{
		if (const FDFItemTableRow* const R = GetEquippedItemDataRaw((EEquipmentSlot)S))
		{
			for (const TPair<FGameplayAttribute, float>& P : R->AttributeModifiers)
			{
				if (P.Key == Attribute)
				{
					Sum += P.Value;
					break;
				}
			}
		}
	}
	return Sum;
}

static bool HasInventoryItemCount(
	const UDFInventoryComponent& Inv, const FName RowName, const int32 MinCount, int32& OutCount)
{
	OutCount = 0;
	for (const FDFInventorySlot& S : Inv.Items)
	{
		if (S.RowName == RowName)
		{
			OutCount += S.Quantity;
		}
	}
	return OutCount >= MinCount;
}

void UDFEquipmentComponent::UnequipSlotInternal(const EEquipmentSlot Slot)
{
	if (Slot == EEquipmentSlot::None)
	{
		return;
	}
	AActor* const O = GetOwner();
	if (!O || !O->HasAuthority())
	{
		return;
	}
	const FName RowN = EquippedItems.FindRef(Slot);
	if (RowN.IsNone())
	{
		return;
	}
	if (FActiveGameplayEffectHandle* H = EquipEffectHandles.Find(Slot))
	{
		if (UAbilitySystemComponent* const ASC = ResolveOwnerASC();
			ASC && ASC->GetAvatarActor())
		{
			ASC->RemoveActiveGameplayEffect(*H);
		}
		EquipEffectHandles.Remove(Slot);
	}
	EquippedItems.Remove(Slot);
	if (UDFInventoryComponent* const Inv = ResolveInventory())
	{
		Inv->AddItem(RowN, 1);
	}
	SyncReplicatedArrayFromMap();
	OnEquipmentChanged.Broadcast(Slot, NAME_None);
	RecalculateVisualsForSlot(Slot);
}

void UDFEquipmentComponent::UnequipSlot(const EEquipmentSlot Slot)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		UnequipSlotInternal(Slot);
	}
	else
	{
		ServerUnequipSlot(Slot);
	}
}

void UDFEquipmentComponent::RequestUnequipSlot(const EEquipmentSlot Slot)
{
	ServerUnequipSlot(Slot);
}

bool UDFEquipmentComponent::EquipItemInternal(
	const FName ItemRowName, const EEquipmentSlot Slot, FString& OutError)
{
	AActor* const O = GetOwner();
	if (!O || !O->HasAuthority())
	{
		OutError = TEXT("Not authority");
		return false;
	}
	if (ItemRowName.IsNone() || Slot == EEquipmentSlot::None)
	{
		OutError = TEXT("Invalid row or slot");
		return false;
	}
	const FDFItemTableRow* const Row = GetItemData(ItemRowName);
	if (!Row)
	{
		OutError = TEXT("Unknown item");
		return false;
	}
	if (!DoesItemMatchEquipmentSlot(*Row, Slot, &OutError))
	{
		return false;
	}
	UDFInventoryComponent* const Inv = ResolveInventory();
	int32 InBag = 0;
	if (!Inv)
	{
		OutError = TEXT("No inventory");
		return false;
	}
	if (!HasInventoryItemCount(*Inv, ItemRowName, 1, InBag))
	{
		OutError = TEXT("Item not in inventory");
		return false;
	}
	UAbilitySystemComponent* const ASC = ResolveOwnerASC();
	if (!ASC)
	{
		OutError = TEXT("No AbilitySystemComponent");
		return false;
	}
	// GAS: ApplyGameplayEffectToSelf can check-fail if InitAbilityActorInfo was not run yet
	// (e.g. equip before possession / before OnRep_PlayerState on client authority edge cases).
	if (!ASC->GetAvatarActor())
	{
		OutError = TEXT("GAS not ready (InitAbilityActorInfo)");
		return false;
	}
	const FName Current = EquippedItems.FindRef(Slot);
	if (!Current.IsNone())
	{
		UnequipSlotInternal(Slot);
	}
	Inv->RemoveItem(ItemRowName, 1);
	{
		if (Row->OnEquipEffect)
		{
			if (const UGameplayEffect* const CDO = Row->OnEquipEffect.GetDefaultObject())
			{
				FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
				if (O)
				{
					Ctx.AddInstigator(O, O);
				}
				const FActiveGameplayEffectHandle H = ASC->ApplyGameplayEffectToSelf(CDO, 1.f, Ctx);
				if (H.IsValid())
				{
					EquipEffectHandles.Add(Slot, H);
				}
			}
		}
	}
	EquippedItems.Add(Slot, ItemRowName);
	SyncReplicatedArrayFromMap();
	OnEquipmentChanged.Broadcast(Slot, ItemRowName);
	RecalculateVisualsForSlot(Slot);
	return true;
}

void UDFEquipmentComponent::RequestEquipItem(const FName ItemRowName, const EEquipmentSlot Slot)
{
	ServerEquipItem(ItemRowName, Slot);
}

void UDFEquipmentComponent::ServerEquipItem_Implementation(const FName ItemRowName, const EEquipmentSlot Slot)
{
	FString E;
	EquipItemInternal(ItemRowName, Slot, E);
}

bool UDFEquipmentComponent::ServerEquipItem_Validate(
	const FName /*ItemRowName*/, const EEquipmentSlot /*Slot*/)
{
	return true;
}

void UDFEquipmentComponent::ServerUnequipSlot_Implementation(const EEquipmentSlot Slot)
{
	UnequipSlotInternal(Slot);
}

bool UDFEquipmentComponent::ServerUnequipSlot_Validate(const EEquipmentSlot /*Slot*/)
{
	return true;
}

bool UDFEquipmentComponent::EquipItem(const FName ItemRowName, const EEquipmentSlot Slot)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		FString E;
		return EquipItemInternal(ItemRowName, Slot, E);
	}
	ServerEquipItem(ItemRowName, Slot);
	return true;
}
