// Source/DungeonForged/Private/Characters/ADFEnemyBase.cpp

#include "Characters/ADFEnemyBase.h"
#include "Boss/ADFBossBase.h"
#include "UI/UDFEnemyHealthBarWidget.h"
#include "Combat/UDFCombatDirectorSubsystem.h"
#include "Animation/DFDeathAnimation.h"
#include "DungeonForgedModule.h"
#include "GAS/DFGameplayTags.h"
#include "GAS/Abilities/UDFAbility_Enemy_Death.h"
#include "GAS/Effects/UGE_EnemyDeath.h"
#include "Abilities/GameplayAbility.h"
#include "AI/DFAIKeys.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "GameplayEffectTypes.h"
#include "AI/ADFAIController.h"
#include "ADFDungeonManager.h"
#include "Audio/UDFMusicManagerSubsystem.h"
#include "Characters/ADFPlayerState.h"
#include "Data/DFDataTableStructs.h"
#include "DFLootGeneratorSubsystem.h"
#include "Progression/UDFLevelingComponent.h"
#include "GAS/UDFAttributeSet.h"
#include "Combat/UDFHitReactionComponent.h"
#include "Combat/UDFMeleeAimComponent.h"
#include "Combat/UDFStaggerComponent.h"
#include "GAS/Elemental/UDFElementalComponent.h"
#include "MotionWarpingComponent.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveFloat.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Run/DFRunManager.h"
#include "GameplayEffectTypes.h"
#include "Perception/AIPerceptionComponent.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UI/Status/UDFEnemyDebuffStatusBarWidget.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogDFEnemy, Log, All);

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
	MeleeAim = CreateDefaultSubobject<UDFMeleeAimComponent>(TEXT("MeleeAim"));
	MotionWarping = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));
	Stagger = CreateDefaultSubobject<UDFStaggerComponent>(TEXT("Stagger"));

	ElementalComponent = CreateDefaultSubobject<UDFElementalComponent>(TEXT("ElementalComponent"));

	HealthBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
	// USceneComponent default subobjects must be attached in the constructor or CDO / Blueprint reinstancing can crash.
	if (USkeletalMeshComponent* const SkelMesh = GetMesh())
	{
		HealthBar->SetupAttachment(SkelMesh);
	}
	else
	{
		HealthBar->SetupAttachment(GetRootComponent());
	}
	HealthBar->SetRelativeLocation(HealthBarRelativeOffset);
	HealthBar->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBar->SetDrawAtDesiredSize(true);

	DebuffStatusBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("DebuffStatusBar"));
	DebuffStatusBar->SetupAttachment(HealthBar);
	DebuffStatusBar->SetWidgetSpace(EWidgetSpace::Screen);
	DebuffStatusBar->SetDrawAtDesiredSize(true);
	DebuffStatusBar->SetRelativeLocation(FVector(0.f, 0.f, 12.f));

	DeathGameplayEffectClass = UGE_EnemyDeath::StaticClass();
	DeathAbilityClass = UUDFAbility_Enemy_Death::StaticClass();

	DeathDissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DeathDissolveTimeline"));
}

void ADFEnemyBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	ApplyHealthBarAttachment();
}

void ADFEnemyBase::ApplyHealthBarAttachment()
{
	if (!HealthBar)
	{
		return;
	}
	if (!bAttachHealthBarToMesh)
	{
		return;
	}
	USkeletalMeshComponent* const InMesh = GetMesh();
	if (!InMesh)
	{
		return;
	}
	if (!HealthBarAttachSocketName.IsNone() && InMesh->DoesSocketExist(HealthBarAttachSocketName))
	{
		HealthBar->AttachToComponent(
			InMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, HealthBarAttachSocketName);
		return;
	}
	if (HealthBar->GetAttachParent() == InMesh)
	{
		return;
	}
	HealthBar->AttachToComponent(InMesh, FAttachmentTransformRules::KeepRelativeTransform);
}

UAbilitySystemComponent* ADFEnemyBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ADFEnemyBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADFEnemyBase, bHasDied);
	DOREPLIFETIME(ADFEnemyBase, ReplicatedDataTableMaxWalkSpeed);
	DOREPLIFETIME(ADFEnemyBase, EnemyDisplayName);
}

void ADFEnemyBase::OnRep_bHasDied()
{
	if (bHasDied && HealthBar)
	{
		HealthBar->SetVisibility(false);
	}
}

void ADFEnemyBase::OnRep_EnemyDisplayName()
{
	RefreshEnemyHealthBarWidget();
}

void ADFEnemyBase::RefreshEnemyHealthBarWidget()
{
	if (!HealthBar)
	{
		return;
	}

	// Accept class from C++ defaults *or* Widget Component "Widget Class" set only on the BP component.
	TSubclassOf<UUserWidget> WidgetClass = HealthBarWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = HealthBar->GetWidgetClass();
	}
	if (!WidgetClass)
	{
		return;
	}

	if (HealthBar->GetWidgetClass() != WidgetClass)
	{
		HealthBar->SetWidgetClass(WidgetClass);
	}

	if (!HealthBar->GetUserWidgetObject())
	{
		HealthBar->InitWidget();
	}

	UUserWidget* const WidgetObject = HealthBar->GetUserWidgetObject();
	if (UDFEnemyHealthBarWidget* const HBar = Cast<UDFEnemyHealthBarWidget>(WidgetObject))
	{
		HBar->SetupObservedEnemy(this, EnemyDisplayName);
		return;
	}
	UE_LOG(
		LogDFEnemy, Warning,
		TEXT("%s: health bar widget '%s' must inherit UDFEnemyHealthBarWidget (ProgressBar: EnemyHealthBar). Set Health Bar Widget Class on the enemy BP or Widget Class on the HealthBar component."),
		*GetName(),
		*GetNameSafe(WidgetClass));
}

void ADFEnemyBase::OnRep_ReplicatedDataTableMaxWalkSpeed()
{
	if (UCharacterMovementComponent* const Move = GetCharacterMovement())
	{
		if (ReplicatedDataTableMaxWalkSpeed > 0.f)
		{
			Move->MaxWalkSpeed = ReplicatedDataTableMaxWalkSpeed;
		}
	}
}

void ADFEnemyBase::Multicast_PlayEnemyCosmeticCue_Implementation(
	USoundBase* const Sound,
	UNiagaraSystem* const VFX,
	const FName AttachSocketName,
	const FVector_NetQuantize Location,
	const FRotator Rotation,
	const FVector_NetQuantize100 Scale,
	const bool bAttachToMesh)
{
	if (IsRunningDedicatedServer() || (!Sound && !VFX))
	{
		return;
	}

	const FVector CueScale(FMath::Max(Scale.X, 0.01f), FMath::Max(Scale.Y, 0.01f), FMath::Max(Scale.Z, 0.01f));
	if (bAttachToMesh && GetMesh())
	{
		if (Sound)
		{
			UGameplayStatics::SpawnSoundAttached(
				Sound,
				GetMesh(),
				AttachSocketName,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget,
				true);
		}
		if (VFX)
		{
			UNiagaraComponent* const Spawned = UNiagaraFunctionLibrary::SpawnSystemAttached(
				VFX,
				GetMesh(),
				AttachSocketName,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget,
				true,
				true,
				ENCPoolMethod::AutoRelease,
				true);
			if (Spawned)
			{
				Spawned->SetWorldScale3D(CueScale);
			}
		}
		return;
	}

	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, Location, Rotation);
	}
	if (VFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			VFX,
			Location,
			Rotation,
			CueScale,
			true,
			true,
			ENCPoolMethod::AutoRelease,
			true);
	}
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

	RefreshEnemyHealthBarWidget();
	if (HealthBar && !HealthBarWidgetClass)
	{
		UE_LOG(
			LogDFEnemy, Verbose,
			TEXT("%s: HealthBarWidgetClass not set — assign UDFEnemyHealthBarWidget (boss HUD uses UDFBossHealthBarWidget)."),
			*GetName());
	}
	if (DebuffStatusBar && DebuffStatusBarWidgetClass)
	{
		DebuffStatusBar->SetWidgetClass(DebuffStatusBarWidgetClass);
		DebuffStatusBar->InitWidget();
		if (UDFEnemyDebuffStatusBarWidget* const DBar = Cast<UDFEnemyDebuffStatusBarWidget>(DebuffStatusBar->GetUserWidgetObject()))
		{
			UAbilitySystemComponent* LocalAsc = nullptr;
			if (UWorld* const W = GetWorld())
			{
				if (APlayerController* const PC = W->GetFirstPlayerController())
				{
					if (ADFPlayerState* const PS = PC->GetPlayerState<ADFPlayerState>())
					{
						LocalAsc = PS->GetAbilitySystemComponent();
					}
				}
			}
			DBar->SetupObservedEnemy(this, EnemyDebuffStatusLibrary, LocalAsc);
		}
	}
	SetupDeathDissolveTimeline();

	// Manual spawn paths without InitializeFromDataTable: arm only if health is already valid.
	if (AttributeSet && AttributeSet->GetHealth() > 0.f)
	{
		bDeathDetectionArmed = true;
	}
	if (HasAuthority())
	{
		if (UWorld* const World = GetWorld())
		{
			if (UDFCombatDirectorSubsystem* const Director = World->GetSubsystem<UDFCombatDirectorSubsystem>())
			{
				Director->RegisterEnemy(this);
			}
		}
	}
}

void ADFEnemyBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bDeathFlowActive = false;
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(SpawnBirthBTDelayTimer);
		W->GetTimerManager().ClearTimer(DeathDestroyTimer);
		W->GetTimerManager().ClearTimer(DeathPoseLockTimer);
		W->GetTimerManager().ClearTimer(PostDeathCleanupTimer);
		W->GetTimerManager().ClearTimer(FallbackDeathTimer);
	}
	if (DeathDissolveTimeline)
	{
		DeathDissolveTimeline->Stop();
	}
	UnbindAttributeDelegates();
	if (HasAuthority())
	{
		if (UWorld* const World = GetWorld())
		{
			if (UDFCombatDirectorSubsystem* const Director = World->GetSubsystem<UDFCombatDirectorSubsystem>())
			{
				Director->UnregisterEnemy(this);
			}
		}
	}
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
	GrantDeathAbility();
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

	if (!bDeathDetectionArmed)
	{
		return;
	}
	if (bHasDied || bDeathFlowActive)
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

	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(SpawnBirthBTDelayTimer);
	}
	bDeferAIForSpawnBirth = false;

	const bool bPlaySpawnBirth = Row->SpawnBirthMontage != nullptr;
	const bool bDeferBT = bPlaySpawnBirth && Row->bDelayAIUntilSpawnBirthMontageFinishes;

	bDeathDetectionArmed = false;
	bDeathLootSpawned = false;
	bDeathPresentationFinalized = false;

	CachedExperienceReward = Row->ExperienceReward;
	CachedGoldDropMin = Row->GoldDropMin;
	CachedGoldDropMax = Row->GoldDropMax;
	CachedLootTableRowNames = Row->LootTableRows;
	CachedAIBehaviorTree = Row->AIBehaviorTree;

	if (!Row->EnemyName.IsEmpty())
	{
		EnemyDisplayName = Row->EnemyName;
	}
	if (ADFBossBase* const Boss = Cast<ADFBossBase>(this))
	{
		if (!Row->EnemyName.IsEmpty())
		{
			Boss->BossDisplayName = Row->EnemyName;
		}
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	ApplyBaseStatsFromRow(*Row);
	ApplyMovementConfigFromRow(*Row);

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
	if (ElementalComponent && !Row->ElementalAffinityRowName.IsNone())
	{
		UDataTable* const ElemTable = Row->ElementalAffinityTableOverride
			? Row->ElementalAffinityTableOverride
			: DefaultElementalAffinityTable;
		if (ElemTable)
		{
			ElementalComponent->InitFromTable(ElemTable, Row->ElementalAffinityRowName);
		}
	}
	LastDamageAttacker.Reset();

	if (HasActorBegunPlay())
	{
		InitAbilityAndBindHealth();
		RefreshEnemyHealthBarWidget();
	}

	// Base stats applied — safe to react to Health <= 0.
	bDeathDetectionArmed = true;

	if (bPlaySpawnBirth)
	{
		Multicast_PlaySpawnBirthMontage(Row->SpawnBirthMontage.Get());
	}

	if (bDeferBT)
	{
		bDeferAIForSpawnBirth = true;
		const float Dur = FMath::Max(0.05f, Row->SpawnBirthMontage->GetPlayLength());
		if (UWorld* const W = GetWorld())
		{
			W->GetTimerManager().SetTimer(SpawnBirthBTDelayTimer, this, &ADFEnemyBase::OnSpawnBirthMontageDelayElapsed, Dur, false);
		}
		else
		{
			bDeferAIForSpawnBirth = false;
			TryStartBehaviorTreeFromCache();
		}
	}
	else
	{
		TryStartBehaviorTreeFromCache();
	}
}

void ADFEnemyBase::TryStartBehaviorTreeFromCache()
{
	if (!CachedAIBehaviorTree)
	{
		return;
	}
	if (AAIController* const AIC = Cast<AAIController>(GetController()))
	{
		AIC->RunBehaviorTree(CachedAIBehaviorTree);
	}
}

void ADFEnemyBase::Multicast_PlaySpawnBirthMontage_Implementation(UAnimMontage* Montage)
{
	if (!Montage)
	{
		return;
	}
	USkeletalMeshComponent* const InMesh = GetMesh();
	if (!InMesh)
	{
		return;
	}
	UAnimInstance* const AnimInst = InMesh->GetAnimInstance();
	if (!AnimInst)
	{
		return;
	}
	AnimInst->Montage_Play(Montage, 1.f);
}

void ADFEnemyBase::OnSpawnBirthMontageDelayElapsed()
{
	if (!HasAuthority())
	{
		return;
	}
	bDeferAIForSpawnBirth = false;
	TryStartBehaviorTreeFromCache();
}

void ADFEnemyBase::ApplyMovementConfigFromRow(const FDFEnemyTableRow& Row)
{
	if (Row.MaxWalkSpeed <= 0.f)
	{
		return;
	}
	UCharacterMovementComponent* const Move = GetCharacterMovement();
	if (!Move)
	{
		return;
	}
	Move->MaxWalkSpeed = Row.MaxWalkSpeed;
	if (HasAuthority())
	{
		ReplicatedDataTableMaxWalkSpeed = Row.MaxWalkSpeed;
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

	CachedEnemyTier = Row.Tier;
	CachedEnemyArchetype = Row.Archetype;

	int32 Floor = 0;
	float DifficultyMultiplier = 1.f;
	if (UWorld* const W = GetWorld())
	{
		if (UGameInstance* const GI = W->GetGameInstance())
		{
			if (UDFDungeonManager* const Dm = GI->GetSubsystem<UDFDungeonManager>())
			{
				Floor = FMath::Max(0, Dm->CurrentFloor);
				DifficultyMultiplier = Dm->GetCurrentFloorDifficultyMultiplier();
			}
		}
	}
	const float FloorScale = 1.f + 0.15f * static_cast<float>(Floor);
	const float ScaleFinal = FloorScale * DifficultyMultiplier;

	float Hp = FMath::Max(1.f, Row.BaseHealth * ScaleFinal);
	float Armor = Row.BaseArmor * FMath::Sqrt(ScaleFinal);
	float Damage = FMath::Max(0.f, Row.BaseDamage * ScaleFinal);
	if (Row.Tier == EEnemyTier::Elite)
	{
		Hp *= 2.5f;
		Damage *= 1.5f;
	}

	S->SetMaxHealth(Hp);
	S->SetHealth(Hp);
	S->SetArmor(Armor);
	S->SetStrength(Damage);
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
	if (K && K != GetOwner())
	{
		LastDamageAttacker = K;
	}
	if (CachedEnemyTier == EEnemyTier::Elite && !bEliteMusicNotified && K)
	{
		if (Cast<APawn>(K) && Cast<APawn>(K)->IsPlayerControlled())
		{
			bEliteMusicNotified = true;
			Multicast_NotifyEliteEngaged();
		}
	}
}

void ADFEnemyBase::Multicast_NotifyEliteEngaged_Implementation()
{
	if (IsRunningDedicatedServer())
	{
		return;
	}
	if (UWorld* const W = GetWorld())
	{
		if (UDFMusicManagerSubsystem* const Music = W->GetSubsystem<UDFMusicManagerSubsystem>())
		{
			Music->SetMusicState(EMusicState::Elite);
		}
	}
}

void ADFEnemyBase::GrantDeathAbility()
{
	if (!HasAuthority() || !AbilitySystemComponent || DeathAbilitySpecHandle.IsValid())
	{
		return;
	}
	TSubclassOf<UGameplayAbility> Class = DeathAbilityClass;
	if (!Class || !Class->IsChildOf(UUDFAbility_Enemy_Death::StaticClass()))
	{
		if (Class)
		{
			UE_LOG(
				LogDungeonForged, Warning,
				TEXT("[EnemyDeath] %s: DeathAbilityClass=%s is not a child of UUDFAbility_Enemy_Death — using C++ default."),
				*GetName(), *GetNameSafe(Class));
		}
		Class = UUDFAbility_Enemy_Death::StaticClass();
	}
	DeathAbilitySpecHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(Class, 1, INDEX_NONE, this));
}

bool ADFEnemyBase::TriggerDeathGameplayAbility()
{
	if (!AbilitySystemComponent)
	{
		return false;
	}

	if (FDFGameplayTags::Event_Death.IsValid())
	{
		FGameplayEventData EventData;
		EventData.Instigator = this;
		EventData.Target = this;
		const int32 NumTriggered =
			AbilitySystemComponent->HandleGameplayEvent(FDFGameplayTags::Event_Death, &EventData);
		if (NumTriggered > 0)
		{
			DFDeathAnimation::LogEnemyDeath(
				1, this,
				FString::Printf(TEXT("TriggerDeathGameplayAbility: Event.Death -> %d"), NumTriggered));
			return true;
		}
	}

	if (TryActivateDeathAbility())
	{
		return true;
	}

	TSubclassOf<UGameplayAbility> Class = DeathAbilityClass;
	if (!Class)
	{
		Class = UUDFAbility_Enemy_Death::StaticClass();
	}
	const bool bByClass = AbilitySystemComponent->TryActivateAbilityByClass(Class, true);
	DFDeathAnimation::LogEnemyDeath(
		1, this,
		FString::Printf(
			TEXT("TriggerDeathGameplayAbility: fallback class(%s) -> %d"),
			*GetNameSafe(Class),
			bByClass ? 1 : 0));
	return bByClass;
}

bool ADFEnemyBase::TryActivateDeathAbility()
{
	if (!AbilitySystemComponent)
	{
		DFDeathAnimation::LogEnemyDeath(1, this, TEXT("TryActivateDeathAbility: no ASC"));
		return false;
	}
	bool bActivated = false;
	if (DeathAbilitySpecHandle.IsValid())
	{
		bActivated = AbilitySystemComponent->TryActivateAbility(DeathAbilitySpecHandle);
	}
	else if (FDFGameplayTags::Ability_Death.IsValid())
	{
		FGameplayTagContainer DeathTagContainer;
		DeathTagContainer.AddTag(FDFGameplayTags::Ability_Death);
		bActivated = AbilitySystemComponent->TryActivateAbilitiesByTag(DeathTagContainer, true);
	}
	DFDeathAnimation::LogEnemyDeath(
		1, this,
		FString::Printf(
			TEXT("TryActivateDeathAbility -> %d | SpecValid=%d | DeathAbilityClass=%s"),
			bActivated ? 1 : 0,
			DeathAbilitySpecHandle.IsValid() ? 1 : 0,
			*GetNameSafe(DeathAbilityClass)));
	return bActivated;
}

void ADFEnemyBase::SyncDeathToBlackboardAndAI()
{
	if (AAIController* const AI = Cast<AAIController>(GetController()))
	{
		if (UBlackboardComponent* const BB = AI->GetBlackboardComponent())
		{
			BB->SetValueAsBool(DFAIKeys::bIsDead, true);
		}
		AI->StopMovement();
		AI->ClearFocus(EAIFocusPriority::Gameplay);
		if (UBrainComponent* const Brain = AI->GetBrainComponent())
		{
			Brain->StopLogic(TEXT("Death"));
		}
	}
}

void ADFEnemyBase::ScheduleDeathDestroyBackup()
{
	if (!HasAuthority())
	{
		return;
	}
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(DeathDestroyTimer);
		const float DestroyDelay = ScheduleDestroyAfterDeath();
		DFDeathAnimation::LogEnemyDeath(
			1, this, FString::Printf(TEXT("Destroy backup scheduled in %.2fs"), DestroyDelay));
		W->GetTimerManager().SetTimer(DeathDestroyTimer, this, &ADFEnemyBase::OnDestroyAfterDeath, DestroyDelay, false);
	}
}

void ADFEnemyBase::ClearDeathDestroyBackup()
{
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(DeathDestroyTimer);
	}
}

void ADFEnemyBase::Multicast_PlayDeathCosmetic_Implementation()
{
	if (IsRunningDedicatedServer() || !DeathMontage)
	{
		return;
	}
	// Authority / listen-server host: montage is driven by UUDFAbility_Enemy_Death (PlayMontageAndWait).
	// Simulated proxies (remote clients) need this multicast for replication.
	if (GetLocalRole() != ROLE_SimulatedProxy)
	{
		return;
	}
	ApplyDeathCorpseMovementState();
	DFDeathAnimation::PlayDeathMontage(GetMesh(), DeathMontage, true, this, false);
}

void ADFEnemyBase::FallbackDeathPresentation()
{
	DFDeathAnimation::LogEnemyDeath(1, this, TEXT("FallbackDeathPresentation (GA_Enemy_Death did not activate)"));
	ApplyDeathCorpseMovementState();
	DisableEnemyActions();
	if (HealthBar)
	{
		HealthBar->SetVisibility(false);
	}
	ExecuteEnemyDeathPresentationCue();
	float MontageLen = 0.f;
	if (DeathMontage && GetMesh())
	{
		MontageLen = DFDeathAnimation::PlayDeathMontage(GetMesh(), DeathMontage, true, this, true);
	}
	if (HasAuthority())
	{
		if (UWorld* const W = GetWorld())
		{
			W->GetTimerManager().ClearTimer(FallbackDeathTimer);
			const float Wait = MontageLen > KINDA_SMALL_NUMBER ? MontageLen : 0.5f;
			W->GetTimerManager().SetTimer(
				FallbackDeathTimer, this, &ADFEnemyBase::OnFallbackDeathPresentationFinished, Wait, false);
		}
	}
}

void ADFEnemyBase::FinalizeDeathPresentation()
{
	if (bDeathPresentationFinalized)
	{
		return;
	}
	bDeathPresentationFinalized = true;
	DFDeathAnimation::LockDeathPoseOnMesh(GetMesh(), DeathMontage);
}

void ADFEnemyBase::Multicast_FinalizeDeathPresentation_Implementation()
{
	if (IsRunningDedicatedServer())
	{
		return;
	}
	FinalizeDeathPresentation();
}

void ADFEnemyBase::Dissolve()
{
	if (!HasAuthority())
	{
		return;
	}
	Multicast_StartDeathDissolve();
}

void ADFEnemyBase::SetupDeathDissolveTimeline()
{
	if (!DeathDissolveTimeline || bDeathDissolveTimelineInitialized)
	{
		return;
	}
	bDeathDissolveTimelineInitialized = true;

	UCurveFloat* CurveToUse = DissolveCurve;
	if (!CurveToUse)
	{
		UCurveFloat* const Linear = NewObject<UCurveFloat>(this, TEXT("EnemyDeathDissolveLinear"));
		Linear->FloatCurve.AddKey(0.f, 0.f);
		Linear->FloatCurve.AddKey(1.f, 1.f);
		CurveToUse = Linear;
	}
	FOnTimelineFloatStatic UpdateDelegate;
	UpdateDelegate.BindUObject(this, &ADFEnemyBase::OnDeathDissolveTimelineUpdate);
	DeathDissolveTimeline->AddInterpFloat(CurveToUse, UpdateDelegate);

	FOnTimelineEventStatic FinishedDelegate;
	FinishedDelegate.BindUObject(this, &ADFEnemyBase::OnDeathDissolveTimelineFinished);
	DeathDissolveTimeline->SetTimelineFinishedFunc(FinishedDelegate);
	DeathDissolveTimeline->SetLooping(false);
}

void ADFEnemyBase::BeginPostDeathCleanup(const float DestroyDelayOverride)
{
	if (!HasAuthority())
	{
		return;
	}
	UWorld* const W = GetWorld();
	if (!W)
	{
		return;
	}
	W->GetTimerManager().ClearTimer(PostDeathCleanupTimer);
	if (DeathDissolveTimeline)
	{
		DeathDissolveTimeline->Stop();
	}

	const float GroundWait = bDissolveOnDeath
		? DissolveDelayAfterMontageEnd
		: (DestroyDelayOverride >= 0.f ? DestroyDelayOverride : CorpseDestroyDelay);

	DFDeathAnimation::LogEnemyDeath(
		1, this,
		FString::Printf(
			TEXT("BeginPostDeathCleanup | dissolve=%d | groundWait=%.2fs"),
			bDissolveOnDeath ? 1 : 0,
			GroundWait));

	W->GetTimerManager().SetTimer(
		PostDeathCleanupTimer, this, &ADFEnemyBase::OnPostDeathDelayElapsed, FMath::Max(0.f, GroundWait), false);
}

void ADFEnemyBase::OnPostDeathDelayElapsed()
{
	if (!HasAuthority())
	{
		return;
	}
	if (bDissolveOnDeath)
	{
		Multicast_StartDeathDissolve();
		if (UWorld* const W = GetWorld())
		{
			W->GetTimerManager().SetTimer(
				PostDeathCleanupTimer, this, &ADFEnemyBase::OnDestroyAfterDeath, DissolveDuration, false);
		}
	}
	else
	{
		OnDestroyAfterDeath();
	}
}

void ADFEnemyBase::Multicast_StartDeathDissolve_Implementation()
{
	if (IsRunningDedicatedServer())
	{
		return;
	}
	StartDeathDissolvePresentation();
}

void ADFEnemyBase::StartDeathDissolvePresentation()
{
	if (!DeathDissolveTimeline)
	{
		return;
	}
	DeathDissolveMIDs.Reset();
	if (DeathDissolveMode == EDFEnemyDeathDissolveMode::SwapDissolveMaterial)
	{
		ApplySwapDissolveMaterials();
	}
	else
	{
		ApplyScalarDissolveOnExistingMaterials();
	}
	if (DeathDissolveMIDs.IsEmpty())
	{
		DFDeathAnimation::LogEnemyDeath(
			1, this,
			TEXT("StartDeathDissolvePresentation: no dissolve MIDs (assign DissolveMaterialInstance or check mesh materials)"));
		return;
	}
	DeathDissolveTimeline->SetTimelineLength(FMath::Max(0.05f, DissolveDuration));
	DeathDissolveTimeline->SetPlayRate(1.f);
	DeathDissolveTimeline->PlayFromStart();
}

void ADFEnemyBase::ApplySwapDissolveMaterials()
{
	auto AddDissolveMID = [this](UPrimitiveComponent* const Prim, UMaterialInterface* const Template, const int32 Slot)
	{
		if (!Prim || !Template)
		{
			return;
		}
		UMaterialInstanceDynamic* const MID = UMaterialInstanceDynamic::Create(Template, this);
		if (!MID)
		{
			return;
		}
		Prim->SetMaterial(Slot, MID);
		if (!DissolveParameterName.IsNone())
		{
			MID->SetScalarParameterValue(DissolveParameterName, 0.f);
		}
		DeathDissolveMIDs.Add(MID);
	};

	USkeletalMeshComponent* const SkelMesh = GetMesh();
	if (SkelMesh && DissolveMaterialInstance)
	{
		if (bDissolveAllBodyMaterialSlots)
		{
			const int32 NumMats = SkelMesh->GetNumMaterials();
			for (int32 Idx = 0; Idx < NumMats; ++Idx)
			{
				AddDissolveMID(SkelMesh, DissolveMaterialInstance, Idx);
			}
		}
		else
		{
			AddDissolveMID(SkelMesh, DissolveMaterialInstance, DissolveBodyMaterialSlot);
		}
	}
	if (WeaponDissolveMaterialInstance && DissolveWeaponMesh)
	{
		AddDissolveMID(DissolveWeaponMesh, WeaponDissolveMaterialInstance, 0);
	}
}

void ADFEnemyBase::ApplyScalarDissolveOnExistingMaterials()
{
	USkeletalMeshComponent* const SkelMesh = GetMesh();
	if (!SkelMesh)
	{
		return;
	}
	const int32 NumMats = SkelMesh->GetNumMaterials();
	for (int32 Idx = 0; Idx < NumMats; ++Idx)
	{
		UMaterialInstanceDynamic* const MID = SkelMesh->CreateAndSetMaterialInstanceDynamic(Idx);
		if (MID)
		{
			if (!DissolveParameterName.IsNone())
			{
				MID->SetScalarParameterValue(DissolveParameterName, 0.f);
			}
			DeathDissolveMIDs.Add(MID);
		}
	}
}

void ADFEnemyBase::OnDeathDissolveTimelineUpdate(const float Alpha)
{
	if (DissolveParameterName.IsNone())
	{
		return;
	}
	for (UMaterialInstanceDynamic* const MID : DeathDissolveMIDs)
	{
		if (MID)
		{
			MID->SetScalarParameterValue(DissolveParameterName, Alpha);
		}
	}
}

void ADFEnemyBase::OnDeathDissolveTimelineFinished()
{
	if (!DissolveParameterName.IsNone())
	{
		for (UMaterialInstanceDynamic* const MID : DeathDissolveMIDs)
		{
			if (MID)
			{
				MID->SetScalarParameterValue(DissolveParameterName, 1.f);
			}
		}
	}
}

void ADFEnemyBase::OnFallbackDeathPresentationFinished()
{
	if (!HasAuthority() || bHasDied)
	{
		return;
	}
	Multicast_FinalizeDeathPresentation();
	ApplyDeathGameplayState();
	MarkDied();
	ClearDeathDestroyBackup();
	if (UCapsuleComponent* const Cap = GetCapsuleComponent())
	{
		Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	BeginPostDeathCleanup(-1.f);
}

void ADFEnemyBase::MarkDied()
{
	if (bHasDied)
	{
		return;
	}
	bHasDied = true;
	bDeathFlowActive = false;
}

void ADFEnemyBase::HandleServerDeath(AActor* Killer)
{
	if (bHasDied || bDeathFlowActive)
	{
		DFDeathAnimation::LogEnemyDeath(2, this, TEXT("HandleServerDeath: ignored (already dead or in flow)"));
		return;
	}
	bDeathFlowActive = true;

	AActor* EffectiveKiller = Killer;
	if (EffectiveKiller == this)
	{
		if (LastDamageAttacker.IsValid() && LastDamageAttacker.Get() != this)
		{
			EffectiveKiller = LastDamageAttacker.Get();
		}
		else
		{
			EffectiveKiller = nullptr;
		}
	}

	SyncDeathToBlackboardAndAI();
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(DeathDestroyTimer);
	}
	DFDeathAnimation::LogEnemyDeath(
		1, this,
		FString::Printf(
			TEXT("HandleServerDeath START | Killer=%s | DeathMontage=%s | HealthBarWidget=%s"),
			*GetNameSafe(EffectiveKiller),
			*GetNameSafe(DeathMontage),
			HealthBarWidgetClass ? *HealthBarWidgetClass->GetName() : TEXT("NONE")));
	TSubclassOf<UGameplayAbility> DeathClass = DeathAbilityClass;
	if (!DeathClass)
	{
		DeathClass = UUDFAbility_Enemy_Death::StaticClass();
	}
	UE_LOG(
		LogDungeonForged, Log, TEXT("[EnemyDeath] %s HP=0 | Montage=%s | DeathGA=%s"),
		*GetName(), *GetNameSafe(DeathMontage), *GetNameSafe(DeathClass));
	if (HasAuthority())
	{
		if (ADFPlayerState* const PState = ResolveKillerPlayerState(EffectiveKiller))
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
	OnEnemyDied.Broadcast(this, EffectiveKiller, CachedExperienceReward);
	MulticastOnDeath(EffectiveKiller);

	const bool bDeathAbilityActivated = TriggerDeathGameplayAbility();
	if (!bDeathAbilityActivated)
	{
		Multicast_PlayDeathCosmetic();
		FallbackDeathPresentation();
		SpawnDeathLoot();
	}
	// Always schedule backup; cleared when death pipeline completes normally.
	ScheduleDeathDestroyBackup();
}

void ADFEnemyBase::CancelAbilitiesForDeath(UGameplayAbility* IgnoreAbility)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->CancelAllAbilities(IgnoreAbility);
	}
}

void ADFEnemyBase::ExecuteEnemyDeathPresentationCue()
{
	if (!AbilitySystemComponent || !FDFGameplayTags::GameplayCue_Enemy_Death.IsValid())
	{
		return;
	}
	FGameplayCueParameters CueParams;
	CueParams.Instigator = this;
	CueParams.EffectCauser = this;
	AbilitySystemComponent->ExecuteGameplayCue(FDFGameplayTags::GameplayCue_Enemy_Death, CueParams);
}

void ADFEnemyBase::ApplyDeathGameplayState(UGameplayAbility* IgnoreAbility)
{
	if (!AbilitySystemComponent)
	{
		return;
	}
	CancelAbilitiesForDeath(IgnoreAbility);

	TSubclassOf<UGameplayEffect> EffectClass = DeathGameplayEffectClass;
	if (!EffectClass)
	{
		EffectClass = UGE_EnemyDeath::StaticClass();
	}
	if (EffectClass)
	{
		FGameplayEffectContextHandle Ctx = AbilitySystemComponent->MakeEffectContext();
		Ctx.AddSourceObject(this);
		const FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.f, Ctx);
		if (Spec.IsValid() && Spec.Data.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			DFDeathAnimation::LogEnemyDeath(1, this, FString::Printf(TEXT("Applied death GE %s"), *GetNameSafe(EffectClass)));
			return;
		}
		DFDeathAnimation::LogEnemyDeath(1, this, TEXT("ApplyDeathGameplayState: MakeOutgoingSpec failed, falling back to loose State.Dead"));
	}
	if (FDFGameplayTags::State_Dead.IsValid())
	{
		AbilitySystemComponent->AddLooseGameplayTag(FDFGameplayTags::State_Dead, 1);
	}
}

void ADFEnemyBase::ApplyDeathCorpseMovementState()
{
	if (UCharacterMovementComponent* const Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
		Move->GravityScale = 0.f;
		Move->Velocity = FVector::ZeroVector;
	}
}

float ADFEnemyBase::ScheduleDestroyAfterDeath()
{
	const float MontageLen = DeathMontage ? DeathMontage->GetPlayLength() : 0.f;
	if (bDissolveOnDeath)
	{
		return MontageLen + DissolveDelayAfterMontageEnd + DissolveDuration + 5.f;
	}
	return MontageLen + FMath::Max(CorpseDestroyDelay, 1.f) + 5.f;
}

void ADFEnemyBase::MulticastOnDeath_Implementation(AActor* /*Killer*/)
{
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
	if (bDeathLootSpawned)
	{
		return;
	}
	if (CachedLootTableRowNames.IsEmpty())
	{
		return;
	}
	bDeathLootSpawned = true;
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

void ADFEnemyBase::DisableEnemyActions()
{
	ApplyDeathCorpseMovementState();
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
}

void ADFEnemyBase::OnDestroyAfterDeath()
{
	ClearDeathDestroyBackup();
	if (UWorld* const W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(PostDeathCleanupTimer);
		W->GetTimerManager().ClearTimer(FallbackDeathTimer);
	}
	if (DeathDissolveTimeline)
	{
		DeathDissolveTimeline->Stop();
	}
	if (!bHasDied)
	{
		ApplyDeathGameplayState();
		MarkDied();
	}
	bDeathFlowActive = false;
	DFDeathAnimation::LogEnemyDeath(1, this, TEXT("OnDestroyAfterDeath -> Destroy()"));
	if (UCapsuleComponent* const Cap = GetCapsuleComponent())
	{
		Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	Destroy();
}
