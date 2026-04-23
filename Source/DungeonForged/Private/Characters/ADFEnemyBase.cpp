// Source/DungeonForged/Private/Characters/ADFEnemyBase.cpp

#include "Characters/ADFEnemyBase.h"
#include "Data/DFDataTableStructs.h"
#include "DFLootGeneratorSubsystem.h"
#include "GAS/UDFAttributeSet.h"
#include "Combat/UDFHitReactionComponent.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffectTypes.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "TimerManager.h"

ADFEnemyBase::ADFEnemyBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	TeamId = FGenericTeamId(DefaultEnemyTeamId);

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UDFAttributeSet>(TEXT("AttributeSet"));

	HitReaction = CreateDefaultSubobject<UDFHitReactionComponent>(TEXT("HitReaction"));

	BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));

	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	UAISenseConfig_Sight* const Sight = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	if (Sight)
	{
		Sight->SightRadius = 2000.f;
		Sight->LoseSightRadius = 2500.f;
		Sight->PeripheralVisionAngleDegrees = 60.f;
		Sight->DetectionByAffiliation.bDetectEnemies = true;
		Sight->DetectionByAffiliation.bDetectNeutrals = true;
		Sight->DetectionByAffiliation.bDetectFriendlies = false;
	}
	AIPerception->ConfigureSense(*Sight);
	AIPerception->SetDominantSense(UAISense_Sight::StaticClass());

	HealthBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
	HealthBar->SetupAttachment(GetMesh());
	HealthBar->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBar->SetDrawAtDesiredSize(true);
	HealthBar->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
}

UAbilitySystemComponent* ADFEnemyBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

FGenericTeamId ADFEnemyBase::GetGenericTeamId() const
{
	return TeamId;
}

void ADFEnemyBase::SetGenericTeamId(const FGenericTeamId& InTeamId)
{
	TeamId = InTeamId;
}

ETeamAttitude::Type ADFEnemyBase::GetTeamAttitudeTowards(const AActor& Other) const
{
	const IGenericTeamAgentInterface* O = Cast<IGenericTeamAgentInterface>(&Other);
	if (!O)
	{
		return ETeamAttitude::Neutral;
	}
	const FGenericTeamId OtherId = O->GetGenericTeamId();
	if (OtherId == FGenericTeamId::NoTeam)
	{
		return ETeamAttitude::Neutral;
	}
	// Allies: same team id
	if (GetGenericTeamId() == OtherId)
	{
		return ETeamAttitude::Friendly;
	}
	// Player / typical heroes
	if (OtherId.GetId() == DefaultPlayerTeamId)
	{
		return ETeamAttitude::Hostile;
	}
	return ETeamAttitude::Neutral;
}

void ADFEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	InitAbilityAndBindHealth();
	StartBehaviorIfReady();

	if (HealthBar && HealthBarWidgetClass)
	{
		HealthBar->SetWidgetClass(HealthBarWidgetClass);
		HealthBar->InitWidget();
	}
}

void ADFEnemyBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindAttributeDelegates();
	Super::EndPlay(EndPlayReason);
}

void ADFEnemyBase::InitAbilityAndBindHealth()
{
	if (bAttributeDelegatesBound || !AttributeSet)
	{
		return;
	}
	AttributeSet->OnHealthChanged.AddUObject(this, &ADFEnemyBase::OnHealthOrMaxChanged);
	bAttributeDelegatesBound = true;
}

void ADFEnemyBase::UnbindAttributeDelegates()
{
	if (AttributeSet && bAttributeDelegatesBound)
	{
		AttributeSet->OnHealthChanged.RemoveAll(this);
	}
	bAttributeDelegatesBound = false;
}

void ADFEnemyBase::OnHealthOrMaxChanged(float Current, float /*Max*/)
{
	if (bHasDied)
	{
		return;
	}
	if (Current > 0.f)
	{
		return;
	}
	if (HasAuthority())
	{
		HandleServerDeath(GetInstigator() ? GetInstigator() : nullptr);
	}
}

void ADFEnemyBase::InitializeFromDataTable(UDataTable* EnemyTable, FName RowName)
{
	if (!HasAuthority() || !EnemyTable || RowName.IsNone() || !AbilitySystemComponent)
	{
		return;
	}

	const FDFEnemyTableRow* Row = EnemyTable->FindRow<FDFEnemyTableRow>(RowName, TEXT("ADFEnemyBase::InitializeFromDataTable"), false);
	if (!Row)
	{
		return;
	}

	CachedExperienceReward = Row->ExperienceReward;
	CachedLootTableRowNames = Row->LootTableRows;
	CachedAIBehaviorTree = Row->AIBehaviorTree;

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	ApplyBaseStatsFromRow(*Row);

	if (OptionalInitGameplayEffect)
	{
		FGameplayEffectContextHandle Ctx = AbilitySystemComponent->MakeEffectContext();
		Ctx.AddSourceObject(this);
		const FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(
			OptionalInitGameplayEffect, 1.f, Ctx);
		if (Spec.IsValid() && Spec.Data)
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	GrantAbilitiesForRow(*Row);

	if (HasActorBegunPlay())
	{
		InitAbilityAndBindHealth();
		StartBehaviorIfReady();
	}
}

void ADFEnemyBase::ApplyBaseStatsFromRow(const FDFEnemyTableRow& Row)
{
	if (!AbilitySystemComponent)
	{
		return;
	}
	UDFAttributeSet* const S = const_cast<UDFAttributeSet*>(AbilitySystemComponent->GetSet<UDFAttributeSet>());
	if (!S)
	{
		return;
	}
	const float Hp = FMath::Max(1.f, Row.BaseHealth);
	S->SetMaxHealth(Hp);
	S->SetHealth(Hp);
	S->SetArmor(Row.BaseArmor);
	S->SetStrength(FMath::Max(0.f, Row.BaseDamage));
}

void ADFEnemyBase::GrantAbilitiesForRow(const FDFEnemyTableRow& Row)
{
	if (!AbilitySystemComponent)
	{
		return;
	}
	for (const FGameplayTag& Tag : Row.GrantedAbilities)
	{
		if (!Tag.IsValid())
		{
			continue;
		}
		if (const TSubclassOf<UGameplayAbility>* AbClass = GrantedAbilitiesByTag.Find(Tag))
		{
			if (*AbClass)
			{
				const FGameplayAbilitySpec Spec(*AbClass, 1, INDEX_NONE, this);
				AbilitySystemComponent->GiveAbility(Spec);
			}
		}
	}
}

void ADFEnemyBase::StartBehaviorIfReady()
{
	if (!BehaviorTreeComponent || !CachedAIBehaviorTree)
	{
		return;
	}
	BehaviorTreeComponent->StartTree(*CachedAIBehaviorTree, EBTExecutionMode::Looped);
}

UBlackboardComponent* ADFEnemyBase::GetBehaviorTreeBlackboard() const
{
	if (!BehaviorTreeComponent)
	{
		return nullptr;
	}
	// UBrainComponent
	return BehaviorTreeComponent->GetBlackboardComponent();
}

void ADFEnemyBase::HandleServerDeath(AActor* Killer)
{
	if (bHasDied)
	{
		return;
	}
	bHasDied = true;
	OnEnemyDied.Broadcast(this, Killer, CachedExperienceReward);
	SpawnDeathLoot();
	MulticastOnDeath(Killer);
	DisableEnemyActions();
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(DeathDestroyTimer, this, &ADFEnemyBase::OnDestroyAfterDeath, 3.f, false);
	}
}

void ADFEnemyBase::MulticastOnDeath_Implementation(AActor* /*Killer*/)
{
	if (UAnimInstance* const Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (DeathMontage)
		{
			Anim->Montage_Play(DeathMontage, 1.f);
		}
	}
	if (HealthBar)
	{
		HealthBar->SetVisibility(false);
	}
}

void ADFEnemyBase::SpawnDeathLoot_Implementation()
{
	if (GetNetMode() == NM_Client)
	{
		return;
	}
	if (CachedLootTableRowNames.IsEmpty())
	{
		return;
	}
	if (UWorld* const W = GetWorld())
	{
		if (UDFLootGeneratorSubsystem* const LootSys = W->GetSubsystem<UDFLootGeneratorSubsystem>())
		{
			FDFEnemyTableRow Row;
			Row.LootTableRows = CachedLootTableRowNames;
			LootSys->RollLoot(Row, GetActorLocation() + FVector(0.f, 0.f, 20.f), FVector::ZeroVector);
		}
	}
}

void ADFEnemyBase::OnDestroyAfterDeath()
{
	Destroy();
}

void ADFEnemyBase::DisableEnemyActions()
{
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->DisableMovement();
	}
	if (AIPerception)
	{
		AIPerception->Deactivate();
	}
	if (BehaviorTreeComponent)
	{
		BehaviorTreeComponent->StopTree(EBTStopMode::Forced);
	}
	if (AAIController* const AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
		AI->ClearFocus(EAIFocusPriority::Gameplay);
		if (UBrainComponent* const Brain = AI->GetBrainComponent())
		{
			Brain->StopLogic(TEXT("Death"));
		}
	}
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
