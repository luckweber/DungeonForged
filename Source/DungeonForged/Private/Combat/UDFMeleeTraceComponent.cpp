// Source/DungeonForged/Private/Combat/UDFMeleeTraceComponent.cpp
#include "Combat/UDFMeleeTraceComponent.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Combat/AN/AN_TraceEnd.h"
#include "Combat/AN/AN_TraceStart.h"
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
#include "Engine/OverlapResult.h"
#include "Components/CapsuleComponent.h"
#include "Characters/ADFEnemyBase.h"
#include "Characters/ADFPlayerCharacter.h"
#include "DFLootDrop.h"
#include "EngineUtils.h"
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

static TAutoConsoleVariable<int32> CVarDF_MeleeTraceLog(
	TEXT("df.MeleeTraceLog"),
	0,
	TEXT("Logs detalhados de melee trace no Output Log.\n")
	TEXT(" 0: Off (default)\n")
	TEXT(" 1: Com df.meleedebug / bVerboseTraceLog (Start/End/sweep)\n")
	TEXT(" 2: Cada tick do sweep enquanto bTracing"),
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

namespace
{
bool FindMeleeTraceNotifyTimes(UAnimMontage* const Montage, float& OutStartSec, float& OutEndSec)
{
	OutStartSec = -1.f;
	OutEndSec = -1.f;
	if (!Montage)
	{
		return false;
	}

	for (const FAnimNotifyEvent& Ev : Montage->Notifies)
	{
		if (!Ev.Notify)
		{
			continue;
		}
		const float T = Ev.GetTriggerTime();
		UClass* const NotifyClass = Ev.Notify->GetClass();
		if (NotifyClass->IsChildOf(UAN_TraceStart::StaticClass()))
		{
			OutStartSec = T;
		}
		else if (NotifyClass->IsChildOf(UAN_TraceEnd::StaticClass()))
		{
			OutEndSec = T;
		}
	}
	return OutStartSec >= 0.f || OutEndSec >= 0.f;
}
} // namespace

FCollisionObjectQueryParams UDFMeleeTraceComponent::BuildMeleeTraceObjectQuery() const
{
	FCollisionObjectQueryParams Obj;
	Obj.AddObjectTypesToQuery(ECC_Pawn);
	Obj.AddObjectTypesToQuery(ECC_PhysicsBody);
	if (bIncludeWorldDynamicInTraceQuery)
	{
		Obj.AddObjectTypesToQuery(ECC_WorldDynamic);
	}
	return Obj;
}

USkeletalMeshComponent* UDFMeleeTraceComponent::GetOwnerBodyMesh() const
{
	if (const ACharacter* const C = Cast<ACharacter>(GetOwner()))
	{
		return C->GetMesh();
	}
	return nullptr;
}

bool UDFMeleeTraceComponent::IsTraceSegmentNearOwner(const AActor* Owner, const FVector& A, const FVector& B, const float MaxDistFromOwner)
{
	if (!Owner || FVector::Dist(A, B) < 1.f)
	{
		return false;
	}
	return FVector::Dist(Owner->GetActorLocation(), (A + B) * 0.5f) <= MaxDistFromOwner;
}

bool UDFMeleeTraceComponent::TrySegmentFromBodyMesh(AActor* const Owner, FVector& OutStart, FVector& OutEnd) const
{
	if (!Owner)
	{
		return false;
	}

	USkeletalMeshComponent* const Body = GetOwnerBodyMesh();
	if (!Body)
	{
		return false;
	}

	if (Body->DoesSocketExist(TraceStartSocket) && Body->DoesSocketExist(TraceEndSocket))
	{
		const FVector A = Body->GetSocketLocation(TraceStartSocket);
		const FVector B = Body->GetSocketLocation(TraceEndSocket);
		if (IsTraceSegmentNearOwner(Owner, A, B, MaxTraceSocketDistanceFromOwner))
		{
			OutStart = A;
			OutEnd = B;
			return true;
		}
	}

	static const FName NWeaponR(TEXT("weapon_r"));
	if (Body->DoesSocketExist(NWeaponR))
	{
		const FVector Hand = Body->GetSocketLocation(NWeaponR);
		const FVector Fwd = Owner->GetActorForwardVector().GetSafeNormal();
		OutStart = Hand + Fwd * 8.f + FVector(0.f, 0.f, 8.f);
		OutEnd = OutStart + Fwd * FallbackReachCm;
		if (IsTraceSegmentNearOwner(Owner, OutStart, OutEnd, MaxTraceSocketDistanceFromOwner))
		{
			return true;
		}
	}

	return false;
}

bool UDFMeleeTraceComponent::TryBuildFallbackTraceSegment(FVector& OutStart, FVector& OutEnd) const
{
	AActor* const Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	const FVector Hand = Owner->GetActorLocation();
	const FVector Fwd = Owner->GetActorForwardVector().GetSafeNormal();
	OutStart = Hand + FVector(0.f, 0.f, 10.f);
	OutEnd = OutStart + Fwd * FallbackReachCm;
	return FVector::Dist(OutStart, OutEnd) >= 1.f;
}

bool UDFMeleeTraceComponent::ResolveTraceSegmentWorld(FVector& OutStart, FVector& OutEnd)
{
	bLastTraceUsedFallbackSegment = false;
	AActor* const Owner = GetOwner();
	USkeletalMeshComponent* const WeaponMesh = GetMesh();
	if (!Owner || !WeaponMesh)
	{
		return false;
	}

	// Leader-pose weapon meshes often return component-local socket coords; use animated body mesh first.
	if (TrySegmentFromBodyMesh(Owner, OutStart, OutEnd))
	{
		return true;
	}

	if (WeaponMesh->DoesSocketExist(TraceStartSocket) && WeaponMesh->DoesSocketExist(TraceEndSocket))
	{
		const FVector A = WeaponMesh->GetSocketLocation(TraceStartSocket);
		const FVector B = WeaponMesh->GetSocketLocation(TraceEndSocket);
		if (IsTraceSegmentNearOwner(Owner, A, B, MaxTraceSocketDistanceFromOwner))
		{
			OutStart = A;
			OutEnd = B;
			return true;
		}
	}

	bLastTraceUsedFallbackSegment = true;
	return TryBuildFallbackTraceSegment(OutStart, OutEnd);
}

namespace
{
static const TCHAR* DF_CollisionEnabledStr(const ECollisionEnabled::Type V)
{
	switch (V)
	{
	case ECollisionEnabled::NoCollision: return TEXT("NoCollision");
	case ECollisionEnabled::QueryOnly: return TEXT("QueryOnly");
	case ECollisionEnabled::PhysicsOnly: return TEXT("PhysicsOnly");
	case ECollisionEnabled::QueryAndPhysics: return TEXT("QueryAndPhysics");
	default: return TEXT("?");
	}
}

static const TCHAR* DF_ObjectTypeStr(const ECollisionChannel Ch)
{
	switch (Ch)
	{
	case ECC_WorldStatic: return TEXT("WorldStatic");
	case ECC_WorldDynamic: return TEXT("WorldDynamic");
	case ECC_Pawn: return TEXT("Pawn");
	case ECC_PhysicsBody: return TEXT("PhysicsBody");
	case ECC_Vehicle: return TEXT("Vehicle");
	case ECC_Destructible: return TEXT("Destructible");
	default: return TEXT("Other");
	}
}
} // namespace

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

FString UDFMeleeTraceComponent::GetMeleeTraceDiagnosticString() const
{
	USkeletalMeshComponent* const Mesh = GetMesh();
	if (!IsValid(Mesh))
	{
		if (SkeletalMesh && !IsValid(SkeletalMesh))
		{
			return TEXT("INVALID SkeletalMesh ref (clear MeleeTrace->Skeletal Mesh in BP, leave None)");
		}
		return TEXT("(no mesh — equip weapon or assign Mesh_Weapon)");
	}

	USkeletalMesh* const Sk = Mesh->GetSkeletalMeshAsset();
	const FString SkName = Sk ? Sk->GetName() : TEXT("(no SK asset)");
	const bool bStart = Mesh->DoesSocketExist(TraceStartSocket);
	const bool bEnd = Mesh->DoesSocketExist(TraceEndSocket);
	if (!bStart || !bEnd)
	{
		return FString::Printf(
			TEXT("%s / SK=%s | MISSING socket start=%d end=%d — fix SK_sword or rename sockets"),
			*Mesh->GetName(),
			*SkName,
			bStart ? 1 : 0,
			bEnd ? 1 : 0);
	}

	const FVector A = Mesh->GetSocketLocation(TraceStartSocket);
	const FVector B = Mesh->GetSocketLocation(TraceEndSocket);
	const float SpanCm = FVector::Dist(A, B);
	if (SpanCm < 1.f)
	{
		return FString::Printf(
			TEXT("%s / SK=%s | sockets COINCIDENT (span=%.2f cm) — move weapon_end to blade tip in SK_sword"),
			*Mesh->GetName(),
			*SkName,
			SpanCm);
	}

	return FString::Printf(TEXT("%s / SK=%s | span=%.1f cm"), *Mesh->GetName(), *SkName, SpanCm);
}

bool UDFMeleeTraceComponent::ShouldLogMeleeTraceDetail() const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	if (bVerboseTraceLog || bDrawDebugTrace)
	{
		return true;
	}
	const IConsoleVariable* const Cv = IConsoleManager::Get().FindConsoleVariable(TEXT("df.MeleeTraceLog"));
	return Cv && Cv->GetInt() > 0;
#endif
}

static bool DF_ShouldLogMeleeTracePerTick()
{
#if UE_BUILD_SHIPPING
	return false;
#else
	const IConsoleVariable* const Cv = IConsoleManager::Get().FindConsoleVariable(TEXT("df.MeleeTraceLog"));
	return Cv && Cv->GetInt() >= 2;
#endif
}

void UDFMeleeTraceComponent::LogActorCollisionForMelee(AActor* const Actor, const TCHAR* const Label) const
{
	if (!Actor || !ShouldLogMeleeTraceDetail())
	{
		return;
	}
	UE_LOG(LogDungeonForged, Log, TEXT("[MeleeTrace|Col] %s: Actor=%s Loc=%s Auth=%d"),
		Label,
		*Actor->GetName(),
		*Actor->GetActorLocation().ToCompactString(),
		Actor->HasAuthority() ? 1 : 0);

	if (const UCapsuleComponent* const Cap = Actor->FindComponentByClass<UCapsuleComponent>())
	{
		UE_LOG(LogDungeonForged, Log,
			TEXT("  Capsule: enabled=%s obj=%s r=%.1f h=%.1f"),
			DF_CollisionEnabledStr(Cap->GetCollisionEnabled()),
			DF_ObjectTypeStr(Cap->GetCollisionObjectType()),
			Cap->GetUnscaledCapsuleRadius(),
			Cap->GetUnscaledCapsuleHalfHeight());
	}

	TArray<UPrimitiveComponent*> Prims;
	Actor->GetComponents<UPrimitiveComponent>(Prims);
	int32 Logged = 0;
	for (UPrimitiveComponent* const P : Prims)
	{
		if (!P || P->IsA<UCapsuleComponent>())
		{
			continue;
		}
		if (P->GetCollisionEnabled() == ECollisionEnabled::NoCollision && Logged >= 2)
		{
			continue;
		}
		UE_LOG(LogDungeonForged, Log,
			TEXT("  Prim[%d] %s: enabled=%s obj=%s"),
			Logged,
			*P->GetName(),
			DF_CollisionEnabledStr(P->GetCollisionEnabled()),
			DF_ObjectTypeStr(P->GetCollisionObjectType()));
		++Logged;
		if (Logged >= 4)
		{
			break;
		}
	}

	if (UAbilitySystemComponent* const ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor))
	{
		const float H = ASC->GetNumericAttribute(UDFAttributeSet::GetHealthAttribute());
		const float MaxH = ASC->GetNumericAttribute(UDFAttributeSet::GetMaxHealthAttribute());
		UE_LOG(LogDungeonForged, Log, TEXT("  ASC: Health=%.1f/%.1f"), H, MaxH);
	}
	else
	{
		UE_LOG(LogDungeonForged, Warning, TEXT("  ASC: (none)"));
	}
}

void UDFMeleeTraceComponent::LogMeleeTraceWeaponAndSockets() const
{
	if (!ShouldLogMeleeTraceDetail())
	{
		return;
	}
	AActor* const Owner = GetOwner();
	USkeletalMeshComponent* const TraceMesh = GetMesh();
	UE_LOG(LogDungeonForged, Log, TEXT("[MeleeTrace|Weapon] --------"));
	UE_LOG(LogDungeonForged, Log,
		TEXT("  Owner=%s Auth=%d bTracing=%d bServerOnlyTraces=%d"),
		Owner ? *Owner->GetName() : TEXT("(none)"),
		Owner && Owner->HasAuthority() ? 1 : 0,
		bTracing ? 1 : 0,
		bServerOnlyTraces ? 1 : 0);

	if (!TraceMesh)
	{
		UE_LOG(LogDungeonForged, Warning, TEXT("  TraceMesh=NULL (assign Mesh_Weapon or equip sword)"));
		return;
	}

	USkeletalMesh* const Sk = TraceMesh->GetSkeletalMeshAsset();
	UE_LOG(LogDungeonForged, Log,
		TEXT("  TraceMesh=%s SK=%s Visible=%d Tick=%s"),
		*TraceMesh->GetName(),
		Sk ? *Sk->GetName() : TEXT("(none)"),
		TraceMesh->IsVisible() ? 1 : 0,
		TraceMesh->IsComponentTickEnabled() ? TEXT("on") : TEXT("off"));
	UE_LOG(LogDungeonForged, Log,
		TEXT("  MeshCollision=%s obj=%s (sword mesh collision is NOT used for hits)"),
		DF_CollisionEnabledStr(TraceMesh->GetCollisionEnabled()),
		DF_ObjectTypeStr(TraceMesh->GetCollisionObjectType()));
	UE_LOG(LogDungeonForged, Log,
		TEXT("  CompLoc=%s | %s"),
		*TraceMesh->GetComponentLocation().ToCompactString(),
		*GetMeleeTraceDiagnosticString());

	if (TraceMesh->DoesSocketExist(TraceStartSocket) && TraceMesh->DoesSocketExist(TraceEndSocket))
	{
		const FVector A = TraceMesh->GetSocketLocation(TraceStartSocket);
		const FVector B = TraceMesh->GetSocketLocation(TraceEndSocket);
		const float MidDistOwner = Owner
			? FVector::Dist(Owner->GetActorLocation(), (A + B) * 0.5f)
			: -1.f;
		UE_LOG(LogDungeonForged, Log,
			TEXT("  SocketWS: %s=%s | %s=%s | segDist=%.1f midDistOwner=%.0f (max=%.0f)"),
			*TraceStartSocket.ToString(),
			*A.ToCompactString(),
			*TraceEndSocket.ToString(),
			*B.ToCompactString(),
			FVector::Dist(A, B),
			MidDistOwner,
			MaxTraceSocketDistanceFromOwner);
		if (Owner && MidDistOwner > MaxTraceSocketDistanceFromOwner)
		{
			UE_LOG(LogDungeonForged, Warning,
				TEXT("  Sockets too far from owner — fallback hand/forward segment will be used"));
		}
	}

	if (const ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(Owner))
	{
		if (PC->Mesh_Weapon && PC->Mesh_Weapon != TraceMesh)
		{
			UE_LOG(LogDungeonForged, Warning,
				TEXT("  Mesh_Weapon(%s) != trace mesh(%s)"),
				*PC->Mesh_Weapon->GetName(),
				*TraceMesh->GetName());
		}
		if (USkeletalMeshComponent* const Body = PC->GetMesh())
		{
			static const FName NWeaponR(TEXT("weapon_r"));
			if (Body->DoesSocketExist(NWeaponR))
			{
				UE_LOG(LogDungeonForged, Log,
					TEXT("  BodyMesh %s socket weapon_r WS=%s"),
					*Body->GetName(),
					*Body->GetSocketLocation(NWeaponR).ToCompactString());
			}
		}
	}

	UE_LOG(LogDungeonForged, Log,
		TEXT("  GAS: GE=%s BaseDmg=%.1f SpecValid=%d DamageTag=%s"),
		MeleeDamageGameplayEffect ? *MeleeDamageGameplayEffect->GetName() : TEXT("None"),
		BaseDamage,
		CachedDamageSpec.IsValid() ? 1 : 0,
		DamageTag.IsValid() ? *DamageTag.ToString() : TEXT("(invalid)"));
}

void UDFMeleeTraceComponent::LogMeleeTraceNearestTargets(const FVector& TraceStart, const FVector& TraceEnd) const
{
	if (!ShouldLogMeleeTraceDetail())
	{
		return;
	}
	UWorld* const World = GetWorld();
	AActor* const Owner = GetOwner();
	if (!World || !Owner)
	{
		return;
	}

	const FVector Mid = (TraceStart + TraceEnd) * 0.5f;
	const float SearchR = FMath::Max(400.f, FVector::Dist(TraceStart, TraceEnd) + TraceRadius + 100.f);
	FCollisionObjectQueryParams Obj = BuildMeleeTraceObjectQuery();
	FCollisionQueryParams Q(SCENE_QUERY_STAT(DF_MeleeTraceDiag), false, Owner);
	Q.bTraceComplex = true;
	TArray<FOverlapResult> Overlaps;
	if (!World->OverlapMultiByObjectType(Overlaps, Mid, FQuat::Identity, Obj, FCollisionShape::MakeSphere(SearchR), Q))
	{
		UE_LOG(LogDungeonForged, Warning,
			TEXT("[MeleeTrace|Targets] No overlaps in %.0f cm around trace mid %s"),
			SearchR,
			*Mid.ToCompactString());
		return;
	}

	UE_LOG(LogDungeonForged, Log,
		TEXT("[MeleeTrace|Targets] %d overlaps in %.0f cm (trace mid %s)"),
		Overlaps.Num(),
		SearchR,
		*Mid.ToCompactString());

	TSet<AActor*> Logged;
	int32 Idx = 0;
	for (const FOverlapResult& O : Overlaps)
	{
		AActor* const A = O.GetActor();
		if (!A || A == Owner || A->IsA<ADFLootDrop>() || Logged.Contains(A))
		{
			continue;
		}
		Logged.Add(A);
		const float DistOwner = FVector::Dist(Owner->GetActorLocation(), A->GetActorLocation());
		const float DistSeg = FMath::PointDistToSegment(A->GetActorLocation(), TraceStart, TraceEnd);
		UE_LOG(LogDungeonForged, Log,
			TEXT("  [%d] %s distOwner=%.0f distToTraceSeg=%.0f comp=%s"),
			Idx++,
			*A->GetName(),
			DistOwner,
			DistSeg,
			O.GetComponent() ? *O.GetComponent()->GetName() : TEXT("?"));
		LogActorCollisionForMelee(A, TEXT("Target"));
		if (Idx >= 5)
		{
			break;
		}
	}
}

void UDFMeleeTraceComponent::DumpMeleeTraceDiagnostics() const
{
#if UE_BUILD_SHIPPING
	UE_LOG(LogDungeonForged, Log, TEXT("DumpMeleeTraceDiagnostics: shipping build — limited output."));
#else
	UE_LOG(LogDungeonForged, Log, TEXT("========== MeleeTrace DUMP =========="));
	UDFMeleeTraceComponent* const Self = const_cast<UDFMeleeTraceComponent*>(this);
	Self->LogMeleeTraceWeaponAndSockets();
	FVector SegA = FVector::ZeroVector;
	FVector SegB = FVector::ZeroVector;
	if (Self->ResolveTraceSegmentWorld(SegA, SegB))
	{
		Self->LogMeleeTraceNearestTargets(SegA, SegB);
	}
	if (AActor* const O = GetOwner())
	{
		Self->LogActorCollisionForMelee(O, TEXT("Owner"));
		if (UWorld* const W = GetWorld())
		{
			ADFEnemyBase* Nearest = nullptr;
			float NearestDist = MAX_FLT;
			for (TActorIterator<ADFEnemyBase> It(W); It; ++It)
			{
				ADFEnemyBase* const E = *It;
				if (!E || E->HasDied())
				{
					continue;
				}
				const float D = FVector::Dist(O->GetActorLocation(), E->GetActorLocation());
				if (D < NearestDist)
				{
					NearestDist = D;
					Nearest = E;
				}
			}
			if (Nearest)
			{
				UE_LOG(LogDungeonForged, Log,
					TEXT("[MeleeTrace|Enemy] nearest=%s distOwner=%.0f distToSeg=%.0f"),
					*Nearest->GetName(),
					NearestDist,
					FMath::PointDistToSegment(Nearest->GetActorLocation(), SegA, SegB));
				Self->LogActorCollisionForMelee(Nearest, TEXT("NearestEnemy"));
			}
			else
			{
				UE_LOG(LogDungeonForged, Warning, TEXT("[MeleeTrace|Enemy] no ADFEnemyBase in level"));
			}
		}
	}
	UE_LOG(LogDungeonForged, Log, TEXT("========== END MeleeTrace DUMP =========="));
#endif
}

void UDFMeleeTraceComponent::StartTrace()
{
	if (bTracing)
	{
		return;
	}
	HitActorsThisSwing.Empty();
	bTracing = true;
	const float Dmg = bUseOverrideBaseDamage ? OverrideBaseDamage : BaseDamage;
	const float Kb = bUseOverrideKnockback ? OverrideBaseKnockback : BaseKnockback;
	bUseOverrideBaseDamage = false;
	bUseOverrideKnockback = false;
	CachedDamageSpec = BuildDamageSpec(Dmg, Kb);
	SetComponentTickEnabled(true);
	bLastTraceUsedFallbackSegment = false;

#if !UE_BUILD_SHIPPING
	if (!CachedDamageSpec.IsValid())
	{
		UE_LOG(LogDungeonForged, Warning,
			TEXT("MeleeTrace::StartTrace: invalid damage spec (MeleeDamageGameplayEffect=%s, owner=%s). Assign GE_MeleeDamage on MeleeTrace or DT_Items."),
			MeleeDamageGameplayEffect ? *MeleeDamageGameplayEffect->GetName() : TEXT("None"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("(none)"));
	}
	else if (bDrawDebugTrace)
	{
		UE_LOG(LogDungeonForged, Log, TEXT("MeleeTrace::StartTrace: owner=%s | %s"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("(none)"),
			*GetMeleeTraceDiagnosticString());
	}
	if (ShouldLogMeleeTraceDetail())
	{
		LogMeleeTraceWeaponAndSockets();
		FVector DbgA = FVector::ZeroVector;
		FVector DbgB = FVector::ZeroVector;
		if (ResolveTraceSegmentWorld(DbgA, DbgB))
		{
			LogMeleeTraceNearestTargets(DbgA, DbgB);
		}
	}
#endif
}

void UDFMeleeTraceComponent::ClearScheduledTraceWindows()
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ScheduledTraceStartTimer);
		World->GetTimerManager().ClearTimer(ScheduledTraceEndTimer);
	}
}

void UDFMeleeTraceComponent::ScheduleAuthorityTraceWindowsFromMontage(UAnimMontage* const Montage, const float PlayRate)
{
	ClearScheduledTraceWindows();
	AActor* const Owner = GetOwner();
	UWorld* const World = GetWorld();
	if (!Montage || !Owner || !World || !Owner->HasAuthority())
	{
		return;
	}

	float StartSec = -1.f;
	float EndSec = -1.f;
	if (!FindMeleeTraceNotifyTimes(Montage, StartSec, EndSec))
	{
#if !UE_BUILD_SHIPPING
		if (ShouldLogMeleeTraceDetail())
		{
			UE_LOG(LogDungeonForged, Warning,
				TEXT("[MeleeTrace|Schedule] montage %s has no AN_TraceStart/End notifies"),
				*Montage->GetName());
		}
#endif
		return;
	}

#if !UE_BUILD_SHIPPING
	if (ShouldLogMeleeTraceDetail())
	{
		UE_LOG(LogDungeonForged, Log,
			TEXT("[MeleeTrace|Schedule] %s start=%.3fs end=%.3fs (auth timer)"),
			*Montage->GetName(),
			StartSec,
			EndSec);
	}
#endif

	const float Rate = FMath::Max(PlayRate, KINDA_SMALL_NUMBER);
	if (StartSec >= 0.f)
	{
		World->GetTimerManager().SetTimer(
			ScheduledTraceStartTimer,
			this,
			&UDFMeleeTraceComponent::StartTrace,
			StartSec / Rate,
			false);
	}
	if (EndSec >= 0.f)
	{
		World->GetTimerManager().SetTimer(
			ScheduledTraceEndTimer,
			this,
			&UDFMeleeTraceComponent::EndTrace,
			EndSec / Rate,
			false);
	}
}

void UDFMeleeTraceComponent::EndTrace()
{
	ClearScheduledTraceWindows();
#if !UE_BUILD_SHIPPING
	if (bTracing && (bDrawDebugTrace || ShouldLogMeleeTraceDetail()))
	{
		UE_LOG(LogDungeonForged, Log,
			TEXT("MeleeTrace::EndTrace: owner=%s uniqueHits=%d auth=%d lastSeg=%s -> %s"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("(none)"),
			HitActorsThisSwing.Num(),
			GetOwner() && GetOwner()->HasAuthority() ? 1 : 0,
			bHasLastTraceSegment ? *LastTraceStartWS.ToCompactString() : TEXT("(none)"),
			bHasLastTraceSegment ? *LastTraceEndWS.ToCompactString() : TEXT("(none)"));
		if (ShouldLogMeleeTraceDetail() && bHasLastTraceSegment)
		{
			if (AActor* const O = GetOwner())
			{
				const float MidDist = FVector::Dist(O->GetActorLocation(), (LastTraceStartWS + LastTraceEndWS) * 0.5f);
				UE_LOG(LogDungeonForged, Log,
					TEXT("  traceMidDistFromOwner=%.0f fallbackSeg=%d"),
					MidDist,
					bLastTraceUsedFallbackSegment ? 1 : 0);
			}
			LogMeleeTraceNearestTargets(LastTraceStartWS, LastTraceEndWS);
		}
	}
#if ENABLE_DRAW_DEBUG
	if (bDrawDebugTrace && bHasLastTraceSegment)
	{
		if (UWorld* const DbgWorld = GetWorld())
		{
			const FColor SegColor = HitActorsThisSwing.Num() > 0 ? FColor::Green : FColor::Red;
			DrawDebugLine(DbgWorld, LastTraceStartWS, LastTraceEndWS, SegColor, false, 3.f, 0, 2.5f);
			DrawDebugSphere(DbgWorld, LastTraceStartWS, TraceRadius, 10, SegColor, false, 3.f, 0, 1.5f);
			DrawDebugSphere(DbgWorld, LastTraceEndWS, TraceRadius, 10, SegColor, false, 3.f, 0, 1.5f);
			if (HitActorsThisSwing.Num() == 0)
			{
				DrawDebugString(DbgWorld, (LastTraceStartWS + LastTraceEndWS) * 0.5f,
					TEXT("MeleeTrace: 0 hits (sweep uses sockets, not sword mesh collision)"),
					nullptr, FColor::Red, 3.f, true, 1.1f);
			}
		}
	}
#endif
#endif
	bHasLastTraceSegment = false;
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

void UDFMeleeTraceComponent::ProcessHitResults(TArray<FHitResult>& Hits, UWorld* World)
{
	AActor* const Owner = GetOwner();
	if (!Owner || !World)
	{
		return;
	}
	for (FHitResult& H : Hits)
	{
		AActor* const HitActor = H.GetActor();
		if (!HitActor || HitActor == Owner || HitActor->IsA<ADFLootDrop>())
		{
#if !UE_BUILD_SHIPPING
			if (ShouldLogMeleeTraceDetail() && HitActor && HitActor != Owner)
			{
				UE_LOG(LogDungeonForged, Verbose, TEXT("[MeleeTrace|Sweep] skip non-target %s"), *HitActor->GetName());
			}
#endif
			continue;
		}
		if (HitActorsThisSwing.ContainsByPredicate([HitActor](const TWeakObjectPtr<AActor>& W) { return W.Get() == HitActor; }))
		{
#if !UE_BUILD_SHIPPING
			if (ShouldLogMeleeTraceDetail())
			{
				UE_LOG(LogDungeonForged, Verbose, TEXT("[MeleeTrace|Sweep] skip already hit %s"), *HitActor->GetName());
			}
#endif
			continue;
		}
		if (!UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor))
		{
#if !UE_BUILD_SHIPPING
			if (ShouldLogMeleeTraceDetail())
			{
				UE_LOG(LogDungeonForged, Warning,
					TEXT("[MeleeTrace|Sweep] hit %s comp=%s but NO ASC — ignored"),
					*HitActor->GetName(),
					H.GetComponent() ? *H.GetComponent()->GetName() : TEXT("?"));
			}
#endif
			continue;
		}
#if !UE_BUILD_SHIPPING
		if (ShouldLogMeleeTraceDetail())
		{
			UE_LOG(LogDungeonForged, Log,
				TEXT("[MeleeTrace|Sweep] VALID hit %s comp=%s obj=%s impact=%s"),
				*HitActor->GetName(),
				H.GetComponent() ? *H.GetComponent()->GetName() : TEXT("?"),
				H.GetComponent() ? DF_ObjectTypeStr(H.GetComponent()->GetCollisionObjectType()) : TEXT("?"),
				*H.ImpactPoint.ToCompactString());
		}
#endif
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
		if (!CachedDamageSpec.IsValid())
		{
			UE_LOG(LogDungeonForged, Warning, TEXT("MeleeTrace: hit %s but CachedDamageSpec invalid — no damage applied."),
				*GetNameSafe(HitActor));
			continue;
		}
		ApplyDamageToTarget(HitActor, CachedDamageSpec, &H);
	}
}

bool UDFMeleeTraceComponent::TryMeleeOverlapFallback(
	UWorld* World, AActor* Owner, const FVector& Center, const float Radius)
{
	if (!World || !Owner)
	{
		return false;
	}
	FCollisionObjectQueryParams Obj = BuildMeleeTraceObjectQuery();
	FCollisionQueryParams Q(SCENE_QUERY_STAT(DF_MeleeTraceOverlap), false, Owner);
	Q.bTraceComplex = true;
	Q.AddIgnoredActor(Owner);
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);
	TArray<FOverlapResult> Overlaps;
	if (!World->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, Obj, Sphere, Q))
	{
#if !UE_BUILD_SHIPPING
		if (ShouldLogMeleeTraceDetail())
		{
			UE_LOG(LogDungeonForged, Verbose,
				TEXT("[MeleeTrace|Overlap] none at %s r=%.1f"),
				*Center.ToCompactString(),
				Radius);
		}
#endif
		return false;
	}
#if !UE_BUILD_SHIPPING
	if (ShouldLogMeleeTraceDetail())
	{
		UE_LOG(LogDungeonForged, Log,
			TEXT("[MeleeTrace|Overlap] %d overlaps at %s r=%.1f"),
			Overlaps.Num(),
			*Center.ToCompactString(),
			Radius);
	}
#endif
	TSet<AActor*> Seen;
	for (const FOverlapResult& O : Overlaps)
	{
		AActor* const A = O.GetActor();
		if (!A || A == Owner || A->IsA<ADFLootDrop>() || Seen.Contains(A))
		{
			continue;
		}
		if (!UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(A))
		{
#if !UE_BUILD_SHIPPING
			if (ShouldLogMeleeTraceDetail())
			{
				UE_LOG(LogDungeonForged, Warning,
					TEXT("[MeleeTrace|Overlap] %s comp=%s — no ASC"),
					*A->GetName(),
					O.GetComponent() ? *O.GetComponent()->GetName() : TEXT("?"));
			}
#endif
			continue;
		}
		Seen.Add(A);
		if (HitActorsThisSwing.ContainsByPredicate([A](const TWeakObjectPtr<AActor>& W) { return W.Get() == A; }))
		{
			continue;
		}
		HitActorsThisSwing.Add(A);
		if (!CachedDamageSpec.IsValid())
		{
			continue;
		}
#if !UE_BUILD_SHIPPING
		if (ShouldLogMeleeTraceDetail())
		{
			UE_LOG(LogDungeonForged, Log, TEXT("[MeleeTrace|Overlap] applying damage to %s"), *A->GetName());
		}
#endif
		ApplyDamageToTarget(A, CachedDamageSpec, nullptr);
	}
	return Seen.Num() > 0;
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
#if !UE_BUILD_SHIPPING
		UE_LOG(LogDungeonForged, Warning, TEXT("MeleeTrace: hit %s has no ASC — no damage."), *GetNameSafe(Target));
#endif
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

	const FActiveGameplayEffectHandle Applied =
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	const bool bInstantGE = SpecHandle.Data->Def
		&& SpecHandle.Data->Def->DurationPolicy == EGameplayEffectDurationType::Instant;
#if !UE_BUILD_SHIPPING
	if (!Applied.IsValid() && ShouldLogMeleeTraceDetail() && !bInstantGE)
	{
		UE_LOG(LogDungeonForged, Warning,
			TEXT("MeleeTrace: ApplyGameplayEffectSpecToTarget failed on %s (GE=%s)."),
			*GetNameSafe(Target),
			MeleeDamageGameplayEffect ? *MeleeDamageGameplayEffect->GetName() : TEXT("None"));
	}
	else if (ShouldLogMeleeTraceDetail())
	{
		UE_LOG(LogDungeonForged, Log,
			TEXT("MeleeTrace: damage applied to %s (SetByCaller=%.1f HP %.1f->%.1f)."),
			*GetNameSafe(Target),
			DmgMagnitude,
			Health,
			TargetASC->GetNumericAttribute(UDFAttributeSet::GetHealthAttribute()));
	}
#endif

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
	UWorld* const World = GetWorld();
	AActor* const Owner = GetOwner();
	if (!World || !Owner)
	{
		return;
	}

	FVector TraceStart = FVector::ZeroVector;
	FVector TraceEnd = FVector::ZeroVector;
	if (!ResolveTraceSegmentWorld(TraceStart, TraceEnd))
	{
		UE_LOG(LogDungeonForged, Warning, TEXT("MeleeTrace: could not build trace segment — %s"),
			*GetMeleeTraceDiagnosticString());
		return;
	}

	LastTraceStartWS = TraceStart;
	LastTraceEndWS = TraceEnd;
	bHasLastTraceSegment = true;

	const bool bAuthorityTrace = !bServerOnlyTraces || Owner->HasAuthority();

#if ENABLE_DRAW_DEBUG
	if (bDrawDebugTrace)
	{
		const FColor Color = FColor::Yellow;
		DrawDebugSphere(World, TraceStart, TraceRadius, 8, Color, false, 0.05f, 0, 0.5f);
		DrawDebugSphere(World, TraceEnd, TraceRadius, 8, Color, false, 0.05f, 0, 0.5f);
		DrawDebugLine(World, TraceStart, TraceEnd, Color, false, 0.05f, 0, 0.5f);
	}
#endif

	if (!bAuthorityTrace)
	{
		return;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(DF_MeleeTrace), false, Owner);
	Params.bReturnPhysicalMaterial = true;
	Params.bTraceComplex = true;
	Params.AddIgnoredActor(Owner);

	FCollisionShape Shape = FCollisionShape::MakeSphere(TraceRadius);
	TArray<FHitResult> Hits;
	const FCollisionObjectQueryParams ObjParams = BuildMeleeTraceObjectQuery();
	const bool bHit = World->SweepMultiByObjectType(
		Hits, TraceStart, TraceEnd, FQuat::Identity, ObjParams, Shape, Params);

#if !UE_BUILD_SHIPPING
	if (ShouldLogMeleeTraceDetail() && (DF_ShouldLogMeleeTracePerTick() || !bHit))
	{
		UE_LOG(LogDungeonForged, Log,
			TEXT("[MeleeTrace|Sweep] raw=%d auth=%d seg=%.1fcm r=%.1f %s->%s"),
			Hits.Num(),
			bAuthorityTrace ? 1 : 0,
			FVector::Dist(TraceStart, TraceEnd),
			TraceRadius,
			*TraceStart.ToCompactString(),
			*TraceEnd.ToCompactString());
		for (int32 i = 0; i < Hits.Num() && i < 6; ++i)
		{
			const FHitResult& H = Hits[i];
			UE_LOG(LogDungeonForged, Log,
				TEXT("  raw[%d] actor=%s comp=%s obj=%s"),
				i,
				*GetNameSafe(H.GetActor()),
				H.GetComponent() ? *H.GetComponent()->GetName() : TEXT("?"),
				H.GetComponent() ? DF_ObjectTypeStr(H.GetComponent()->GetCollisionObjectType()) : TEXT("?"));
		}
	}
#endif

	const int32 HitsBefore = HitActorsThisSwing.Num();
	if (bHit)
	{
		ProcessHitResults(Hits, World);
	}

	if (HitActorsThisSwing.Num() == HitsBefore)
	{
		const FVector Mid = (TraceStart + TraceEnd) * 0.5f;
		const float FallbackRadius = FMath::Max(TraceRadius + 15.f, FVector::Dist(TraceStart, TraceEnd) * 0.35f);
#if !UE_BUILD_SHIPPING
		if (ShouldLogMeleeTraceDetail())
		{
			UE_LOG(LogDungeonForged, Log, TEXT("[MeleeTrace|Sweep] no valid hits — trying overlap fallback"));
		}
#endif
		TryMeleeOverlapFallback(World, Owner, Mid, FallbackRadius);
	}
}
