// Source/DungeonForged/Private/GameModes/Nexus/ADFNexusGameMode.cpp
#include "GameModes/Nexus/ADFNexusGameMode.h"
#include "Data/DFDataTableStructs.h"
#include "DungeonForgedModule.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "GameModes/Nexus/ADFNexusGameState.h"
#include "GameModes/Nexus/ADFNexusHUD.h"
#include "GameModes/Nexus/ADFNexusNPCBase.h"
#include "GameModes/Nexus/ADFNexusCharacter.h"
#include "GameModes/Nexus/ADFNexusPlayerController.h"
#include "Run/DFSaveGame.h"
#include "Run/DFRunManager.h"
#include "Run/UDFSaveLibrary.h"
#include "Run/UDFSaveSlotManagerSubsystem.h"
#include "Merchant/ADFMerchantActor.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameModes/Nexus/DFNexusTypes.h"

ADFNexusGameMode::ADFNexusGameMode()
{
	GameStateClass = ADFNexusGameState::StaticClass();
	PlayerControllerClass = ADFNexusPlayerController::StaticClass();
	HUDClass = ADFNexusHUD::StaticClass();
	NexusPawnClass = ADFNexusCharacter::StaticClass();
	DefaultPawnClass = NexusPawnClass;
}

namespace
{
	static void DfClearModularMeshesForNexusPreview(ADFPlayerCharacter* const Hero)
	{
		if (!Hero)
		{
			return;
		}
		auto ClearPart = [](USkeletalMeshComponent* const Part)
		{
			if (!Part)
			{
				return;
			}
			Part->SetSkeletalMesh(nullptr);
			Part->SetHiddenInGame(true);
			Part->EmptyOverrideMaterials();
		};
		ClearPart(Hero->Mesh_Helmet);
		ClearPart(Hero->Mesh_Chest);
		ClearPart(Hero->Mesh_Legs);
		ClearPart(Hero->Mesh_Boots);
		ClearPart(Hero->Mesh_Gloves);
		ClearPart(Hero->Mesh_Weapon);
		ClearPart(Hero->Mesh_OffHand);
	}
}

void ADFNexusGameMode::ApplyActiveClassVisualsToNexusPawn(APlayerController* const PlayerController) const
{
	if (!PlayerController)
	{
		return;
	}
	UGameInstance* const GI = GetGameInstance();
	UDFRunManager* const RM = GI ? GI->GetSubsystem<UDFRunManager>() : nullptr;
	if (!RM)
	{
		return;
	}
	const FName ClassRowName = RM->ResolveActiveClassRowName();
	const FDFClassTableRow* const Row = RM->FindClassTableRow(ClassRowName);
	if (!Row)
	{
		return;
	}
	if (!Row->NexusCharacterClass.IsNull())
	{
		if (Row->CharacterMesh)
		{
			if (ACharacter* const PawnChar = Cast<ACharacter>(PlayerController->GetPawn()))
			{
				if (USkeletalMeshComponent* const BodyMesh = PawnChar->GetMesh())
				{
					BodyMesh->SetSkeletalMesh(Row->CharacterMesh);
				}
			}
		}
		return;
	}
	ACharacter* const PawnChar = Cast<ACharacter>(PlayerController->GetPawn());
	if (!PawnChar)
	{
		return;
	}
	USkeletalMeshComponent* const Mesh = PawnChar->GetMesh();
	if (!Mesh)
	{
		return;
	}

	USkeletalMesh* SkelMesh = Row->CharacterMesh;
	TSubclassOf<UAnimInstance> AnimClass = nullptr;

	if (!Row->CharacterClass.IsNull())
	{
		if (UClass* const PawnClass = Row->CharacterClass.LoadSynchronous())
		{
			if (ACharacter* const CDO = Cast<ACharacter>(PawnClass->GetDefaultObject()))
			{
				if (USkeletalMeshComponent* const CdoMesh = CDO->GetMesh())
				{
					AnimClass = CdoMesh->GetAnimClass();
					if (!SkelMesh)
					{
						SkelMesh = CdoMesh->GetSkeletalMeshAsset();
					}
				}
			}
		}
	}

	if (ADFPlayerCharacter* const Hero = Cast<ADFPlayerCharacter>(PawnChar))
	{
		DfClearModularMeshesForNexusPreview(Hero);
	}

	if (SkelMesh)
	{
		Mesh->SetSkeletalMesh(SkelMesh);
	}
	if (AnimClass)
	{
		Mesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		Mesh->SetAnimInstanceClass(AnimClass);
		Mesh->InitAnim(true);
		DF_LOG(Log,
			"ADFNexusGameMode::ApplyActiveClassVisuals: row '%s' mesh=%s anim=%s",
			*ClassRowName.ToString(),
			SkelMesh ? *SkelMesh->GetName() : TEXT("(none)"),
			*AnimClass->GetName());
	}
	else if (SkelMesh)
	{
		Mesh->InitAnim(true);
	}

	if (PlayerController->IsLocalController())
	{
		PlayerController->SetShowMouseCursor(false);
		PlayerController->SetInputMode(FInputModeGameOnly());
	}
}

UClass* ADFNexusGameMode::GetDefaultPawnClassForController_Implementation(AController* const InController)
{
	(void)InController;
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>())
		{
			const FName ClassRowName = RM->ResolveActiveClassRowName();
			if (const FDFClassTableRow* const Row = RM->FindClassTableRow(ClassRowName))
			{
				if (!Row->NexusCharacterClass.IsNull())
				{
					if (UClass* const Loaded = Row->NexusCharacterClass.LoadSynchronous())
					{
						DF_LOG(Log,
							"ADFNexusGameMode::GetDefaultPawnClassForController: class row '%s' -> %s",
							*ClassRowName.ToString(),
							*Loaded->GetPathName());
						return Loaded;
					}
					DF_LOG(Warning,
						"ADFNexusGameMode::GetDefaultPawnClassForController: row '%s' NexusCharacterClass failed to load.",
						*ClassRowName.ToString());
				}
			}
		}
	}
	if (NexusPawnClass)
	{
		return NexusPawnClass;
	}
	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void ADFNexusGameMode::PostLogin(APlayerController* const NewPlayer)
{
	if (GetNetMode() != NM_Client && NewPlayer)
	{
		UGameInstance* const GI = GetGameInstance();
		UDFRunManager* const RM = GI ? GI->GetSubsystem<UDFRunManager>() : nullptr;
		const ERunNexusTravelReason Arrival = RM ? RM->GetNexusArrivalReason() : ERunNexusTravelReason::FirstLaunch;
		ActivePlayerStartTag = SelectSpawnTagForArrival(Arrival);

		UDFSaveGame* S = UDFSaveLibrary::ResolveMutableMetaSave(this);
		if (S)
		{
			if (ADFNexusGameState* const GS = GetGameState<ADFNexusGameState>())
			{
				GS->ApplyFromSave(S);
			}
			if (RM && !S->LastRunClass.IsNone())
			{
				RM->SetSessionSelectedClass(S->LastRunClass);
			}
			ProcessPendingUnlocks(S, NewPlayer);
			if (S->MerchantRestockRunCounter >= 3)
			{
				if (UWorld* const W = GetWorld())
				{
					for (TActorIterator<ADFMerchantActor> MIt(W); MIt; ++MIt)
					{
						MIt->GenerateStock();
					}
				}
				S->MerchantRestockRunCounter = 0;
			}
			(void)UDFSaveLibrary::SaveMetaSave(this, S);
		}
	}

	Super::PostLogin(NewPlayer);

	if (GetNetMode() == NM_Client || !NewPlayer)
	{
		return;
	}
	ApplyActiveClassVisualsToNexusPawn(NewPlayer);
	UGameInstance* const GI = GetGameInstance();
	UDFRunManager* const RM = GI ? GI->GetSubsystem<UDFRunManager>() : nullptr;
	const ERunNexusTravelReason R = RM ? RM->GetNexusArrivalReason() : ERunNexusTravelReason::FirstLaunch;
	PlayNexusArrivalPresentation(R, NewPlayer);
	if (RM)
	{
		RM->ClearNexusArrivalContext();
	}
}

FName ADFNexusGameMode::SelectSpawnTagForArrival(const ERunNexusTravelReason Reason) const
{
	if (Reason == ERunNexusTravelReason::Victory)
	{
		return CenterPlazaStartTag;
	}
	return DefaultEntranceStartTag;
}

AActor* ADFNexusGameMode::FindPlayerStart_Implementation(
	AController* const Player, const FString& IncomingName)
{
	if (!GetWorld() || ActivePlayerStartTag.IsNone())
	{
		return Super::FindPlayerStart_Implementation(Player, IncomingName);
	}
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		if (It->PlayerStartTag == ActivePlayerStartTag)
		{
			return *It;
		}
	}
	return Super::FindPlayerStart_Implementation(Player, IncomingName);
}

void ADFNexusGameMode::ProcessPendingUnlocks(UDFSaveGame* const Save, APlayerController* const ForNotifications)
{
	ProcessPendingUnlocksFromSave(Save, ForNotifications);
}

void ADFNexusGameMode::ProcessPendingUnlocksFromSave(UDFSaveGame* const Save, APlayerController* const ForNotifications)
{
	if (!Save)
	{
		return;
	}
	const TArray<FDFPendingUnlockEntry> ToProcess = Save->PendingUnlocks;
	for (const FDFPendingUnlockEntry& E : ToProcess)
	{
		switch (E.Type)
		{
		case ENexusPendingUnlockType::UnlockClass:
			if (!E.ClassRow.IsNone() && !Save->UnlockedClasses.Contains(E.ClassRow))
			{
				Save->UnlockedClasses.Add(E.ClassRow);
			}
			break;
		case ENexusPendingUnlockType::UnlockNPC:
			if (!E.NPCId.IsNone() && !Save->UnlockedNPCs.Contains(E.NPCId))
			{
				Save->UnlockedNPCs.Add(E.NPCId);
			}
			if (UWorld* const W = GetWorld())
			{
				for (TActorIterator<ADFNexusNPCBase> NIt(W); NIt; ++NIt)
				{
					if (NIt->GetNPCId() == E.NPCId)
					{
						NIt->SetNexusUnlockedFromSave(true);
						break;
					}
				}
			}
			break;
		case ENexusPendingUnlockType::UnlockUpgrade:
			if (!E.UpgradeRow.IsNone() && !Save->CompletedUpgrades.Contains(E.UpgradeRow))
			{
				Save->CompletedUpgrades.Add(E.UpgradeRow);
			}
			break;
		default: break;
		}
	}
	Save->PendingUnlocks.Reset();
	(void)UDFSaveLibrary::SaveMetaSave(this, Save);
	if (ADFNexusGameState* const GS = GetGameState<ADFNexusGameState>())
	{
		GS->ApplyFromSave(Save);
	}
	if (ADFNexusHUD* const H = ForNotifications ? ForNotifications->GetHUD<ADFNexusHUD>() : nullptr)
	{
		for (const FDFPendingUnlockEntry& E : ToProcess)
		{
			H->QueueUnlockNotificationForEntry(E);
		}
	}
}

void ADFNexusGameMode::PlayNexusArrivalPresentation_Implementation(
	ERunNexusTravelReason const Reason, APlayerController* const LocalPC)
{
	FText Title = NSLOCTEXT("Nexus", "ArrivalDefault", "Bem-vindo ao Nexus");
	FText Body = FText::GetEmpty();
	switch (Reason)
	{
	case ERunNexusTravelReason::Victory:
		Title = NSLOCTEXT("Nexus", "ArrivalVictory", "Run vitoriosa");
		Body = NSLOCTEXT("Nexus", "ArrivalVictoryBody", "O Chronister registrou seu progresso. Meta XP aplicado.");
		break;
	case ERunNexusTravelReason::Defeat:
		Title = NSLOCTEXT("Nexus", "ArrivalDefeat", "Retorno da masmorra");
		Body = NSLOCTEXT("Nexus", "ArrivalDefeatBody", "Voce sobreviveu para treinar outra vez.");
		break;
	case ERunNexusTravelReason::Abandon:
		Title = NSLOCTEXT("Nexus", "ArrivalAbandon", "Run encerrada");
		Body = NSLOCTEXT("Nexus", "ArrivalAbandonBody", "Seu progresso meta foi salvo.");
		break;
	case ERunNexusTravelReason::FirstLaunch:
		Title = NSLOCTEXT("Nexus", "ArrivalFirst", "Primeira visita");
		Body = NSLOCTEXT("Nexus", "ArrivalFirstBody", "Fale com o Ferreiro e o Cronista para comecar.");
		break;
	default:
		break;
	}
	if (ADFNexusHUD* const H = LocalPC ? LocalPC->GetHUD<ADFNexusHUD>() : nullptr)
	{
		H->ShowArrivalBanner(Title, Body, 4.f);
	}
}
