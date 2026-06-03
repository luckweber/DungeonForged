// Source/DungeonForged/Private/Equipment/UDFEquipmentComponent.cpp
#include "Equipment/UDFEquipmentComponent.h"
#include "DFInventoryComponent.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/DFDataTableStructs.h"
#include "Engine/DataTable.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Net/UnrealNetwork.h"
#include "DungeonForgedModule.h"
#include "UObject/UnrealType.h"

#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Animation/UDFAnimInstance.h"
#include "DFLootDrop.h"
#include "Engine/World.h"

namespace
{
void SpawnDroppedItemAtFeet(AActor* Owner, UDataTable* ItemTable, FName ItemRow)
{
	if (!Owner || !Owner->HasAuthority() || ItemRow.IsNone())
	{
		return;
	}
	UWorld* const World = Owner->GetWorld();
	if (!World)
	{
		return;
	}
	FVector Loc = Owner->GetActorLocation();
	Loc.Z += 20.f;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	if (ADFLootDrop* const Drop = World->SpawnActor<ADFLootDrop>(ADFLootDrop::StaticClass(), Loc, FRotator::ZeroRotator, Params))
	{
		Drop->InitLoot(ItemTable, ItemRow, FVector::UpVector * 120.f, false);
	}
}
} // namespace

static FString DF_DebugEqSlotName(const EEquipmentSlot Slot)
{
	if (const UEnum* Enum = StaticEnum<EEquipmentSlot>())
	{
		const FString Raw = Enum->GetAuthoredNameStringByValue(static_cast<int64>(Slot));
		if (!Raw.IsEmpty())
		{
			return Raw;
		}
	}
	return FString::Printf(TEXT("Slot%u"), static_cast<uint8>(Slot));
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────
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

static bool HasInventoryItemCount(
	const UDFInventoryComponent& Inv,
	const FName RowName,
	const int32 MinCount,
	int32& OutCount)
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

// ─────────────────────────────────────────────────────────────────────────────
// Static utilities
// ─────────────────────────────────────────────────────────────────────────────
EEquipmentSlot UDFEquipmentComponent::ResolveItemEquipmentSlot(const FDFItemTableRow& Row)
{
	if (Row.TargetEquipmentSlot != EEquipmentSlot::None)
	{
		return Row.TargetEquipmentSlot;
	}
	switch (Row.ItemType)
	{
		case EItemType::Weapon:  return EEquipmentSlot::Weapon;
		case EItemType::OffHand: return EEquipmentSlot::OffHand;
		case EItemType::Helmet:  return EEquipmentSlot::Helmet;
		case EItemType::Chest:   return EEquipmentSlot::Chest;
		case EItemType::Legs:    return EEquipmentSlot::Legs;
		case EItemType::Boots:   return EEquipmentSlot::Boots;
		case EItemType::Gloves:  return EEquipmentSlot::Gloves;
		case EItemType::Ring:    return EEquipmentSlot::Ring1;
		case EItemType::Amulet:  return EEquipmentSlot::Amulet;
		case EItemType::Armor:   return EEquipmentSlot::Chest;
		default:                 return EEquipmentSlot::None;
	}
}

bool UDFEquipmentComponent::DoesItemMatchEquipmentSlot(
	const FDFItemTableRow& Row,
	const EEquipmentSlot RequestedSlot,
	FString* const OutError)
{
	if (RequestedSlot == EEquipmentSlot::None)
	{
		if (OutError) { *OutError = TEXT("Invalid equipment slot"); }
		return false;
	}
	if (!IsEquippableItemType(Row.ItemType))
	{
		if (OutError) { *OutError = TEXT("Item is not equippable"); }
		return false;
	}
	// Rings can go in Ring1 or Ring2 unless the data table pins a specific slot.
	if (Row.ItemType == EItemType::Ring)
	{
		if (Row.TargetEquipmentSlot == EEquipmentSlot::None)
		{
			return (RequestedSlot == EEquipmentSlot::Ring1 ||
			        RequestedSlot == EEquipmentSlot::Ring2);
		}
		if (Row.TargetEquipmentSlot == RequestedSlot)
		{
			return true;
		}
		if (OutError) { *OutError = TEXT("Ring slot mismatch with data"); }
		return false;
	}
	// Items with an explicit target slot.
	if (Row.TargetEquipmentSlot != EEquipmentSlot::None)
	{
		if (Row.TargetEquipmentSlot != RequestedSlot)
		{
			if (OutError) { *OutError = TEXT("Data table target slot != requested"); }
			return false;
		}
		return true;
	}
	// Derive from item type.
	const EEquipmentSlot Required = ResolveItemEquipmentSlot(Row);
	if (Required == EEquipmentSlot::None)
	{
		if (OutError) { *OutError = TEXT("Could not resolve item slot"); }
		return false;
	}
	if (Required != RequestedSlot)
	{
		if (OutError) { *OutError = TEXT("Item does not match this slot"); }
		return false;
	}
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// UObject / UActorComponent
// ─────────────────────────────────────────────────────────────────────────────
UDFEquipmentComponent::UDFEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UDFEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UDFEquipmentComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UDFEquipmentComponent, ReplicatedLoadout);
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────
UAbilitySystemComponent* UDFEquipmentComponent::ResolveOwnerASC() const
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

UDFInventoryComponent* UDFEquipmentComponent::ResolveInventory() const
{
	AActor* const Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}
	if (UDFInventoryComponent* const OnOwner = Owner->FindComponentByClass<UDFInventoryComponent>())
	{
		return OnOwner;
	}
	if (const APawn* const P = Cast<APawn>(Owner))
	{
		if (APlayerState* const PS = P->GetPlayerState())
		{
			return PS->FindComponentByClass<UDFInventoryComponent>();
		}
	}
	return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// Data accessors
// ─────────────────────────────────────────────────────────────────────────────
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

const FDFItemTableRow* UDFEquipmentComponent::GetEquippedItemDataRaw(
	const EEquipmentSlot Slot) const
{
	const FName N = EquippedItems.FindRef(Slot);
	return N.IsNone() ? nullptr : GetItemData(N);
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
	const FName* const P = EquippedItems.Find(Slot);
	return !P || P->IsNone();
}

float UDFEquipmentComponent::GetTotalStatBonus(const FGameplayAttribute Attribute) const
{
	if (!Attribute.IsValid())
	{
		return 0.f;
	}
	float Sum = 0.f;
	for (uint8 S = static_cast<uint8>(EEquipmentSlot::Weapon);
		 S <= static_cast<uint8>(EEquipmentSlot::Amulet);
		 ++S)
	{
		if (const FDFItemTableRow* const R =
				GetEquippedItemDataRaw(static_cast<EEquipmentSlot>(S)))
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

// ─────────────────────────────────────────────────────────────────────────────
// Mesh slots
// ─────────────────────────────────────────────────────────────────────────────
void UDFEquipmentComponent::RegisterSlotMesh(
	const EEquipmentSlot Slot, USkeletalMeshComponent* const Mesh)
{
	if (Mesh)
	{
		SlotMeshComponents.Add(Slot, Mesh);
	}
}

USkeletalMeshComponent* UDFEquipmentComponent::GetSlotMesh(const EEquipmentSlot Slot) const
{
	USkeletalMeshComponent* const* const P = SlotMeshComponents.Find(Slot);
	return P ? *P : nullptr;
}

void UDFEquipmentComponent::SwapSlotMesh(
	const EEquipmentSlot Slot,
	USkeletalMesh* const NewMesh,
	USkeletalMeshComponent* const BaseMesh)
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
	Comp->bReceivesDecals       = false;
	Comp->SetCastShadow(true);
	Comp->SetComponentTickEnabled(false);
	Comp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
}

// ─────────────────────────────────────────────────────────────────────────────
// Weapon melee ability
// ─────────────────────────────────────────────────────────────────────────────
bool UDFEquipmentComponent::HasGrantedWeaponMeleeAbilitySpec() const
{
	const FDFItemTableRow* const Row = GetEquippedItemDataRaw(EEquipmentSlot::Weapon);
	return Row != nullptr && Row->WeaponMeleeGameplayAbility != nullptr;
}

bool UDFEquipmentComponent::TryActivateGrantedWeaponMeleeAbility()
{
	UAbilitySystemComponent* const ASC = ResolveOwnerASC();
	if (!ASC)
	{
		return false;
	}
	const FDFItemTableRow* const Row = GetEquippedItemDataRaw(EEquipmentSlot::Weapon);
	if (!Row || !Row->WeaponMeleeGameplayAbility)
	{
		return false;
	}
	// Prefer class-based activation so clients use specs replicated from server (handles are authority-local).
	return ASC->TryActivateAbilityByClass(Row->WeaponMeleeGameplayAbility, true);
}

void UDFEquipmentComponent::SyncWeaponMeleeGameplayAbilityGrant()
{
	AActor* const O = GetOwner();
	if (!O || !O->HasAuthority())
	{
		return;
	}
	UAbilitySystemComponent* const ASC = ResolveOwnerASC();
	if (!ASC || !ASC->GetAvatarActor())
	{
		return;
	}
	if (IsSlotEmpty(EEquipmentSlot::Weapon))
	{
		RevokeGrantedWeaponMeleeAbility(ASC);
		ClearEquippedWeaponLooseTags(ASC);
		return;
	}
	const FDFItemTableRow* const Row = GetEquippedItemDataRaw(EEquipmentSlot::Weapon);
	if (!Row)
	{
		RevokeGrantedWeaponMeleeAbility(ASC);
		ClearEquippedWeaponLooseTags(ASC);
		return;
	}
	if (Row->WeaponMeleeGameplayAbility)
	{
		TryGrantWeaponMeleeAbilityFromEquippedRow(ASC, Row);
	}
	else
	{
		RevokeGrantedWeaponMeleeAbility(ASC);
	}
	SyncEquippedWeaponLooseTags(ASC, Row);
}

void UDFEquipmentComponent::RevokeGrantedWeaponMeleeAbility(
	UAbilitySystemComponent* const ASC)
{
	if (!GrantedWeaponMeleeAbilitySpecHandle.IsValid())
	{
		return;
	}
	if (ASC)
	{
		ASC->ClearAbility(GrantedWeaponMeleeAbilitySpecHandle);
	}
	GrantedWeaponMeleeAbilitySpecHandle = FGameplayAbilitySpecHandle();
}

void UDFEquipmentComponent::ClearEquippedWeaponLooseTags(UAbilitySystemComponent* const ASC)
{
	if (!ASC || AppliedWeaponLooseTags.IsEmpty())
	{
		AppliedWeaponLooseTags.Reset();
		return;
	}
	ASC->RemoveLooseGameplayTags(AppliedWeaponLooseTags);
	AppliedWeaponLooseTags.Reset();
}

void UDFEquipmentComponent::SyncEquippedWeaponLooseTags(
	UAbilitySystemComponent* const ASC,
	const FDFItemTableRow* const WeaponRow)
{
	if (!ASC)
	{
		return;
	}
	ClearEquippedWeaponLooseTags(ASC);
	if (WeaponRow && !WeaponRow->WeaponTags.IsEmpty())
	{
		ASC->AddLooseGameplayTags(WeaponRow->WeaponTags);
		AppliedWeaponLooseTags = WeaponRow->WeaponTags;
	}
}

void UDFEquipmentComponent::TryGrantWeaponMeleeAbilityFromEquippedRow(
	UAbilitySystemComponent* const ASC,
	const FDFItemTableRow* const Row)
{
	if (!ASC || !Row || !Row->WeaponMeleeGameplayAbility)
	{
		return;
	}
	RevokeGrantedWeaponMeleeAbility(ASC);
	FGameplayAbilitySpec Spec(Row->WeaponMeleeGameplayAbility, 1, INDEX_NONE, this);
	GrantedWeaponMeleeAbilitySpecHandle = ASC->GiveAbility(Spec);
	if (!GrantedWeaponMeleeAbilitySpecHandle.IsValid())
	{
		const UClass* const AbilityClass = Row->WeaponMeleeGameplayAbility.Get();
		DF_LOG(Warning,
			"[DF|Eq|WeaponMelee] GiveAbility failed: ability=%s owner=%s. Check Ability policy and prerequisites.",
			AbilityClass ? *AbilityClass->GetName() : TEXT("(none)"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"));
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Replication
// ─────────────────────────────────────────────────────────────────────────────
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
		R.Slot    = P.Key;
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

void UDFEquipmentComponent::OnRep_Loadout()
{
	const TMap<EEquipmentSlot, FName> PrevEquipped = EquippedItems;
	RebuildMapFromReplicated();

	RecalculateAllVisuals();
	RefreshWeaponAnimSetOnOwner();

	for (uint8 Ui = static_cast<uint8>(EEquipmentSlot::Weapon);
		 Ui <= static_cast<uint8>(EEquipmentSlot::Amulet);
		 ++Ui)
	{
		const EEquipmentSlot S     = static_cast<EEquipmentSlot>(Ui);
		const FName          PrevR = PrevEquipped.FindRef(S);
		const FName          CurrR = EquippedItems.FindRef(S);
		if (PrevR != CurrR)
		{
			OnEquipmentChanged.Broadcast(S, CurrR.IsNone() ? NAME_None : CurrR);
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Visual recalculation
// ─────────────────────────────────────────────────────────────────────────────
void UDFEquipmentComponent::RecalculateAllVisuals()
{
	for (uint8 S = static_cast<uint8>(EEquipmentSlot::Weapon);
		 S <= static_cast<uint8>(EEquipmentSlot::Amulet);
		 ++S)
	{
		RecalculateVisualsForSlot(static_cast<EEquipmentSlot>(S));
	}
}

void UDFEquipmentComponent::RecalculateVisualsForSlot(const EEquipmentSlot Slot)
{
	USkeletalMeshComponent* const Comp = GetSlotMesh(Slot);
	if (!Comp)
	{
		return;
	}
	USkeletalMeshComponent* const Leader = BaseBodyMesh ? BaseBodyMesh.Get() : nullptr;
	const FName N = EquippedItems.FindRef(Slot);
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
		USkeletalMesh* const MeshRow = R->ItemSkeletalMesh.Get();
		USkeletalMesh* const MeshFallback = DefaultNakedMeshes.FindRef(Slot).Get();
		USkeletalMesh* const Mesh = MeshRow ? MeshRow : MeshFallback;
		SwapSlotMesh(Slot, Mesh, Mesh ? Leader : nullptr);
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

// ─────────────────────────────────────────────────────────────────────────────
// Equip validation
// ─────────────────────────────────────────────────────────────────────────────
bool UDFEquipmentComponent::PredictCanEquipItem(
	const FName ItemRowName,
	const EEquipmentSlot Slot,
	FString& OutReason,
	const int32 PreferredSourceBagSlot) const
{
	return ValidateEquipPrerequisites(ItemRowName, Slot, OutReason, PreferredSourceBagSlot);
}

bool UDFEquipmentComponent::ValidateEquipPrerequisites(
	const FName ItemRowName,
	const EEquipmentSlot Slot,
	FString& OutError,
	const int32 PreferredSourceBagSlot) const
{
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
	const UDFInventoryComponent* const Inv = ResolveInventory();
	if (!Inv)
	{
		OutError = TEXT("No inventory");
		return false;
	}
	if (PreferredSourceBagSlot != INDEX_NONE)
	{
		if (!Inv->Items.IsValidIndex(PreferredSourceBagSlot))
		{
			OutError = TEXT("Invalid bag slot index");
			return false;
		}
		const FDFInventorySlot& BagSlot = Inv->Items[PreferredSourceBagSlot];
		if (BagSlot.RowName != ItemRowName || BagSlot.Quantity < 1)
		{
			OutError = TEXT("Item not in specified bag slot");
			return false;
		}
	}
	else
	{
		int32 InBag = 0;
		if (!HasInventoryItemCount(*Inv, ItemRowName, 1, InBag))
		{
			OutError = TEXT("Item not in inventory");
			return false;
		}
	}
	const UAbilitySystemComponent* const ASC = ResolveOwnerASC();
	if (!ASC || !ASC->GetAvatarActor())
	{
		OutError = TEXT("GAS not ready (InitAbilityActorInfo)");
		return false;
	}
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Unequip
// ─────────────────────────────────────────────────────────────────────────────
void UDFEquipmentComponent::UnequipSlotInternal(
	const EEquipmentSlot Slot,
	const int32 TargetBagSlotIndex)
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

	if (Slot == EEquipmentSlot::Weapon)
	{
		if (UAbilitySystemComponent* const ASC = ResolveOwnerASC())
		{
			RevokeGrantedWeaponMeleeAbility(ASC);
			ClearEquippedWeaponLooseTags(ASC);
		}
	}

	if (FActiveGameplayEffectHandle* const H = EquipEffectHandles.Find(Slot))
	{
		if (UAbilitySystemComponent* const ASC = ResolveOwnerASC();
			ASC && ASC->GetAvatarActor())
		{
			ASC->RemoveActiveGameplayEffect(*H);
		}
		EquipEffectHandles.Remove(Slot);
	}

	EquippedItems.Remove(Slot);

	const FString OwnerName = O ? O->GetName() : FString(TEXT("?"));

	// BUG FIX: the original code would silently lose the item if AddItem failed
	// because the inventory was full. Log a warning so the issue is visible
	// during development and gameplay is not affected further.
	if (UDFInventoryComponent* const Inv = ResolveInventory())
	{
		bool bPlaced = false;
		if (TargetBagSlotIndex != INDEX_NONE)
		{
			bPlaced = Inv->ReceiveUnequippedItemAtBagIndex(TargetBagSlotIndex, RowN, 1);
			DF_LOG(Verbose,
				"[DF|Eq|Unequip] owner=%s equipSlot=%s item=%s bagCell[%d] place=%s",
				*OwnerName,
				*DF_DebugEqSlotName(Slot),
				*RowN.ToString(),
				TargetBagSlotIndex,
				bPlaced ? TEXT("OK_CELL") : TEXT("retry_AddItem"));
		}
		else
		{
			DF_LOG(Verbose,
				"[DF|Eq|Unequip] owner=%s equipSlot=%s item=%s bagTarget=AUTO_AddItem",
				*OwnerName,
				*DF_DebugEqSlotName(Slot),
				*RowN.ToString());
		}

		if (!bPlaced)
		{
			if (!Inv->AddItem(RowN, 1))
			{
				DF_LOG(Warning,
					"[DF|Eq|Unequip] INV FULL owner=%s item=%s dropping at feet (EquipSlot=%s BagTarget=%s)",
					*OwnerName,
					*RowN.ToString(),
					*DF_DebugEqSlotName(Slot),
					TargetBagSlotIndex != INDEX_NONE
						? *FString::FromInt(TargetBagSlotIndex)
						: TEXT("AUTO"));
				SpawnDroppedItemAtFeet(O, ItemDataTable, RowN);
			}
			else
			{
				DF_LOG(Verbose,
					"[DF|Eq|Unequip] owner=%s item=%s AddItem(AUTO) OK",
					*OwnerName,
					*RowN.ToString());
			}
		}
	}

	SyncReplicatedArrayFromMap();
	RecalculateVisualsForSlot(Slot);
	OnEquipmentChanged.Broadcast(Slot, NAME_None);
	if (Slot == EEquipmentSlot::Weapon)
	{
		RefreshWeaponAnimSetOnOwner();
	}
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

void UDFEquipmentComponent::RequestUnequipToBagSlot(
	const EEquipmentSlot Slot,
	const int32 TargetBagSlotIndex)
{
	if (!GetOwner() || Slot == EEquipmentSlot::None || TargetBagSlotIndex == INDEX_NONE)
	{
		return;
	}

	if (GetOwner()->HasAuthority())
	{
		UnequipSlotInternal(Slot, TargetBagSlotIndex);
	}
	else
	{
		ServerUnequipSlotToBagIndex(Slot, TargetBagSlotIndex);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Equip
// ─────────────────────────────────────────────────────────────────────────────
bool UDFEquipmentComponent::EquipItemInternal(
	const FName ItemRowName,
	const EEquipmentSlot Slot,
	FString& OutError,
	const int32 SourceBagSlotIndex)
{
	if (!ValidateEquipPrerequisites(ItemRowName, Slot, OutError, SourceBagSlotIndex))
	{
		return false;
	}
	AActor* const O = GetOwner();
	if (!O || !O->HasAuthority())
	{
		OutError = TEXT("Not authority");
		return false;
	}
	const FDFItemTableRow* const Row = GetItemData(ItemRowName);
	if (!Row)
	{
		OutError = TEXT("Unknown item");
		return false;
	}
	UDFInventoryComponent* const Inv = ResolveInventory();
	int32 VerifyBag = 0;
	if (!Inv || !HasInventoryItemCount(*Inv, ItemRowName, 1, VerifyBag))
	{
		OutError = TEXT("Item not in inventory");
		return false;
	}

	UAbilitySystemComponent* const ASC = ResolveOwnerASC();

	// Unequip the item already occupying this slot (returns it to inventory).
	const FName Current = EquippedItems.FindRef(Slot);
	if (!Current.IsNone())
	{
		UnequipSlotInternal(Slot);
	}

	// Consume one instance from inventory (preferred stack when the UI provides a source index).
	Inv->RemoveItem(ItemRowName, 1, SourceBagSlotIndex);

	// Apply equip gameplay effect if provided.
	if (Row->OnEquipEffect)
	{
		if (const UGameplayEffect* const CDO = Row->OnEquipEffect.GetDefaultObject())
		{
			FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
			if (O)
			{
				Ctx.AddInstigator(O, O);
			}
			const FActiveGameplayEffectHandle H =
				ASC->ApplyGameplayEffectToSelf(CDO, 1.f, Ctx);
			if (H.IsValid())
			{
				EquipEffectHandles.Add(Slot, H);
			}
		}
	}

	EquippedItems.Add(Slot, ItemRowName);
	SyncReplicatedArrayFromMap();
	RecalculateVisualsForSlot(Slot);
	OnEquipmentChanged.Broadcast(Slot, ItemRowName);

	if (Slot == EEquipmentSlot::Weapon)
	{
		TryGrantWeaponMeleeAbilityFromEquippedRow(ASC, Row);
		SyncEquippedWeaponLooseTags(ASC, Row);
		RefreshWeaponAnimSetOnOwner();
	}

	const FString OwnerNameEquip = O ? O->GetName() : FString(TEXT("?"));
	DF_LOG(Verbose,
		"[DF|Eq|EquipOK] owner=%s slot=%s item=%s (bag stack -1)",
		*OwnerNameEquip,
		*DF_DebugEqSlotName(Slot),
		*ItemRowName.ToString());
	return true;
}

bool UDFEquipmentComponent::EquipItem(
	const FName ItemRowName,
	const EEquipmentSlot Slot,
	const int32 SourceBagSlotIndex)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		FString E;
		return EquipItemInternal(ItemRowName, Slot, E, SourceBagSlotIndex);
	}
	ServerEquipItem(ItemRowName, Slot, SourceBagSlotIndex);
	return true;
}

void UDFEquipmentComponent::RequestEquipItem(
	const FName ItemRowName,
	const EEquipmentSlot Slot,
	const int32 SourceBagSlotIndex)
{
	ServerEquipItem(ItemRowName, Slot, SourceBagSlotIndex);
}

// ─────────────────────────────────────────────────────────────────────────────
// Animation
// ─────────────────────────────────────────────────────────────────────────────
void UDFEquipmentComponent::RefreshWeaponAnimSetOnOwner()
{
	ACharacter* const Ch = Cast<ACharacter>(GetOwner());
	if (!Ch)
	{
		return;
	}
	USkeletalMeshComponent* const SkelMesh = Ch->GetMesh();
	if (!SkelMesh)
	{
		return;
	}
	UUDFAnimInstance* const Anim =
		Cast<UUDFAnimInstance>(SkelMesh->GetAnimInstance());
	if (!Anim)
	{
		return;
	}
	if (IsSlotEmpty(EEquipmentSlot::Weapon))
	{
		Anim->RevertToDefaultAnimSet();
		return;
	}
	FDFItemTableRow WeaponRow;
	if (TryGetEquippedItemData(EEquipmentSlot::Weapon, WeaponRow))
	{
		if (WeaponRow.WeaponAnimSet.IsValid())
		{
			Anim->ApplyAnimSet(WeaponRow.WeaponAnimSet);
		}
		else
		{
			Anim->RevertToDefaultAnimSet();
		}
	}
	else
	{
		Anim->RevertToDefaultAnimSet();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Server RPCs
// ─────────────────────────────────────────────────────────────────────────────
void UDFEquipmentComponent::ServerEquipItem_Implementation(
	const FName ItemRowName, const EEquipmentSlot Slot, const int32 SourceBagSlotIndex)
{
	FString E;
	EquipItemInternal(ItemRowName, Slot, E, SourceBagSlotIndex);
}

bool UDFEquipmentComponent::ServerEquipItem_Validate(
	const FName ItemRowName, const EEquipmentSlot Slot, const int32 SourceBagSlotIndex)
{
	if (ItemRowName.IsNone() || Slot == EEquipmentSlot::None)
	{
		return false;
	}
	if (SourceBagSlotIndex != INDEX_NONE && SourceBagSlotIndex < 0)
	{
		return false;
	}
	return true;
}

void UDFEquipmentComponent::ServerUnequipSlot_Implementation(const EEquipmentSlot Slot)
{
	UnequipSlotInternal(Slot, INDEX_NONE);
}

bool UDFEquipmentComponent::ServerUnequipSlot_Validate(const EEquipmentSlot Slot)
{
	return Slot != EEquipmentSlot::None;
}

void UDFEquipmentComponent::ServerUnequipSlotToBagIndex_Implementation(
	const EEquipmentSlot Slot,
	const int32 TargetBagSlotIndex)
{
	if (AActor* const Ox = GetOwner())
	{
		DF_LOG(Verbose,
			"[DF|Eq|Unequip|RPC] owner=%s slot=%s -> bag[%d]",
			*Ox->GetName(),
			*DF_DebugEqSlotName(Slot),
			TargetBagSlotIndex);
	}
	UnequipSlotInternal(Slot, TargetBagSlotIndex);
}

bool UDFEquipmentComponent::ServerUnequipSlotToBagIndex_Validate(
	const EEquipmentSlot Slot,
	const int32 TargetBagSlotIndex)
{
	return Slot != EEquipmentSlot::None && TargetBagSlotIndex >= 0 && TargetBagSlotIndex < 512;
}