// Source/DungeonForged/Public/Characters/ADFPlayerCharacter.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Animation/AnimMontage.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "UI/DFAbilityBarTypes.h"
#include "Audio/UDFAudioComponent.h"
#include "Equipment/DFEquipmentTypes.h"
#include "FX/UDFCombatFeedbackTypes.h"
#include "ADFPlayerCharacter.generated.h"

class UCameraComponent;
class UDFAttributeSet;
class UDFLockOnComponent;
class UDFCameraComponent;
class UDFComboComponent;
class UDFComboPointsComponent;
class UDFImpactFramingComponent;
class UDFLauncherComponent;
class UDFStyleRatingComponent;
class UDFHitReactionComponent;
class UDFInteractionComponent;
class UDFMeleeTraceComponent;
class UDFMeleeAimComponent;
class USoundBase;
class UNiagaraSystem;
class UMotionWarpingComponent;
class UInputAction;
class UInputMappingContext;
class UAbilitySystemComponent;
class ADFMerchantActor;
class UDFShopWidget;
class UGameplayEffect;
class UDFTrapDetectionComponent;
class UDFEquipmentComponent;
class UDFInventoryComponent;
class UDFScreenEffectsComponent;
class UDFMinimapFogComponent;
class UDFDebugComponent;
class UDFStaminaExhaustionComponent;
class USkeletalMeshComponent;
class UGameplayEffect;

UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFPlayerCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ADFPlayerCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** Cached from ADFPlayerState — authoritative ASC lives on PlayerState. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	/** Not owned; AttributeSet is created on ADFPlayerState. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "GAS")
	TObjectPtr<UDFAttributeSet> AttributeSet;

	/** RMB: tries tags in order (e.g. block / heavy). Bound on @ref ADFRunPlayerController. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Combat", meta = (Categories = "Ability"))
	FGameplayTagContainer RMBAbilityTryTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UDFCameraComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UDFLockOnComponent> LockOnComponent;

	/** Melee: weapon sweep, GAS application. Wires to combo + anim notifies. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UDFMeleeTraceComponent> MeleeTrace;

	/** Resolves the melee target (lock-on > manual > soft cone) and snaps yaw on activation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UDFMeleeAimComponent> MeleeAim;

	/** Motion Warping: AnimGraph Motion Warping node consumes warp targets set by `UANS_DFMeleeWarp`. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UMotionWarpingComponent> MotionWarping;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UDFComboComponent> Combo;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UDFImpactFramingComponent> ImpactFraming;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UDFLauncherComponent> Launcher;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UDFStyleRatingComponent> StyleRating;

	/** GAS: builder/finisher combo points (Rogue, etc.). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|GAS")
	TObjectPtr<UDFComboPointsComponent> ComboPoints;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UDFHitReactionComponent> HitReaction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Death")
	TObjectPtr<UAnimMontage> DeathMontage;

	/** When true, death ends with ragdoll physics instead of freezing the last montage frame. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Death")
	bool bUseRagdollOnDeath = false;

	UFUNCTION(BlueprintPure, Category = "Combat|Death")
	bool IsPlayerDeathHandled() const { return bPlayerDeathHandled; }

	/** Called from @c UUDFAbility_Player_Death at activation. */
	void BeginDeathPresentationFromAbility();

	/** Server → local client: slow-mo, desaturation, death sting (Patch 1). */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayPlayerDeathCinematic();

	UFUNCTION(BlueprintCallable, Category = "DF|Death")
	void SetLastLethalDamageContext(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayTagContainer& DamageTags);

	UFUNCTION(BlueprintCallable, Category = "DF|Death")
	FString GetLastLethalCauseString() const;

	UFUNCTION(BlueprintPure, Category = "DF|Death")
	UDFScreenEffectsComponent* GetScreenEffects() const { return ScreenEffects; }

	/** Called from @c UUDFAbility_Player_Death / Multicast when montage ends. */
	void FinalizeDeathPresentation();

	/** Server: activates @c UUDFAbility_Player_Death. */
	bool TryActivateDeathAbility();

	/** If true, basic attack skips Ability.Warrior.MeleeSwing and uses only Combo montages. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|GAS")
	bool bDisableWarriorMeleeSwingGameplayAbility = false;

	/** Traces and sphere overlap: interact with IDFInteractable. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Interaction")
	TObjectPtr<UDFInteractionComponent> Interaction;

	/** Reveals hidden trap outlines / optional indicator widgets. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Traps|Sense")
	TObjectPtr<UDFTrapDetectionComponent> TrapDetection;

	/** 3D SFX for abilities, footstep/impact, UI (see UDFAudioComponent::PlayDFUISound). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Audio")
	TObjectPtr<UDFAudioComponent> DFAudio;

	/** Local screen post-process, hit-reaction, death vignette, berserk, etc. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|FX")
	TObjectPtr<UDFScreenEffectsComponent> ScreenEffects = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Combat|Stamina")
	TObjectPtr<UDFStaminaExhaustionComponent> StaminaExhaustion = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "DF|GAS|Passive")
	TSubclassOf<UGameplayEffect> DefaultStaminaRegenEffect;

	/** Minimap fog-of-war: room overlap → reveal/visit, current room for icon pulse. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Minimap")
	TObjectPtr<UDFMinimapFogComponent> MinimapFog = nullptr;

	/** Cheats and dev overlays call into this; heavy work is `df.*` and non-shipping. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Debug")
	TObjectPtr<UDFDebugComponent> DFDebug = nullptr;

	/**
	 * Co-op UI: {@link DFAbilityBarSlotCount} bar slots (DT_Abilities row names, or NAME_None).
	 * Activation uses row AbilityTag; drag-drop swaps entries on the server.
	 * @see UDFReplicationAudit.h
	 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentAbilitySlots, Category = "DF|GAS|Coop")
	TArray<FName> CurrentAbilitySlots;

	UPROPERTY(BlueprintAssignable, Category = "DF|GAS|AbilityBar")
	FDFOnAbilityBarSlotsChanged OnAbilityBarSlotsChanged;

	/** Paper-doll / GAS: slot-based equipping, modular meshes, leader-pose. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Equipment")
	TObjectPtr<UDFEquipmentComponent> Equipment = nullptr;

	/** Bag slots; loot / merchant / run restore; syncs with Equipment via equip from inventory. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Inventory")
	TObjectPtr<UDFInventoryComponent> Inventory = nullptr;

	/** Same as GetMesh() after construction. Body drives leader pose for slave armor pieces. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Mesh|Modular")
	TObjectPtr<USkeletalMeshComponent> Mesh_Base = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Mesh|Modular")
	TObjectPtr<USkeletalMeshComponent> Mesh_Helmet = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Mesh|Modular")
	TObjectPtr<USkeletalMeshComponent> Mesh_Chest = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Mesh|Modular")
	TObjectPtr<USkeletalMeshComponent> Mesh_Legs = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Mesh|Modular")
	TObjectPtr<USkeletalMeshComponent> Mesh_Boots = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Mesh|Modular")
	TObjectPtr<USkeletalMeshComponent> Mesh_Gloves = nullptr;

	/** Sockets: weapon_r / weapon_l on Mesh_Base. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Mesh|Modular")
	TObjectPtr<USkeletalMeshComponent> Mesh_Weapon = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Mesh|Modular")
	TObjectPtr<USkeletalMeshComponent> Mesh_OffHand = nullptr;

	/** Set when ClientOpenMerchantShop creates the shop; cleared in UDFShopWidget::CloseShop. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "DF|UI|Shop")
	TObjectPtr<UDFShopWidget> ActiveShopWidget;

	/** Zoom: TargetArmLength change per 1.0f of the zoom action (mouse wheel). */
	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "0.0"))
	float CameraZoomStep = 25.f;

	UFUNCTION(Client, Reliable, Category = "DF|UI|Shop")
	void ClientOpenMerchantShop(ADFMerchantActor* Shop);

	UFUNCTION(Client, Reliable, Category = "DF|UI|Shop")
	void ClientNotifyMerchantPurchase(int32 SlotIndex);

	UFUNCTION(Server, Reliable, WithValidation, Category = "DF|UI|Shop")
	void ServerMerchantPurchase(ADFMerchantActor* Shop, int32 SlotIndex);

	UFUNCTION(Server, Reliable, WithValidation, Category = "DF|UI|Shop")
	void ServerMerchantReroll(ADFMerchantActor* Shop);

	/** Called from UDFShopWidget when the panel is removed. */
	UFUNCTION(BlueprintCallable, Category = "DF|UI|Shop")
	void ClearActiveShopWidget();

	/**
	 * Victim-only (server → owning client): hit stop + local screen/shake. Runs on the pawn that was hit
	 * (not the instigator) so other players do not get global time dilation.
	 */
	UFUNCTION(Client, Unreliable)
	void Client_HitFeedback(EDFHitFeedbackBand Band, float DamagePercent, AActor* InstigatorActor);

	/** Attacker-only feel on remote clients (hit-stop + shake + impact VFX from tuning data). */
	UFUNCTION(Client, Unreliable)
	void Client_OnAttackHitConfirmed(const FDFHitConfirmedContext& Context);

	/** Local combat spectacle (last kill / room clear). */
	UFUNCTION(Client, Unreliable)
	void Client_PlayCombatSpectacle(bool bRoomClear, AActor* LastVictim = nullptr, AActor* Killer = nullptr);

	/** Server rejected a predicted ability activation — stop local montage (G7/N3). */
	UFUNCTION(Client, Unreliable)
	void Client_NotifyAbilityActivationRejected(TSubclassOf<class UGameplayAbility> AbilityClass);

	/** Server-triggered swing/impact FX for melee (same pattern as ADFEnemyBase::Multicast_PlayEnemyCosmeticCue). */
	UFUNCTION(NetMulticast, Unreliable, Category = "DF|Combat|FX")
	void Multicast_PlayMeleeCosmeticCue(
		USoundBase* Sound,
		UNiagaraSystem* VFX,
		FName AttachSocketName,
		FVector_NetQuantize Location,
		FRotator Rotation,
		FVector_NetQuantize100 Scale,
		bool bAttachToMesh);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayHitReactionMontage(UAnimMontage* Montage, float PlayRate = 1.f);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayDeathMontage();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_FinalizeDeathPresentation();

	/** Local UI: damage direction + combat HUD (source world location, 0–1 intensity). */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerDamageTakenForUI, FVector, DamageSourceWorld, float, Intensity);
	UPROPERTY(BlueprintAssignable, Category = "DF|UI|Combat")
	FOnPlayerDamageTakenForUI OnDamageTakenForUI;

	UFUNCTION(BlueprintPure, Category = "DF|Equipment")
	UDFEquipmentComponent* GetDFEquipment() const { return Equipment; }

	UFUNCTION(BlueprintPure, Category = "DF|Inventory")
	UDFInventoryComponent* GetDFInventory() const { return Inventory; }

	/** DT_Class.DefaultUnarmedMeleeAbilityTag via UDFRunManager (fallback when fist attack is not Warrior swing). */
	UFUNCTION(BlueprintPure, Category = "Combat|GAS")
	FGameplayTag GetDefaultUnarmedMeleeAbilityTag() const;

	UFUNCTION(BlueprintCallable, Category = "Combat|Melee")
	void RefreshMeleeLoadoutAfterEquipmentChange();

	/** Re-snaps Mesh_Weapon / Mesh_OffHand to weapon_r / weapon_l on Mesh_Base after body mesh or skeleton changes. */
	UFUNCTION(BlueprintCallable, Category = "DF|Mesh|Modular")
	void RefreshWeaponAndOffHandSocketAttachments();

	/**
	 * Attach Modular Mesh_Weapon to a socket on Mesh_Base (coldre / mão direita, etc.).
	 * Typical use: Anim Notify na montagem de sacar/guardar, em frames diferentes.
	 */
	UFUNCTION(BlueprintCallable, Category = "DF|Mesh|Modular")
	void SetWeaponAttachedToMeshBaseSocket(FName SocketName);

	/** Mesmo fluxo para off-hand (`Mesh_OffHand`) em socket do esqueleto base (`weapon_l` ou shield). */
	UFUNCTION(BlueprintCallable, Category = "DF|Mesh|Modular")
	void SetOffHandAttachedToMeshBaseSocket(FName SocketName);

	/** Slot1Based in [1, DFAbilityBarSlotCount]; reads CurrentAbilitySlots then DT_Abilities AbilityTag. */
	UFUNCTION(BlueprintCallable, Category = "DF|GAS|AbilityBar")
	void TryActivateAbilitySlot(int32 Slot1Based);

	UFUNCTION(BlueprintCallable, Category = "DF|GAS|AbilityBar")
	void RequestSwapAbilityBarSlots(int32 SlotIndexA, int32 SlotIndexB);

	UFUNCTION(Server, Reliable, Category = "DF|GAS|AbilityBar")
	void Server_SwapAbilityBarSlots(int32 SlotIndexA, int32 SlotIndexB);

	UFUNCTION(BlueprintCallable, Category = "DF|GAS|AbilityBar")
	void EnsureAbilityBarSlotArraySize();

	UFUNCTION(Server, Reliable, Category = "DF|Combat|Heavy")
	void Server_CommitHeavyAttack();

	/**
	 * Syncs the Max-heavy tier flag on the server's `UDFComboComponent` BEFORE the LocalPredicted GA replicates.
	 * Reliable RPC ordering on the same channel guarantees this arrives before `ServerTryActivateAbility`,
	 * so the server's GA reads the correct tier when resolving its montage / multipliers.
	 */
	UFUNCTION(Server, Reliable, Category = "DF|Combat|Heavy")
	void Server_NotifyHeavyAttackTier(bool bMaxTier);

	/** Called by @ref ADFRunPlayerController (run input is on the PC, not on hero BPs). */
	UFUNCTION(BlueprintCallable, Category = "Input|Handlers")
	void ApplyCameraZoomInput(float AxisValue);

	UFUNCTION(BlueprintCallable, Category = "Input|Handlers")
	void HandlePrimaryAttackPressed();

	UFUNCTION(BlueprintCallable, Category = "Input|Handlers")
	void HandlePrimaryAttackReleased();

	UFUNCTION(BlueprintCallable, Category = "Input|Handlers")
	void HandleSecondaryAttackPressed();

	UFUNCTION(BlueprintCallable, Category = "Input|Handlers")
	void HandleInteractPressed();

	UFUNCTION(BlueprintCallable, Category = "Input|Handlers")
	void HandleSprintStart();

	UFUNCTION(BlueprintCallable, Category = "Input|Handlers")
	void HandleSprintEnd();

	UFUNCTION(BlueprintCallable, Category = "Input|Handlers")
	void HandleDodgePressed();

	UFUNCTION(BlueprintCallable, Category = "Input|Handlers")
	void HandleLockOnToggle();

	UFUNCTION(BlueprintCallable, Category = "Input|Handlers")
	void HandleCycleLockOnLeft();

	UFUNCTION(BlueprintCallable, Category = "Input|Handlers")
	void HandleCycleLockOnRight();

	UFUNCTION(BlueprintCallable, Category = "Input|Handlers")
	void HandleEquipmentWeaponTogglePressed();

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitializeGAS();
	void ApplyDefaultPassiveGameplayEffects();

	void TryActivateByGameplayTagName(const FName& TagName);
	void CancelAbilitiesByGameplayTagName(const FName& TagName);
	void BindLockOnStrafeFromTargeting();

	void SetupModularMeshPart(USkeletalMeshComponent* Part);
	void RegisterModularSlotsWithEquipment();
	void RefreshWeaponTraceForMelee();
	void RefreshMeleeLoadoutFromClassAndEquipment();
	void CaptureMeleeComboMontagesBaselineOnce();
	void CaptureMeleeDamageTraceDefaultsOnce();
	void BindPlayerOutOfHealth();
	void UnbindPlayerOutOfHealth();
	void HandlePlayerOutOfHealth();
	void GrantDeathAbility();
	void LockDeathPose();
	void UnlockDeathPose();
	void EnterDeathRagdoll();
	void OnDeathMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);
	void OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnEquipmentEvent(EEquipmentSlot Slot, FName ItemRow);

private:
	/** OnEquipmentChanged bound once; cleared in EndPlay. */
	bool bModularEquipmentDelegateBound = false;
	bool bLockOnStrafeDelegateBound = false;

	bool bHasWeaponRSocket = false;
	bool bHasWeaponLSocket = false;
	bool bPlayerDeathHandled = false;
	bool bDeathPresentationFinalized = false;

	UPROPERTY(Transient)
	TObjectPtr<AActor> LastLethalInstigator = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AActor> LastLethalCauser = nullptr;

	UPROPERTY(Transient)
	FGameplayTagContainer LastLethalTags;
	FActiveGameplayEffectHandle StaminaRegenEffectHandle;

	FGameplayAbilitySpecHandle DeathAbilitySpecHandle;

	TWeakObjectPtr<UDFAttributeSet> BoundOutOfHealthAttributeSet;
	FTimerHandle DeathPoseLockTimerHandle;

	bool bMeleeComboMontagesBaselineCaptured = false;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAnimMontage>> CachedMeleeComboMontagesBaselineSnapshot;
	bool bMeleeTraceDamageBaselineCaptured = false;
	float CachedDefaultMeleeTraceBaseDamage = 0.f;
	TSubclassOf<UGameplayEffect> CachedDefaultMeleeTraceDamageGameplayEffect;

	UFUNCTION()
	void OnRep_CurrentAbilitySlots();

	void BroadcastAbilityBarSlotsChanged();
};
