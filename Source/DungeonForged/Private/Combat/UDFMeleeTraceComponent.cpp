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

UDFMeleeTraceComponent::UDFMeleeTraceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
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

void UDFMeleeTraceComponent::StartTrace()
{
	HitActorsThisSwing.Empty();
	bTracing = true;
	const float Dmg = bUseOverrideBaseDamage ? OverrideBaseDamage : BaseDamage;
	const float Kb = bUseOverrideKnockback ? OverrideBaseKnockback : BaseKnockback;
	bUseOverrideBaseDamage = false;
	bUseOverrideKnockback = false;
	CachedDamageSpec = BuildDamageSpec(Dmg, Kb);
	SetComponentTickEnabled(true);
}

void UDFMeleeTraceComponent::EndTrace()
{
	bTracing = false;
	SetComponentTickEnabled(false);
}

void UDFMeleeTraceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
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
		ApplyDamageToTarget(HitActor, CachedDamageSpec, &H);
	}
}
