// Source/DungeonForged/Private/AI/ADFAIController.cpp

#include "AI/ADFAIController.h"
#include "Characters/ADFEnemyBase.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"

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
		BB->SetValueAsEnum(DFAIKeys::CombatState, static_cast<uint8>(EADFAICombatState::Patrol));
	}

	if (const ADFEnemyBase* const E = Cast<ADFEnemyBase>(InPawn))
	{
		if (UBehaviorTree* const Wood = E->GetAIBehaviorTreeAsset())
		{
			RunBehaviorTree(Wood);
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

void ADFAIController::OnPerceptionUpdated(AActor* Actor, const FAIStimulus Stimulus)
{
	UBlackboardComponent* const BB = GetBlackboardComponent();
	if (!IsValid(BB) || !IsValid(Actor))
	{
		return;
	}
	if (!Stimulus.WasSuccessfullySensed())
	{
		return;
	}
	// Only chase player-controlled pawns (sight/hearing can both deliver updates).
	const APawn* const SensePawn = Cast<APawn>(Actor);
	if (SensePawn == nullptr || !SensePawn->IsPlayerControlled())
	{
		return;
	}
	BB->SetValueAsObject(DFAIKeys::TargetActor, Actor);
	BB->SetValueAsBool(DFAIKeys::bCanSeeTarget, true);
	BB->SetValueAsVector(DFAIKeys::TargetLocation, Actor->GetActorLocation());
	SetCombatState(EADFAICombatState::Chase);
}

void ADFAIController::OnTargetPerceptionForgotten(AActor* Actor)
{
	UBlackboardComponent* const BB = GetBlackboardComponent();
	if (!IsValid(BB) || !IsValid(Actor))
	{
		return;
	}
	AActor* const Current = Cast<AActor>(BB->GetValueAsObject(DFAIKeys::TargetActor));
	if (Current == Actor)
	{
		BB->SetValueAsObject(DFAIKeys::TargetActor, nullptr);
		BB->SetValueAsBool(DFAIKeys::bCanSeeTarget, false);
		SetCombatState(EADFAICombatState::Patrol);
	}
}
