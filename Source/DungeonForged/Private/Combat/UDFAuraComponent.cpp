// Source/DungeonForged/Private/Combat/UDFAuraComponent.cpp
#include "Combat/UDFAuraComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"

static FGenericTeamId DFAura_GetTeam(AActor* A)
{
	if (!A)
	{
		return FGenericTeamId(0);
	}
	if (const IGenericTeamAgentInterface* T = Cast<IGenericTeamAgentInterface>(A))
	{
		return T->GetGenericTeamId();
	}
	if (const APawn* P = Cast<APawn>(A))
	{
		if (P->GetController() && P->GetController()->IsPlayerController())
		{
			return FGenericTeamId(1u);
		}
	}
	return FGenericTeamId(0);
}

static bool DFAura_IsAlly(AActor* Caster, AActor* Other)
{
	if (!Caster || !Other)
	{
		return false;
	}
	if (Caster == Other)
	{
		return true;
	}
	const IGenericTeamAgentInterface* T1 = Cast<IGenericTeamAgentInterface>(Caster);
	const IGenericTeamAgentInterface* T2 = Cast<IGenericTeamAgentInterface>(Other);
	if (T1 && T2)
	{
		return T1->GetTeamAttitudeTowards(*Other) == ETeamAttitude::Friendly;
	}
	if (T1)
	{
		return T1->GetTeamAttitudeTowards(*Other) == ETeamAttitude::Friendly;
	}
	return DFAura_GetTeam(Caster) == DFAura_GetTeam(Other) && DFAura_GetTeam(Caster) != 0u;
}

UDFAuraComponent::UDFAuraComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	AuraSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AuraSphere"));
	AuraSphere->SetupAttachment(this);
	AuraSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AuraSphere->SetCanEverAffectNavigation(false);
	AuraSphere->SetCollisionObjectType(ECC_Pawn);
	AuraSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	AuraSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	AuraSphere->SetGenerateOverlapEvents(true);
}

void UDFAuraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(AuraTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void UDFAuraComponent::BeginPlay()
{
	Super::BeginPlay();
	SetAuraRadius(AuraRadius);
	if (GetOwner() && LoopingNiagara && !LoopingNiagaraComponent)
	{
		UNiagaraComponent* const Nc = NewObject<UNiagaraComponent>(GetOwner());
		if (Nc)
		{
			Nc->SetAutoDestroy(false);
			Nc->bAutoActivate = false;
			Nc->SetAsset(LoopingNiagara);
			Nc->SetupAttachment(this);
			Nc->RegisterComponent();
			LoopingNiagaraComponent = Nc;
		}
	}
	SetAuraEnabled(bAuraEnabled);
}

UAbilitySystemComponent* UDFAuraComponent::ResolveOwnerASC() const
{
	AActor* const O = GetOwner();
	return O ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(O) : nullptr;
}

void UDFAuraComponent::SetAuraEnabled(const bool bEnabled)
{
	bAuraEnabled = bEnabled;
	if (AuraSphere)
	{
		AuraSphere->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
	if (LoopingNiagaraComponent)
	{
		LoopingNiagaraComponent->SetActive(bEnabled, false);
	}
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(AuraTimer);
		if (bEnabled)
		{
			W->GetTimerManager().SetTimer(AuraTimer, this, &UDFAuraComponent::PulseAura, FMath::Max(0.01f, TickInterval), true, 0.f);
		}
	}
}

void UDFAuraComponent::SetAuraRadius(const float NewRadius)
{
	AuraRadius = FMath::Max(0.f, NewRadius);
	if (AuraSphere)
	{
		AuraSphere->SetSphereRadius(AuraRadius, true);
	}
}

void UDFAuraComponent::PulseAura()
{
	AActor* const Owner = GetOwner();
	if (!bAuraEnabled || !Owner || !Owner->HasAuthority() || !GetWorld() || !AuraSphere)
	{
		return;
	}
	UAbilitySystemComponent* const My = ResolveOwnerASC();
	if (!My)
	{
		return;
	}
	TArray<AActor*> Overlaps;
	AuraSphere->GetOverlappingActors(Overlaps, ACharacter::StaticClass());
	for (AActor* const Other : Overlaps)
	{
		if (!IsValid(Other) || Other == Owner)
		{
			continue;
		}
		UAbilitySystemComponent* const Tasc = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Other);
		if (!Tasc)
		{
			continue;
		}
		const bool bAlly = DFAura_IsAlly(Owner, Other);
		if (bAlly)
		{
			if (FriendlyEffect && Tasc->GetGameplayEffectCount(FriendlyEffect, My, true) < 1)
			{
				const FGameplayEffectContextHandle Ctx = My->MakeEffectContext();
				const FGameplayEffectSpecHandle S = My->MakeOutgoingSpec(FriendlyEffect, 1.f, Ctx);
				if (S.IsValid() && S.Data)
				{
					My->ApplyGameplayEffectSpecToTarget(*S.Data, Tasc);
				}
			}
		}
		else if (EnemyEffect && Tasc->GetGameplayEffectCount(EnemyEffect, My, true) < 1)
		{
			const FGameplayEffectContextHandle Ctx2 = My->MakeEffectContext();
			const FGameplayEffectSpecHandle S2 = My->MakeOutgoingSpec(EnemyEffect, 1.f, Ctx2);
			if (S2.IsValid() && S2.Data)
			{
				My->ApplyGameplayEffectSpecToTarget(*S2.Data, Tasc);
			}
		}
	}
}
