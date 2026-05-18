// Source/DungeonForged/Private/Animation/DFDeathAnimation.cpp
#include "Animation/DFDeathAnimation.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogDFEnemyDeath, Log, All);

namespace
{
static TAutoConsoleVariable<int32> CVarDF_EnemyDeathDebug(
	TEXT("df.EnemyDeathDebug"),
	0,
	TEXT("Enemy death montage debug: 0=off, 1=summary, 2=verbose (mesh, skeleton, montage play result)."),
	ECVF_Cheat);
} // namespace

namespace DFDeathAnimation
{
int32 GetEnemyDeathDebugLevel()
{
	return CVarDF_EnemyDeathDebug.GetValueOnAnyThread();
}

void LogEnemyDeath(const int32 MinLevel, const UObject* const Context, const FString& Message)
{
	if (GetEnemyDeathDebugLevel() < MinLevel)
	{
		return;
	}
	const FString Prefix = Context ? FString::Printf(TEXT("[%s] "), *GetNameSafe(Context)) : FString();
	UE_LOG(LogDFEnemyDeath, Log, TEXT("%s%s"), *Prefix, *Message);
}

float PlayDeathMontage(
	USkeletalMeshComponent* const Mesh,
	UAnimMontage* const Montage,
	const bool bStopOtherMontages,
	const UObject* const LogContext,
	const bool bBypassAnimBlueprint)
{
	const int32 Dbg = GetEnemyDeathDebugLevel();
	if (!Mesh)
	{
		if (Dbg >= 1)
		{
			LogEnemyDeath(1, LogContext, TEXT("PlayDeathMontage: Mesh is null"));
		}
		return 0.f;
	}
	if (!Montage)
	{
		if (Dbg >= 1)
		{
			LogEnemyDeath(1, LogContext, TEXT("PlayDeathMontage: DeathMontage is null (assign on BP_DeepSeaLizard / ADFEnemyBase)"));
		}
		return 0.f;
	}
	if (bBypassAnimBlueprint)
	{
		if (UAnimInstance* const Anim = Mesh->GetAnimInstance())
		{
			if (bStopOtherMontages)
			{
				Anim->Montage_Stop(0.f);
			}
		}
		Mesh->bPauseAnims = false;
		Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		Mesh->PlayAnimation(Montage, false);
		const float Len = Montage->GetPlayLength();
		if (Dbg >= 1)
		{
			LogEnemyDeath(
				1, LogContext,
				FString::Printf(
					TEXT("PlayDeathMontage: AnimationSingleNode (bypass ABP) | Montage=%s | Len=%.2fs"),
					*GetNameSafe(Montage),
					Len));
		}
		return Len;
	}
	UAnimInstance* const Anim = Mesh->GetAnimInstance();
	if (!Anim)
	{
		if (Dbg >= 1)
		{
			LogEnemyDeath(
				1, LogContext,
				FString::Printf(
					TEXT("PlayDeathMontage: no AnimInstance on mesh '%s' (bPauseAnims=%d)"),
					*GetNameSafe(Mesh),
					Mesh->bPauseAnims ? 1 : 0));
		}
		return Montage->GetPlayLength();
	}
	if (Dbg >= 2)
	{
		const USkeletalMesh* const SkMesh = Mesh->GetSkeletalMeshAsset();
		const USkeleton* const MeshSkel = SkMesh ? SkMesh->GetSkeleton() : nullptr;
		const USkeleton* const MontageSkel = Montage->GetSkeleton();
		LogEnemyDeath(
			2, LogContext,
			FString::Printf(
				TEXT("AnimClass=%s | MeshSkel=%s | MontageSkel=%s | Montage=%s | Len=%.2fs | Slot=%s"),
				*GetNameSafe(Anim->GetClass()),
				MeshSkel ? *MeshSkel->GetName() : TEXT("null"),
				MontageSkel ? *MontageSkel->GetName() : TEXT("null"),
				*GetNameSafe(Montage),
				Montage->GetPlayLength(),
				*Montage->GetGroupName().ToString()));
		if (MeshSkel && MontageSkel && MeshSkel != MontageSkel)
		{
			LogEnemyDeath(1, LogContext, TEXT("WARNING: montage skeleton != mesh skeleton (Montage_Play may return 0)"));
		}
	}
	if (bStopOtherMontages)
	{
		Anim->Montage_Stop(0.1f);
	}
	const float Duration = Anim->Montage_Play(Montage, 1.f, EMontagePlayReturnType::MontageLength, 0.f);
	FAnimMontageInstance* Inst = Anim->GetActiveInstanceForMontage(Montage);
	if (!Inst)
	{
		Inst = Anim->GetInstanceForMontage(Montage);
	}
	if (Inst)
	{
		Inst->bEnableAutoBlendOut = false;
	}
	const float Resolved = Duration > KINDA_SMALL_NUMBER ? Duration : Montage->GetPlayLength();
	if (Dbg >= 1)
	{
		LogEnemyDeath(
			1, LogContext,
			FString::Printf(
				TEXT("Montage_Play returned %.3f | resolved=%.2fs | isPlaying=%d | instance=%s"),
				Duration,
				Resolved,
				Anim->Montage_IsPlaying(Montage) ? 1 : 0,
				Inst ? TEXT("ok") : TEXT("MISSING")));
		if (Duration <= KINDA_SMALL_NUMBER)
		{
			LogEnemyDeath(
				1, LogContext,
				TEXT("Montage_Play FAILED (0) — check skeleton, slot in ABP, or montage asset"));
		}
	}
	return Resolved;
}

void LockDeathPoseOnMesh(USkeletalMeshComponent* const Mesh, UAnimMontage* const Montage)
{
	if (!Mesh)
	{
		return;
	}

	if (UAnimInstance* const Anim = Mesh->GetAnimInstance())
	{
		if (Montage && Anim->Montage_IsPlaying(Montage))
		{
			const float EndPos = FMath::Max(0.f, Montage->GetPlayLength() - 0.05f);
			Anim->Montage_SetPosition(Montage, EndPos);
			Anim->Montage_Pause(Montage);
		}
	}

	// Freeze the whole mesh tick so the AnimGraph can't blend back to idle and root motion stops.
	// More robust than bPauseAnims alone — also halts locomotion/state machine evaluation and any root motion extraction.
	Mesh->bPauseAnims = true;
	Mesh->SetComponentTickEnabled(false);
}

float GetDeathDestroyDelaySeconds(UAnimMontage* const Montage, const float MinSeconds, const float PaddingSeconds)
{
	const float Len = Montage ? Montage->GetPlayLength() : 0.f;
	return FMath::Max(MinSeconds, Len + PaddingSeconds);
}
} // namespace DFDeathAnimation
