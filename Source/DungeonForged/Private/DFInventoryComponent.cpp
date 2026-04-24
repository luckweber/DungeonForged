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
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

namespace
{
void TryGoldCombatText(AActor* const Owner, FDFItemTableRow const& Row, int32 const Gained)
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
}

void UDFInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UDFInventoryComponent, MaxSlots);
	DOREPLIFETIME(UDFInventoryComponent, ItemDataTable);
	DOREPLIFETIME(UDFInventoryComponent, Items);
}

void UDFInventoryComponent::OnRep_Items()
{
	OnInventoryChanged.Broadcast();
}

UAbilitySystemComponent* UDFInventoryComponent::ResolveOwnerASC() const
{
	if (const IAbilitySystemInterface* I = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		return I->GetAbilitySystemComponent();
	}
	if (const AActor* O = GetOwner())
	{
		if (const APawn* P = Cast<APawn>(O))
		{
			if (APlayerState* PS = P->GetPlayerState())
			{
				if (IAbilitySystemInterface* I2 = Cast<IAbilitySystemInterface>(PS))
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

const FDFItemTableRow* UDFInventoryComponent::GetItemData(FName RowName) const
{
	if (!ItemDataTable || RowName.IsNone())
	{
		return nullptr;
	}
	return ItemDataTable->FindRow<FDFItemTableRow>(RowName, TEXT("UDFInventory|GetItemData"));
}

bool UDFInventoryComponent::AddItem(FName RowName, int32 Quantity)
{
	if (!IsAuthority() || !ItemDataTable || RowName.IsNone() || Quantity < 1)
	{
		return false;
	}
	const FDFItemTableRow* const Row = GetItemData(RowName);
	if (!Row)
	{
		return false;
	}
	int32 QtyLeft = Quantity;
	int32 Gained = 0;
	const int32 MaxStack = FMath::Max(1, Row->MaxStack);

	// fill existing stacks
	for (FDFInventorySlot& S : Items)
	{
		if (S.RowName != RowName)
		{
			continue;
		}
		const int32 Room = MaxStack - S.Quantity;
		if (Room <= 0)
		{
			continue;
		}
		const int32 ToAdd = FMath::Min(Room, QtyLeft);
		S.Quantity += ToAdd;
		Gained += ToAdd;
		QtyLeft -= ToAdd;
		if (QtyLeft <= 0)
		{
			TryGoldCombatText(GetOwner(), *Row, Gained);
			OnInventoryChanged.Broadcast();
			return true;
		}
	}
	// new slots
	while (QtyLeft > 0)
	{
		if (Items.Num() >= MaxSlots)
		{
			TryGoldCombatText(GetOwner(), *Row, Gained);
			return QtyLeft < Quantity; // partial success
		}
		const int32 ToAdd = FMath::Min(MaxStack, QtyLeft);
		FDFInventorySlot N;
		N.RowName = RowName;
		N.Quantity = ToAdd;
		N.bIsEquipped = false;
		Items.Add(N);
		Gained += ToAdd;
		QtyLeft -= ToAdd;
	}
	TryGoldCombatText(GetOwner(), *Row, Gained);
	OnInventoryChanged.Broadcast();
	return true;
}

void UDFInventoryComponent::RemoveItem(FName RowName, int32 Quantity)
{
	if (!IsAuthority() || RowName.IsNone() || Quantity < 1)
	{
		return;
	}
	int32 ToRemove = Quantity;
	for (int32 I = Items.Num() - 1; I >= 0 && ToRemove > 0; --I)
	{
		FDFInventorySlot& S = Items[I];
		if (S.RowName != RowName)
		{
			continue;
		}
		if (S.Quantity <= ToRemove)
		{
			ToRemove -= S.Quantity;
			if (S.bIsEquipped)
			{
				UnequipItem(I);
			}
			Items.RemoveAt(I);
			ReindexEquipHandlesAfterRemoveAt(I);
		}
		else
		{
			S.Quantity -= ToRemove;
			ToRemove = 0;
		}
	}
	if (ToRemove < Quantity)
	{
		OnInventoryChanged.Broadcast();
	}
}

void UDFInventoryComponent::ReindexEquipHandlesAfterRemoveAt(int32 RemovedIndex)
{
	TMap<int32, FActiveGameplayEffectHandle> NewM;
	for (const TPair<int32, FActiveGameplayEffectHandle>& P : EquipHandles)
	{
		if (P.Key < RemovedIndex)
		{
			NewM.Add(P.Key, P.Value);
		}
		else if (P.Key > RemovedIndex)
		{
			NewM.Add(P.Key - 1, P.Value);
		}
	}
	EquipHandles = NewM;
}

void UDFInventoryComponent::UnequipItem(int32 SlotIndex)
{
	if (!IsAuthority() || !Items.IsValidIndex(SlotIndex))
	{
		return;
	}
	FDFInventorySlot& S = Items[SlotIndex];
	if (!S.bIsEquipped)
	{
		return;
	}
	const FDFItemTableRow* const Row = GetItemData(S.RowName);
	if (UAbilitySystemComponent* const ASC = ResolveOwnerASC())
	{
		if (const FActiveGameplayEffectHandle* H = EquipHandles.Find(SlotIndex))
		{
			ASC->RemoveActiveGameplayEffect(*H);
		}
	}
	EquipHandles.Remove(SlotIndex);
	S.bIsEquipped = false;
	OnInventoryChanged.Broadcast();
}

void UDFInventoryComponent::UnequipOthersOfType(const EItemType Type, int32 ExceptSlot)
{
	if (!ItemDataTable)
	{
		return;
	}
	for (int32 I = 0; I < Items.Num(); ++I)
	{
		if (I == ExceptSlot)
		{
			continue;
		}
		if (!Items[I].bIsEquipped)
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

void UDFInventoryComponent::EquipItem(int32 SlotIndex)
{
	if (!IsAuthority() || !Items.IsValidIndex(SlotIndex))
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
	if (!Row->OnEquipEffect)
	{
		S.bIsEquipped = true;
		OnInventoryChanged.Broadcast();
		return;
	}
	UAbilitySystemComponent* const ASC = ResolveOwnerASC();
	if (!ASC)
	{
		return;
	}
	UnequipOthersOfType(Row->ItemType, SlotIndex);
	FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
	AActor* const OwnerActor = GetOwner();
	if (OwnerActor)
	{
		Ctx.AddInstigator(OwnerActor, OwnerActor);
	}
	const UGameplayEffect* const EquipGE = Row->OnEquipEffect
		? Row->OnEquipEffect.GetDefaultObject()
		: nullptr;
	if (!EquipGE)
	{
		return;
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
	AActor* const OwnerActor = GetOwner();
	if (OwnerActor)
	{
		if (FVector::Dist(OwnerActor->GetActorLocation(), Source->GetActorLocation()) > 500.f)
		{
			return;
		}
	}
	if (AddItem(Source->GetItemRowName(), 1))
	{
		AActor* Inst = Cast<APawn>(OwnerActor);
		if (!Inst)
		{
			if (APlayerState* const PS = Cast<APlayerState>(OwnerActor))
			{
				Inst = PS->GetPawn();
			}
		}
		Source->OnPickedUp.Broadcast(Inst, Source->GetItemRowName());
		Source->Multicast_PlayPickupVFX(Inst, Source->GetActorLocation());
		Source->Destroy();
	}
}

bool UDFInventoryComponent::ServerPickUpFromLoot_Validate(ADFLootDrop* Source)
{
	return IsValid(Source);
}
