#include "Characters/ADFPlayerCharacter.h"

#include "Animation/DFDeathAnimation.h"
#include "GAS/Abilities/UDFAbility_Player_Death.h"
#include "GAS/DFGameplayTags.h"
#include "Abilities/GameplayAbility.h"
#include "Characters/ADFPlayerState.h"
#include "Characters/UDFCharacterMovementComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Camera/UDFCameraComponent.h"
#include "Camera/UDFLockOnComponent.h"
#include "Combat/UDFCombatStateLibrary.h"
#include "Combat/UDFStaminaExhaustionComponent.h"
#include "GAS/Effects/UGE_StaminaRegen.h"
#include "Combat/UDFComboComponent.h"
#include "Combat/UDFComboPointsComponent.h"
#include "Combat/UDFHitReactionComponent.h"
#include "Combat/UDFMeleeAimComponent.h"
#include "Combat/UDFMeleeTraceComponent.h"
#include "MotionWarpingComponent.h"
#include "Interaction/UDFInteractionComponent.h"
#include "Dungeon/Traps/UDFTrapDetectionComponent.h"
#include "Audio/UDFAudioComponent.h"
#include "Audio/UDFMusicManagerSubsystem.h"
#include "GameFramework/SpringArmComponent.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimTypes.h"
#include "GAS/UDFAttributeSet.h"
#include "GameModes/Run/ADFRunPlayerController.h"
#include "GameplayTagContainer.h"
#include "Components/CapsuleComponent.h"
#include "Engine/CollisionProfile.h"
#include "Blueprint/UserWidget.h"
#include "Merchant/ADFMerchantActor.h"
#include "UI/UDFShopWidget.h"
#include "DFInventoryComponent.h"
#include "Equipment/UDFEquipmentComponent.h"
#include "FX/UDFHitStopSubsystem.h"
#include "FX/UDFScreenEffectsComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "UI/Minimap/UDFMinimapFogComponent.h"
#include "Debug/UDFDebugComponent.h"
#include "Data/DFDataTableStructs.h"
#include "Run/DFRunManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogDFPlayer, Log, All);

ADFPlayerCharacter::ADFPlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UDFCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	const bool bIsCDO = HasAnyFlags(RF_ClassDefaultObject);
	UE_LOG(LogDFPlayer, Verbose, TEXT("Ctor %s %s (outer=%s)"),
		bIsCDO ? TEXT("[CDO]") : TEXT("[Instance]"), *GetName(), GetOuter() ? *GetOuter()->GetName() : TEXT("null"));

	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		// Camera-relative move input + capsule turns toward velocity (Stellar Blade / Ninja Gaiden style).
		Move->bOrientRotationToMovement = true;
		Move->bUseControllerDesiredRotation = false;
	}

	CameraBoom = CreateDefaultSubobject<UDFCameraComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	LockOnComponent = CreateDefaultSubobject<UDFLockOnComponent>(TEXT("LockOnComponent"));

	MeleeTrace = CreateDefaultSubobject<UDFMeleeTraceComponent>(TEXT("MeleeTrace"));
	MeleeAim = CreateDefaultSubobject<UDFMeleeAimComponent>(TEXT("MeleeAim"));
	MotionWarping = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));
	Combo = CreateDefaultSubobject<UDFComboComponent>(TEXT("Combo"));
	ComboPoints = CreateDefaultSubobject<UDFComboPointsComponent>(TEXT("ComboPoints"));
	HitReaction = CreateDefaultSubobject<UDFHitReactionComponent>(TEXT("HitReaction"));
	Interaction = CreateDefaultSubobject<UDFInteractionComponent>(TEXT("InteractionComponent"));
	TrapDetection = CreateDefaultSubobject<UDFTrapDetectionComponent>(TEXT("TrapDetection"));
	DFAudio = CreateDefaultSubobject<UDFAudioComponent>(TEXT("DFAudio"));
	DFAudio->SetupAttachment(RootComponent);

	ScreenEffects = CreateDefaultSubobject<UDFScreenEffectsComponent>(TEXT("ScreenEffects"));
	StaminaExhaustion = CreateDefaultSubobject<UDFStaminaExhaustionComponent>(TEXT("StaminaExhaustion"));
	DefaultStaminaRegenEffect = UGE_StaminaRegen::StaticClass();

	MinimapFog = CreateDefaultSubobject<UDFMinimapFogComponent>(TEXT("MinimapFog"));
	MinimapFog->SetupAttachment(RootComponent);

	DFDebug = CreateDefaultSubobject<UDFDebugComponent>(TEXT("DFDebug"));

	CurrentAbilitySlots.Init(NAME_None, DFAbilityBarSlotCount);
	if (FDFGameplayTags::Ability_Warrior_ShieldBash.IsValid())
	{
		RMBAbilityTryTags.AddTag(FDFGameplayTags::Ability_Warrior_ShieldBash);
	}

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	Mesh_Base = GetMesh();
	if (Mesh_Base)
	{
		Equipment = CreateDefaultSubobject<UDFEquipmentComponent>(TEXT("Equipment"));
		Inventory = CreateDefaultSubobject<UDFInventoryComponent>(TEXT("Inventory"));

		Mesh_Helmet = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh_Helmet"));
		Mesh_Chest = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh_Chest"));
		Mesh_Legs = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh_Legs"));
		Mesh_Boots = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh_Boots"));
		Mesh_Gloves = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh_Gloves"));
		Mesh_Weapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh_Weapon"));
		Mesh_OffHand = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh_OffHand"));

		Mesh_Helmet->SetupAttachment(Mesh_Base);
		Mesh_Chest->SetupAttachment(Mesh_Base);
		Mesh_Legs->SetupAttachment(Mesh_Base);
		Mesh_Boots->SetupAttachment(Mesh_Base);
		Mesh_Gloves->SetupAttachment(Mesh_Base);
		static const FName NWeaponR(TEXT("weapon_r"));
		static const FName NWeaponL(TEXT("weapon_l"));
		Mesh_Weapon->SetupAttachment(Mesh_Base, NWeaponR);
		Mesh_OffHand->SetupAttachment(Mesh_Base, NWeaponL);

		SetupModularMeshPart(Mesh_Helmet);
		SetupModularMeshPart(Mesh_Chest);
		SetupModularMeshPart(Mesh_Legs);
		SetupModularMeshPart(Mesh_Boots);
		SetupModularMeshPart(Mesh_Gloves);
		SetupModularMeshPart(Mesh_Weapon);
		SetupModularMeshPart(Mesh_OffHand);

		UE_LOG(LogDFPlayer, Verbose, TEXT("Ctor mod meshes ok | weapon sockets validated in PostInitializeComponents"));
	}
	else
	{
		UE_LOG(LogDFPlayer, Warning, TEXT("Ctor: GetMesh() was null, modular equipment/gear not created. Name=%s"), *GetName());
	}
}

UAbilitySystemComponent* ADFPlayerCharacter::GetAbilitySystemComponent() const
{
	if (AbilitySystemComponent)
	{
		return AbilitySystemComponent;
	}
	if (const ADFPlayerState* PS = GetPlayerState<ADFPlayerState>())
	{
		return PS->AbilitySystemComponent;
	}
	return nullptr;
}

FGameplayTag ADFPlayerCharacter::GetDefaultUnarmedMeleeAbilityTag() const
{
	if (!GetWorld())
	{
		return FGameplayTag();
	}
	const UGameInstance* const GI = GetGameInstance();
	if (!GI)
	{
		return FGameplayTag();
	}
	const UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>();
	if (!RM)
	{
		return FGameplayTag();
	}
	const FDFClassTableRow* const Row = RM->FindClassTableRow(RM->GetCurrentRunState().SelectedClass);
	if (!Row)
	{
		return FGameplayTag();
	}
	return Row->DefaultUnarmedMeleeAbilityTag;
}

void ADFPlayerCharacter::CaptureMeleeComboMontagesBaselineOnce()
{
	if (bMeleeComboMontagesBaselineCaptured || !Combo)
	{
		return;
	}
	CachedMeleeComboMontagesBaselineSnapshot = Combo->ComboMontages;
	bMeleeComboMontagesBaselineCaptured = true;
}

void ADFPlayerCharacter::CaptureMeleeDamageTraceDefaultsOnce()
{
	if (bMeleeTraceDamageBaselineCaptured || !MeleeTrace)
	{
		return;
	}
	CachedDefaultMeleeTraceBaseDamage = MeleeTrace->BaseDamage;
	CachedDefaultMeleeTraceDamageGameplayEffect = MeleeTrace->MeleeDamageGameplayEffect;
	bMeleeTraceDamageBaselineCaptured = true;
}

void ADFPlayerCharacter::RefreshMeleeLoadoutAfterEquipmentChange()
{
	if (Equipment && HasAuthority())
	{
		Equipment->SyncWeaponMeleeGameplayAbilityGrant();
	}
	RefreshMeleeLoadoutFromClassAndEquipment();
}

void ADFPlayerCharacter::RefreshMeleeLoadoutFromClassAndEquipment()
{
	CaptureMeleeComboMontagesBaselineOnce();
	CaptureMeleeDamageTraceDefaultsOnce();
	if (!Combo || !MeleeTrace)
	{
		return;
	}

	const FDFClassTableRow* ClassRow = nullptr;
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UDFRunManager* const RM = GI->GetSubsystem<UDFRunManager>())
		{
			ClassRow = RM->FindClassTableRow(RM->GetCurrentRunState().SelectedClass);
		}
	}

	auto AssignComboBaseline = [&]()
	{
		if (!CachedMeleeComboMontagesBaselineSnapshot.IsEmpty())
		{
			Combo->ComboMontages = CachedMeleeComboMontagesBaselineSnapshot;
		}
	};

	auto ApplyEquippedWeaponMeleeProfile = [&](const FDFItemTableRow& WRow)
	{
		if (WRow.WeaponMeleeComboMontages.Num() > 0)
		{
			Combo->ComboMontages = WRow.WeaponMeleeComboMontages;
			Combo->MaxComboSteps = FMath::Max(Combo->MaxComboSteps, WRow.WeaponMeleeComboMontages.Num());
		}
		else if (ClassRow && ClassRow->ArmedMeleeComboMontagesFallback.Num() > 0)
		{
			Combo->ComboMontages = ClassRow->ArmedMeleeComboMontagesFallback;
		}
		else
		{
			AssignComboBaseline();
		}

		if (WRow.WeaponMeleeBaseDamage > KINDA_SMALL_NUMBER)
		{
			MeleeTrace->BaseDamage = WRow.WeaponMeleeBaseDamage;
		}
		else if (bMeleeTraceDamageBaselineCaptured)
		{
			MeleeTrace->BaseDamage = CachedDefaultMeleeTraceBaseDamage;
		}

		if (WRow.WeaponMeleeDamageGameplayEffect)
		{
			MeleeTrace->MeleeDamageGameplayEffect = WRow.WeaponMeleeDamageGameplayEffect;
		}
		else if (bMeleeTraceDamageBaselineCaptured)
		{
			MeleeTrace->MeleeDamageGameplayEffect = CachedDefaultMeleeTraceDamageGameplayEffect;
		}

		if (WRow.WeaponHeavyAttackMontage)
		{
			Combo->HeavyAttackMontage = WRow.WeaponHeavyAttackMontage;
		}
		else if (ClassRow && ClassRow->ArmedHeavyAttackMontageFallback)
		{
			Combo->HeavyAttackMontage = ClassRow->ArmedHeavyAttackMontageFallback;
		}
		else
		{
			Combo->HeavyAttackMontage = nullptr;
		}

		// Max heavy tier (highest charge threshold). Falls back: weapon → class → normal heavy.
		if (WRow.WeaponMaxHeavyAttackMontage)
		{
			Combo->MaxHeavyAttackMontage = WRow.WeaponMaxHeavyAttackMontage;
		}
		else if (ClassRow && ClassRow->ArmedMaxHeavyAttackMontageFallback)
		{
			Combo->MaxHeavyAttackMontage = ClassRow->ArmedMaxHeavyAttackMontageFallback;
		}
		else
		{
			Combo->MaxHeavyAttackMontage = nullptr;
		}

		// Directional combo overrides — class-driven only (no per-weapon directional for now).
		Combo->BackwardComboMontages = ClassRow
			? ClassRow->ArmedBackwardMeleeComboMontagesFallback
			: TArray<TObjectPtr<UAnimMontage>>();
		Combo->SideComboMontages = ClassRow
			? ClassRow->ArmedSideMeleeComboMontagesFallback
			: TArray<TObjectPtr<UAnimMontage>>();
	};

	if (!Equipment || Equipment->IsSlotEmpty(EEquipmentSlot::Weapon))
	{
		if (ClassRow && ClassRow->UnarmedMeleeComboMontages.Num() > 0)
		{
			Combo->ComboMontages = ClassRow->UnarmedMeleeComboMontages;
		}
		else
		{
			AssignComboBaseline();
		}

		if (bMeleeTraceDamageBaselineCaptured)
		{
			MeleeTrace->BaseDamage = CachedDefaultMeleeTraceBaseDamage;
			MeleeTrace->MeleeDamageGameplayEffect = CachedDefaultMeleeTraceDamageGameplayEffect;
		}

		Combo->HeavyAttackMontage = nullptr;
		Combo->MaxHeavyAttackMontage = nullptr;
		// Directional fallbacks may still exist for unarmed combos in class data.
		Combo->BackwardComboMontages = ClassRow
			? ClassRow->ArmedBackwardMeleeComboMontagesFallback
			: TArray<TObjectPtr<UAnimMontage>>();
		Combo->SideComboMontages = ClassRow
			? ClassRow->ArmedSideMeleeComboMontagesFallback
			: TArray<TObjectPtr<UAnimMontage>>();

		return;
	}

	const FDFItemTableRow* const WItem = Equipment->GetEquippedItemDataRaw(EEquipmentSlot::Weapon);
	if (WItem)
	{
		ApplyEquippedWeaponMeleeProfile(*WItem);
	}
}

void ADFPlayerCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	static const FName NWeaponR(TEXT("weapon_r"));
	static const FName NWeaponL(TEXT("weapon_l"));
	bHasWeaponRSocket = Mesh_Base && Mesh_Base->DoesSocketExist(NWeaponR);
	bHasWeaponLSocket = Mesh_Base && Mesh_Base->DoesSocketExist(NWeaponL);
	RefreshWeaponAndOffHandSocketAttachments();
}

void ADFPlayerCharacter::RefreshWeaponAndOffHandSocketAttachments()
{
	if (!Mesh_Base || !Mesh_Weapon || !Mesh_OffHand)
	{
		return;
	}
	static const FName NWeaponR(TEXT("weapon_r"));
	static const FName NWeaponL(TEXT("weapon_l"));
	const FAttachmentTransformRules Rules(FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	if (bHasWeaponRSocket)
	{
		Mesh_Weapon->AttachToComponent(Mesh_Base, Rules, NWeaponR);
	}
	else
	{
		Mesh_Weapon->AttachToComponent(Mesh_Base, Rules);
	}
	if (bHasWeaponLSocket)
	{
		Mesh_OffHand->AttachToComponent(Mesh_Base, Rules, NWeaponL);
	}
	else
	{
		Mesh_OffHand->AttachToComponent(Mesh_Base, Rules);
	}
}

void ADFPlayerCharacter::SetWeaponAttachedToMeshBaseSocket(const FName SocketName)
{
	if (!Mesh_Base || !Mesh_Weapon || SocketName.IsNone())
	{
		return;
	}
	const FAttachmentTransformRules Rules(FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	if (Mesh_Base->DoesSocketExist(SocketName))
	{
		Mesh_Weapon->AttachToComponent(Mesh_Base, Rules, SocketName);
	}
}

void ADFPlayerCharacter::SetOffHandAttachedToMeshBaseSocket(const FName SocketName)
{
	if (!Mesh_Base || !Mesh_OffHand || SocketName.IsNone())
	{
		return;
	}
	const FAttachmentTransformRules Rules(FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	if (Mesh_Base->DoesSocketExist(SocketName))
	{
		Mesh_OffHand->AttachToComponent(Mesh_Base, Rules, SocketName);
	}
}

void ADFPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogDFPlayer, Log, TEXT("BeginPlay %s | NetMode=%d HasAuth=%d Local=%d"),
		*GetName(), GetWorld() ? (int32)GetWorld()->GetNetMode() : -1, HasAuthority() ? 1 : 0, IsLocallyControlled() ? 1 : 0);
	// GAS: PossessedBy/OnRep may run after component BeginPlay; init early when PlayerState is already valid
	// so inventory/equip paths that apply GameplayEffects do not run before InitAbilityActorInfo.
	InitializeGAS();
	if (HasAuthority())
	{
		EnsureAbilityBarSlotArraySize();
	}
	RegisterModularSlotsWithEquipment();
	RefreshWeaponAndOffHandSocketAttachments();
	RefreshWeaponTraceForMelee();
	RefreshMeleeLoadoutAfterEquipmentChange();
}

void ADFPlayerCharacter::SetupModularMeshPart(USkeletalMeshComponent* const Part)
{
	if (!Part)
	{
		return;
	}
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetComponentTickEnabled(false);
	Part->bReceivesDecals = false;
	Part->SetCastShadow(true);
	Part->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
}

void ADFPlayerCharacter::RegisterModularSlotsWithEquipment()
{
	if (!Mesh_Base || !Equipment)
	{
		UE_LOG(LogDFPlayer, Verbose, TEXT("RegisterModularSlotsWithEquipment: skip (Mesh_Base=%d Equipment=%d) %s"),
			Mesh_Base != nullptr, Equipment != nullptr, *GetName());
		return;
	}
	if (bModularEquipmentDelegateBound)
	{
		UE_LOG(LogDFPlayer, Verbose, TEXT("RegisterModularSlotsWithEquipment: already bound, refresh only %s"), *GetName());
		Equipment->RefreshEquipmentVisuals();
		return;
	}
	UE_LOG(LogDFPlayer, Verbose, TEXT("RegisterModularSlotsWithEquipment: initial bind + refresh %s"), *GetName());
	Equipment->RegisterBaseBodyMesh(Mesh_Base);
	Equipment->RegisterSlotMesh(EEquipmentSlot::Helmet, Mesh_Helmet);
	Equipment->RegisterSlotMesh(EEquipmentSlot::Chest, Mesh_Chest);
	Equipment->RegisterSlotMesh(EEquipmentSlot::Legs, Mesh_Legs);
	Equipment->RegisterSlotMesh(EEquipmentSlot::Boots, Mesh_Boots);
	Equipment->RegisterSlotMesh(EEquipmentSlot::Gloves, Mesh_Gloves);
	Equipment->RegisterSlotMesh(EEquipmentSlot::Weapon, Mesh_Weapon);
	Equipment->RegisterSlotMesh(EEquipmentSlot::OffHand, Mesh_OffHand);
	Equipment->OnEquipmentChanged.AddDynamic(this, &ADFPlayerCharacter::OnEquipmentEvent);
	bModularEquipmentDelegateBound = true;
	Equipment->RefreshEquipmentVisuals();
}

void ADFPlayerCharacter::RefreshWeaponTraceForMelee()
{
	if (!MeleeTrace)
	{
		return;
	}
	if (MeleeTrace->SkeletalMesh && !IsValid(MeleeTrace->SkeletalMesh))
	{
		MeleeTrace->SkeletalMesh = nullptr;
	}
	if (Mesh_Weapon && Mesh_Weapon->GetSkeletalMeshAsset())
	{
		MeleeTrace->SkeletalMesh = Mesh_Weapon;
	}
	else if (Mesh_Base)
	{
		MeleeTrace->SkeletalMesh = Mesh_Base;
	}
}

void ADFPlayerCharacter::OnEquipmentEvent(const EEquipmentSlot Slot, const FName /*ItemRow*/)
{
	if (Slot == EEquipmentSlot::Weapon || Slot == EEquipmentSlot::OffHand)
	{
		RefreshWeaponAndOffHandSocketAttachments();
		RefreshWeaponTraceForMelee();
		RefreshMeleeLoadoutAfterEquipmentChange();
	}
}

void ADFPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindPlayerOutOfHealth();
	if (IsLocallyControlled() && GetWorld() && GetWorld()->GetNetMode() != NM_DedicatedServer)
	{
		if (UDFMusicManagerSubsystem* const Music = GetWorld()->GetSubsystem<UDFMusicManagerSubsystem>())
		{
			Music->UnregisterLocalPlayerForCombatMusic();
		}
	}
	if (Equipment && bModularEquipmentDelegateBound)
	{
		Equipment->OnEquipmentChanged.RemoveDynamic(this, &ADFPlayerCharacter::OnEquipmentEvent);
		bModularEquipmentDelegateBound = false;
	}
	UE_LOG(LogDFPlayer, Log, TEXT("EndPlay %s | Reason=%d"), *GetName(), (int32)EndPlayReason);
	Super::EndPlay(EndPlayReason);
}

void ADFPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	UE_LOG(LogDFPlayer, Verbose, TEXT("PossessedBy %s | Controller=%s"),
		*GetName(), NewController ? *NewController->GetName() : TEXT("null"));
	if (HasAuthority())
	{
		InitializeGAS();
	}
	if (ADFRunPlayerController* const RunPC = Cast<ADFRunPlayerController>(NewController))
	{
		if (IsLocallyControlled())
		{
			RunPC->EnsureGameplayInputReady();
		}
	}
}

void ADFPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	UE_LOG(LogDFPlayer, Verbose, TEXT("OnRep_PlayerState %s"), *GetName());
	InitializeGAS();
}

void ADFPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADFPlayerCharacter, CurrentAbilitySlots);
}

void ADFPlayerCharacter::OnRep_CurrentAbilitySlots()
{
	EnsureAbilityBarSlotArraySize();
	BroadcastAbilityBarSlotsChanged();
}

void ADFPlayerCharacter::BindPlayerOutOfHealth()
{
	if (!HasAuthority() || !AttributeSet)
	{
		return;
	}
	UnbindPlayerOutOfHealth();
	AttributeSet->OnOutOfHealth.AddUObject(this, &ADFPlayerCharacter::HandlePlayerOutOfHealth);
	BoundOutOfHealthAttributeSet = AttributeSet;
}

void ADFPlayerCharacter::UnbindPlayerOutOfHealth()
{
	if (UDFAttributeSet* const BoundSet = BoundOutOfHealthAttributeSet.Get())
	{
		BoundSet->OnOutOfHealth.RemoveAll(this);
	}
	BoundOutOfHealthAttributeSet.Reset();
}

void ADFPlayerCharacter::GrantDeathAbility()
{
	if (!HasAuthority())
	{
		return;
	}
	UAbilitySystemComponent* const ASC = GetAbilitySystemComponent();
	if (!ASC || DeathAbilitySpecHandle.IsValid())
	{
		return;
	}
	DeathAbilitySpecHandle = ASC->GiveAbility(
		FGameplayAbilitySpec(UUDFAbility_Player_Death::StaticClass(), 1, INDEX_NONE, this));
}

bool ADFPlayerCharacter::TryActivateDeathAbility()
{
	UAbilitySystemComponent* const ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}
	if (DeathAbilitySpecHandle.IsValid())
	{
		return ASC->TryActivateAbility(DeathAbilitySpecHandle);
	}
	if (FDFGameplayTags::Ability_Death_Player.IsValid())
	{
		FGameplayTagContainer DeathTagContainer;
		DeathTagContainer.AddTag(FDFGameplayTags::Ability_Death_Player);
		return ASC->TryActivateAbilitiesByTag(DeathTagContainer, true);
	}
	return false;
}

void ADFPlayerCharacter::BeginDeathPresentationFromAbility()
{
	SetCanBeDamaged(false);
	if (MeleeTrace)
	{
		MeleeTrace->EndTrace();
	}
	if (UCharacterMovementComponent* const Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}
	if (APlayerController* const PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}
}

void ADFPlayerCharacter::HandlePlayerOutOfHealth()
{
	if (!HasAuthority() || bPlayerDeathHandled)
	{
		return;
	}
	bPlayerDeathHandled = true;

	bool bDeathActivated = false;
	if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponent())
	{
		FGameplayEventData EventData;
		EventData.Instigator = this;
		EventData.Target = this;
		if (FDFGameplayTags::Event_Death.IsValid()
			&& ASC->HandleGameplayEvent(FDFGameplayTags::Event_Death, &EventData) > 0)
		{
			bDeathActivated = true;
		}
	}
	if (!bDeathActivated && !TryActivateDeathAbility())
	{
		BeginDeathPresentationFromAbility();
		if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponent())
		{
			ASC->CancelAllAbilities();
			if (FDFGameplayTags::State_Dead.IsValid())
			{
				ASC->AddLooseGameplayTag(FDFGameplayTags::State_Dead, 1);
			}
		}
		Multicast_PlayDeathMontage();
	}
}

void ADFPlayerCharacter::LockDeathPose()
{
	FinalizeDeathPresentation();
}

void ADFPlayerCharacter::FinalizeDeathPresentation()
{
	if (bDeathPresentationFinalized)
	{
		return;
	}
	bDeathPresentationFinalized = true;
	UDFCombatStateLibrary::ForceExitCombat(this, this);

	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathPoseLockTimerHandle);
	}
	if (bUseRagdollOnDeath)
	{
		EnterDeathRagdoll();
		return;
	}
	DFDeathAnimation::LockDeathPoseOnMesh(GetMesh(), DeathMontage);
}

void ADFPlayerCharacter::EnterDeathRagdoll()
{
	UnlockDeathPose();
	USkeletalMeshComponent* const MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComp->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	MeshComp->SetAllBodiesSimulatePhysics(true);
	MeshComp->WakeAllRigidBodies();
	if (UCapsuleComponent* const Cap = GetCapsuleComponent())
	{
		Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ADFPlayerCharacter::UnlockDeathPose()
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathPoseLockTimerHandle);
	}
	if (USkeletalMeshComponent* const MeshComp = GetMesh())
	{
		MeshComp->bPauseAnims = false;
		MeshComp->SetComponentTickEnabled(true);
	}
}

void ADFPlayerCharacter::InitializeGAS()
{
	ADFPlayerState* PS = GetPlayerState<ADFPlayerState>();
	if (!PS)
	{
		UE_LOG(LogDFPlayer, Verbose, TEXT("InitializeGAS: no PlayerState (cleared ASC) %s"), *GetName());
		if (IsLocallyControlled() && GetWorld() && GetWorld()->GetNetMode() != NM_DedicatedServer)
		{
			if (UDFMusicManagerSubsystem* const Music = GetWorld()->GetSubsystem<UDFMusicManagerSubsystem>())
			{
				Music->UnregisterLocalPlayerForCombatMusic();
			}
		}
		AbilitySystemComponent = nullptr;
		AttributeSet = nullptr;
		UnbindPlayerOutOfHealth();
		return;
	}

	AbilitySystemComponent = PS->AbilitySystemComponent;
	AttributeSet = PS->AttributeSet;

	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		ASC->InitAbilityActorInfo(PS, this);
		const bool bDeadByTag = FDFGameplayTags::State_Dead.IsValid()
			&& ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Dead);
		const bool bAliveByHealth = AttributeSet && AttributeSet->GetHealth() > 0.f;
		if (bAliveByHealth && !bDeadByTag)
		{
			SetCanBeDamaged(true);
			UnlockDeathPose();
			bPlayerDeathHandled = false;
			bDeathPresentationFinalized = false;
			if (FDFGameplayTags::State_Dead.IsValid())
			{
				ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Dead, 0);
			}
		}
		UE_LOG(LogDFPlayer, Verbose, TEXT("InitializeGAS: InitAbilityActorInfo OK | PS=%s Pawn=%s"),
			*PS->GetName(), *GetName());
	}
	BindPlayerOutOfHealth();
	GrantDeathAbility();
	ApplyDefaultPassiveGameplayEffects();

	if (IsLocallyControlled() && GetWorld() && GetWorld()->GetNetMode() != NM_DedicatedServer)
	{
		if (UAbilitySystemComponent* const ASC = AbilitySystemComponent.Get())
		{
			if (UDFMusicManagerSubsystem* const Music = GetWorld()->GetSubsystem<UDFMusicManagerSubsystem>())
			{
				Music->RegisterLocalPlayerForCombatMusic(ASC, this);
			}
		}
	}
	if (IsLocallyControlled() && !IsRunningDedicatedServer() && AttributeSet && ScreenEffects)
	{
		ScreenEffects->OnGASReady(AttributeSet);
	}
	if (IsLocallyControlled() && AttributeSet && AttributeSet->GetHealth() > 0.f)
	{
		if (ADFRunPlayerController* const RunPC = Cast<ADFRunPlayerController>(GetController()))
		{
			RunPC->EnsureGameplayInputReady();
		}
	}
}

void ADFPlayerCharacter::ApplyDefaultPassiveGameplayEffects()
{
	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}
	if (DefaultStaminaRegenEffect && !StaminaRegenEffectHandle.IsValid())
	{
		const FGameplayEffectContextHandle Ctx = AbilitySystemComponent->MakeEffectContext();
		const FGameplayEffectSpecHandle Spec =
			AbilitySystemComponent->MakeOutgoingSpec(DefaultStaminaRegenEffect, 1.f, Ctx);
		if (Spec.IsValid() && Spec.Data.IsValid())
		{
			StaminaRegenEffectHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
}

void ADFPlayerCharacter::ApplyCameraZoomInput(const float AxisValue)
{
	UDFCameraComponent* const Cam = CameraBoom;
	if (!Cam)
	{
		return;
	}
	Cam->OnZoomInput(-AxisValue * (CameraZoomStep / 50.f));
}

void ADFPlayerCharacter::HandlePrimaryAttackPressed()
{
	if (Combo)
	{
		Combo->OnPrimaryAttackPressed();
	}
}

void ADFPlayerCharacter::HandlePrimaryAttackReleased()
{
	if (Combo)
	{
		Combo->OnPrimaryAttackReleased();
	}
	else
	{
		TryActivateByGameplayTagName(FName("Ability.Attack"));
	}
}

void ADFPlayerCharacter::Server_CommitHeavyAttack_Implementation()
{
	if (Combo)
	{
		Combo->ServerCommitHeavyAttack();
	}
}

void ADFPlayerCharacter::Server_NotifyHeavyAttackTier_Implementation(const bool bMaxTier)
{
	if (Combo)
	{
		Combo->SetMaxHeavyPending(bMaxTier);
	}
}

void ADFPlayerCharacter::HandleSecondaryAttackPressed()
{
	for (const FGameplayTag& Tag : RMBAbilityTryTags)
	{
		if (!Tag.IsValid())
		{
			continue;
		}
		if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponent())
		{
			FGameplayTagContainer C;
			C.AddTag(Tag);
			if (ASC->TryActivateAbilitiesByTag(C, true))
			{
				return;
			}
		}
	}
}

void ADFPlayerCharacter::HandleInteractPressed()
{
	if (Interaction)
	{
		Interaction->TryInteract();
	}
}

void ADFPlayerCharacter::HandleSprintStart()
{
	TryActivateByGameplayTagName(FName("Ability.Movement.Sprint"));
}

void ADFPlayerCharacter::HandleSprintEnd()
{
	CancelAbilitiesByGameplayTagName(FName("Ability.Movement.Sprint"));
}

void ADFPlayerCharacter::HandleDodgePressed()
{
	TryActivateByGameplayTagName(FName("Ability.Movement.Dodge"));
}

void ADFPlayerCharacter::HandleEquipmentWeaponTogglePressed()
{
	if (!FDFGameplayTags::Ability_Equipment_WeaponToggle.IsValid())
	{
		return;
	}
	UAbilitySystemComponent* const ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}
	FGameplayTagContainer ToggleTags;
	ToggleTags.AddTag(FDFGameplayTags::Ability_Equipment_WeaponToggle);
	ASC->TryActivateAbilitiesByTag(ToggleTags, true);
}

void ADFPlayerCharacter::CancelAbilitiesByGameplayTagName(const FName& TagName)
{
	if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponent())
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
		if (!Tag.IsValid())
		{
			return;
		}
		FGameplayTagContainer T;
		T.AddTag(Tag);
		ASC->CancelAbilities(&T, nullptr, nullptr);
	}
}

void ADFPlayerCharacter::TryActivateByGameplayTagName(const FName& TagName)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
		if (!Tag.IsValid())
		{
			return;
		}
		FGameplayTagContainer C;
		C.AddTag(Tag);
		ASC->TryActivateAbilitiesByTag(C, true);
	}
}

void ADFPlayerCharacter::EnsureAbilityBarSlotArraySize()
{
	if (CurrentAbilitySlots.Num() == DFAbilityBarSlotCount)
	{
		return;
	}
	const int32 OldNum = CurrentAbilitySlots.Num();
	CurrentAbilitySlots.SetNum(DFAbilityBarSlotCount);
	for (int32 i = OldNum; i < DFAbilityBarSlotCount; ++i)
	{
		CurrentAbilitySlots[i] = NAME_None;
	}
}

void ADFPlayerCharacter::BroadcastAbilityBarSlotsChanged()
{
	OnAbilityBarSlotsChanged.Broadcast();
}

void ADFPlayerCharacter::TryActivateAbilitySlot(const int32 Slot1Based)
{
	if (Slot1Based < 1 || Slot1Based > DFAbilityBarSlotCount)
	{
		return;
	}

	const int32 SlotIndex = Slot1Based - 1;
	if (!CurrentAbilitySlots.IsValidIndex(SlotIndex))
	{
		return;
	}

	const FName RowName = CurrentAbilitySlots[SlotIndex];
	if (RowName.IsNone())
	{
		return;
	}

	UGameInstance* const GI = GetGameInstance();
	const UDFRunManager* const RM = GI ? GI->GetSubsystem<UDFRunManager>() : nullptr;
	UDataTable* const AbilityDT = RM ? RM->AbilityDataTable : nullptr;
	if (!AbilityDT)
	{
		const FName LegacyTag(*FString::Printf(TEXT("Ability.Slot.%d"), Slot1Based));
		TryActivateByGameplayTagName(LegacyTag);
		return;
	}

	const FDFAbilityTableRow* const Row =
		AbilityDT->FindRow<FDFAbilityTableRow>(RowName, TEXT("ADFPlayerCharacter::TryActivateAbilitySlot"), false);
	if (!Row || !Row->AbilityTag.IsValid())
	{
		return;
	}

	if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponent())
	{
		FGameplayTagContainer ActivateTags;
		ActivateTags.AddTag(Row->AbilityTag);
		ASC->TryActivateAbilitiesByTag(ActivateTags, true);
	}
}

void ADFPlayerCharacter::RequestSwapAbilityBarSlots(const int32 SlotIndexA, const int32 SlotIndexB)
{
	if (SlotIndexA == SlotIndexB)
	{
		return;
	}
	if (SlotIndexA < 0 || SlotIndexA >= DFAbilityBarSlotCount || SlotIndexB < 0 || SlotIndexB >= DFAbilityBarSlotCount)
	{
		return;
	}
	if (HasAuthority())
	{
		Server_SwapAbilityBarSlots_Implementation(SlotIndexA, SlotIndexB);
	}
	else
	{
		Server_SwapAbilityBarSlots(SlotIndexA, SlotIndexB);
	}
}

void ADFPlayerCharacter::Server_SwapAbilityBarSlots_Implementation(const int32 SlotIndexA, const int32 SlotIndexB)
{
	if (SlotIndexA == SlotIndexB)
	{
		return;
	}
	EnsureAbilityBarSlotArraySize();
	if (!CurrentAbilitySlots.IsValidIndex(SlotIndexA) || !CurrentAbilitySlots.IsValidIndex(SlotIndexB))
	{
		return;
	}
	CurrentAbilitySlots.Swap(SlotIndexA, SlotIndexB);
	ForceNetUpdate();
	BroadcastAbilityBarSlotsChanged();
}

void ADFPlayerCharacter::ClientOpenMerchantShop_Implementation(ADFMerchantActor* Shop)
{
	if (!IsLocallyControlled() || !Shop)
	{
		return;
	}
	if (!Shop->ShopWidgetClass)
	{
		return;
	}
	APlayerController* const PC = GetController<APlayerController>();
	if (UDFShopWidget* const W = CreateWidget<UDFShopWidget>(PC, Shop->ShopWidgetClass))
	{
		ActiveShopWidget = W;
		W->OpenForMerchant(Shop);
		W->AddToViewport(1000);
	}
}

void ADFPlayerCharacter::ClientNotifyMerchantPurchase_Implementation(int32 SlotIndex)
{
	if (!IsLocallyControlled())
	{
		return;
	}
	if (UDFShopWidget* const W = ActiveShopWidget)
	{
		W->PlaySlotPurchaseFeedback(SlotIndex);
	}
}

bool ADFPlayerCharacter::ServerMerchantPurchase_Validate(ADFMerchantActor* /*Shop*/, int32 /*SlotIndex*/)
{
	return true;
}

void ADFPlayerCharacter::ServerMerchantPurchase_Implementation(ADFMerchantActor* Shop, int32 SlotIndex)
{
	if (!HasAuthority() || !IsValid(Shop) || SlotIndex < 0)
	{
		return;
	}
	const float MaxDist = 900.f;
	if (FVector::Dist(Shop->GetActorLocation(), GetActorLocation()) > MaxDist)
	{
		return;
	}
	Shop->PurchaseItem(SlotIndex, this);
}

bool ADFPlayerCharacter::ServerMerchantReroll_Validate(ADFMerchantActor* /*Shop*/)
{
	return true;
}

void ADFPlayerCharacter::ServerMerchantReroll_Implementation(ADFMerchantActor* Shop)
{
	if (!HasAuthority() || !IsValid(Shop))
	{
		return;
	}
	if (FVector::Dist(Shop->GetActorLocation(), GetActorLocation()) > 900.f)
	{
		return;
	}
	Shop->RerollStock(this);
}

void ADFPlayerCharacter::ClearActiveShopWidget()
{
	ActiveShopWidget = nullptr;
}

void ADFPlayerCharacter::Client_HitFeedback_Implementation(
	const EDFHitFeedbackBand Band,
	const float DamagePercent,
	AActor* const InstigatorActor)
{
	if (IsRunningDedicatedServer())
	{
		return;
	}
	if (UWorld* const W = GetWorld())
	{
		if (UDFHitStopSubsystem* const HS = W->GetSubsystem<UDFHitStopSubsystem>())
		{
			AActor* const Ex = IsValid(InstigatorActor) ? InstigatorActor : nullptr;
			switch (Band)
			{
			case EDFHitFeedbackBand::Light: HS->LightHit(Ex); break;
			case EDFHitFeedbackBand::Heavy: HS->HeavyHit(Ex); break;
			case EDFHitFeedbackBand::Critical: HS->CriticalHit(Ex); break;
			case EDFHitFeedbackBand::Knockback: HS->BossSlam(Ex); break;
			default: break;
			}
		}
	}
	if (IsLocallyControlled() && ScreenEffects)
	{
		ScreenEffects->ApplyHitFromCombat(
			Band, DamagePercent, InstigatorActor, GetController<APlayerController>());
	}
	if (IsLocallyControlled())
	{
		FVector SourceLoc = GetActorLocation() - GetActorForwardVector() * 200.f;
		if (IsValid(InstigatorActor))
		{
			SourceLoc = InstigatorActor->GetActorLocation();
		}
		OnDamageTakenForUI.Broadcast(SourceLoc, FMath::Clamp(DamagePercent, 0.05f, 1.f));
	}
	UDFCombatStateLibrary::NotifyCombatActivity(this, this);
}

void ADFPlayerCharacter::Multicast_PlayMeleeCosmeticCue_Implementation(
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
	USkeletalMeshComponent* AttachMesh = GetMesh();
	if (bAttachToMesh && Mesh_Weapon && !AttachSocketName.IsNone() && Mesh_Weapon->DoesSocketExist(AttachSocketName))
	{
		AttachMesh = Mesh_Weapon;
	}
	if (bAttachToMesh && AttachMesh)
	{
		if (Sound)
		{
			UGameplayStatics::SpawnSoundAttached(
				Sound,
				AttachMesh,
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
				AttachMesh,
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

void ADFPlayerCharacter::Multicast_PlayHitReactionMontage_Implementation(UAnimMontage* Montage, const float PlayRate)
{
	if (IsRunningDedicatedServer() || !Montage)
	{
		return;
	}
	if (UAnimInstance* const Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		Anim->Montage_Play(Montage, PlayRate);
	}
}

void ADFPlayerCharacter::Multicast_FinalizeDeathPresentation_Implementation()
{
	FinalizeDeathPresentation();
}

void ADFPlayerCharacter::Multicast_PlayDeathMontage_Implementation()
{
	if (IsRunningDedicatedServer())
	{
		return;
	}
	UnlockDeathPose();
	USkeletalMeshComponent* const MeshComp = GetMesh();
	if (!MeshComp)
	{
		FinalizeDeathPresentation();
		return;
	}
	if (!DeathMontage)
	{
		FinalizeDeathPresentation();
		return;
	}
	if (UAnimInstance* const Anim = MeshComp->GetAnimInstance())
	{
		FOnMontageBlendingOutStarted BlendOut;
		BlendOut.BindUObject(this, &ADFPlayerCharacter::OnDeathMontageBlendingOut);
		Anim->Montage_SetBlendingOutDelegate(BlendOut, DeathMontage);

		FOnMontageEnded OnEnded;
		OnEnded.BindUObject(this, &ADFPlayerCharacter::OnDeathMontageEnded);
		Anim->Montage_SetEndDelegate(OnEnded, DeathMontage);

		const float Duration = DFDeathAnimation::PlayDeathMontage(MeshComp, DeathMontage, true, this);
		if (Duration > KINDA_SMALL_NUMBER)
		{
			const float LockDelay = FMath::Max(0.01f, Duration - 0.03f);
			if (UWorld* const World = GetWorld())
			{
				World->GetTimerManager().SetTimer(
					DeathPoseLockTimerHandle,
					this,
					&ADFPlayerCharacter::FinalizeDeathPresentation,
					LockDelay,
					false);
			}
		}
		else
		{
			FinalizeDeathPresentation();
		}
	}
	else
	{
		FinalizeDeathPresentation();
	}
}

void ADFPlayerCharacter::OnDeathMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	(void)Montage;
	(void)bInterrupted;
	LockDeathPose();
}

void ADFPlayerCharacter::OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	(void)Montage;
	(void)bInterrupted;
	LockDeathPose();
}
