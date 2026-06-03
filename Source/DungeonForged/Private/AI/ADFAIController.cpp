// Source/DungeonForged/Private/AI/ADFAIController.cpp

#include "AI/ADFAIController.h"
#include "AI/UDFAILibrary.h"
#include "Characters/ADFEnemyBase.h"
#include "Combat/UDFCombatDirectorSubsystem.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/UDFAttributeSet.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"

namespace
{
bool IsDFPerceptionTargetAlive(AActor* const Actor)
{
	if (!IsValid(Actor))
	{
		return false;
	}
	UAbilitySystemComponent* const ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
	if (!ASC)
	{
		return true;
	}
	if (FDFGameplayTags::State_Dead.IsValid() && ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Dead))
	{
		return false;
	}
	const FGameplayAttribute HealthAttribute = UDFAttributeSet::GetHealthAttribute();
	return !HealthAttribute.IsValid() || ASC->GetNumericAttribute(HealthAttribute) > 0.f;
}

bool IsHostilePlayerPawn(AActor* const Actor)
{
	const APawn* const SensePawn = Cast<APawn>(Actor);
	return SensePawn && SensePawn->IsPlayerControlled() && IsDFPerceptionTargetAlive(Actor);
}
} // namespace

UBehaviorTreeComponent* ADFAIController::GetDFBehaviorTreeComponent() const
{
	return Cast<UBehaviorTreeComponent>(GetBrainComponent());
}

ADFAIController::ADFAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DFPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("DFPerception"));
	SetPerceptionComponent(*DFPerception);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("DFSightConfig"));
	if (IsValid(SightConfig))
	{
		SightConfig->SightRadius = 2500.f;
		SightConfig->LoseSightRadius = 3000.f;
		SightConfig->PeripheralVisionAngleDegrees = 70.f;
		SightConfig->AutoSuccessRangeFromLastSeenLocation = 500.f;
		SightConfig->DetectionByAffiliation.bDetectEnemies = true;
		SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
		DFPerception->ConfigureSense(*SightConfig);
	}

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("DFHearingConfig"));
	if (IsValid(HearingConfig))
	{
		HearingConfig->HearingRange = 2000.f;
		HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
		HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
		DFPerception->ConfigureSense(*HearingConfig);
	}

	DFPerception->SetDominantSense(UAISense_Sight::StaticClass());
}

void ADFAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	DFPerception->OnTargetPerceptionUpdated.AddDynamic(
		this, &ADFAIController::OnPerceptionUpdated);
	DFPerception->OnTargetPerceptionForgotten.AddDynamic(
		this, &ADFAIController::OnTargetPerceptionForgotten);

	UBlackboardComponent* const BB = GetBlackboardComponent();
	if (IsValid(BB) && InPawn)
	{
		BB->SetValueAsObject(DFAIKeys::TargetActor, nullptr);
		BB->SetValueAsBool(DFAIKeys::bCanSeeTarget, false);
		BB->SetValueAsBool(DFAIKeys::bCanTelegraph, true);
		BB->SetValueAsBool(DFAIKeys::bPrefersRangedCombat, false);
		BB->SetValueAsBool(DFAIKeys::bHasLastKnownTarget, false);
		BB->ClearValue(DFAIKeys::LastKnownTargetLocation);
		BB->SetValueAsEnum(DFAIKeys::CombatState, static_cast<uint8>(EADFAICombatState::Patrol));
	}

	if (const ADFEnemyBase* const E = Cast<ADFEnemyBase>(InPawn))
	{
		if (!E->IsDelayingAIForSpawnBirth())
		{
			if (UBehaviorTree* const Wood = E->GetAIBehaviorTreeAsset())
			{
				RunBehaviorTree(Wood);
			}
		}
	}
}

void ADFAIController::OnUnPossess()
{
	if (IsValid(DFPerception))
	{
		DFPerception->OnTargetPerceptionUpdated.RemoveDynamic(
			this, &ADFAIController::OnPerceptionUpdated);
		DFPerception->OnTargetPerceptionForgotten.RemoveDynamic(
			this, &ADFAIController::OnTargetPerceptionForgotten);
	}
	Super::OnUnPossess();
}

void ADFAIController::SetCombatState(const EADFAICombatState State)
{
	if (UBlackboardComponent* const BB = GetBlackboardComponent())
	{
		BB->SetValueAsEnum(DFAIKeys::CombatState, static_cast<uint8>(State));
	}
}

float ADFAIController::GetDistanceToTarget() const
{
	const UBlackboardComponent* const BB = GetBlackboardComponent();
	APawn* const Self = GetPawn();
	if (!IsValid(BB) || !IsValid(Self))
	{
		return -1.f;
	}
	AActor* const T = Cast<AActor>(const_cast<UBlackboardComponent*>(BB)->GetValueAsObject(DFAIKeys::TargetActor));
	if (!IsValid(T))
	{
		return -1.f;
	}
	return FVector::Dist(Self->GetActorLocation(), T->GetActorLocation());
}

void ADFAIController::ReceivePackAlert(
	AActor* const SuspectedTarget,
	const FVector LastKnownLocation,
	const bool bHeardOnly)
{
	UBlackboardComponent* const BB = GetBlackboardComponent();
	if (!IsValid(BB))
	{
		return;
	}
	const EADFAICombatState CurrentState = static_cast<EADFAICombatState>(BB->GetValueAsEnum(DFAIKeys::CombatState));
	if (CurrentState == EADFAICombatState::Flee || CurrentState == EADFAICombatState::Recover)
	{
		return;
	}
	if (BB->GetValueAsBool(DFAIKeys::bCanSeeTarget))
	{
		return;
	}
	BB->SetValueAsVector(DFAIKeys::LastKnownTargetLocation, LastKnownLocation);
	BB->SetValueAsVector(DFAIKeys::TargetLocation, LastKnownLocation);
	BB->SetValueAsBool(DFAIKeys::bHasLastKnownTarget, true);
	if (UDFAILibrary::IsValidHostilePlayerTarget(SuspectedTarget))
	{
		BB->SetValueAsObject(DFAIKeys::TargetActor, SuspectedTarget);
	}
	BB->SetValueAsBool(DFAIKeys::bCanSeeTarget, false);
	SetCombatState(bHeardOnly ? EADFAICombatState::Investigate : EADFAICombatState::Chase);
}

void ADFAIController::HandleHostilePerceived(AActor* const Actor, const FAIStimulus& Stimulus)
{
	UBlackboardComponent* const BB = GetBlackboardComponent();
	if (!IsValid(BB) || !IsValid(Actor))
	{
		return;
	}
	const FAISenseID HearingId = UAISense::GetSenseID<UAISense_Hearing>();
	const FAISenseID SightId = UAISense::GetSenseID<UAISense_Sight>();
	const FVector StimulusLocation = Stimulus.StimulusLocation.IsNearlyZero()
		? Actor->GetActorLocation()
		: Stimulus.StimulusLocation;
	const bool bHeardOnly = Stimulus.Type == HearingId;
	const bool bSawTarget = Stimulus.Type == SightId;

	BB->SetValueAsObject(DFAIKeys::TargetActor, Actor);
	BB->SetValueAsVector(DFAIKeys::LastKnownTargetLocation, StimulusLocation);
	BB->SetValueAsVector(DFAIKeys::TargetLocation, StimulusLocation);
	BB->SetValueAsBool(DFAIKeys::bHasLastKnownTarget, true);
	BB->SetValueAsBool(DFAIKeys::bCanSeeTarget, bSawTarget && !bHeardOnly);
	SetCombatState(bHeardOnly ? EADFAICombatState::Investigate : EADFAICombatState::Chase);

	if (UWorld* const World = GetWorld())
	{
		if (UDFCombatDirectorSubsystem* const Director = World->GetSubsystem<UDFCombatDirectorSubsystem>())
		{
			if (ADFEnemyBase* const SelfEnemy = Cast<ADFEnemyBase>(GetPawn()))
			{
				Director->PropagatePackAlert(SelfEnemy, Actor, StimulusLocation, Director->PackAlertRadiusCm);
			}
		}
	}
}

void ADFAIController::OnPerceptionUpdated(AActor* Actor, const FAIStimulus Stimulus)
{
	if (!Stimulus.WasSuccessfullySensed() || !IsHostilePlayerPawn(Actor))
	{
		return;
	}
	HandleHostilePerceived(Actor, Stimulus);
}

void ADFAIController::OnTargetPerceptionForgotten(AActor* Actor)
{
	UBlackboardComponent* const BB = GetBlackboardComponent();
	if (!IsValid(BB) || !IsValid(Actor))
	{
		return;
	}
	AActor* const Current = Cast<AActor>(BB->GetValueAsObject(DFAIKeys::TargetActor));
	if (Current != Actor)
	{
		return;
	}
	const FVector LastSeen = Actor->GetActorLocation();
	BB->SetValueAsVector(DFAIKeys::LastKnownTargetLocation, LastSeen);
	BB->SetValueAsVector(DFAIKeys::TargetLocation, LastSeen);
	BB->SetValueAsBool(DFAIKeys::bHasLastKnownTarget, true);
	BB->SetValueAsObject(DFAIKeys::TargetActor, nullptr);
	BB->SetValueAsBool(DFAIKeys::bCanSeeTarget, false);
	const EADFAICombatState CurrentState = static_cast<EADFAICombatState>(BB->GetValueAsEnum(DFAIKeys::CombatState));
	if (CurrentState != EADFAICombatState::Flee && CurrentState != EADFAICombatState::Recover)
	{
		SetCombatState(EADFAICombatState::Investigate);
	}
}
