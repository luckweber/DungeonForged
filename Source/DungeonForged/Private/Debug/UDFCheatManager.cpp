// Source/DungeonForged/Private/Debug/UDFCheatManager.cpp

#include "Debug/UDFCheatManager.h"

#if !UE_BUILD_SHIPPING
#include "AbilitySystemBlueprintLibrary.h"
#include "ADFDungeonManager.h"
#include "Boss/ADFBossBase.h"
#include "Characters/ADFEnemyBase.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Debug/UDFDebugComponent.h"
#include "Characters/ADFPlayerController.h"
#include "Characters/ADFPlayerState.h"
#include "Data/DFDataTableStructs.h"
#include "DFInventoryComponent.h"
#include "DungeonForgedModule.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GAS/DFGameplayTags.h"
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
		return;
	}
	UWorld* const W = GetCheatWorld();
	ADFPlayerCharacter* const P = GetLocalDFPawn(W);
	if (!P)
	{
		return;
	}
	if (UDFInventoryComponent* const Inv = P->FindComponentByClass<UDFInventoryComponent>())
	{
		if (!HasServerAuth(W, GetLocalPC(W)))
		{
			return;
		}
		Inv->AddItem(FName(*Args[0]), 1);
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
	TEXT("df.giveitem RowName"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_giveitem));
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

} // namespace

#endif
