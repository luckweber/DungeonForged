// Source/DungeonForged/Private/Characters/ADFEnemyBase.cpp

#include "Characters/ADFEnemyBase.h"
#include "AI/ADFAIController.h"
#include "ADFDungeonManager.h"
#include "Characters/ADFPlayerState.h"
#include "Data/DFDataTableStructs.h"
#include "DFLootGeneratorSubsystem.h"
#include "Progression/UDFLevelingComponent.h"
#include "GAS/UDFAttributeSet.h"
#include "Combat/UDFHitReactionComponent.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/GameInstance.h"
#include "Run/DFRunManager.h"
#include "GameplayEffectTypes.h"
#include "Perception/AIPerceptionComponent.h"
#include "TimerManager.h"

namespace
{
ADFPlayerState* ResolveKillerPlayerState(AActor* const Killer)
{
	if (!Killer)
	{
		return nullptr;
	}
	if (APlayerState* const PS = Cast<APlayerState>(Killer))
	{
		return Cast<ADFPlayerState>(PS);
	}
	if (const APawn* const P = Cast<APawn>(Killer))
	{
		return P->GetPlayerState<ADFPlayerState>();
	}
	if (const AController* const C = Cast<AController>(Killer))
	{
		return C->GetPlayerState<ADFPlayerState>();
	}
	return nullptr;
}
} // namespace

ADFEnemyBase::ADFEnemyBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = ADFAIController::StaticClass();

	TeamId = FGenericTeamId(DefaultEnemyTeamId);

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UDFAttributeSet>(TEXT("AttributeSet"));

	HitReaction = CreateDefaultSubobject<UDFHitReactionComponent>(TEXT("HitReaction"));

	HealthBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
	// USceneComponent default subobjects must be attached in the constructor or CDO / Blueprint reinstancing can crash.
	HealthBar->SetupAttachment(GetRootComponent());
	HealthBar->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBar->SetDrawAtDesiredSize(true);
}

void ADFEnemyBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	// Re-attach to skeletal mesh when available (BP may have swapped the mesh; ctor only used capsule root).
	if (HealthBar && GetMesh())
	{
		HealthBar->AttachToComponent(
			GetMesh(), FAttachmentTransformRules::KeepRelativeTransform);
		HealthBar->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	}
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

void ADFEnemyBase::NotifyHealthChangedFromAttributes(float /*Current*/, float /*Max*/)
{
}

void ADFEnemyBase::OnHealthOrMaxChanged(float Current, float Max)
{
	NotifyHealthChangedFromAttributes(Current, Max);
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
		AActor* const Killer = LastDamageAttacker.IsValid()
			? LastDamageAttacker.Get()
			: (GetInstigator() ? GetInstigator() : nullptr);
		HandleServerDeath(Killer);
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
	CachedGoldDropMin = Row->GoldDropMin;
	CachedGoldDropMax = Row->GoldDropMax;
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
	ApplyAIConfigFromRow(*Row);
	LastDamageAttacker.Reset();

	if (HasActorBegunPlay())
	{
		InitAbilityAndBindHealth();
	}
}

void ADFEnemyBase::ApplyAIConfigFromRow(const FDFEnemyTableRow& Row)
{
	MeleeRange = FMath::Max(0.f, Row.MeleeRange);
	RangedRange = FMath::Max(0.f, Row.RangedRange);
	AttackRange = FMath::Max(0.f, Row.AttackRange);
	if (Row.PatrolPathPoints.Num() > 0)
	{
		PatrolPoints = Row.PatrolPathPoints;
	}
	if (Row.TauntMontages.Num() > 0)
	{
		TauntMontages = Row.TauntMontages;
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

UBlackboardComponent* ADFEnemyBase::GetBehaviorTreeBlackboard() const
{
	AAIController* const AI = Cast<AAIController>(GetController());
	return AI ? AI->GetBlackboardComponent() : nullptr;
}

void ADFEnemyBase::RegisterDamageFromContext(const FGameplayEffectContextHandle& Ctx)
{
	if (!HasAuthority() || !Ctx.IsValid())
	{
		return;
	}
	AActor* K = Ctx.GetEffectCauser();
	if (!K)
	{
		K = Ctx.GetInstigator();
	}
	if (K)
	{
		LastDamageAttacker = K;
	}
}

void ADFEnemyBase::HandleServerDeath(AActor* Killer)
{
	if (bHasDied)
	{
		return;
	}
	bHasDied = true;
	if (HasAuthority())
	{
		if (ADFPlayerState* const PState = ResolveKillerPlayerState(Killer))
		{
			if (CachedExperienceReward > 0.f)
			{
				if (UDFLevelingComponent* const Lv = PState->GetLevelingComponent())
				{
					int32 Floor = 0;
					if (UWorld* const W = GetWorld())
					{
						if (UGameInstance* const GI = W->GetGameInstance())
						{
							if (UDFDungeonManager* const Dm = GI->GetSubsystem<UDFDungeonManager>())
							{
								Floor = Dm->CurrentFloor;
							}
						}
					}
					const int32 XpAward = FMath::RoundToInt(
						CachedExperienceReward * (1.f + 0.1f * static_cast<float>(Floor)));
					if (XpAward > 0)
					{
						Lv->AddXP(XpAward);
					}
				}
			}
			if (CachedGoldDropMax > 0)
			{
				if (UWorld* const Wg = GetWorld())
				{
					if (UGameInstance* const GI = Wg->GetGameInstance())
					{
						if (UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>())
						{
							const int32 G = FMath::RandRange(
								FMath::Min(CachedGoldDropMin, CachedGoldDropMax),
								FMath::Max(CachedGoldDropMin, CachedGoldDropMax));
							if (G > 0)
							{
								RM->AddRunGold(G);
							}
						}
					}
				}
			}
		}
	}
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
	if (AAIController* const AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
		AI->ClearFocus(EAIFocusPriority::Gameplay);
		if (UBrainComponent* const Brain = AI->GetBrainComponent())
		{
			Brain->StopLogic(TEXT("Death"));
		}
		if (ADFAIController* const DFAI = Cast<ADFAIController>(AI))
		{
			if (UAIPerceptionComponent* const P = DFAI->GetDFPerception())
			{
				P->Deactivate();
			}
		}
	}
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
