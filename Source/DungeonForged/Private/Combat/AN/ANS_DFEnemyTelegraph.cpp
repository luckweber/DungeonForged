// Source/DungeonForged/Private/Combat/AN/ANS_DFEnemyTelegraph.cpp
#include "Combat/AN/ANS_DFEnemyTelegraph.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "Combat/UDFMeleeAimComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GAS/DFGameplayTags.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

namespace
{
UAbilitySystemComponent* GetOwnerASC(USkeletalMeshComponent* const MeshComp)
{
	AActor* const Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
}

UDFMeleeAimComponent* GetAimComp_Tele(USkeletalMeshComponent* const MeshComp)
{
	AActor* const Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	return IsValid(Owner) ? Owner->FindComponentByClass<UDFMeleeAimComponent>() : nullptr;
}
} // namespace

UANS_DFEnemyTelegraph::UANS_DFEnemyTelegraph()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(220, 30, 30);
#endif
}

FString UANS_DFEnemyTelegraph::GetNotifyName_Implementation() const
{
	return TEXT("DF Enemy Telegraph");
}

FVector UANS_DFEnemyTelegraph::ResolveWarningLocation(USkeletalMeshComponent* const MeshComp, AActor* const Target) const
{
	if (bUseAttackerForwardInsteadOfTarget && MeshComp)
	{
		if (AActor* const Owner = MeshComp->GetOwner())
		{
			return Owner->GetActorLocation() + Owner->GetActorForwardVector() * ForwardOffsetCm + LocationOffset;
		}
	}
	if (IsValid(Target))
	{
		return Target->GetActorLocation() + LocationOffset;
	}
	if (MeshComp && MeshComp->GetOwner())
	{
		AActor* const Owner = MeshComp->GetOwner();
		return Owner->GetActorLocation() + Owner->GetActorForwardVector() * ForwardOffsetCm + LocationOffset;
	}
	return FVector::ZeroVector;
}

void UANS_DFEnemyTelegraph::NotifyBegin(USkeletalMeshComponent* const MeshComp,
	UAnimSequenceBase* const Animation, const float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (!MeshComp)
	{
		return;
	}
	AActor* const Owner = MeshComp->GetOwner();
	UWorld* const World = MeshComp->GetWorld();
	if (!IsValid(Owner) || !World)
	{
		return;
	}

	UDFMeleeAimComponent* const Aim = GetAimComp_Tele(MeshComp);
	AActor* const Target = Aim ? Aim->ResolveCurrentTarget() : nullptr;

	if (GroundWarningVFX)
	{
		const FVector WarnLoc = ResolveWarningLocation(MeshComp, Target);
		UNiagaraComponent* const Spawned = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World, GroundWarningVFX, WarnLoc, FRotator::ZeroRotator, FVector::OneVector,
			/*bAutoDestroy=*/ false, /*bAutoActivate=*/ true);
		if (Spawned)
		{
			GroundFXByMesh.Add(MeshComp, Spawned);
		}
	}

	if (WeaponChargeVFX)
	{
		UNiagaraComponent* const Spawned = UNiagaraFunctionLibrary::SpawnSystemAttached(
			WeaponChargeVFX,
			MeshComp,
			WeaponChargeSocketName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			/*bAutoDestroy=*/ false,
			/*bAutoActivate=*/ true,
			ENCPoolMethod::None,
			true);
		if (Spawned)
		{
			Spawned->SetWorldScale3D(FVector::OneVector);
			WeaponFXByMesh.Add(MeshComp, Spawned);
		}
	}

	if (WindupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(World, WindupSound, Owner->GetActorLocation());
	}

	if (UAbilitySystemComponent* const ASC = GetOwnerASC(MeshComp))
	{
		if (bAddStateTag && FDFGameplayTags::State_Combat_Telegraph_Active.IsValid())
		{
			ASC->AddLooseGameplayTag(FDFGameplayTags::State_Combat_Telegraph_Active);
		}
		if (bSendGameplayEvents && FDFGameplayTags::Event_Combat_Telegraph_Begin.IsValid())
		{
			FGameplayEventData Payload;
			Payload.EventTag = FDFGameplayTags::Event_Combat_Telegraph_Begin;
			Payload.Instigator = Owner;
			Payload.Target = Target;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
				Owner, FDFGameplayTags::Event_Combat_Telegraph_Begin, Payload);
		}
	}
}

void UANS_DFEnemyTelegraph::NotifyTick(USkeletalMeshComponent* const MeshComp,
	UAnimSequenceBase* const Animation, const float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	if (!bUpdateTargetEveryTick || !MeshComp)
	{
		return;
	}
	const TWeakObjectPtr<UNiagaraComponent>* const Found = GroundFXByMesh.Find(MeshComp);
	if (!Found || !Found->IsValid())
	{
		return;
	}
	UDFMeleeAimComponent* const Aim = GetAimComp_Tele(MeshComp);
	AActor* const Target = Aim ? Aim->ResolveCurrentTarget() : nullptr;
	const FVector NewLoc = ResolveWarningLocation(MeshComp, Target);
	Found->Get()->SetWorldLocation(NewLoc);

#if ENABLE_DRAW_DEBUG
	if (bDrawDebug && MeshComp->GetWorld())
	{
		DrawDebugCircle(MeshComp->GetWorld(), NewLoc, 80.f, 24,
			FColor::Red, false, 0.05f, 0, 2.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
	}
#endif
}

void UANS_DFEnemyTelegraph::NotifyEnd(USkeletalMeshComponent* const MeshComp,
	UAnimSequenceBase* const Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (!MeshComp)
	{
		return;
	}
	if (TWeakObjectPtr<UNiagaraComponent>* const Found = GroundFXByMesh.Find(MeshComp))
	{
		if (UNiagaraComponent* const N = Found->Get())
		{
			N->Deactivate();
			N->DestroyComponent();
		}
		GroundFXByMesh.Remove(MeshComp);
	}
	if (TWeakObjectPtr<UNiagaraComponent>* const Found = WeaponFXByMesh.Find(MeshComp))
	{
		if (UNiagaraComponent* const N = Found->Get())
		{
			N->Deactivate();
			N->DestroyComponent();
		}
		WeaponFXByMesh.Remove(MeshComp);
	}
	if (UAbilitySystemComponent* const ASC = GetOwnerASC(MeshComp))
	{
		if (bAddStateTag && FDFGameplayTags::State_Combat_Telegraph_Active.IsValid())
		{
			ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Combat_Telegraph_Active);
		}
		if (bSendGameplayEvents && FDFGameplayTags::Event_Combat_Telegraph_End.IsValid())
		{
			AActor* const Owner = MeshComp->GetOwner();
			FGameplayEventData Payload;
			Payload.EventTag = FDFGameplayTags::Event_Combat_Telegraph_End;
			Payload.Instigator = Owner;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
				Owner, FDFGameplayTags::Event_Combat_Telegraph_End, Payload);
		}
	}
}
