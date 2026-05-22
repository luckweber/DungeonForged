// Source/DungeonForged/Private/Debug/UDFCheatManager.cpp

#include "Debug/UDFCheatManager.h"

#if !UE_BUILD_SHIPPING
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ADFDungeonManager.h"
#include "Boss/ADFBossBase.h"
#include "Characters/ADFEnemyBase.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Camera/UDFLockOnComponent.h"
#include "Combat/DFDodgeDebug.h"
#include "Combat/DFLockOnDebug.h"
#include "Combat/UDFMeleeTraceComponent.h"
#include "Characters/UDFCharacterMovementComponent.h"
#include "Debug/UDFDebugComponent.h"
#include "Characters/ADFPlayerController.h"
#include "Characters/ADFPlayerState.h"
#include "Data/DFDataTableStructs.h"
#include "DFInventoryComponent.h"
#include "Equipment/DFEquipmentTypes.h"
#include "Equipment/UDFEquipmentComponent.h"
#include "DungeonForgedModule.h"
#include "Engine/Engine.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFGameplayAbility.h"
#include "GAS/Effects/UDFGameplayEffectLibrary.h"
#include "GAS/UDFAttributeSet.h"
#include "HAL/IConsoleManager.h"
#include "EngineUtils.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Progression/UDFLevelingComponent.h"
#include "Run/DFRunManager.h"

namespace
{
static UWorld* GetCheatWorld()
{
	if (!GEngine)
	{
		return nullptr;
	}
	if (GEngine->GameViewport && GEngine->GameViewport->GetWorld())
	{
		return GEngine->GameViewport->GetWorld();
	}
	for (const FWorldContext& C : GEngine->GetWorldContexts())
	{
		if (C.World())
		{
			return C.World();
		}
	}
	return nullptr;
}

static APlayerController* GetLocalPC(UWorld* const W)
{
	return W ? GEngine->GetFirstLocalPlayerController(W) : nullptr;
}

static ADFPlayerCharacter* GetLocalDFPawn(UWorld* const W)
{
	APlayerController* const PC = GetLocalPC(W);
	return PC ? Cast<ADFPlayerCharacter>(PC->GetPawn()) : nullptr;
}

static ADFPlayerState* GetLocalDFPS(UWorld* const W)
{
	APlayerController* const PC = GetLocalPC(W);
	return PC ? PC->GetPlayerState<ADFPlayerState>() : nullptr;
}

static UAbilitySystemComponent* GetLocalASC(UWorld* const W)
{
	if (ADFPlayerState* const PS = GetLocalDFPS(W))
	{
		return PS->AbilitySystemComponent;
	}
	return nullptr;
}

static bool HasServerAuth(UWorld* const W, APlayerController* const PC)
{
	if (!W || !PC)
	{
		return false;
	}
	if (APawn* const P = PC->GetPawn())
	{
		return P->HasAuthority();
	}
	return W->GetNetMode() != NM_Client;
}

static UClass* ResolveGameplayEffectClass(FString const& In)
{
	FString Name = In.TrimStartAndEnd();
	if (Name.IsEmpty())
	{
		return nullptr;
	}
	if (!Name.Contains(TEXT("/")))
	{
		Name = FString::Printf(TEXT("/Script/DungeonForged.%s"), *Name);
	}
	if (UClass* const C = FindObject<UClass>(nullptr, *Name))
	{
		return C;
	}
	return LoadClass<UGameplayEffect>(nullptr, *Name);
}

static void Cmd_df_god(TArray<FString> const& Args)
{
	UWorld* const W = GetCheatWorld();
	UAbilitySystemComponent* const ASC = GetLocalASC(W);
	if (!ASC)
	{
		DF_LOG(Warning, "df.god: no ASC");
		return;
	}
	if (!HasServerAuth(W, GetLocalPC(W)))
	{
		DF_LOG(Warning, "df.god: need authority (host / listen server)");
		return;
	}
	int32 On = 1;
	if (Args.Num() > 0)
	{
		On = FCString::Atoi(*Args[0]) != 0 ? 1 : 0;
	}
	if (On)
	{
		ASC->AddLooseGameplayTag(FDFGameplayTags::State_Invulnerable);
	}
	else
	{
		ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Invulnerable, 1);
	}
}

static void Cmd_df_levelup(TArray<FString> const& Args)
{
	UWorld* const W = GetCheatWorld();
	ADFPlayerState* const PS = GetLocalDFPS(W);
	if (!PS || !PS->LevelingComponent)
	{
		return;
	}
	if (!HasServerAuth(W, GetLocalPC(W)))
	{
		return;
	}
	int32 N = 1;
	if (Args.Num() > 0)
	{
		N = FMath::Max(1, FCString::Atoi(*Args[0]));
	}
	PS->LevelingComponent->Dev_CheatLevelUp(N);
}

static void Cmd_df_setlevel(TArray<FString> const& Args)
{
	UWorld* const W = GetCheatWorld();
	ADFPlayerState* const PS = GetLocalDFPS(W);
	if (!PS || !PS->LevelingComponent || Args.Num() < 1)
	{
		return;
	}
	if (!HasServerAuth(W, GetLocalPC(W)))
	{
		return;
	}
	PS->LevelingComponent->Dev_CheatSetLevel(FCString::Atoi(*Args[0]));
}

static void Cmd_df_addxp(TArray<FString> const& Args)
{
	UWorld* const W = GetCheatWorld();
	ADFPlayerState* const PS = GetLocalDFPS(W);
	if (!PS || !PS->LevelingComponent || Args.Num() < 1)
	{
		return;
	}
	if (!HasServerAuth(W, GetLocalPC(W)))
	{
		return;
	}
	PS->LevelingComponent->AddXP(FCString::Atoi(*Args[0]));
}

static void Cmd_df_fullheal(TArray<FString> const& /*Args*/)
{
	UWorld* const W = GetCheatWorld();
	if (UAbilitySystemComponent* const ASC = GetLocalASC(W))
	{
		if (!HasServerAuth(W, GetLocalPC(W)))
		{
			return;
		}
		const float Mh = ASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute());
		const float Ch = ASC->GetNumericAttribute(UDFAttributeSet::GetHealthAttribute());
		const float D = FMath::Max(0.f, Mh - Ch);
		if (ADFPlayerCharacter* const P = GetLocalDFPawn(W))
		{
			const FGameplayEffectSpecHandle H = UDFGameplayEffectLibrary::MakeHealEffect(D, P);
			if (H.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*H.Data);
			}
		}
	}
}

static void Cmd_df_fullmana(TArray<FString> const& /*Args*/)
{
	UWorld* const W = GetCheatWorld();
	if (UAbilitySystemComponent* const ASC = GetLocalASC(W))
	{
		if (!HasServerAuth(W, GetLocalPC(W)))
		{
			return;
		}
		const float Mm = ASC->GetNumericAttribute(UDFAttributeSet::GetMaxManaAttribute());
		ASC->SetNumericAttributeBase(UDFAttributeSet::GetManaAttribute(), Mm);
	}
}

static void Cmd_df_addgold(TArray<FString> const& Args)
{
	if (Args.Num() < 1)
	{
		return;
	}
	UWorld* const W = GetCheatWorld();
	UGameInstance* const GI = W ? W->GetGameInstance() : nullptr;
	UDFRunManager* const RM = GI ? GI->GetSubsystem<UDFRunManager>() : nullptr;
	if (!RM)
	{
		return;
	}
	if (!HasServerAuth(W, GetLocalPC(W)))
	{
		return;
	}
	RM->AddGold(FCString::Atoi(*Args[0]));
}

static void Cmd_df_giveitem(TArray<FString> const& Args)
{
	if (Args.Num() < 1)
	{
		DF_LOG(Warning, "df.giveitem RowName - adds to inventory only; use df.equip to wear (host / listen).");
		return;
	}
	UWorld* const W = GetCheatWorld();
	ADFPlayerCharacter* const P = GetLocalDFPawn(W);
	if (!P)
	{
		DF_LOG(Warning, "df.giveitem: no ADFPlayerCharacter pawn");
		return;
	}
	UDFInventoryComponent* const Inv = P->GetDFInventory();
	if (!Inv)
	{
		DF_LOG(Warning, "df.giveitem: no UDFInventoryComponent on pawn");
		return;
	}
	if (!HasServerAuth(W, GetLocalPC(W)))
	{
		DF_LOG(Warning, "df.giveitem: need host / listen-server authority on pawn");
		return;
	}
	const FName Row(*Args[0]);
	if (Inv->AddItem(Row, 1))
	{
		DF_LOG(Log, "df.giveitem: +1 %s (bag only, not equipped)", *Row.ToString());
	}
	else
	{
		DF_LOG(Warning,
			"df.giveitem: AddItem failed (%s) - set ItemDataTable on Inventory or Equipment plus valid row",
			*Row.ToString());
	}
}

static void Cmd_df_equip(TArray<FString> const& Args)
{
	if (Args.Num() < 1)
	{
		DF_LOG(Warning,
			"df.equip RowName [Weapon|Helmet|...] - adds to bag then equips via UDFEquipmentComponent (host/listen).");
		return;
	}
	UWorld* const W = GetCheatWorld();
	APlayerController* const PC = GetLocalPC(W);
	ADFPlayerCharacter* const P = GetLocalDFPawn(W);
	if (!P || !PC)
	{
		DF_LOG(Warning, "df.equip: no local pawn");
		return;
	}
	if (!HasServerAuth(W, PC))
	{
		DF_LOG(Warning, "df.equip: need host / listen pawn authority (equip runs on server)");
		return;
	}
	UDFInventoryComponent* const Inv = P->GetDFInventory();
	UDFEquipmentComponent* const Eq = P->GetDFEquipment();
	if (!Inv || !Eq)
	{
		DF_LOG(Warning, "df.equip: missing Inventory or Equipment");
		return;
	}
	if (!Inv->ItemDataTable && Eq->ItemDataTable)
	{
		Inv->ItemDataTable = Eq->ItemDataTable;
	}
	const FName Row(*Args[0]);
	if (!Inv->AddItem(Row, 1))
	{
		DF_LOG(Warning, "df.equip: AddItem failed for %s - check DT + row name", *Row.ToString());
	}
	const FDFItemTableRow* const R = Inv->GetItemData(Row);
	if (!R)
	{
		DF_LOG(Warning, "df.equip: row not in Inv->ItemDataTable: %s", *Row.ToString());
		return;
	}
	EEquipmentSlot TargetSlot = EEquipmentSlot::None;
	if (Args.Num() >= 2)
	{
		const UEnum* const En = StaticEnum<EEquipmentSlot>();
		const int64 V = En->GetValueByName(FName(*Args[1]));
		if (V == INDEX_NONE)
		{
			DF_LOG(Warning, "df.equip: unknown slot %s (try Weapon, Helmet, ...)", *Args[1]);
			return;
		}
		TargetSlot = static_cast<EEquipmentSlot>(V);
	}
	else if (R->TargetEquipmentSlot != EEquipmentSlot::None)
	{
		TargetSlot = R->TargetEquipmentSlot;
	}
	else
	{
		TargetSlot = UDFEquipmentComponent::ResolveItemEquipmentSlot(*R);
	}
	if (TargetSlot == EEquipmentSlot::None)
	{
		DF_LOG(Warning, "df.equip: could not resolve slot for %s", *Row.ToString());
		return;
	}
	FString Err;
	if (!Eq->PredictCanEquipItem(Row, TargetSlot, Err) && R->ItemType == EItemType::Ring
		&& R->TargetEquipmentSlot == EEquipmentSlot::None)
	{
		if (Eq->PredictCanEquipItem(Row, EEquipmentSlot::Ring1, Err))
		{
			TargetSlot = EEquipmentSlot::Ring1;
		}
		else if (Eq->PredictCanEquipItem(Row, EEquipmentSlot::Ring2, Err))
		{
			TargetSlot = EEquipmentSlot::Ring2;
		}
	}
	if (!Eq->PredictCanEquipItem(Row, TargetSlot, Err))
	{
		DF_LOG(Warning, "df.equip: cannot equip %s to %s: %s", *Row.ToString(),
			*StaticEnum<EEquipmentSlot>()->GetNameStringByValue(static_cast<int64>(TargetSlot)), *Err);
		return;
	}
	Eq->EquipItem(Row, TargetSlot);
	DF_LOG(Log, "df.equip: sent %s -> %s (listen host applies immediately; check df.dumpgear)",
		*Row.ToString(),
		*StaticEnum<EEquipmentSlot>()->GetNameStringByValue(static_cast<int64>(TargetSlot)));
}

static void Cmd_df_dumpgear(TArray<FString> const& /*Args*/)
{
	UWorld* const W = GetCheatWorld();
	ADFPlayerCharacter* const P = GetLocalDFPawn(W);
	if (!P)
	{
		DF_LOG(Warning, "df.dumpgear: no local pawn");
		return;
	}
	UDFInventoryComponent* const Inv = P->GetDFInventory();
	UDFEquipmentComponent* const Eq = P->GetDFEquipment();
	DF_LOG(Log, "--- df.dumpgear ---");
	if (!Eq)
	{
		DF_LOG(Warning, "  (no UDFEquipmentComponent)");
	}
	else
	{
		DF_LOG(Log, "  Equipped (EquippedItems map):");
		for (const TPair<EEquipmentSlot, FName>& Kvp : Eq->EquippedItems)
		{
			if (!Kvp.Value.IsNone())
			{
				const FDFItemTableRow* Row = Inv ? Inv->GetItemData(Kvp.Value) : nullptr;
				if (!Row && Eq->ItemDataTable)
				{
					Row = Eq->ItemDataTable->FindRow<FDFItemTableRow>(Kvp.Value, TEXT("dumpgear"));
				}
				const TCHAR* IconState = Row && Row->Icon ? TEXT("has Icon") : TEXT("NO Icon");
				DF_LOG(Log, "    %s = %s (%s)",
					*StaticEnum<EEquipmentSlot>()->GetNameStringByValue(static_cast<int64>(Kvp.Key)),
					*Kvp.Value.ToString(), IconState);
			}
		}
	}
	if (!Inv)
	{
		DF_LOG(Warning, "  (no UDFInventoryComponent)");
		return;
	}
	DF_LOG(Log, "  Bag (%d slots, MaxSlots=%d), ItemDataTable=%s",
		Inv->Items.Num(),
		Inv->MaxSlots,
		Inv->ItemDataTable ? *Inv->ItemDataTable->GetName() : TEXT("NULL"));
	for (int32 I = 0; I < Inv->Items.Num(); ++I)
	{
		const FDFInventorySlot& S = Inv->Items[I];
		const FDFItemTableRow* const Row = Inv->GetItemData(S.RowName);
		const TCHAR* IconState = Row && Row->Icon ? TEXT("has Icon") : TEXT("NO Icon");
		DF_LOG(Log, "    [%d] %s x%d equipped=%d - %s", I, *S.RowName.ToString(), S.Quantity,
			S.bIsEquipped ? 1 : 0, IconState);
	}
}

static void Cmd_df_giveability(TArray<FString> const& Args)
{
	if (Args.Num() < 1)
	{
		return;
	}
	UWorld* const W = GetCheatWorld();
	UGameInstance* const GI = W ? W->GetGameInstance() : nullptr;
	UDFRunManager* const RM = GI ? GI->GetSubsystem<UDFRunManager>() : nullptr;
	ADFPlayerState* const PS = GetLocalDFPS(W);
	if (!RM || !PS)
	{
		return;
	}
	if (!HasServerAuth(W, GetLocalPC(W)))
	{
		return;
	}
	RM->AddAbilityReward(FName(*Args[0]));
	RM->GrantAbilitiesForCurrentRun(PS);
}

static void Cmd_df_nextfloor(TArray<FString> const& /*Args*/)
{
	UWorld* const W = GetCheatWorld();
	UGameInstance* const GI = W ? W->GetGameInstance() : nullptr;
	UDFDungeonManager* const DM = GI ? GI->GetSubsystem<UDFDungeonManager>() : nullptr;
	if (!DM)
	{
		return;
	}
	if (!HasServerAuth(W, GetLocalPC(W)))
	{
		return;
	}
	DM->Dev_ForceFloorCleared();
}

static void Cmd_df_skipboss(TArray<FString> const& /*Args*/)
{
	UWorld* const W = GetCheatWorld();
	if (!W || !HasServerAuth(W, GetLocalPC(W)))
	{
		return;
	}
	ADFPlayerCharacter* const P = GetLocalDFPawn(W);
	UAbilitySystemComponent* const Src = P ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(P) : nullptr;
	for (TActorIterator<ADFBossBase> It(W); It; ++It)
	{
		ADFBossBase* const B = *It;
		if (!B)
		{
			continue;
		}
		if (UAbilitySystemComponent* const T = B->GetAbilitySystemComponent())
		{
			if (Src)
			{
				const FGameplayEffectSpecHandle H = UDFGameplayEffectLibrary::MakeDamageEffect(
					999999.f, FDFGameplayTags::Effect_Damage_True, P);
				if (H.IsValid())
				{
					Src->ApplyGameplayEffectSpecToTarget(*H.Data, T);
				}
			}
		}
	}
	if (UGameInstance* const GI = W->GetGameInstance())
	{
		if (UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>())
		{
			RM->OnRunCompleted();
		}
	}
}

static void Cmd_df_spawnboss(TArray<FString> const& Args)
{
	if (Args.Num() < 1)
	{
		return;
	}
	UWorld* const W = GetCheatWorld();
	ADFPlayerCharacter* const P = GetLocalDFPawn(W);
	UGameInstance* const GI = W ? W->GetGameInstance() : nullptr;
	UDFDungeonManager* const DM = GI ? GI->GetSubsystem<UDFDungeonManager>() : nullptr;
	if (!DM || !P)
	{
		return;
	}
	if (!HasServerAuth(W, GetLocalPC(W)))
	{
		return;
	}
	DM->Dev_SpawnAt(FName(*Args[0]), P);
}

static void Cmd_df_spawnenemy(TArray<FString> const& Args)
{
	if (Args.Num() < 1)
	{
		return;
	}
	UWorld* const W = GetCheatWorld();
	ADFPlayerCharacter* const P = GetLocalDFPawn(W);
	UGameInstance* const GI = W ? W->GetGameInstance() : nullptr;
	UDFDungeonManager* const DM = GI ? GI->GetSubsystem<UDFDungeonManager>() : nullptr;
	if (!DM || !P)
	{
		return;
	}
	if (!HasServerAuth(W, GetLocalPC(W)))
	{
		return;
	}
	const int32 C = Args.Num() > 1 ? FMath::Max(1, FCString::Atoi(*Args[1])) : 1;
	DM->Dev_SpawnEnemiesAt(FName(*Args[0]), C, P);
}

static void Cmd_df_killall(TArray<FString> const& /*Args*/)
{
	UWorld* const W = GetCheatWorld();
	ADFPlayerCharacter* const P = GetLocalDFPawn(W);
	UAbilitySystemComponent* const Src = P ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(P) : nullptr;
	if (!W || !Src || !HasServerAuth(W, GetLocalPC(W)))
	{
		return;
	}
	for (TActorIterator<ADFEnemyBase> It(W); It; ++It)
	{
		ADFEnemyBase* const E = *It;
		if (!E)
		{
			continue;
		}
		if (UAbilitySystemComponent* const T = E->GetAbilitySystemComponent())
		{
			const FGameplayEffectSpecHandle H = UDFGameplayEffectLibrary::MakeDamageEffect(
				99999.f, FDFGameplayTags::Effect_Damage_True, P);
			if (H.IsValid())
			{
				Src->ApplyGameplayEffectSpecToTarget(*H.Data, T);
			}
		}
	}
}

static void Cmd_df_revealminimap(TArray<FString> const& /*Args*/)
{
	UWorld* const W = GetCheatWorld();
	UGameInstance* const GI = W ? W->GetGameInstance() : nullptr;
	UDFDungeonManager* const DM = GI ? GI->GetSubsystem<UDFDungeonManager>() : nullptr;
	if (!DM)
	{
		return;
	}
	DM->Dev_RevealAllMinimapRooms();
}

static void Cmd_df_showtags(TArray<FString> const& /*Args*/)
{
	UWorld* const W = GetCheatWorld();
	APlayerController* const PC = GetLocalPC(W);
	if (ADFPlayerController* const DPC = Cast<ADFPlayerController>(PC))
	{
		DPC->ToggleGASDebugOverlay();
	}
}

static void Cmd_df_showattributes(TArray<FString> const& /*Args*/)
{
	UWorld* const W = GetCheatWorld();
	ADFPlayerCharacter* const Ch = GetLocalDFPawn(W);
	if (!Ch)
	{
		return;
	}
	if (Ch->DFDebug)
	{
		Ch->DFDebug->DrawAttributeDebug();
	}
}

static void Cmd_df_granteffect(TArray<FString> const& Args)
{
	if (Args.Num() < 1)
	{
		return;
	}
	UWorld* const W = GetCheatWorld();
	UAbilitySystemComponent* const ASC = GetLocalASC(W);
	ADFPlayerCharacter* const P = GetLocalDFPawn(W);
	if (!ASC || !P)
	{
		return;
	}
	if (!HasServerAuth(W, GetLocalPC(W)))
	{
		return;
	}
	UClass* const Ge = ResolveGameplayEffectClass(Args[0]);
	if (!Ge || !Ge->IsChildOf(UGameplayEffect::StaticClass()))
	{
		DF_LOG(Warning, "df.granteffect: unknown GE class %s", *Args[0]);
		return;
	}
	const float Dur = Args.Num() > 1 ? FCString::Atof(*Args[1]) : -1.f;
	FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
	Ctx.AddInstigator(P, P);
	FGameplayEffectSpecHandle Sh = ASC->MakeOutgoingSpec(Ge, 1.f, Ctx);
	if (Sh.IsValid() && Sh.Data && Dur > 0.f)
	{
		Sh.Data->SetDuration(Dur, true);
	}
	if (Sh.IsValid() && Sh.Data)
	{
		ASC->ApplyGameplayEffectSpecToSelf(*Sh.Data);
	}
}

static void Cmd_df_removeeffect(TArray<FString> const& Args)
{
	if (Args.Num() < 1)
	{
		return;
	}
	UWorld* const W = GetCheatWorld();
	UAbilitySystemComponent* const ASC = GetLocalASC(W);
	if (!ASC || !HasServerAuth(W, GetLocalPC(W)))
	{
		return;
	}
	UClass* const Ge = ResolveGameplayEffectClass(Args[0]);
	if (!Ge || !Ge->IsChildOf(UGameplayEffect::StaticClass()))
	{
		return;
	}
	TSet<FActiveGameplayEffectHandle> Seen;
	auto AddQuery = [ASC, &Seen](const FGameplayTag& T)
	{
		if (!T.IsValid())
		{
			return;
		}
		FGameplayTagContainer C;
		C.AddTag(T);
		const FGameplayEffectQuery Q = FGameplayEffectQuery::MakeQuery_MatchAnyEffectTags(C);
		for (const FActiveGameplayEffectHandle& H : ASC->GetActiveEffects(Q))
		{
			Seen.Add(H);
		}
	};
	AddQuery(FGameplayTag::RequestGameplayTag(FName("Effect"), false));
	AddQuery(FGameplayTag::RequestGameplayTag(FName("State"), false));
	AddQuery(FGameplayTag::RequestGameplayTag(FName("Ability"), false));
	AddQuery(FDFGameplayTags::Effect_Damage_Physical);
	AddQuery(FDFGameplayTags::Effect_Damage_True);
	for (const FActiveGameplayEffectHandle& H : Seen)
	{
		if (const FActiveGameplayEffect* const E = ASC->GetActiveGameplayEffect(H))
		{
			if (E->Spec.Def && E->Spec.Def->GetClass() == Ge)
			{
				ASC->RemoveActiveGameplayEffect(H, 1);
			}
		}
	}
}

static void Cmd_df_clearcd(TArray<FString> const& /*Args*/)
{
	UWorld* const W = GetCheatWorld();
	UAbilitySystemComponent* const ASC = GetLocalASC(W);
	if (!ASC || !HasServerAuth(W, GetLocalPC(W)))
	{
		return;
	}
	FGameplayTagContainer Owned;
	ASC->GetOwnedGameplayTags(Owned);
	TArray<FGameplayTag> Arr;
	Owned.GetGameplayTagArray(Arr);
	for (const FGameplayTag& T : Arr)
	{
		if (T.MatchesTag(FDFGameplayTags::Ability_Cooldown) && !T.MatchesTagExact(FDFGameplayTags::Ability_Cooldown))
		{
			const int32 Cnt = ASC->GetGameplayTagCount(T);
			if (Cnt > 0)
			{
				ASC->RemoveLooseGameplayTag(T, Cnt);
			}
		}
	}
}

static void Cmd_df_meleedebug(TArray<FString> const& Args)
{
	UWorld* const W = GetCheatWorld();
	APlayerController* const PC = GetLocalPC(W);
	if (!PC)
	{
		DF_LOG(Warning, "df.meleedebug: no local PlayerController");
		return;
	}

	if (Args.Num() > 0)
	{
		const FString A0 = Args[0].ToLower();
		if (A0 == TEXT("collision") || A0 == TEXT("col"))
		{
			static bool bCollisionVis = false;
			bCollisionVis = !bCollisionVis;
			PC->ConsoleCommand(FString::Printf(TEXT("ShowFlag.Collision %d"), bCollisionVis ? 1 : 0));
			DF_LOG(Log, "df.meleedebug: ShowFlag.Collision = %s (run again to toggle off)", bCollisionVis ? TEXT("1") : TEXT("0"));
			return;
		}
	}

	ADFPlayerCharacter* const P = GetLocalDFPawn(W);
	if (!P || !P->MeleeTrace)
	{
		DF_LOG(Warning, "df.meleedebug: no ADFPlayerCharacter pawn or MeleeTrace");
		return;
	}

	UDFMeleeTraceComponent* const M = P->MeleeTrace;
	if (Args.Num() > 0)
	{
		const FString A = Args[0].ToLower();
		if (A == TEXT("dump"))
		{
			M->bVerboseTraceLog = true;
			M->DumpMeleeTraceDiagnostics();
			DF_LOG(Log, "df.meleedebug dump: full report in Output Log (filter: MeleeTrace)");
			return;
		}
		if (A == TEXT("verbose") || A == TEXT("log"))
		{
			M->bVerboseTraceLog = !M->bVerboseTraceLog;
			if (M->bVerboseTraceLog)
			{
				M->bDrawDebugTrace = true;
			}
			DF_LOG(Log, "df.meleedebug: bVerboseTraceLog=%d bDrawDebugTrace=%d (df.MeleeTraceLog 2 = log every sweep tick)",
				M->bVerboseTraceLog ? 1 : 0, M->bDrawDebugTrace ? 1 : 0);
			if (M->bVerboseTraceLog)
			{
				M->DumpMeleeTraceDiagnostics();
			}
			return;
		}
		if (A == TEXT("1") || A == TEXT("true") || A == TEXT("on"))
		{
			M->bDrawDebugTrace = true;
			M->bVerboseTraceLog = true;
		}
		else if (A == TEXT("0") || A == TEXT("false") || A == TEXT("off"))
		{
			M->bDrawDebugTrace = false;
			M->bVerboseTraceLog = false;
		}
		else
		{
			M->bDrawDebugTrace = !M->bDrawDebugTrace;
			if (M->bDrawDebugTrace)
			{
				M->bVerboseTraceLog = true;
			}
		}
	}
	else
	{
		M->bDrawDebugTrace = !M->bDrawDebugTrace;
		if (M->bDrawDebugTrace)
		{
			M->bVerboseTraceLog = true;
		}
	}

	const FString MeshDiag = M->GetMeleeTraceDiagnosticString();
	DF_LOG(Log, "df.meleedebug: draw=%d verbose=%d | radius=%.1f | %s -> %s | %s",
		M->bDrawDebugTrace ? 1 : 0,
		M->bVerboseTraceLog ? 1 : 0,
		M->TraceRadius,
		*M->TraceStartSocket.ToString(),
		*M->TraceEndSocket.ToString(),
		*MeshDiag);
	DF_LOG(Log, "df.meleedebug dump | verbose | collision — df.MeleeTraceLog 2 = cada tick do sweep");
	DF_LOG(Log, "df.DebugMeleeWeapon 1: linha continua nos sockets da espada");
}

static void Cmd_df_dumpabilities(TArray<FString> const& /*Args*/)
{
	UWorld* const W = GetCheatWorld();
	ADFPlayerCharacter* const P = GetLocalDFPawn(W);
	UAbilitySystemComponent* const ASC = GetLocalASC(W);
	if (!P)
	{
		DF_LOG(Warning, "df.dumpabilities: no local ADFPlayerCharacter pawn");
		return;
	}
	if (!ASC)
	{
		DF_LOG(Warning, "df.dumpabilities: no AbilitySystemComponent (PlayerState?)");
		return;
	}

	UGameInstance* const GI = W ? W->GetGameInstance() : nullptr;
	const UDFRunManager* const RM = GI ? GI->GetSubsystem<UDFRunManager>() : nullptr;
	UDataTable* const AbilityDT = RM ? RM->AbilityDataTable : nullptr;

	static constexpr float DumpScreenDuration = 22.f;
	static const FVector2D DumpTextScale(1.65f, 1.65f);
	// Key -1: cada linha fica visível em stack (evita sobrescrever com mesma key); escala ajuda a ler no viewport.
	auto OnScreen = [](const int32 Key, const float Duration, const FColor Color, const FString& Msg)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(Key, Duration, Color, Msg, true, DumpTextScale);
		}
	};

	DF_LOG(Log, "=== df.dumpabilities (CurrentAbilitySlots + cooldown query) ===");
	OnScreen(-1, DumpScreenDuration, FColor::Cyan, TEXT("=== df.dumpabilities (detalhe no Output Log) ==="));

	const int32 MaxSlots = P->CurrentAbilitySlots.Num();
	for (int32 SlotIndex = 0; SlotIndex < MaxSlots; ++SlotIndex)
	{
		const FName RowName = P->CurrentAbilitySlots.IsValidIndex(SlotIndex) ? P->CurrentAbilitySlots[SlotIndex] : NAME_None;
		if (RowName.IsNone())
		{
			const FString Line = FString::Printf(TEXT("[%d] (empty slot)"), SlotIndex);
			DF_LOG(Log, "%s", *Line);
			OnScreen(-1, DumpScreenDuration, FColor::Silver, Line);
			continue;
		}
		if (!AbilityDT)
		{
			const FString Line = FString::Printf(TEXT("[%d] Row=%s | (no AbilityDataTable on RunManager)"), SlotIndex, *RowName.ToString());
			DF_LOG(Warning, "%s", *Line);
			OnScreen(-1, DumpScreenDuration, FColor::Orange, Line.Left(200));
			continue;
		}

		const FDFAbilityTableRow* const Row = AbilityDT->FindRow<FDFAbilityTableRow>(RowName, TEXT("df.dumpabilities"), false);
		if (!Row)
		{
			const FString Line = FString::Printf(TEXT("[%d] Row=%s | NOT FOUND in DT_Abilities"), SlotIndex, *RowName.ToString());
			DF_LOG(Warning, "%s", *Line);
			OnScreen(-1, DumpScreenDuration, FColor::Red, Line.Left(200));
			continue;
		}

		FString CdoLine = TEXT("CDO=(no AbilityClass)");
		FString SpecLine = TEXT("Spec=(no matching granted ability)");
		if (Row->AbilityClass)
		{
			const UGameplayAbility* const GaDef = Row->AbilityClass->GetDefaultObject<UGameplayAbility>();
			const UDFGameplayAbility* const DfDef = Cast<const UDFGameplayAbility>(GaDef);
			const UGameplayEffect* const CdGeCdo = GaDef ? GaDef->GetCooldownGameplayEffect() : nullptr;
			const FString GeN = (CdGeCdo && CdGeCdo->GetClass()) ? CdGeCdo->GetClass()->GetName() : TEXT("None");
			const float BaseCdVal = DfDef ? DfDef->BaseCooldown : 0.f;
			const FString DfKind = DfDef ? TEXT("UDF") : TEXT("non-UDF");
			CdoLine = FString::Printf(
				TEXT("CDO=%s (%s BaseCooldown=%.2f, CooldownGE=%s)"),
				*Row->AbilityClass->GetName(),
				*DfKind,
				BaseCdVal,
				*GeN);

			for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
			{
				if (!Spec.Ability)
				{
					continue;
				}
				if (Spec.Ability->GetClass() == *Row->AbilityClass)
				{
					SpecLine = FString::Printf(TEXT("Spec Lv=%d IsActive=%s"), Spec.Level, Spec.IsActive() ? TEXT("true") : TEXT("false"));
					break;
				}
			}
		}

		const FString TagStr = Row->AbilityTag.IsValid() ? Row->AbilityTag.ToString() : TEXT("(AbilityTag invalid)");
		const FString IconStr = Row->Icon ? Row->Icon->GetName() : TEXT("NULL");
		FString CdPart = TEXT("| CD: n/a (no AbilityTag)");
		if (Row->AbilityTag.IsValid())
		{
			FGameplayTagContainer QueryTags;
			QueryTags.AddTag(FDFGameplayTags::Ability_Cooldown);
			QueryTags.AddTag(Row->AbilityTag);
			const FGameplayEffectQuery QAll = FGameplayEffectQuery::MakeQuery_MatchAllEffectTags(QueryTags);
			TArray<TPair<float, float>> Times = ASC->GetActiveEffectsTimeRemainingAndDuration(QAll);
			if (Times.Num() == 0)
			{
				FGameplayTagContainer LegacyTags;
				LegacyTags.AddTag(Row->AbilityTag);
				const FGameplayEffectQuery LegacyQuery = FGameplayEffectQuery::MakeQuery_MatchAnyEffectTags(LegacyTags);
				Times = ASC->GetActiveEffectsTimeRemainingAndDuration(LegacyQuery);
			}
			if (Times.Num() > 0)
			{
				const float Rem = Times[0].Key;
				const float Dur = Times[0].Value;
				CdPart = FString::Printf(TEXT("| CD rem=%.2fs total=%.2fs"), Rem, Dur);
			}
			else
			{
				CdPart = TEXT("| CD GE: none (idle; ok if BaseCooldown=0 / no Cooldown GE, e.g. Sprint)");
			}
		}

		const FString LogLine = FString::Printf(
			TEXT("[%d] Row=%s | Tag=%s | %s | Icon=%s %s | %s | %s"),
			SlotIndex,
			*RowName.ToString(),
			*TagStr,
			*Row->DisplayName.ToString(),
			*IconStr,
			*CdPart,
			*SpecLine,
			*CdoLine);
		DF_LOG(Log, "%s", *LogLine);

		const FString Short = FString::Printf(TEXT("[%d] %s | %s | %s"), SlotIndex, *RowName.ToString(), *SpecLine, *CdPart).Left(220);
		OnScreen(-1, DumpScreenDuration, FColor::Yellow, Short);
	}

	DF_LOG(Log, "--- Active GEs matching effect-tag query Ability.Cooldown (any) ---");
	FGameplayTagContainer CdTag;
	CdTag.AddTag(FDFGameplayTags::Ability_Cooldown);
	const FGameplayEffectQuery QCd = FGameplayEffectQuery::MakeQuery_MatchAnyEffectTags(CdTag);
	const TArray<FActiveGameplayEffectHandle> CdHandles = ASC->GetActiveEffects(QCd);
	if (CdHandles.Num() == 0)
	{
		DF_LOG(Log, "  (none)");
		OnScreen(-1, DumpScreenDuration, FColor::Green, TEXT("Ability.Cooldown GEs ativos: 0 (esperado se nada em CD)"));
	}
	else
	{
		for (const FActiveGameplayEffectHandle& H : CdHandles)
		{
			const FActiveGameplayEffect* const E = ASC->GetActiveGameplayEffect(H);
			const float Now = W ? W->GetTimeSeconds() : 0.f;
			const float Rem = E ? E->GetTimeRemaining(Now) : -1.f;
			const float Tot = E ? E->GetDuration() : -1.f;
			const FString GeName = (E && E->Spec.Def) ? E->Spec.Def->GetName() : TEXT("(unknown)");
			const FString GeLine = FString::Printf(TEXT("  %s | rem=%.2f total=%.2f"), *GeName, Rem, Tot);
			DF_LOG(Log, "%s", *GeLine);
		}
		const FString Summary = FString::Printf(TEXT("Ability.Cooldown GEs ativos: %d (ver log)"), CdHandles.Num());
		OnScreen(-1, DumpScreenDuration, FColor::White, Summary);
	}
}

static FAutoConsoleCommand GCmdGod(
	TEXT("df.god"),
	TEXT("df.god [0|1]"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_god));
static FAutoConsoleCommand GCmdLevelUp(
	TEXT("df.levelup"),
	TEXT("df.levelup [N]"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_levelup));
static FAutoConsoleCommand GCmdSetLevel(
	TEXT("df.setlevel"),
	TEXT("df.setlevel N"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_setlevel));
static FAutoConsoleCommand GCmdAddXp(
	TEXT("df.addxp"),
	TEXT("df.addxp N"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_addxp));
static FAutoConsoleCommand GCmdFullHeal(
	TEXT("df.fullheal"),
	TEXT(""),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_fullheal));
static FAutoConsoleCommand GCmdFullMana(
	TEXT("df.fullmana"),
	TEXT(""),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_fullmana));
static FAutoConsoleCommand GCmdAddGold(
	TEXT("df.addgold"),
	TEXT("df.addgold N"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_addgold));
static FAutoConsoleCommand GCmdGiveItem(
	TEXT("df.giveitem"),
	TEXT("Adds one item to inventory by DT row name (bag only — not worn). Requires host/listen authority."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_giveitem));
static FAutoConsoleCommand GCmdEquip(
	TEXT("df.equip"),
	TEXT("df.equip RowName [EquipmentSlot] — add to bag if needed then equip via UDFEquipmentComponent (host/listen)."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_equip));
static FAutoConsoleCommand GCmdDumpGear(
	TEXT("df.dumpgear"),
	TEXT("Log equipped slots + bag contents + Icon presence per row."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_dumpgear));
static FAutoConsoleCommand GCmdGiveAbility(
	TEXT("df.giveability"),
	TEXT("df.giveability RowName"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_giveability));
static FAutoConsoleCommand GCmdNextFloor(
	TEXT("df.nextfloor"),
	TEXT(""),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_nextfloor));
static FAutoConsoleCommand GCmdSkipBoss(
	TEXT("df.skipboss"),
	TEXT(""),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_skipboss));
static FAutoConsoleCommand GCmdSpawnBoss(
	TEXT("df.spawnboss"),
	TEXT("df.spawnboss RowName"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_spawnboss));
static FAutoConsoleCommand GCmdSpawnEnemy(
	TEXT("df.spawnenemy"),
	TEXT("df.spawnenemy RowName [Count]"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_spawnenemy));
static FAutoConsoleCommand GCmdKillAll(
	TEXT("df.killall"),
	TEXT(""),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_killall));
static FAutoConsoleCommand GCmdRevealMinimap(
	TEXT("df.revealminimap"),
	TEXT(""),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_revealminimap));
static FAutoConsoleCommand GCmdShowTags(
	TEXT("df.showtags"),
	TEXT(""),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_showtags));
static FAutoConsoleCommand GCmdShowAttributes(
	TEXT("df.showattributes"),
	TEXT(""),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_showattributes));
static FAutoConsoleCommand GCmdGrantEffect(
	TEXT("df.granteffect"),
	TEXT("df.granteffect ClassName [Duration]"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_granteffect));
static FAutoConsoleCommand GCmdRemoveEffect(
	TEXT("df.removeeffect"),
	TEXT("df.removeeffect ClassName"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_removeeffect));
static FAutoConsoleCommand GCmdClearCd(
	TEXT("df.clearcd"),
	TEXT(""),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_clearcd));
static FAutoConsoleCommand GCmdDumpAbilities(
	TEXT("df.dumpabilities"),
	TEXT("Hotbar slots (CurrentAbilitySlots): row, AbilityTag, icon, CD (same query as HUD) + active GEs with Ability.Cooldown. Log + on-screen."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_dumpabilities));
static FAutoConsoleCommand GCmdMeleeDebug(
	TEXT("df.MeleeDebug"),
	TEXT("Toggle sphere-sweep melee debug durante o sweep. Preview continuo nos sockets: df.DebugMeleeWeapon 1|2. df.MeleeDebug [0|1|on|off] | collision — ShowFlag.Collision"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_meleedebug));

static void Cmd_df_dodgedebug(TArray<FString> const& Args)
{
	IConsoleVariable* const Cv = IConsoleManager::Get().FindConsoleVariable(TEXT("df.DebugDodge"));
	if (!Cv)
	{
		DF_LOG(Warning, "df.DodgeDebug: df.DebugDodge CVar missing (shipping build?)");
		return;
	}

	if (Args.Num() > 0)
	{
		const FString A = Args[0].ToLower();
		if (A == TEXT("dump"))
		{
			Cv->Set(1, ECVF_SetByConsole);
			UWorld* const W = GetCheatWorld();
			ADFPlayerCharacter* const P = GetLocalDFPawn(W);
			UDFCharacterMovementComponent* const CMC = P ? Cast<UDFCharacterMovementComponent>(P->GetCharacterMovement()) : nullptr;
			UAbilitySystemComponent* const ASC = GetLocalASC(W);
			DFDodgeDebug::DumpLocalDodgeState(CMC, ASC);
			DF_LOG(Log, "df.DodgeDebug dump: see Output Log filter [Dodge]");
			return;
		}
		if (A == TEXT("0") || A == TEXT("off"))
		{
			Cv->Set(0, ECVF_SetByConsole);
		}
		else if (A == TEXT("1") || A == TEXT("log"))
		{
			Cv->Set(1, ECVF_SetByConsole);
		}
		else if (A == TEXT("2") || A == TEXT("draw") || A == TEXT("on"))
		{
			Cv->Set(2, ECVF_SetByConsole);
		}
		else
		{
			DF_LOG(Warning, "df.DodgeDebug: use [0|1|2|dump|log|draw|on|off]");
			return;
		}
	}
	else
	{
		const int32 Next = Cv->GetInt() >= 2 ? 0 : (Cv->GetInt() + 1);
		Cv->Set(Next, ECVF_SetByConsole);
	}

	DF_LOG(Log, "df.DodgeDebug: df.DebugDodge=%d (0=off 1=log 2=log+draw) — filter Output Log: Dodge",
		Cv->GetInt());
	DF_LOG(Log, "df.DodgeDebug dump — estado CMC/ASC/stamina; showdebug AbilitySystem para tags");
}

static FAutoConsoleCommand GCmdDodgeDebug(
	TEXT("df.DodgeDebug"),
	TEXT("Dodge debug: toggle df.DebugDodge (0/1/2). Args: dump | 0 | 1 | 2 | log | draw | on | off. Log prefix [Dodge]."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_dodgedebug));

static void Cmd_df_lockondebug(TArray<FString> const& Args)
{
	IConsoleVariable* const Cv = IConsoleManager::Get().FindConsoleVariable(TEXT("df.DebugLockOn"));
	if (!Cv)
	{
		DF_LOG(Warning, "df.LockOnDebug: df.DebugLockOn CVar missing (shipping build?)");
		return;
	}

	if (Args.Num() > 0)
	{
		const FString A = Args[0].ToLower();
		if (A == TEXT("dump"))
		{
			Cv->Set(1, ECVF_SetByConsole);
			UWorld* const W = GetCheatWorld();
			if (ADFPlayerCharacter* const P = GetLocalDFPawn(W))
			{
				if (UDFLockOnComponent* const LOC = P->LockOnComponent)
				{
					const bool bLocked = LOC->IsLockedOn();
					const AActor* const T = LOC->GetCurrentTarget();
					DF_LOG(Log, "df.LockOnDebug dump: locked=%d target=%s range=%.0f",
						bLocked ? 1 : 0, *GetNameSafe(T), LOC->GetLockOnRange());
					DFLockOnDebug::DumpLocomotionAnimState(P, true);
				}
			}
			return;
		}
		if (A == TEXT("0") || A == TEXT("off"))
		{
			Cv->Set(0, ECVF_SetByConsole);
		}
		else if (A == TEXT("1") || A == TEXT("log"))
		{
			Cv->Set(1, ECVF_SetByConsole);
		}
		else if (A == TEXT("2") || A == TEXT("draw") || A == TEXT("on"))
		{
			Cv->Set(2, ECVF_SetByConsole);
		}
		else
		{
			DF_LOG(Warning, "df.LockOnDebug: use [0|1|2|dump|log|draw|on|off]");
			return;
		}
	}
	else
	{
		const int32 Next = Cv->GetInt() >= 2 ? 0 : (Cv->GetInt() + 1);
		Cv->Set(Next, ECVF_SetByConsole);
	}

	DF_LOG(Log, "df.LockOnDebug: df.DebugLockOn=%d (0=off 1=log 2=log+draw) — filter Output Log: LockOn",
		Cv->GetInt());
}

static FAutoConsoleCommand GCmdLockOnDebug(
	TEXT("df.LockOnDebug"),
	TEXT("Lock-on debug: toggle df.DebugLockOn (0/1/2). Args: dump | 0 | 1 | 2 | log | draw | on | off."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_lockondebug));

} // namespace

#endif
