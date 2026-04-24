// Source/DungeonForged/Public/Characters/ADFPlayerCharacter.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Input/DFInputConfig.h"
#include "InputAction.h"
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
class UDFInteractionComponent;
class UDFMeleeTraceComponent;
class UInputAction;
class UInputMappingContext;
class UAbilitySystemComponent;
class ADFMerchantActor;
class UDFShopWidget;
class UDFTrapDetectionComponent;
class UDFEquipmentComponent;
class UDFScreenEffectsComponent;
class USkeletalMeshComponent;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> IMC_Default;

	/** If set, ability bindings (Attack, abilities, interact) use AbilityInputActions + GAS local input instead of per-IA handlers below. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UDFInputConfig> InputConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Move;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Look;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Jump;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_CameraZoom;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Attack;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Ability1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Ability2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Ability3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Ability4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Interact;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Sprint;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Dodge;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UDFCameraComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UDFLockOnComponent> LockOnComponent;

	/** Melee: weapon sweep, GAS application. Wires to combo + anim notifies. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UDFMeleeTraceComponent> MeleeTrace;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UDFComboComponent> Combo;

	/** GAS: builder/finisher combo points (Rogue, etc.). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|GAS")
	TObjectPtr<UDFComboPointsComponent> ComboPoints;

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

	/** Paper-doll / GAS: slot-based equipping, modular meshes, leader-pose. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Equipment")
	TObjectPtr<UDFEquipmentComponent> Equipment = nullptr;

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

	UFUNCTION(BlueprintPure, Category = "DF|Equipment")
	UDFEquipmentComponent* GetDFEquipment() const { return Equipment; }

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PawnClientRestart() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	void AddDefaultMappingContext();
	void InitializeGAS();

	/** Binds each AbilityInputAction to ASC::AbilityLocalInputPressed/Released; InputID from FDFInputAction::GameplayInputId or 1-based array index. */
	void RegisterAbilityInputFromConfig(class UEnhancedInputComponent* EIC);

	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_JumpStart();
	void Input_JumpEnd();
	void Input_CameraZoom(const FInputActionValue& Value);
	void Input_Attack();
	void Input_Ability1();
	void Input_Ability2();
	void Input_Ability3();
	void Input_Ability4();
	void Input_Interact();
	void Input_SprintStart();
	void Input_SprintEnd();
	void Input_Dodge();

	void TryActivateByGameplayTagName(const FName& TagName);
	void CancelAbilitiesByGameplayTagName(const FName& TagName);
	void TryActivateAbilitySlot(int32 Slot1Based);

	void SetupModularMeshPart(USkeletalMeshComponent* Part);
	void RegisterModularSlotsWithEquipment();
	void RefreshWeaponTraceForMelee();
	UFUNCTION()
	void OnEquipmentEvent(EEquipmentSlot Slot, FName ItemRow);

	/** MMO-style look: one SetControlRotation with pitch clamp (avoids AddPitch/AddYaw + second clamp). */
	static constexpr float MinLookPitch = -60.f;
	static constexpr float MaxLookPitch = 60.f;

private:
	/** Set after IMC_Default is added so we do not register twice (BeginPlay + PawnClientRestart). */
	bool bDefaultInputContextAdded = false;

	/** OnEquipmentChanged bound once; cleared in EndPlay. */
	bool bModularEquipmentDelegateBound = false;
};
