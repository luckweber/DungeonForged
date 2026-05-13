// Source/DungeonForged/Private/Combat/UDFMeleeTraceComponent.cpp
#include "Combat/UDFMeleeTraceComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Combat/UDFHitReactionComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "GameFramework/Character.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "CollisionQueryParams.h"
#include "WorldCollision.h"
#include "Engine/EngineTypes.h"
#include "DungeonForgedModule.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

static constexpr int32 DF_EnableDrawDebugValue =
#if ENABLE_DRAW_DEBUG
	1;
#else
	0;
#endif

static TAutoConsoleVariable<int32> CVarDF_DebugMeleeWeapon(
	TEXT("df.DebugMeleeWeapon"),
	0,
	TEXT("DungeonForged: sempre desenhar o volume de melee (sweep esferico entre sockets).\n")
	TEXT(" 0: Off (default)\n")
	TEXT(" 1: Linha weapon_start -> weapon_end + esferas (raio TraceRadius)\n")
	TEXT(" 2: Idem + capsula aproximada do segmento barrido"),
	ECVF_Cheat);

/** Int value for df.DebugMeleeWeapon; -1 = CVar not registered. */
static int32 DF_DebugMeleeWeaponCVarValue(bool* OutCVarFound = nullptr)
{
	if (OutCVarFound)
	{
		*OutCVarFound = false;
	}
	const IConsoleVariable* const Cv = IConsoleManager::Get().FindConsoleVariable(TEXT("df.DebugMeleeWeapon"));
	if (!Cv)
	{
		return -1;
	}
	if (OutCVarFound)
	{
		*OutCVarFound = true;
	}
	int32 V = Cv->GetInt();
	if (V != 0)
	{
		return V;
	}
	const FString S = Cv->GetString().TrimStartAndEnd();
	if (S.IsEmpty())
	{
		return 0;
	}
	if (S.Equals(TEXT("true"), ESearchCase::IgnoreCase) || S.Equals(TEXT("on"), ESearchCase::IgnoreCase))
	{
		return 1;
	}
	const int32 Parsed = FCString::Atoi(*S);
	return Parsed != 0 ? Parsed : 0;
}

static void DF_DumpMeleeWeaponTracesConsole()
{
	UE_LOG(LogDungeonForged, Log, TEXT("df.MeleeWeaponDump: -------"));
	bool      CvFound = false;
	const int32 DbgVal = DF_DebugMeleeWeaponCVarValue(&CvFound);
	if (!CvFound)
	{
		UE_LOG(LogDungeonForged, Warning,
			TEXT("df.MeleeWeaponDump: CVar df.DebugMeleeWeapon not found (shipping build?)"));
	}
	else
	{
		const IConsoleVariable* const Cv = IConsoleManager::Get().FindConsoleVariable(TEXT("df.DebugMeleeWeapon"));
		UE_LOG(LogDungeonForged, Log,
			TEXT("df.MeleeWeaponDump: df.DebugMeleeWeapon GetInt=%d GetString=\"%s\" Parsed=%d ENABLE_DRAW_DEBUG=%d"),
			Cv->GetInt(),
			*Cv->GetString(),
			DbgVal,
			DF_EnableDrawDebugValue
		);
	}

	int32 Worlds = 0;
	int32 Comps  = 0;
	if (!GEngine)
	{
		UE_LOG(LogDungeonForged, Warning, TEXT("df.MeleeWeaponDump: no GEngine"));
		return;
	}
	for (const FWorldContext& Cxt : GEngine->GetWorldContexts())
	{
		UWorld* const W = Cxt.World();
		if (!W || !W->IsGameWorld())
		{
			continue;
		}
		++Worlds;

		for (TActorIterator<AActor> ActorIt(W); ActorIt; ++ActorIt)
		{
			AActor* const A = *ActorIt;
			if (!A)
			{
				continue;
			}
			TArray<UDFMeleeTraceComponent*> Mes;
			A->GetComponents<UDFMeleeTraceComponent>(Mes);
			for (UDFMeleeTraceComponent* const M : Mes)
			{
				if (!M)
				{
					continue;
				}
				++Comps;
				USkeletalMeshComponent* const Skel = M->GetResolvedTraceMesh();
				USkeletalMesh* const Asset         = Skel ? Skel->GetSkeletalMeshAsset() : nullptr;
				const FName SockA                  = M->TraceStartSocket;
				const FName SockB                  = M->TraceEndSocket;
				const bool bHasStart               = Skel && Skel->DoesSocketExist(SockA);
				const bool bHasEnd                 = Skel && Skel->DoesSocketExist(SockB);
				UE_LOG(LogDungeonForged, Log,
					TEXT("  [%d] Actor=%s MeleeTick=%s | meshComp=%s | skAsset=%s | sockets [%s|%s ok=%d/%d | radius=%.1f"),
					Comps,
					*GetNameSafe(A),
					M->IsComponentTickEnabled() ? TEXT("on") : TEXT("off"),
					Skel ? *Skel->GetName() : TEXT("(null)"),
					Asset ? *Asset->GetName() : TEXT("(none)"),
					*SockA.ToString(),
					*SockB.ToString(),
					bHasStart ? 1 : 0,
					bHasEnd ? 1 : 0,
					M->TraceRadius);
			}
		}
	}
	UE_LOG(LogDungeonForged, Log,
		TEXT("df.MeleeWeaponDump: worlds=%d totalMeleeComponents=%d (no components => no Tick, no visuals)"),
		Worlds,
		Comps);
}

static FAutoConsoleCommand GCmdMeleeWeaponDump(
	TEXT("df.MeleeWeaponDump"),
	TEXT("Log df.DebugMeleeWeapon / ENABLE_DRAW_DEBUG and every UDFMeleeTraceComponent + sockets."),
	FConsoleCommandDelegate::CreateLambda([]() { DF_DumpMeleeWeaponTracesConsole(); }));

static void DF_DrawMeleeWeaponDebugVisual(
	const UWorld* const World,
	USkeletalMeshComponent* const Mesh,
	const FName TraceStartSocket,
	const FName TraceEndSocket,
	const float TraceRadius,
	const int32 DebugMode)
{
	if (!World || !Mesh || DebugMode <= 0)
	{
		return;
	}

	const bool bStartOk = Mesh->DoesSocketExist(TraceStartSocket);
	const bool bEndOk   = Mesh->DoesSocketExist(TraceEndSocket);

	if (!bStartOk || !bEndOk)
	{
		USkeletalMesh* const SkAsset     = Mesh->GetSkeletalMeshAsset();
		const FString      AssetName     = SkAsset ? SkAsset->GetName() : FString(TEXT("(no skeletal asset)"));
		const AActor* const OwnerActor   = Mesh->GetOwner();

		// Always emit to log — do NOT nest under ENABLE_DRAW_DEBUG (some targets compile draw out).
		UE_LOG(LogDungeonForged, Warning,
			TEXT("df.DebugMeleeWeapon: sockets %s/%s missing on skeletal mesh \"%s\" (component %s, owner %s). ")
			TEXT("Melee trace uses Mesh_Weapon when armed; equip weapon or rename sockets."),
			*TraceStartSocket.ToString(),
			*TraceEndSocket.ToString(),
			*AssetName,
			*Mesh->GetName(),
			OwnerActor ? *OwnerActor->GetName() : TEXT("(none)"));

#if ENABLE_DRAW_DEBUG
		DrawDebugString(
			World,
			Mesh->GetComponentLocation(),
			FString::Printf(
				TEXT("df.DebugMeleeWeapon: missing %s / %s on %s — see Output Log"),
				*TraceStartSocket.ToString(),
				*TraceEndSocket.ToString(),
				*AssetName),
			nullptr,
			FColor::Red,
			0.2f,
			true,
			1.25f);
#endif
		return;
	}

#if ENABLE_DRAW_DEBUG
	const FVector TraceStart = Mesh->GetSocketLocation(TraceStartSocket);
	const FVector TraceEnd   = Mesh->GetSocketLocation(TraceEndSocket);
	const FColor  SpineColor = FColor::Green;
	DrawDebugSphere(World, TraceStart, TraceRadius, 10, SpineColor, false, -1.f, 0, 1.f);
	DrawDebugSphere(World, TraceEnd, TraceRadius, 10, SpineColor, false, -1.f, 0, 1.f);
	DrawDebugLine(World, TraceStart, TraceEnd, FColor::Yellow, false, -1.f, 0, 1.5f);

	if (DebugMode >= 2)
	{
		const FVector Delta      = TraceEnd - TraceStart;
		const float   SegmentLen = Delta.Size();
		if (SegmentLen >= KINDA_SMALL_NUMBER)
		{
			const FVector Unit          = Delta / SegmentLen;
			const FVector Mid           = (TraceStart + TraceEnd) * 0.5f;
			const FQuat   CapsuleOrient = FQuat::FindBetweenNormals(FVector::UpVector, Unit.GetSafeNormal());
			const float HalfCapsule     = SegmentLen * 0.5f + TraceRadius;
			DrawDebugCapsule(
				World,
				Mid,
				HalfCapsule,
				TraceRadius,
				CapsuleOrient,
				FColor::Cyan,
				false,
				-1.f,
				0,
				1.f);
		}
	}
#endif // ENABLE_DRAW_DEBUG
}
#endif // !UE_BUILD_SHIPPING

UDFMeleeTraceComponent::UDFMeleeTraceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
#if !UE_BUILD_SHIPPING
	PrimaryComponentTick.bStartWithTickEnabled = true;
#else
	PrimaryComponentTick.bStartWithTickEnabled = false;
#endif
	SetIsReplicatedByDefault(false);
}

void UDFMeleeTraceComponent::BeginPlay()
{
	Super::BeginPlay();
	if (DamageTag.IsValid() == false)
	{
		DamageTag = FDFGameplayTags::ResolveDataDamageTag();
	}
	if (KnockbackTag.IsValid() == false)
	{
		KnockbackTag = FDFGameplayTags::ResolveDataKnockbackTag();
	}
}

USkeletalMeshComponent* UDFMeleeTraceComponent::GetMesh() const
{
	if (SkeletalMesh)
	{
		return SkeletalMesh;
	}
	if (ACharacter* C = Cast<ACharacter>(GetOwner()))
	{
		return C->GetMesh();
	}
	return nullptr;
}

USkeletalMeshComponent* UDFMeleeTraceComponent::GetResolvedTraceMesh() const
{
	return GetMesh();
}

void UDFMeleeTraceComponent::StartTrace()
{
	HitActorsThisSwing.Empty();
	bTracing = true;
	const float Dmg = bUseOverrideBaseDamage ? OverrideBaseDamage : BaseDamage;
	const float Kb = bUseOverrideKnockback ? OverrideBaseKnockback : BaseKnockback;
	bUseOverrideBaseDamage = false;
	bUseOverrideKnockback = false;
	CachedDamageSpec = BuildDamageSpec(Dmg, Kb);
#if UE_BUILD_SHIPPING
	SetComponentTickEnabled(true);
#endif
}

void UDFMeleeTraceComponent::EndTrace()
{
	bTracing = false;
#if UE_BUILD_SHIPPING
	SetComponentTickEnabled(false);
#endif
}

void UDFMeleeTraceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
#if !UE_BUILD_SHIPPING
	bool      CvOk = false;
	const int32 WeaponDbg = DF_DebugMeleeWeaponCVarValue(&CvOk);
	if (!CvOk)
	{
		static uint64 NoCvarSpam = 0;
		if ((++NoCvarSpam % 3600ull) == 1ull)
		{
			UE_LOG(LogDungeonForged, Warning,
				TEXT("df.DebugMeleeWeapon: FindConsoleVariable failed (shipping / module not linked?). Use Development Editor."));
		}
	}
	else if (WeaponDbg > 0 && GetWorld())
	{
		UWorld* const World = GetWorld();
		static uint32 StatusSpam = 0;
		const bool bStatusLine = (++StatusSpam % 180u) == 1u;

		if (bStatusLine)
		{
			UE_LOG(LogDungeonForged, Log,
				TEXT("df.DebugMeleeWeapon: mode=%d owner=%s MeleeCmpTick=%s ENABLE_DRAW_DEBUG=%d"),
				WeaponDbg,
				GetOwner() ? *GetOwner()->GetName() : TEXT("(none)"),
				IsComponentTickEnabled() ? TEXT("on") : TEXT("off"),
				DF_EnableDrawDebugValue
			);
		}

#if ENABLE_DRAW_DEBUG
		if (USkeletalMeshComponent* TraceMesh = GetMesh())
		{
			DF_DrawMeleeWeaponDebugVisual(World, TraceMesh, TraceStartSocket, TraceEndSocket, TraceRadius, WeaponDbg);
		}
		else if (bStatusLine)
		{
			UE_LOG(LogDungeonForged, Warning,
				TEXT("df.DebugMeleeWeapon: GetMesh()==null (no weapon skeletal / owner mesh). Owner=%s"),
				GetOwner() ? *GetOwner()->GetName() : TEXT("(none)"));
		}
#else
		if (bStatusLine)
		{
			UE_LOG(LogDungeonForged, Warning,
				TEXT("df.DebugMeleeWeapon: ENABLE_DRAW_DEBUG=0 in this compilation — no spheres/lines. Run df.MeleeWeaponDump for socket state."));
		}
#endif
	}
#endif
	if (bTracing)
	{
		TickTrace(DeltaTime);
	}
}

FGameplayEffectSpecHandle UDFMeleeTraceComponent::BuildDamageSpec(const float BaseDamageValue, const float KnockbackForce)
{
	AActor* const Owner = GetOwner();
	if (!Owner || MeleeDamageGameplayEffect == nullptr)
	{
		return FGameplayEffectSpecHandle();
	}

	UAbilitySystemComponent* const SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!SourceASC)
	{
		return FGameplayEffectSpecHandle();
	}

	FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
	Ctx.AddSourceObject(this);
	Ctx.AddInstigator(Owner, Owner);

	const float Level = 1.f;
	FGameplayEffectSpecHandle Out = SourceASC->MakeOutgoingSpec(MeleeDamageGameplayEffect, Level, Ctx);
	if (Out.IsValid() && Out.Data)
	{
		if (DamageTag.IsValid())
		{
			Out.Data->SetSetByCallerMagnitude(DamageTag, BaseDamageValue);
		}
		if (KnockbackTag.IsValid())
		{
			Out.Data->SetSetByCallerMagnitude(KnockbackTag, KnockbackForce);
		}
	}
	return Out;
}

void UDFMeleeTraceComponent::ApplyDamageToTargetBP(AActor* const Target, const FGameplayEffectSpecHandle SpecHandle)
{
	ApplyDamageToTarget(Target, SpecHandle, nullptr);
}

void UDFMeleeTraceComponent::ApplyDamageToTarget(AActor* const Target, const FGameplayEffectSpecHandle& SpecHandle, const FHitResult* const OptionalHit)
{
	AActor* const Owner = GetOwner();
	if (!Owner || !Target || Target == Owner || !SpecHandle.IsValid() || !SpecHandle.Data)
	{
		return;
	}

	if (bServerOnlyTraces)
	{
		if (!GetOwner() || !GetOwner()->HasAuthority())
		{
			return;
		}
	}

	UAbilitySystemComponent* const SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!SourceASC)
	{
		return;
	}

	UAbilitySystemComponent* const TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC)
	{
		return;
	}

	const float DmgMagnitude = DamageTag.IsValid() ? SpecHandle.Data->GetSetByCallerMagnitude(DamageTag, false, 0.f) : 0.f;
	const float KbMagnitude = KnockbackTag.IsValid() ? SpecHandle.Data->GetSetByCallerMagnitude(KnockbackTag, false, 0.f) : 0.f;

	const float Health = TargetASC->GetNumericAttribute(UDFAttributeSet::GetHealthAttribute());
	const float MaxH = TargetASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute());
	if (MaxH > KINDA_SMALL_NUMBER && (Health / MaxH) < FinishingHealthFractionThreshold
		&& FinishingBlowGameplayEffect
		&& FinishingSetByCallerTag.IsValid())
	{
		FGameplayEffectContextHandle FCtx = SourceASC->MakeEffectContext();
		FCtx.AddInstigator(Owner, Owner);
		FCtx.AddSourceObject(this);
		const FGameplayEffectSpecHandle FinSpec = SourceASC->MakeOutgoingSpec(
			FinishingBlowGameplayEffect, 1.f, FCtx);
		if (FinSpec.IsValid() && FinSpec.Data)
		{
			FinSpec.Data->SetSetByCallerMagnitude(FinishingSetByCallerTag, FinishingSetByCallerMagnitude);
			SourceASC->ApplyGameplayEffectSpecToTarget(*FinSpec.Data.Get(), TargetASC);
		}
	}

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

	if (UDFHitReactionComponent* Hit = Target->FindComponentByClass<UDFHitReactionComponent>())
	{
		FVector ToTarget = Target->GetActorLocation() - Owner->GetActorLocation();
		ToTarget.Z = 0.f;
		ToTarget.Normalize();
		const FVector P = (OptionalHit && OptionalHit->bBlockingHit) ? OptionalHit->ImpactPoint : FVector::ZeroVector;
		const FVector N = (OptionalHit && OptionalHit->bBlockingHit) ? OptionalHit->ImpactNormal : FVector::UpVector;
		Hit->OnHitReceived(DmgMagnitude, KbMagnitude, ToTarget, Owner, P, N);
	}
}

void UDFMeleeTraceComponent::TickTrace(float /*DeltaTime*/)
{
	if (bServerOnlyTraces && (!GetOwner() || !GetOwner()->HasAuthority()))
	{
		return;
	}
	UWorld* const World = GetWorld();
	USkeletalMeshComponent* const Mesh = GetMesh();
	AActor* const Owner = GetOwner();
	if (!World || !Mesh || !Owner)
	{
		return;
	}

	if (!Mesh->DoesSocketExist(TraceStartSocket) || !Mesh->DoesSocketExist(TraceEndSocket))
	{
		return;
	}

	const FVector TraceStart = Mesh->GetSocketLocation(TraceStartSocket);
	const FVector TraceEnd = Mesh->GetSocketLocation(TraceEndSocket);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(DF_MeleeTrace), false, Owner);
	Params.bReturnPhysicalMaterial = true;
	Params.AddIgnoredActor(Owner);

	FCollisionShape Shape = FCollisionShape::MakeSphere(TraceRadius);
	TArray<FHitResult> Hits;
	const bool bHit = World->SweepMultiByChannel(
		Hits, TraceStart, TraceEnd, FQuat::Identity, TraceChannel, Shape, Params);

	if (bDrawDebugTrace)
	{
#if ENABLE_DRAW_DEBUG
		const FColor Color = bHit ? FColor::Green : FColor::Red;
		DrawDebugSphere(World, TraceStart, TraceRadius, 8, Color, false, 0.05f, 0, 0.5f);
		DrawDebugSphere(World, TraceEnd, TraceRadius, 8, Color, false, 0.05f, 0, 0.5f);
		DrawDebugLine(World, TraceStart, TraceEnd, FColor::Yellow, false, 0.05f, 0, 0.5f);
#endif
	}

	if (!bHit)
	{
		return;
	}

	for (FHitResult& H : Hits)
	{
		AActor* const HitActor = H.GetActor();
		if (!HitActor || HitActor == Owner)
		{
			continue;
		}
		if (HitActorsThisSwing.ContainsByPredicate([HitActor](const TWeakObjectPtr<AActor>& W) { return W.Get() == HitActor; }))
		{
			continue;
		}
		HitActorsThisSwing.Add(HitActor);
#if ENABLE_DRAW_DEBUG
		if (bDrawDebugTrace)
		{
			const float PersistSec = 2.f;
			const FVector Mark = H.ImpactPoint.IsNearlyZero() ? H.Location : H.ImpactPoint;
			DrawDebugSphere(World, Mark, 10.f, 10, FColor::Cyan, false, PersistSec, 0, 1.5f);
			const FVector N = H.ImpactNormal.IsNearlyZero() ? FVector::UpVector : H.ImpactNormal.GetSafeNormal();
			DrawDebugDirectionalArrow(World, Mark, Mark + N * 35.f, 24.f, FColor::Magenta, false, PersistSec, 0, 1.25f);
		}
#endif
		ApplyDamageToTarget(HitActor, CachedDamageSpec, &H);
	}
}
