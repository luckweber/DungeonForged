// Source/DungeonForged/Private/Combat/ADFSmokeBombActor.cpp
#include "Combat/ADFSmokeBombActor.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Effects/UGE_Buff_SmokeCover.h"
#include "GAS/Effects/UGE_Debuff_Blind.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AIController.h"
#include "AI/DFAIKeys.h"
#include "Characters/ADFEnemyBase.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"
#include "CollisionQueryParams.h"
#include "WorldCollision.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

ADFSmokeBombActor::ADFSmokeBombActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Overlap = CreateDefaultSubobject<USphereComponent>(TEXT("Overlap"));
	Overlap->SetSphereRadius(300.f);
	Overlap->SetCollisionProfileName(TEXT("OverlapAll"));
	Overlap->SetGenerateOverlapEvents(true);
	RootComponent = Overlap;
	SmokeVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SmokeVFX"));
	SmokeVFX->SetupAttachment(Overlap);
	BlindDebuff = UGE_Debuff_Blind::StaticClass();
	PlayerSmokeCover = UGE_Buff_SmokeCover::StaticClass();
}

void ADFSmokeBombActor::BeginPlay()
{
	Super::BeginPlay();
	Overlap->SetSphereRadius(SmokeRadius);
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			TimerHandle, this, &ADFSmokeBombActor::TickDamage, TickInterval, true, 0.05f);
		FTimerHandle End;
		W->GetTimerManager().SetTimer(End, FTimerDelegate::CreateWeakLambda(
				this, [this] { Destroy(); }),
			LifetimeSeconds, false);
	}
}

void ADFSmokeBombActor::TickDamage()
{
	if (!GetWorld() || !HasAuthority() || !BlindDebuff || !PlayerSmokeCover)
	{
		return;
	}
	APawn* const Inst = GetInstigator();
	UAbilitySystemComponent* const SourceASC = Inst
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Inst)
		: nullptr;
	if (!SourceASC)
	{
		return;
	}
	const FVector C = GetActorLocation();
	FCollisionObjectQueryParams Obj(ECC_Pawn);
	Obj.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams Q(SCENE_QUERY_STAT(SmokeTick), false, this);
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, C, FQuat::Identity, Obj, FCollisionShape::MakeSphere(SmokeRadius), Q);
	for (const FOverlapResult& R : Overlaps)
	{
		AActor* A = R.GetActor();
		if (!IsValid(A) || A == this)
		{
			continue;
		}
		// Blinded enemies
		if (Cast<ADFEnemyBase>(A))
		{
			if (UAbilitySystemComponent* Tgt = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(A))
			{
				FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
				Ctx.AddSourceObject(this);
				const FGameplayEffectSpecHandle S = SourceASC->MakeOutgoingSpec(BlindDebuff, 1.f, Ctx);
				if (S.IsValid() && S.Data && FDFGameplayTags::Data_Duration.IsValid())
				{
					S.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, 0.5f);
					SourceASC->ApplyGameplayEffectSpecToTarget(*S.Data, Tgt);
				}
			}
			if (APawn* const Pn = Cast<APawn>(A))
			{
				if (AAIController* AC = Cast<AAIController>(Pn->GetController()))
				{
					if (UBlackboardComponent* BB = AC->GetBlackboardComponent())
					{
						BB->SetValueAsBool(DFAIKeys::bCanSeeTarget, false);
					}
				}
			}
		}
		// Player in smoke: conceal
		if (A->IsA<ADFPlayerCharacter>())
		{
			if (UAbilitySystemComponent* Pasc = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(A))
			{
				FGameplayEffectContextHandle CtxP = Pasc->MakeEffectContext();
				CtxP.AddSourceObject(this);
				const FGameplayEffectSpecHandle Sp = Pasc->MakeOutgoingSpec(PlayerSmokeCover, 1.f, CtxP);
				if (Sp.IsValid() && Sp.Data && FDFGameplayTags::Data_Duration.IsValid())
				{
					Sp.Data->SetSetByCallerMagnitude(FDFGameplayTags::Data_Duration, TickInterval * 1.5f);
					Pasc->ApplyGameplayEffectSpecToSelf(*Sp.Data);
				}
			}
		}
	}
}
