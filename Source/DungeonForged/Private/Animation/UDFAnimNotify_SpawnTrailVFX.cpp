// Source/DungeonForged/Private/Animation/UDFAnimNotify_SpawnTrailVFX.cpp
#include "Animation/UDFAnimNotify_SpawnTrailVFX.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Combat/DFCombatDebug.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "DungeonForgedModule.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

#if !UE_BUILD_SHIPPING
static TAutoConsoleVariable<int32> CVarDF_TrailVFXDebug(
	TEXT("df.TrailVFXDebug"),
	0,
	TEXT("DungeonForged: debug UDFAnimNotify_SpawnTrailVFX.\n")
	TEXT(" 0: Off (also enable df.DebugCombat 16)\n")
	TEXT(" 1: Log + on-screen\n")
	TEXT(" 2: Log + on-screen + draw socket sphere"),
	ECVF_Cheat);

static bool IsTrailVFXDebugEnabled()
{
	return CVarDF_TrailVFXDebug.GetValueOnGameThread() > 0
		|| DFCombatDebug::IsChannelEnabled(DFCombatDebug::EChannel::Trail);
}

static bool IsTrailVFXDrawEnabled()
{
	return CVarDF_TrailVFXDebug.GetValueOnGameThread() >= 2;
}
#endif

namespace
{
static const FAttachmentTransformRules TrailAttachRules = FAttachmentTransformRules::SnapToTargetIncludingScale;

static const USkeletalMesh* GetSkelMeshAsset(const USkeletalMeshComponent* Mesh)
{
	return Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr;
}

static int32 CountMeshSockets(const USkeletalMesh* SkelMesh)
{
	return SkelMesh ? SkelMesh->GetActiveSocketList().Num() : 0;
}

static FName ResolveSocketOnMesh(USkeletalMeshComponent* Mesh, const FName DesiredSocket, FString* OutNote)
{
	if (!Mesh || DesiredSocket.IsNone())
	{
		return DesiredSocket;
	}
	if (Mesh->DoesSocketExist(DesiredSocket))
	{
		return DesiredSocket;
	}

	const USkeletalMesh* const SkelMesh = GetSkelMeshAsset(Mesh);
	if (SkelMesh)
	{
		const FString DesiredStr = DesiredSocket.ToString();
		for (const USkeletalMeshSocket* const Socket : SkelMesh->GetActiveSocketList())
		{
			if (!Socket)
			{
				continue;
			}
			const FName Candidate = Socket->SocketName;
			if (Candidate.ToString().Equals(DesiredStr, ESearchCase::IgnoreCase))
			{
				if (OutNote)
				{
					*OutNote = FString::Printf(
						TEXT("socket case fix: '%s' -> '%s'"), *DesiredSocket.ToString(), *Candidate.ToString());
				}
				return Candidate;
			}
		}
	}

	if (OutNote)
	{
		*OutNote = FString::Printf(TEXT("socket '%s' MISSING on %s"), *DesiredSocket.ToString(), *Mesh->GetName());
	}
	return DesiredSocket;
}

static FString BuildSocketList(USkeletalMeshComponent* Mesh, const int32 MaxNames = 12)
{
	if (!Mesh)
	{
		return TEXT("(no mesh)");
	}
	const USkeletalMesh* const SkelMesh = GetSkelMeshAsset(Mesh);
	if (!SkelMesh)
	{
		return TEXT("(no skeletal mesh asset)");
	}

	FString List;
	const TArray<USkeletalMeshSocket*>& Sockets = SkelMesh->GetActiveSocketList();
	const int32 Count = Sockets.Num();
	for (int32 Idx = 0; Idx < Count && Idx < MaxNames; ++Idx)
	{
		const USkeletalMeshSocket* const Socket = Sockets[Idx];
		if (!Socket)
		{
			continue;
		}
		if (!List.IsEmpty())
		{
			List += TEXT(", ");
		}
		List += Socket->SocketName.ToString();
	}
	if (Count > MaxNames)
	{
		List += FString::Printf(TEXT(" ... +%d"), Count - MaxNames);
	}
	return List.IsEmpty() ? TEXT("(no sockets)") : List;
}

static FString MeshDebugLabel(const USkeletalMeshComponent* Mesh)
{
	if (!Mesh)
	{
		return TEXT("NULL");
	}
	const USkeletalMesh* const SkelMesh = GetSkelMeshAsset(Mesh);
	return FString::Printf(
		TEXT("%s | SK=%s | sockets=%d"),
		*Mesh->GetName(),
		SkelMesh ? *SkelMesh->GetName() : TEXT("(none)"),
		CountMeshSockets(SkelMesh));
}

static USkeletalMeshComponent* ResolveTrailAttachMesh(
	USkeletalMeshComponent* MeshComp,
	const EDFTrailVFXAttachMesh AttachMesh,
	FString* OutResolveNote)
{
	if (!MeshComp)
	{
		if (OutResolveNote)
		{
			*OutResolveNote = TEXT("MeshComp=NULL");
		}
		return nullptr;
	}
	if (AttachMesh == EDFTrailVFXAttachMesh::CharacterHands)
	{
		if (OutResolveNote)
		{
			*OutResolveNote = FString::Printf(TEXT("CharacterHands -> %s"), *MeshDebugLabel(MeshComp));
		}
		return MeshComp;
	}

	AActor* const Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		if (OutResolveNote)
		{
			*OutResolveNote = TEXT("WeaponMesh: no owner, fallback notify mesh");
		}
		return MeshComp;
	}

	if (const ADFPlayerCharacter* const Player = Cast<ADFPlayerCharacter>(Owner))
	{
		if (Player->Mesh_Weapon)
		{
			if (Player->Mesh_Weapon->GetSkeletalMeshAsset())
			{
				if (OutResolveNote)
				{
					*OutResolveNote = FString::Printf(TEXT("ADFPlayerCharacter::Mesh_Weapon -> %s"),
						*MeshDebugLabel(Player->Mesh_Weapon));
				}
				return Player->Mesh_Weapon;
			}
			if (OutResolveNote)
			{
				*OutResolveNote = TEXT("Mesh_Weapon exists but has NO skeletal mesh asset (unequipped?)");
			}
		}
		else if (OutResolveNote)
		{
			*OutResolveNote = TEXT("ADFPlayerCharacter but Mesh_Weapon=NULL");
		}
	}

	TArray<USkeletalMeshComponent*> SkelMeshes;
	Owner->GetComponents<USkeletalMeshComponent>(SkelMeshes);
	for (USkeletalMeshComponent* const Skel : SkelMeshes)
	{
		if (Skel && Skel != MeshComp && Skel->GetFName() == FName(TEXT("Mesh_Weapon"))
			&& Skel->GetSkeletalMeshAsset())
		{
			if (OutResolveNote)
			{
				*OutResolveNote = FString::Printf(TEXT("component Mesh_Weapon -> %s"), *MeshDebugLabel(Skel));
			}
			return Skel;
		}
	}

	FString SkelList;
	for (USkeletalMeshComponent* const Skel : SkelMeshes)
	{
		if (!Skel)
		{
			continue;
		}
		if (!SkelList.IsEmpty())
		{
			SkelList += TEXT(" | ");
		}
		SkelList += MeshDebugLabel(Skel);
	}

	if (OutResolveNote)
	{
		OutResolveNote->Append(TEXT(" | FALLBACK notify mesh. Owner skels: "));
		OutResolveNote->Append(SkelList.IsEmpty() ? TEXT("(none)") : SkelList);
	}
	return MeshComp;
}

/** Requires a non-none tag; never touches untagged Niagara components. */
static UNiagaraComponent* FindAndPruneTaggedTrail(const TArray<UNiagaraComponent*>& Niagaras, const FName Tag)
{
	if (Tag.IsNone())
	{
		return nullptr;
	}

	UNiagaraComponent* Keep = nullptr;
	for (UNiagaraComponent* N : Niagaras)
	{
		if (!N || !N->ComponentHasTag(Tag))
		{
			continue;
		}
		if (!Keep)
		{
			Keep = N;
			continue;
		}
		N->Deactivate();
		N->DestroyComponent();
	}
	return Keep;
}

static bool AttachTrailToMesh(UNiagaraComponent* Trail, USkeletalMeshComponent* AttachTarget, const FName Socket)
{
	if (!Trail || !AttachTarget || Socket.IsNone() || !AttachTarget->DoesSocketExist(Socket))
	{
		return false;
	}
	return Trail->AttachToComponent(AttachTarget, TrailAttachRules, Socket);
}

static bool ReactivateTrailOnMesh(
	UNiagaraComponent* Trail,
	USkeletalMeshComponent* AttachTarget,
	const FName Socket)
{
	if (!AttachTrailToMesh(Trail, AttachTarget, Socket))
	{
		return false;
	}
	Trail->Activate(true);
	return true;
}

static UNiagaraComponent* SpawnTrailOnMesh(
	UNiagaraSystem* NiagaraTemplate,
	USkeletalMeshComponent* AttachTarget,
	const FName Socket,
	const FName Tag)
{
	if (!NiagaraTemplate || !AttachTarget || Socket.IsNone() || !AttachTarget->DoesSocketExist(Socket))
	{
		return nullptr;
	}

	UNiagaraComponent* const Spawned = UNiagaraFunctionLibrary::SpawnSystemAttached(
		NiagaraTemplate,
		AttachTarget,
		Socket,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget,
		/*bAutoDestroy*/ false,
		/*bAutoActivate*/ true,
		ENCPoolMethod::None);
	if (Spawned && !Tag.IsNone())
	{
		Spawned->ComponentTags.AddUnique(Tag);
	}
	return Spawned;
}

enum class ETrailVFXOutcome : uint8
{
	SkippedNoTemplate,
	SkippedAlreadyActive,
	SkippedSpawnIfMissingOff,
	SpawnFailed,
	Spawned,
	Reactivated,
	ReactivatedAfterStaleDestroy,
	NotAttempted,
};

struct FTrailVFXApplyResult
{
	ETrailVFXOutcome Outcome = ETrailVFXOutcome::NotAttempted;
	UNiagaraComponent* TrailComp = nullptr;
};

static FTrailVFXApplyResult ApplyTrailVFX(
	AActor* Owner,
	UNiagaraSystem* NiagaraTemplate,
	USkeletalMeshComponent* AttachTarget,
	const FName ResolvedSocket,
	const bool bSocketOk,
	const FName Tag,
	const bool bAllowSpawn)
{
	FTrailVFXApplyResult Result;
	if (!Owner || !AttachTarget)
	{
		return Result;
	}

	TArray<UNiagaraComponent*> Niagaras;
	Owner->GetComponents<UNiagaraComponent>(Niagaras, true);

	UNiagaraComponent* ExistingTrail = FindAndPruneTaggedTrail(Niagaras, Tag);
	if (ExistingTrail)
	{
		if (ExistingTrail->IsActive())
		{
			Result.Outcome = ETrailVFXOutcome::SkippedAlreadyActive;
			Result.TrailComp = ExistingTrail;
			return Result;
		}
		if (ReactivateTrailOnMesh(ExistingTrail, AttachTarget, ResolvedSocket))
		{
			Result.Outcome = ETrailVFXOutcome::Reactivated;
			Result.TrailComp = ExistingTrail;
			return Result;
		}

		ExistingTrail->Deactivate();
		ExistingTrail->DestroyComponent();
		ExistingTrail = nullptr;
	}

	if (!bAllowSpawn || !NiagaraTemplate)
	{
		Result.Outcome = !NiagaraTemplate ? ETrailVFXOutcome::SkippedNoTemplate : ETrailVFXOutcome::SkippedSpawnIfMissingOff;
		return Result;
	}

	if (!bSocketOk)
	{
		Result.Outcome = ETrailVFXOutcome::SpawnFailed;
		return Result;
	}

	if (Tag.IsNone())
	{
		Result.TrailComp = SpawnTrailOnMesh(NiagaraTemplate, AttachTarget, ResolvedSocket, Tag);
		Result.Outcome = Result.TrailComp ? ETrailVFXOutcome::Spawned : ETrailVFXOutcome::SpawnFailed;
		return Result;
	}

	Result.TrailComp = SpawnTrailOnMesh(NiagaraTemplate, AttachTarget, ResolvedSocket, Tag);
	if (Result.TrailComp)
	{
		Result.Outcome = ExistingTrail ? ETrailVFXOutcome::ReactivatedAfterStaleDestroy : ETrailVFXOutcome::Spawned;
	}
	else
	{
		Result.Outcome = ETrailVFXOutcome::SpawnFailed;
	}
	return Result;
}

static float GetMontageDebugTime(USkeletalMeshComponent* MeshComp, FString* OutMontageName)
{
	if (!MeshComp)
	{
		return -1.f;
	}
	UAnimInstance* const Anim = MeshComp->GetAnimInstance();
	if (!Anim)
	{
		if (OutMontageName)
		{
			*OutMontageName = TEXT("(no AnimInstance)");
		}
		return -1.f;
	}
	if (UAnimMontage* const Montage = Anim->GetCurrentActiveMontage())
	{
		if (OutMontageName)
		{
			*OutMontageName = Montage->GetName();
		}
		return Anim->Montage_GetPosition(Montage);
	}
	if (OutMontageName)
	{
		*OutMontageName = TEXT("(no active montage)");
	}
	return -1.f;
}

#if !UE_BUILD_SHIPPING
static const TCHAR* TrailOutcomeStr(const ETrailVFXOutcome Outcome)
{
	switch (Outcome)
	{
	case ETrailVFXOutcome::SkippedNoTemplate: return TEXT("skip(no template)");
	case ETrailVFXOutcome::SkippedAlreadyActive: return TEXT("skip(already active)");
	case ETrailVFXOutcome::SkippedSpawnIfMissingOff: return TEXT("skip(spawn off)");
	case ETrailVFXOutcome::SpawnFailed: return TEXT("SPAWN FAILED");
	case ETrailVFXOutcome::Spawned: return TEXT("spawned");
	case ETrailVFXOutcome::Reactivated: return TEXT("reactivated");
	case ETrailVFXOutcome::ReactivatedAfterStaleDestroy: return TEXT("respawn(stale)");
	default: return TEXT("no-op");
	}
}

static void TrailVFXDebugLog(
	AActor* Owner,
	const FString& Line,
	const FColor ScreenColor = FColor::Cyan)
{
	DF_LOG(Log, "%s", *Line);
	if (GEngine && IsTrailVFXDebugEnabled())
	{
		DF_SCREEN(ScreenColor, 2.f, "%s", *Line);
	}
	(void)Owner;
}
#endif
} // namespace

void UUDFAnimNotify_SpawnTrailVFX::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	(void)EventReference;
	if (!MeshComp)
	{
		return;
	}

	const UWorld* const World = MeshComp->GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	FString ResolveNote;
	USkeletalMeshComponent* const AttachTarget = ResolveTrailAttachMesh(MeshComp, AttachMesh, &ResolveNote);
	if (!AttachTarget)
	{
		return;
	}

	FString SocketNote;
	const FName ResolvedSocket = ResolveSocketOnMesh(AttachTarget, AttachSocket, &SocketNote);
	const bool bSocketOk = AttachTarget->DoesSocketExist(ResolvedSocket);

	AActor* const RootOwner = MeshComp->GetOwner();
	const FTrailVFXApplyResult ApplyResult = ApplyTrailVFX(
		RootOwner, Template, AttachTarget, ResolvedSocket, bSocketOk, ComponentTag, bSpawnIfMissing);

	const int32 ActivatedCount = ApplyResult.TrailComp && ApplyResult.TrailComp->IsActive() ? 1 : 0;

#if !UE_BUILD_SHIPPING
	if (IsTrailVFXDebugEnabled())
	{
		FString MontageName;
		const float MontageTime = GetMontageDebugTime(MeshComp, &MontageName);
		const ETrailVFXOutcome Outcome = ApplyResult.Outcome;

		const FString Line = FString::Printf(
			TEXT("[TrailVFX] %s | anim=%s | montage=%s t=%.3fs | attach=%s | notifyMesh=%s | %s | socket=%s ok=%d %s | active=%d | %s | tag=%s"),
			RootOwner ? *RootOwner->GetName() : TEXT("?"),
			Animation ? *Animation->GetName() : TEXT("?"),
			*MontageName,
			MontageTime,
			AttachMesh == EDFTrailVFXAttachMesh::WeaponMesh ? TEXT("WeaponMesh") : TEXT("CharacterHands"),
			*MeshDebugLabel(MeshComp),
			*ResolveNote,
			*ResolvedSocket.ToString(),
			bSocketOk ? 1 : 0,
			*SocketNote,
			ActivatedCount,
			TrailOutcomeStr(Outcome),
			*ComponentTag.ToString());

		const FColor ScreenColor = (Outcome == ETrailVFXOutcome::SpawnFailed || !bSocketOk)
			? FColor::Red
			: (Outcome == ETrailVFXOutcome::SkippedAlreadyActive ? FColor::Yellow : FColor::Cyan);
		TrailVFXDebugLog(RootOwner, Line, ScreenColor);

		if (!bSocketOk)
		{
			TrailVFXDebugLog(RootOwner,
				FString::Printf(TEXT("[TrailVFX] sockets on %s: %s"), *AttachTarget->GetName(),
					*BuildSocketList(AttachTarget)),
				FColor::Orange);
		}

		if (IsTrailVFXDrawEnabled() && bSocketOk)
		{
			const FVector Loc = AttachTarget->GetSocketLocation(ResolvedSocket);
			DrawDebugSphere(World, Loc, 8.f, 10, FColor::Green, false, 2.f, 0, 1.5f);
		}
	}
#else
	(void)ActivatedCount;
	(void)ApplyResult;
	(void)ResolveNote;
	(void)SocketNote;
	(void)bSocketOk;
#endif
}
