# 🤖 CURSOR PROMPTS — DungeonForged (UE5.4 Roguelike)

Cole cada prompt diretamente no Cursor para gerar o código correspondente.
Recomenda-se usar no modo **Agent** com contexto de arquivos relevantes abertos.

---

## ════════════════════════════════════════
## 🏗️ PROMPT 0 — CONTEXTO GLOBAL (cole sempre primeiro)
## ════════════════════════════════════════

```
@ue-project-context
@ue-cpp-foundations
@ue-module-build-system

You are an expert Unreal Engine 5.4 C++ developer specializing in:
- Gameplay Ability System (GAS) with full C++ implementation
- Enhanced Input System (UE5)
- Data-Driven design using DataTables and PrimaryDataAssets
- Third-person ARPG / Roguelike Dungeon Crawler architecture
- The game is called "DungeonForged" - a WoW-style ARPG roguelike

Always read and follow the referenced @skills above before generating any code.
Each individual prompt will reference additional specific skills — always load them.

Project conventions:
- C++ class prefix: ADF (Actors), UDF (UObjects/Components)
- All DataTables use FTableRowBase-derived structs
- GAS: AbilitySystemComponent on PlayerState and EnemyBase
- No Blueprint logic for gameplay — BPs only call C++ functions
- Always use Enhanced Input, never legacy input
- Prefer composition over inheritance
- Use GameplayTags for all state/ability identification
- Code must compile for UE 5.4, no deprecated APIs

File structure convention (ALWAYS follow this):
- .h  files → Source/DungeonForged/Public/<Subsystem>/ClassName.h
- .cpp files → Source/DungeonForged/Private/<Subsystem>/ClassName.cpp
- Subsystem folders: Characters, GAS, Input, Dungeon, Items, UI, Camera, Combat
- Example:
    Source/DungeonForged/Public/Characters/DFPlayerCharacter.h
    Source/DungeonForged/Private/Characters/DFPlayerCharacter.cpp
- Always show the full file path as a comment at the top of each file:
    // Source/DungeonForged/Public/Characters/DFPlayerCharacter.h

When generating code:
1. Always output the file path before each code block
2. Always include required headers using the correct relative path
3. Add UPROPERTY/UFUNCTION macros correctly
4. Include .Build.cs module dependencies when needed
5. Comment complex GAS interactions in English
```

---

## ════════════════════════════════════════
## 📦 PROMPT 1 — BUILD.CS E MÓDULOS
## ════════════════════════════════════════

```
@ue-module-build-system
@ue-cpp-foundations

Generate the DungeonForged.Build.cs file for UE 5.4 with all required modules for:
- Gameplay Ability System (GAS): GameplayAbilities, GameplayTags, GameplayTasks
- Enhanced Input: EnhancedInput
- UMG / CommonUI: UMG, CommonUI, CommonInput
- AI: AIModule, NavigationSystem
- PCG: PCG
- Core engine: Core, CoreUObject, Engine, InputCore, NetCore

Also add these optional editor modules inside #if WITH_EDITOR.
Output the complete .Build.cs ready to use.
```

---

## ════════════════════════════════════════
## ⚔️ PROMPT 2 — ATTRIBUTESET COMPLETO
## ════════════════════════════════════════

```
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-cpp-foundations

Create UDFAttributeSet in C++ for UE 5.4 GAS with these attributes:
- Health, MaxHealth
- Mana, MaxMana
- Stamina, MaxStamina
- Strength (physical damage bonus)
- Intelligence (magic damage bonus)
- Agility (movement speed, crit chance bonus)
- Armor (physical damage reduction)
- MagicResist (magic damage reduction)
- CritChance (0.0 to 1.0)
- CritMultiplier (default 2.0)
- CooldownReduction (0.0 to 1.0)
- MovementSpeedMultiplier (default 1.0)

Requirements:
- Use ATTRIBUTE_ACCESSORS macro for all attributes
- Implement PreAttributeChange to clamp values (Health 0..MaxHealth, etc.)
- Implement PostGameplayEffectExecute for death detection (Health <= 0 → broadcast delegate)
- Add OnHealthChanged and OnManaChanged multicast delegates
- Derive from UAttributeSet
- Include full .h and .cpp

Output both UDFAttributeSet.h and UDFAttributeSet.cpp.
```

---

## ════════════════════════════════════════
## 🧍 PROMPT 3 — PLAYER CHARACTER
## ════════════════════════════════════════

```
@ue-gameplay-framework
@ue-gameplay-abilities
@ue-input-system
@ue-character-movement
@ue-cpp-foundations

Create ADFPlayerCharacter in C++ for UE 5.4 with:

Camera Setup:
- USpringArmComponent with: TargetArmLength=400, bUsePawnControlRotation=true
- UCameraComponent attached to spring arm
- Mouse scroll wheel adjusts TargetArmLength (200 to 800 range)
- bOrientRotationToMovement=true, bUseControllerRotationYaw=false

GAS Setup:
- Implement IAbilitySystemInterface
- UAbilitySystemComponent* AbilitySystemComponent (replicated)
- UDFAttributeSet* AttributeSet (pointer, not owned here — owned by PlayerState)
- Override GetAbilitySystemComponent()
- PossessedBy: call InitializeGAS() on server
- OnRep_PlayerState: call InitializeGAS() on client

Enhanced Input Setup:
- Override SetupPlayerInputComponent
- Cast to UEnhancedInputComponent
- Add IMC_Default mapping context in BeginPlay via UEnhancedInputLocalPlayerSubsystem
- Bind: IA_Move, IA_Look, IA_Jump, IA_Attack, IA_Ability1, IA_Ability2, IA_Ability3, IA_Ability4, IA_Interact
- Move: uses AddMovementInput with camera-relative direction
- Look: AddControllerYawInput / AddControllerPitchInput with clamped pitch (-60 to +60)

Ability activation:
- IA_Attack → TryActivateAbilitiesByTag(FGameplayTag::RequestGameplayTag("Ability.Attack"))
- IA_Ability1-4 → TryActivateAbilitiesByTag with Ability.Slot.1 through Ability.Slot.4

Output ADFPlayerCharacter.h and ADFPlayerCharacter.cpp
```

---

## ════════════════════════════════════════
## 🏛️ PROMPT 4 — PLAYER STATE COM GAS
## ════════════════════════════════════════

```
@ue-gameplay-framework
@ue-gameplay-abilities
@ue-networking-replication

Create ADFPlayerState in C++ for UE 5.4:
- Owns UAbilitySystemComponent (replicated, Mixed replication mode)
- Owns UDFAttributeSet (initialized here)
- Implements IAbilitySystemInterface
- SetNetUpdateFrequency(100.f) in constructor
- Function GrantAbilitiesFromDataTable(UDataTable* AbilityTable) that reads FDFAbilityTableRow and grants abilities to ASC
- Function InitializeAttributesFromDataTable(UDataTable* AttributeTable, FName RowName) to apply a startup GameplayEffect

Output ADFPlayerState.h and ADFPlayerState.cpp
```

---

## ════════════════════════════════════════
## 💥 PROMPT 5 — BASE GAMEPLAY ABILITY
## ════════════════════════════════════════

```
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-animation-system

Create UDFGameplayAbility in C++ for UE 5.4 GAS, extending UGameplayAbility:

Properties (UPROPERTY EditDefaultsOnly):
- FGameplayTagContainer AbilityTags (auto-assigned)
- FGameplayTagContainer CancelAbilitiesWithTag
- FGameplayTagContainer BlockAbilitiesWithTag
- EAbilityActivationPolicy ActivationPolicy (OnInputTriggered, OnInputStarted, Passive)
- UAnimMontage* AbilityMontage
- float AbilityCost_Mana
- float AbilityCost_Stamina
- float BaseCooldown

Functions:
- CanActivateAbility: check Mana and Stamina via GetNumericAttribute before calling Super
- ActivateAbility: blueprint-callable internal flow with CommitAbility guard
- GetAbilitySystemComponentFromActorInfo helper
- PlayAbilityMontage helper using UAbilitySystemGlobals

Also create the EAbilityActivationPolicy enum class.

Output UDFGameplayAbility.h and UDFGameplayAbility.cpp
```

---

## ════════════════════════════════════════
## 📊 PROMPT 6 — DATATABLES E STRUCTS
## ════════════════════════════════════════

```
@ue-data-assets-tables
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-asset-manager

Create all DataTable row structs for DungeonForged in C++:

1. FDFAbilityTableRow : FTableRowBase
   - TSubclassOf<UDFGameplayAbility> AbilityClass
   - FGameplayTag AbilityTag
   - int32 AbilityLevel
   - UTexture2D* Icon
   - FText DisplayName, Description

2. FDFItemTableRow : FTableRowBase
   - FText ItemName, Description
   - EItemRarity Rarity (Common, Uncommon, Rare, Epic, Legendary)
   - EItemType ItemType (Weapon, Armor, Helmet, Ring, Amulet, Consumable)
   - UStaticMesh* ItemMesh
   - UTexture2D* Icon
   - TMap<FGameplayAttribute, float> AttributeModifiers
   - TSubclassOf<UGameplayEffect> OnEquipEffect

3. FDFEnemyTableRow : FTableRowBase
   - TSubclassOf<AActor> EnemyClass
   - FText EnemyName
   - float BaseHealth, BaseDamage, BaseArmor
   - float ExperienceReward
   - TArray<FName> LootTableRows (from DT_Items)
   - TArray<FGameplayTag> GrantedAbilities
   - EEnemyTier Tier (Normal, Elite, Boss)

4. FDFClassTableRow : FTableRowBase
   - FText ClassName, ClassDescription
   - USkeletalMesh* CharacterMesh
   - TArray<FName> StartingAbilities (from DT_Abilities)
   - TMap<FGameplayAttribute, float> BaseAttributeValues

5. FDFDungeonFloorRow : FTableRowBase
   - int32 FloorNumber
   - TArray<FName> PossibleEnemyRows
   - int32 MinEnemies, MaxEnemies
   - bool bHasBoss
   - FName BossEnemyRow
   - float DifficultyMultiplier

Also create the EItemRarity, EItemType, EEnemyTier enums.
Put all in DFDataTableStructs.h
```

---

## ════════════════════════════════════════
## 🎮 PROMPT 7 — ENHANCED INPUT SETUP
## ════════════════════════════════════════

```
@ue-input-system
@ue-gameplay-abilities
@ue-cpp-foundations

For DungeonForged UE 5.4, generate:

1. Complete list of Input Actions (IA_) needed as UInputAction assets (describe settings):
   - IA_Move: Axis2D (WASD / Left Stick) with Swizzle + Negate modifiers
   - IA_Look: Axis2D (Mouse XY / Right Stick)
   - IA_Jump: Bool (Space / South Button), Triggered
   - IA_Attack: Bool (LMB / Right Trigger), Started
   - IA_Ability1-4: Bool, Started (Q, E, R, F / Face buttons)
   - IA_Interact: Bool (F... wait, conflict — use G), Started
   - IA_ToggleInventory: Bool (I / Select), Started
   - IA_Sprint: Bool (Shift / Left Stick Click), Started+Completed
   - IA_Dodge: Bool (Space second tap or Circle/B), Started
   - IA_LockOn: Bool (Middle Mouse / Right Stick Click), Started
   - IA_CameraZoom: Axis1D (Scroll Wheel)

2. C++ helper class UDFInputConfig (UDataAsset):
   - TArray<FDFInputAction> NativeInputActions (struct with UInputAction* and FGameplayTag)
   - TArray<FDFInputAction> AbilityInputActions
   - Function: const UInputAction* FindNativeInputActionByTag(const FGameplayTag& Tag) const

3. Binding pattern in ADFPlayerCharacter that iterates AbilityInputActions and calls 
   ASC->AbilityLocalInputPressed / Released using GameplayAbilityLocalInput enum

Output: DFInputConfig.h, DFInputConfig.cpp, and the binding code snippet
```

---

## ════════════════════════════════════════
## 🤖 PROMPT 8 — INIMIGO BASE COM GAS
## ════════════════════════════════════════

```
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-ai-navigation
@ue-actor-component-archit...
@ue-networking-replication

Create ADFEnemyBase in C++ for UE 5.4:

- Extends ACharacter
- Implements IAbilitySystemInterface
- Has UAbilitySystemComponent (Server Only replication mode)
- Has UDFAttributeSet* (owned by enemy)
- Has UWidgetComponent* for health bar above head

InitializeFromDataTable(UDataTable* EnemyTable, FName RowName):
- Read FDFEnemyTableRow
- Apply base stats using startup GameplayEffect
- Grant abilities from row's GrantedAbilities tags

AI Integration:
- Has UAIPerceptionComponent with SightConfig
- Has UBehaviorTreeComponent + UBlackboardComponent
- BeginPlay: RunBehaviorTree from data row

Death handling:
- Bind to AttributeSet OnHealthChanged delegate
- On Health <= 0: DisableInput, play death montage, spawn loot, destroy after 3s
- Broadcast OnEnemyDied delegate (for quest/progression tracking)

Combat:
- GetTeamAttitudeTowards override: Hostile to player, Friendly to other enemies

Output ADFEnemyBase.h and ADFEnemyBase.cpp
```

---

## ════════════════════════════════════════
## 🗺️ PROMPT 9 — DUNGEON MANAGER
## ════════════════════════════════════════

```
@ue-procedural-generation
@ue-world-level-streaming
@ue-gameplay-framework
@ue-data-assets-tables

Create ADFDungeonManager in C++ for UE 5.4 as a singleton GameMode subsystem:

Properties:
- int32 CurrentFloor
- UDataTable* DungeonFloorTable (DT_Dungeon)
- TArray<AActor*> SpawnedEnemies
- int32 EnemiesRemaining
- bool bFloorCleared

Functions:
- StartFloor(int32 FloorNumber): reads FDFDungeonFloorRow, calls GenerateDungeon + SpawnEnemies
- GenerateDungeon(): triggers PCG graph via UPCGComponent, places room templates
- SpawnEnemies(const FDFDungeonFloorRow& FloorData): spawns enemies at PCG-generated spawn points
  with random selection from PossibleEnemyRows weighted by floor difficulty
- OnEnemyKilled(AActor* Enemy): decrement EnemiesRemaining, if 0 → OnFloorCleared()
- OnFloorCleared(): open exit door, trigger loot drop, broadcast delegate
- AdvanceToNextFloor(): increment floor, if boss floor → trigger boss sequence
- GetCurrentFloorData(): returns current FDFDungeonFloorRow

Events (Multicast delegates):
- FOnFloorCleared OnFloorCleared
- FOnBossSpawned OnBossSpawned  
- FOnRunCompleted OnRunCompleted (win)
- FOnPlayerDied OnRunFailed (death = run reset)

Output ADFDungeonManager.h and ADFDungeonManager.cpp
```

---

## ════════════════════════════════════════
## 🎁 PROMPT 10 — SISTEMA DE LOOT
## ════════════════════════════════════════

```
@ue-data-assets-tables
@ue-gameplay-abilities
@ue-actor-component-archit...
@ue-physics-collision

Create the loot system for DungeonForged UE 5.4:

1. ADFLootDrop (Actor):
   - Spawned on enemy death with physics impulse
   - USphereComponent for pickup trigger
   - UStaticMeshComponent for item visual
   - FName ItemRowName (from DT_Items)
   - OnPickup delegate
   - Auto-pickup when player overlaps (plays VFX + SFX)
   - Glow material with rarity color (Common=grey, Uncommon=green, Rare=blue, Epic=purple, Legendary=orange)

2. UDFInventoryComponent (ActorComponent):
   - TArray<FDFInventorySlot> Items (struct: FName RowName, int32 Quantity, bool bIsEquipped)
   - int32 MaxSlots = 20
   - AddItem(FName RowName): reads DT_Items, checks stack, broadcasts OnInventoryChanged
   - RemoveItem(FName RowName, int32 Quantity)
   - EquipItem(int32 SlotIndex): unequip current slot, apply GE_Equip from item row
   - UnequipItem(int32 SlotIndex): remove gameplay effect
   - GetItemData(FName RowName): returns FDFItemTableRow*

3. UDFLootGeneratorSubsystem (WorldSubsystem):
   - RollLoot(const FDFEnemyTableRow& EnemyData, FVector SpawnLocation): 
     rolls each LootTableRow with rarity-weighted chance, spawns ADFLootDrop

Output: DFLootDrop.h/.cpp, DFInventoryComponent.h/.cpp, DFLootGeneratorSubsystem.h/.cpp
```

---

## ════════════════════════════════════════
## 🖥️ PROMPT 11 — HUD E UI
## ════════════════════════════════════════

```
@ue-ui-umg-slate
@ue-gameplay-abilities
@ue-gameplay-tags

Create the HUD C++ backend for DungeonForged UE 5.4:

1. ADFHUDBase extends AHUD:
   - TSubclassOf<UUserWidget> MainHUDWidgetClass
   - UUserWidget* MainHUDWidget
   - ShowHUD() / HideHUD()
   - BeginPlay: create and add MainHUDWidget to viewport

2. UDFUserWidgetBase extends UUserWidget:
   - GetDFPlayerCharacter() helper
   - GetDFPlayerState() helper  
   - GetAbilitySystemComponent() helper
   - BindToAttributeChanges(): subscribes to ASC attribute change delegates

3. UDFAttributeBarWidget extends UDFUserWidgetBase:
   - UPROPERTY BindWidget: UProgressBar* AttributeBar
   - UPROPERTY BindWidget: UTextBlock* ValueText
   - FGameplayAttribute TrackedAttribute
   - FGameplayAttribute MaxAttribute
   - NativeConstruct: bind to ASC attribute change
   - OnAttributeChanged: update bar percentage and text
   (Use this for Health Bar, Mana Bar, Stamina Bar)

4. UDFAbilitySlotWidget extends UDFUserWidgetBase:
   - FGameplayTag AbilityTag
   - UPROPERTY BindWidget: UImage* AbilityIcon
   - UPROPERTY BindWidget: UImage* CooldownOverlay
   - UPROPERTY BindWidget: UTextBlock* CooldownText
   - Listen to ASC for cooldown GameplayEffect with matching tag
   - Animate cooldown overlay with material parameter

Output all .h and .cpp files
```

---

## ════════════════════════════════════════
## 🔥 PROMPT 12 — HABILIDADE DE EXEMPLO (FIREBALL)
## ════════════════════════════════════════

```
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-niagara-effects
@ue-physics-collision
@ue-animation-system

Create GA_Fireball (UDFAbility_Fireball extends UDFGameplayAbility) for DungeonForged:

- AbilityTag: "Ability.Fire.Fireball"
- Costs 30 Mana (use GE_Cost_Fireball GameplayEffect)
- Cooldown: 3 seconds (use GE_Cooldown_Fireball GameplayEffect)
- On activate:
  1. CommitAbility (check cost + apply cooldown)
  2. Play montage (AbilityMontage) with WaitGameplayEvent for "Event.Ability.Fire.Launch"
  3. On event received: spawn ADFFireballProjectile at muzzle socket
  4. EndAbility on montage end or interrupted

Create ADFFireballProjectile:
- UProjectileMovementComponent: InitialSpeed=2000, MaxSpeed=2000
- USphereComponent collision
- UParticleSystemComponent (Niagara) for trail
- OnHit: apply GE_FireDamage to target (magnitude from Strength + Intelligence attributes)
  using SetByCallerMagnitude with tag "Data.Damage"
- GE_FireDamage: Instant, damages Health, applies GE_DoT_Fire (3 ticks over 3s)

Create UDFDamageCalculation extends UGameplayEffectExecutionCalculation:
- Captures Source: Strength, Intelligence
- Captures Target: Armor, MagicResist
- Calculates final damage: (BaseDamage + Intelligence * 0.5) * (1 - MagicResist/100)
- Applies CritChance roll with CritMultiplier

Output all files: DFAbility_Fireball.h/.cpp, DFFireballProjectile.h/.cpp, DFDamageCalculation.h/.cpp
```

---

## ════════════════════════════════════════
## 🎲 PROMPT 13 — ROGUELIKE META E RUN MANAGER
## ════════════════════════════════════════

```
@ue-serialization-savegames
@ue-gameplay-framework
@ue-game-features
@ue-data-assets-tables

Create the roguelike run management system for DungeonForged UE 5.4:

UDFRunManager (GameInstance Subsystem):

RunState struct:
- int32 CurrentFloor
- FName SelectedClass
- TArray<FName> EquippedItems
- TArray<FName> GrantedAbilities (accumulated during run)
- int32 Gold
- int32 Score
- float RunStartTime

Functions:
- StartNewRun(FName ClassName): reset state, load class from DT_Classes, initialize player
- OnPlayerDied(): save high score, broadcast run failed, show death screen after 2s
- OnRunCompleted(): calculate final score, unlock permanent rewards, save to SaveGame
- GetCurrentRunState(): const ref
- AddAbilityReward(FName AbilityRow): between floors, player picks 1 of 3 random abilities
- ApplyRunStateToPlayer(ADFPlayerCharacter* Player): restores state after level travel

UDFSaveGame extends USaveGame:
- int32 HighScore
- int32 TotalRuns, TotalWins
- TArray<FName> UnlockedClasses
- TArray<FName> UnlockedAbilities (permanent unlocks)
- Save() / Load() static helpers

Output: DFRunManager.h/.cpp, DFSaveGame.h/.cpp
```

---

## ════════════════════════════════════════
## 🔧 PROMPT 14 — GAMEPLAY TAGS SETUP
## ════════════════════════════════════════

```
@ue-gameplay-tags
@ue-asset-manager
@ue-gameplay-abilities

Generate the complete GameplayTags configuration for DungeonForged UE 5.4:

1. Create DFGameplayTags.h with static const FGameplayTag declarations for:

Ability tags:
- Ability.Attack.Melee, Ability.Attack.Ranged
- Ability.Fire.Fireball, Ability.Fire.FlameStrike
- Ability.Ice.FrostBolt, Ability.Ice.Blizzard
- Ability.Physical.Charge, Ability.Physical.Whirlwind
- Ability.Slot.1, Ability.Slot.2, Ability.Slot.3, Ability.Slot.4
- Ability.Passive.HealthRegen, Ability.Passive.ManaRegen

State tags:
- State.Dead, State.Stunned, State.Rooted, State.Silenced
- State.Invulnerable, State.Targeting, State.InCombat
- State.Sprinting, State.Dodging

Effect tags:
- Effect.DoT.Fire, Effect.DoT.Poison, Effect.DoT.Bleed
- Effect.Buff.Speed, Effect.Buff.DamageUp, Effect.Buff.Shield
- Effect.Debuff.Slow, Effect.Debuff.Weaken, Effect.Debuff.ArmorBreak

Event tags:
- Event.Ability.Fire.Launch, Event.Ability.Melee.Hit
- Event.Ability.Montage.End

Data tags:
- Data.Damage, Data.Healing, Data.Duration

UI tags:
- UI.MenuOpen, UI.InventoryOpen, UI.AbilityMenuOpen

2. Create DFGameplayTags.cpp with RegisterGameplayTags() using UGameplayTagsManager
3. Add call to RegisterGameplayTags() in UDFAssetManager::StartInitialLoading()

Output: DFGameplayTags.h, DFGameplayTags.cpp, DFAssetManager.h/.cpp
```

---

## ════════════════════════════════════════
## 🔁 PROMPT 15 — REVISÃO E INTEGRAÇÃO FINAL
## ════════════════════════════════════════

```
@ue-project-context
@ue-cpp-foundations
@ue-module-build-system
@ue-gameplay-abilities
@ue-networking-replication
@ue-testing-debugging

Review the DungeonForged UE 5.4 project structure and identify:

1. Any circular dependencies between modules
2. Missing GENERATED_BODY() macros
3. Missing module dependencies in Build.cs for classes used
4. GAS replication setup issues (check ASC ReplicationMode for each actor type):
   - Player: Mixed mode (PlayerState owns ASC)
   - Enemies: Minimal mode (Enemy owns ASC)
5. Ensure all UPROPERTY references to GAS types use the correct specifiers
6. Check Enhanced Input bindings for missing ETriggerEvent parameters
7. Verify all delegates are properly declared with DECLARE_DYNAMIC_MULTICAST_DELEGATE macros
8. Check DataTable struct inheritance from FTableRowBase
9. Suggest any missing systems for a complete roguelike loop

List all issues found and provide corrected code snippets for each.
```

---

## ════════════════════════════════════════
## 🎥 PROMPT 16 — CAMERA SYSTEM
## ════════════════════════════════════════

```
@ue-gameplay-framework
@ue-gameplay-tags
@ue-actor-component-archit...
@ue-ui-umg-slate

Create the complete Camera System for DungeonForged UE 5.4.
Follow the Public/Private folder convention:
  .h  → Source/DungeonForged/Public/Camera/<FileName>.h
  .cpp → Source/DungeonForged/Private/Camera/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── UDFCameraComponent ────────────────────────────────────────
Extends USpringArmComponent. Manages all camera states.

Properties (EditDefaultsOnly):
- float DefaultArmLength = 400.f
- float CombatArmLength = 300.f
- float LockOnArmLength = 350.f
- float MinZoom = 150.f, MaxZoom = 800.f
- float ZoomSpeed = 50.f
- float InterpSpeed = 8.f          ← smooth all transitions
- FVector DefaultSocketOffset = (0, 60, 80)    ← right shoulder
- FVector LockOnSocketOffset  = (0, 80, 60)    ← tighter in lock-on
- float CameraLag = 0.12f

States (enum ECameraState): Default, Combat, LockOn

Functions:
- TickCamera(float DeltaTime): lerp ArmLength + SocketOffset toward target
- OnZoomInput(float AxisValue): clamp + add to CurrentTargetArmLength
- EnterCombatMode(): transition to CombatArmLength over 0.5s
- ExitCombatMode(): return to DefaultArmLength
- EnableLockOn(AActor* Target): set LockOn state, store TargetActor
- DisableLockOn(): clear target, return to previous state
- UpdateLockOnRotation(float DeltaTime): slerp control rotation toward Target
- HandleCameraOcclusion(): use ProbeSize=12, enable DoCollisionTest

─── UDFLockOnComponent ────────────────────────────────────────
ActorComponent attached to PlayerCharacter.

Properties:
- float LockOnRange = 1500.f
- float LockOnAngle = 60.f         ← cone in front of player
- TWeakObjectPtr<AActor> CurrentTarget
- bool bIsLockedOn

Functions:
- TryLockOn(): sphere overlap in range → filter by angle + visibility trace
  → pick nearest valid enemy → call UDFCameraComponent::EnableLockOn
- CycleLockOnTarget(float Direction): switch to next/prev enemy in range
  (bind to right stick flick or Q/E while locked)
- ReleaseLockOn(): clear target, notify camera
- UpdateIndicator(float DeltaTime): update WBP_LockOnIndicator world position
- IsTargetValid(AActor* Target): alive + in range + line of sight check

─── WBP_LockOnIndicator hint ──────────────────────────────────
Create UDFLockOnWidget (UUserWidget):
- UImage* IndicatorImage (rotating ring)
- void UpdateScreenPosition(APlayerController* PC, FVector WorldPos)
  using ProjectWorldLocationToScreen

Output all files with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## ⚔️ PROMPT 17 — COMBAT SYSTEM
## ════════════════════════════════════════

```
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-animation-system
@ue-physics-collision
@ue-niagara-effects
@ue-actor-component-archit...

Create the complete Combat System for DungeonForged UE 5.4.
Follow the Public/Private folder convention:
  .h  → Source/DungeonForged/Public/Combat/<FileName>.h
  .cpp → Source/DungeonForged/Private/Combat/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── UDFMeleeTraceComponent ────────────────────────────────────
ActorComponent that handles weapon hitbox tracing.

Properties:
- FName TraceStartSocket = "weapon_start"
- FName TraceEndSocket   = "weapon_end"
- float TraceRadius = 20.f
- TArray<TWeakObjectPtr<AActor>> HitActorsThisSwing  ← prevent multi-hit
- FGameplayTag DamageTag = "Data.Damage"
- FGameplayEffectSpecHandle CachedDamageSpec

Functions:
- StartTrace(): clears HitActorsThisSwing, sets bTracing = true
- EndTrace(): bTracing = false
- TickTrace(float DeltaTime): if bTracing → SweepMultiByChannel between sockets
  → for each new hit actor: ApplyDamage, PlayHitReaction, SpawnHitVFX
- BuildDamageSpec(float BaseDamage, float KnockbackForce): creates GESpec
  with SetByCaller tags: Data.Damage + Data.Knockback
- ApplyDamageToTarget(AActor* Target, const FGameplayEffectSpecHandle& Spec)

─── UDFComboComponent ─────────────────────────────────────────
ActorComponent managing melee combo chains.

Combo state:
- int32 CurrentComboStep = 0
- int32 MaxComboSteps = 3
- float ComboWindowDuration = 0.6s   ← time to register next input
- bool bComboInputBuffered
- FTimerHandle ComboWindowTimer
- TArray<UAnimMontage*> ComboMontages   ← one per step, set in BP

Functions:
- OnAttackInput(): if in ComboWindow → buffer input; else → StartCombo
- StartCombo(): reset step, play ComboMontages[0], activate MeleeTraceComponent
- AdvanceCombo(): called via AnimNotify "AN_ComboWindow"
  → if buffered: play next montage, clear buffer; else open window timer
- ResetCombo(): step = 0, clear timers
- OnMontageEnded(): if no buffer → ResetCombo

AnimNotify hooks (create these C++ AnimNotify classes):
- UAN_ComboWindowOpen  → calls ComboComponent->AdvanceCombo()
- UAN_TraceStart       → calls MeleeTraceComponent->StartTrace()
- UAN_TraceEnd         → calls MeleeTraceComponent->EndTrace()

─── UDFHitReactionComponent ───────────────────────────────────
ActorComponent on enemies and player for receiving hits.

Properties:
- UAnimMontage* LightHitMontage
- UAnimMontage* HeavyHitMontage
- UAnimMontage* KnockbackMontage
- float StaggerThreshold = 30.f    ← damage in one frame to trigger stagger
- float KnockbackThreshold = 60.f

Functions:
- OnHitReceived(float DamageAmount, FVector HitDirection, AActor* Instigator):
  → choose reaction based on DamageAmount vs thresholds
  → apply GameplayTag State.Stunned via GAS for stagger duration
  → add impulse for knockback: CharacterMovement->AddImpulse
- PlayHitReaction(UAnimMontage* Montage, float PlayRate = 1.f)
- SpawnHitVFX(FVector Location, FRotator Normal): Niagara system at hit point
- SpawnHitDecal(FVector Location, FRotator Normal): blood/impact decal

─── GE_MeleeDamage ────────────────────────────────────────────
Describe the GameplayEffect setup:
- Duration: Instant
- Executions: UDFDamageCalculation (from Prompt 12)
- SetByCaller: Data.Damage, Data.Knockback
- GrantedTags on target: none (hit reaction handled by HitReactionComponent)
- Conditional GE: if target Health < 20% → apply GE_Finishing (extra damage mult)

Output all files with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🏃 PROMPT 18 — MOVEMENT SYSTEM
## ════════════════════════════════════════

```
@ue-character-movement
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-animation-system
@ue-networking-replication

Create the complete Movement System for DungeonForged UE 5.4.
Follow the Public/Private folder convention:
  .h  → Source/DungeonForged/Public/Characters/<FileName>.h
  .cpp → Source/DungeonForged/Private/Characters/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── UDFCharacterMovementComponent ────────────────────────────
Extends UCharacterMovementComponent. Custom CMC for the player.

Movement constants (EditDefaultsOnly):
- float WalkSpeed        = 400.f
- float SprintSpeed      = 700.f
- float CrouchSpeed      = 200.f
- float SprintStaminaDrain = 15.f   ← per second
- float DodgeCooldown    = 0.8f
- float DodgeDistance    = 600.f
- float DodgeDuration    = 0.35f
- float IFrameDuration   = 0.25f    ← invulnerability window inside dodge
- bool bIsSprinting
- bool bIsDodging

Saved Move (for network):
- Extend FSavedMove_Character with bWantsSprint flag
- Override GetCompressedFlags, SetMoveFor, PrepMoveFor, CanCombineWith

Functions:
- SetSprinting(bool bSprint): toggle sprint, update MaxWalkSpeed
- TickSprintStamina(float DeltaTime): drain Stamina via ASC attribute
  → if Stamina <= 0 → force SetSprinting(false) + apply GE_SprintExhaustion
- PerformDodge(FVector Direction): 
  → check DodgeCooldown timer
  → apply GameplayTag State.Dodging to self via GAS
  → add impulse with DodgeDistance/DodgeDuration curve
  → apply GameplayTag State.Invulnerable for IFrameDuration
  → on end: clear both tags
- GetDodgeDirection(): if input direction → use it; else → use actor backward
- OnMovementModeChanged override: broadcast delegate for animation graph

─── Sprint integration with GAS ──────────────────────────────
Create GA_Sprint (UDFGameplayAbility):
- File: Public/GAS/Abilities/DFAbility_Sprint.h
        Private/GAS/Abilities/DFAbility_Sprint.cpp
- ActivationPolicy: OnInputStarted
- Tags: Ability.Movement.Sprint
- BlockAbilitiesWith: State.Stunned, State.Rooted, State.Dodging
- On Activate: call DFCharacterMovementComponent->SetSprinting(true)
  apply GE_SprintStaminaDrain (Infinite duration, period 0.1s, drains Stamina)
- On End / Cancelled: SetSprinting(false), RemoveActiveGameplayEffect for drain GE
- Input: IA_Sprint Started → Activate | Completed → Cancel

─── Dodge integration with GAS ───────────────────────────────
Create GA_Dodge (UDFGameplayAbility):
- File: Public/GAS/Abilities/DFAbility_Dodge.h
        Private/GAS/Abilities/DFAbility_Dodge.cpp
- Cost: 20 Stamina (GE_Cost_Dodge, Instant)
- Cooldown: 0.8s (GE_Cooldown_Dodge)
- Tags: Ability.Movement.Dodge
- BlockAbilitiesWith: State.Dead, State.Stunned, State.Dodging
- CancelAbilitiesWith: Ability.Attack.Melee (dodge cancels current attack)
- On Activate:
  1. CommitAbility (deduct stamina + start cooldown)
  2. Apply GameplayTag State.Dodging
  3. Call DFCharacterMovementComponent->PerformDodge(InputDirection)
  4. WaitDelay(DodgeDuration) → EndAbility
- Play dodge AnimMontage via PlayMontageAndWait task

─── Animation integration ────────────────────────────────────
Create UDFAnimInstance (UAnimInstance):
- File: Public/Characters/DFAnimInstance.h
        Private/Characters/DFAnimInstance.cpp
- Cache: ADFPlayerCharacter* OwningCharacter, UDFCharacterMovementComponent* CMC
- NativeUpdateAnimation(float DeltaSeconds): update all anim variables
- Variables:
  float Speed                  ← Velocity.Size()
  float Direction              ← CalculateDirection (for strafe)
  bool bIsInAir
  bool bIsSprinting
  bool bIsDodging
  bool bIsDead
  bool bIsInCombat             ← from GameplayTag State.InCombat
  bool bIsLockedOn             ← from GameplayTag State.Targeting
  FVector Velocity
- HasTag(FGameplayTag Tag): checks ASC for tag → used in AnimGraph conditions
- Locomotion blend space: uses Speed + Direction for 8-directional movement
  (required for lock-on strafing)

Output all files with correct Public/Private paths and full path comment headers.
```

---

## ════════════════════════════════════════
## 🤖 PROMPT 19 — AI BEHAVIOR TREE
## ════════════════════════════════════════

```
@ue-ai-navigation
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-actor-component-archit...

Create the complete AI system for DungeonForged UE 5.4 enemies.
Follow the Public/Private folder convention:
  .h  → Source/DungeonForged/Public/AI/<FileName>.h
  .cpp → Source/DungeonForged/Private/AI/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── UDFAIController ───────────────────────────────────────────
Extends AAIController.

Properties:
- UBehaviorTreeComponent* BehaviorTreeComponent
- UBlackboardComponent* BlackboardComponent
- UAIPerceptionComponent* PerceptionComponent
- UAISenseConfig_Sight* SightConfig
- UAISenseConfig_Hearing* HearingConfig

Blackboard Keys (define as FName constants in a header DFAIKeys.h):
- BB_TargetActor      (Object)
- BB_TargetLocation   (Vector)
- BB_bCanSeeTarget    (Bool)
- BB_bIsInAttackRange (Bool)
- BB_bIsDead          (Bool)
- BB_PatrolIndex      (Int)
- BB_CombatState      (Enum: Idle, Patrol, Chase, Attack, Flee)

Functions:
- OnPossess: RunBehaviorTree from enemy data row
- OnPerceptionUpdated: on sight → set BB_TargetActor + BB_bCanSeeTarget
- OnTargetPerceptionForgotten: clear BB_TargetActor, return to Patrol
- SetCombatState(ECombatState State): update BB_CombatState
- GetDistanceToTarget(): returns distance to BB_TargetActor

─── Behavior Tree Tasks ───────────────────────────────────────
Create these UBTTaskNode_DF* classes:

1. UDFBTTask_MeleeAttack:
   - RequiresAbilityTag: "Ability.Attack.Melee"
   - ExecuteTask: TryActivateAbilityByTag on enemy ASC
   - WaitForMontageEnd via delegate before returning EBTNodeResult::Succeeded

2. UDFBTTask_RangedAttack:
   - RequiresAbilityTag: "Ability.Attack.Ranged"
   - ExecuteTask: face target → TryActivateAbilityByTag

3. UDFBTTask_FindPatrolPoint:
   - Reads patrol points from ADFEnemyBase::PatrolPoints array
   - Increments BB_PatrolIndex cyclically
   - Writes next point to BB_TargetLocation

4. UDFBTTask_FleeFromPlayer:
   - Finds navigation point away from player (180° opposite direction)
   - MoveToLocation with AcceptanceRadius=200

5. UDFBTTask_PlayTauntMontage:
   - Plays random montage from ADFEnemyBase::TauntMontages
   - Returns Succeeded immediately (fire and forget)

─── Behavior Tree Services ────────────────────────────────────

1. UDFBTService_UpdateTarget:
   - TickNode every 0.2s
   - Sphere overlap for nearest player
   - Update BB_TargetActor, BB_bCanSeeTarget via line trace
   - Update BB_bIsInAttackRange (AttackRange from enemy data row)

2. UDFBTService_CheckHealth:
   - TickNode every 0.5s
   - If Health < 20% → set BB_CombatState to Flee
   - If Health <= 0  → set BB_bIsDead = true

─── Behavior Tree Decorators ──────────────────────────────────

1. UDFBTDecorator_HasGASTag:
   - Property: FGameplayTag RequiredTag
   - CalculateRawConditionValue: check enemy ASC HasMatchingGameplayTag

2. UDFBTDecorator_IsInRange:
   - Property: float Range
   - CalculateRawConditionValue: distance to BB_TargetActor <= Range

─── Behavior Tree layout description ─────────────────────────
Describe the BT_EnemyBase tree structure as a comment block:
Root → Selector
  ├─ [Decorator: BB_bIsDead == true] → BTTask_Die
  ├─ Sequence (Chase + Attack)
  │   ├─ [Decorator: HasTarget]
  │   ├─ Service: UpdateTarget (0.2s)
  │   ├─ Service: CheckHealth (0.5s)
  │   ├─ Selector (Attack or Chase)
  │   │   ├─ [Decorator: IsInRange(MeleeRange)] → Task_MeleeAttack
  │   │   ├─ [Decorator: IsInRange(RangedRange)] → Task_RangedAttack
  │   │   └─ MoveTo BB_TargetActor
  │   └─ [Decorator: Health<20%] → Task_FleeFromPlayer
  └─ Sequence (Patrol)
      ├─ Task_FindPatrolPoint
      └─ MoveTo BB_TargetLocation

Output all .h and .cpp files with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🌀 PROMPT 20 — GAMEPLAY EFFECT LIBRARY
## ════════════════════════════════════════

```
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-data-assets-tables

Create the complete GameplayEffect Library for DungeonForged UE 5.4.
Follow the Public/Private folder convention:
  .h  → Source/DungeonForged/Public/GAS/Effects/<FileName>.h
  .cpp → Source/DungeonForged/Private/GAS/Effects/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── UDFGameplayEffectLibrary (UBlueprintFunctionLibrary) ──────
Static helpers callable from C++ and Blueprint:
- MakeDamageEffect(float BaseDamage, FGameplayTag DamageTypeTag, AActor* Instigator)
  → returns FGameplayEffectSpecHandle with SetByCaller Data.Damage
- MakeHealEffect(float Amount, AActor* Instigator)
  → returns FGameplayEffectSpecHandle with SetByCaller Data.Healing
- ApplyEffectToSelf(AActor* Target, TSubclassOf<UGameplayEffect> GEClass, float Level=1)
- ApplyEffectToTarget(AActor* Source, AActor* Target, TSubclassOf<UGameplayEffect> GEClass)

─── Damage GameplayEffects ────────────────────────────────────
Describe setup for each (C++ defaults via CDO in constructor):

GE_Damage_Physical (Instant)
- Execution: UDFDamageCalculation
- SetByCaller: Data.Damage, Data.Knockback
- Tag: Effect.Damage.Physical

GE_Damage_Magic (Instant)
- Execution: UDFDamageCalculation
- SetByCaller: Data.Damage
- Tag: Effect.Damage.Magic

GE_Damage_True (Instant)
- Modifier: Health -= SetByCaller(Data.Damage)  ← bypasses armor
- Tag: Effect.Damage.True

─── DoT GameplayEffects ───────────────────────────────────────

GE_DoT_Fire (HasDuration, Period=1s, Duration=3s)
- Modifier: Health -= (Strength * 0.1) per tick
- GrantedTag: Effect.DoT.Fire
- StackingType: AggregateBySource, MaxStacks=3
- Period Inhibition: none

GE_DoT_Poison (HasDuration, Period=1s, Duration=5s)
- Modifier: Health -= flat 5 per tick (SetByCaller Data.Damage)
- GrantedTag: Effect.DoT.Poison
- StackingType: AggregateByTarget, MaxStacks=1

GE_DoT_Bleed (HasDuration, Period=0.5s, Duration=4s)
- Modifier: Health -= flat (SetByCaller Data.Damage * 0.2) per tick
- GrantedTag: Effect.DoT.Bleed

─── Buff GameplayEffects ──────────────────────────────────────

GE_Buff_Speed (HasDuration, Duration=SetByCaller Data.Duration)
- Modifier: MovementSpeedMultiplier += 0.3
- GrantedTag: Effect.Buff.Speed
- StackingType: None (replace)

GE_Buff_DamageUp (HasDuration, Duration=SetByCaller Data.Duration)
- Modifier: Strength += SetByCaller Data.Magnitude
- GrantedTag: Effect.Buff.DamageUp

GE_Buff_Shield (HasDuration)
- GrantedTag: State.Invulnerable
- On Expire: remove tag

─── Debuff GameplayEffects ────────────────────────────────────

GE_Debuff_Slow (HasDuration, Duration=SetByCaller Data.Duration)
- Modifier: MovementSpeedMultiplier -= 0.4 (clamp min 0.1)
- GrantedTag: Effect.Debuff.Slow
- Immunity: if target has Effect.Buff.Speed → reduce slow by 50%

GE_Debuff_Stun (HasDuration, Duration=SetByCaller Data.Duration)
- GrantedTag: State.Stunned
- CancelAbilitiesWithTag: Ability.*   ← cancels all active abilities

GE_Debuff_ArmorBreak (HasDuration, Duration=5s)
- Modifier: Armor -= SetByCaller Data.Magnitude
- GrantedTag: Effect.Debuff.ArmorBreak
- StackingType: AggregateBySource, MaxStacks=3

GE_Debuff_Silence (HasDuration, Duration=SetByCaller Data.Duration)
- GrantedTag: State.Silenced
- BlockAbilitiesWithTag: Ability.Fire.*, Ability.Ice.*  ← blocks magic only

─── Cost & Cooldown GameplayEffects ───────────────────────────

GE_Cost_Mana_Base (Instant)
- Modifier: Mana -= SetByCaller Data.Cost

GE_Cost_Stamina_Base (Instant)
- Modifier: Stamina -= SetByCaller Data.Cost

GE_Cooldown_Base (HasDuration)
- Duration: SetByCaller Data.Cooldown
- GrantedTag: set per ability (e.g., Ability.Cooldown.Fireball)

─── Status GameplayEffects ────────────────────────────────────

GE_Death (Instant)
- GrantedTag: State.Dead
- Modifier: Health = 0 (clamp)

GE_SprintStaminaDrain (Infinite, Period=0.1s)
- Modifier: Stamina -= (SprintStaminaDrain * 0.1) per tick
- GrantedTag: State.Sprinting

GE_HealthRegen (Infinite, Period=1s)
- Modifier: Health += (MaxHealth * 0.02) per tick
- Condition: !HasTag(State.InCombat)

GE_ManaRegen (Infinite, Period=1s)
- Modifier: Mana += (MaxMana * 0.03) per tick

GE_StaminaRegen (Infinite, Period=0.2s)
- Modifier: Stamina += (MaxStamina * 0.08) per tick
- Condition: !HasTag(State.Sprinting) && !HasTag(State.Dodging)

Create UDFGameplayEffectLibrary.h/.cpp with all static helpers.
For each GE, create a C++ class with CDO configuration in constructor.
Output all files with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🎬 PROMPT 21 — ANIMATION BLUEPRINT
## ════════════════════════════════════════

```
@ue-animation-system
@ue-character-movement
@ue-gameplay-tags
@ue-gameplay-abilities

Create the complete Animation Blueprint system for DungeonForged UE 5.4.
Follow the Public/Private folder convention:
  .h  → Source/DungeonForged/Public/Animation/<FileName>.h
  .cpp → Source/DungeonForged/Private/Animation/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── UDFAnimInstance (already started in Prompt 18 — extend it) ──
Add to the existing UDFAnimInstance:

Additional cached variables:
- float LeanAngle          ← for banking on turns
- float AimPitch           ← for aim offset (-90 to 90)
- float AimYaw             ← for aim offset (-180 to 180)
- bool bIsAttacking        ← from GameplayTag State.Attacking
- bool bIsCasting          ← from GameplayTag State.Casting
- bool bIsStunned          ← from GameplayTag State.Stunned
- bool bIsLockedOn         ← from GameplayTag State.Targeting
- bool bShouldStrafe       ← true when bIsLockedOn || bIsInCombat
- EMovementDirection MovementDirection  ← Forward/Backward/Left/Right
- float GroundDistance     ← for landing prediction (foot IK)

New functions:
- CalculateLean(float DeltaTime): smooth lean using angular velocity delta
- CalculateAimOffsets(): get AimPitch/Yaw from control rotation vs actor rotation
- DetermineMovementDirection(): 4-way or 8-way based on velocity vs facing
- UpdateFootIK(float DeltaTime): two-bone IK for foot placement on uneven terrain
  using line traces downward from foot sockets

─── Locomotion Blend Space description ────────────────────────
Describe BS_Locomotion_8Way setup:
- X axis: Direction (-180 to 180) — strafing direction
- Y axis: Speed (0 to 700)
- Samples:
  (0,0)      → Idle
  (0,400)    → Walk_Fwd
  (0,700)    → Run_Fwd
  (180,400)  → Walk_Bwd
  (90,400)   → Walk_Right
  (-90,400)  → Walk_Left
  (180,700)  → Run_Bwd
  (90,700)   → Run_Right
  (-90,700)  → Run_Left
- Used only when bShouldStrafe = true (lock-on mode)

Describe BS_Locomotion_Standard setup:
- X axis: Speed (0 to 700) only
- Used when not strafing (free movement)

─── State Machine description ─────────────────────────────────
Describe ABP_Player state machine layers:

Base Layer (full body):
States: Idle → Locomotion → InAir → Land
- Idle: plays idle animation, transitions to Locomotion on Speed > 10
- Locomotion: BlendSpace (Standard or 8Way based on bShouldStrafe)
- InAir: blend falling anim, speed on Y axis
- Land: transition on IsGrounded, plays land montage

Upper Body Layer (Layered Blend per Bone, spine_01 up):
States: None → Aiming → Casting → Dead
- Aiming: AimOffset pose using AimPitch/AimYaw
- Casting: upper body override for cast animations
- Dead: full body ragdoll enable

─── AnimNotify C++ Classes ────────────────────────────────────
Create these additional AnimNotify classes in Public/Animation/:

UDFAnimNotify_EnableRootMotion    → calls CMC->SetMovementMode(MOVE_Custom)
UDFAnimNotify_DisableRootMotion   → restores MOVE_Walking
UDFAnimNotify_FootStep            → plays footstep sound by surface type
  (use PhysicalMaterial to detect surface: Stone, Dirt, Wood, Metal)
UDFAnimNotify_SpawnTrailVFX       → enables weapon trail Niagara system
UDFAnimNotify_DisableTrailVFX     → disables weapon trail

─── UDFAnimInstance_Enemy ─────────────────────────────────────
Separate AnimInstance for enemies:
- Simpler: Speed, bIsInCombat, bIsDead, bIsStunned, HitReactionDirection
- HitReactionDirection: FVector from last hit source (for directional hit anim)
- Function SelectHitMontage(FVector HitDirection): returns correct montage
  based on angle (Front/Back/Left/Right hit reactions)

Output all .h and .cpp files with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 👑 PROMPT 22 — BOSS SYSTEM
## ════════════════════════════════════════

```
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-animation-system
@ue-ui-umg-slate
@ue-niagara-effects
@ue-actor-component-archit...

Create the complete Boss System for DungeonForged UE 5.4.
Follow the Public/Private folder convention:
  .h  → Source/DungeonForged/Public/Boss/<FileName>.h
  .cpp → Source/DungeonForged/Private/Boss/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── ADFBossBase extends ADFEnemyBase ──────────────────────────

Properties:
- int32 CurrentPhase = 1
- int32 MaxPhases = 2
- TArray<float> PhaseThresholds  ← e.g. {0.6f, 0.3f} = 60% and 30% HP
- TArray<TSubclassOf<UDFGameplayAbility>> PhaseAbilities  ← unlocked per phase
- UAnimMontage* PhaseTransitionMontage
- UNiagaraSystem* PhaseTransitionVFX
- float EnrageTimer = 120.f       ← seconds until enrage
- bool bIsEnraged

Functions:
- BeginPlay: start FTimerHandle EnrageTimerHandle
- OnHealthChanged override: check phase thresholds → TriggerPhaseTransition
- TriggerPhaseTransition(int32 NewPhase):
  1. Apply GE_Debuff_Stun to self (brief 1.5s pause — cinematic moment)
  2. Play PhaseTransitionMontage
  3. Spawn PhaseTransitionVFX
  4. Grant PhaseAbilities[NewPhase] to ASC
  5. Apply stat scaling GE (damage +20%, speed +10% per phase)
  6. Broadcast OnBossPhaseChanged delegate
- OnEnrageTimerExpired:
  1. Apply GE_BossEnrage (Infinite: +50% damage, +30% speed, immunity to CC)
  2. Set bIsEnraged = true
  3. Play enrage roar montage + VFX

Signature abilities (create 3 examples):
1. UDFBossAbility_GroundSlam:
   - Melee AOE: sphere overlap at feet, apply GE_Damage_Physical to all in radius
   - Camera shake via UGameplayStatics::PlayWorldCameraShake
   - Spawn ground crack Niagara decal

2. UDFBossAbility_SummonMinions:
   - Spawn 3 ADFEnemyBase at preset spawn sockets around boss
   - Minions get "Spawned.Boss" tag (removed on boss death)
   - Max concurrent minions: 6 (block ability if exceeded)

3. UDFBossAbility_ChargeAttack:
   - Root motion charge toward player
   - UDFMeleeTraceComponent active during charge
   - Hit → apply GE_Damage_Physical + GE_Debuff_Stun(2s)
   - Miss → boss stumbles (play stumble montage, 1s vulnerable window)

─── UDFBossHealthBarWidget extends UDFUserWidgetBase ──────────
Cinematic boss health bar (displayed at bottom of screen):
- UProgressBar* BossHealthBar  (full width, dramatic)
- TArray<UImage*> PhaseMarkers ← vertical lines at threshold percentages
- UTextBlock* BossNameText
- UTextBlock* PhaseText         ← "Phase 2", "ENRAGED"
- UImage* EnrageIcon            ← appears when enraged
- Functions:
  - ShowBossBar(FText BossName, int32 MaxPhases)
  - HideBossBar() with fade-out animation
  - OnPhaseChanged(int32 NewPhase): update PhaseText + animate marker crossing
  - OnEnrage(): pulse red animation on bar

─── ADFBossTriggerVolume ──────────────────────────────────────
Box trigger that starts the boss encounter:
- OnPlayerEnter:
  1. Lock all exit doors (send GameplayEvent to doors)
  2. Play cinematic intro (Level Sequence via ULevelSequencePlayer)
  3. Show WBP_BossHealthBar
  4. Disable player input during cinematic (add UI GameplayTag)
  5. After cinematic: restore input, start boss AI BehaviorTree

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🃏 PROMPT 23 — ABILITY SELECTION SCREEN
## ════════════════════════════════════════

```
@ue-ui-umg-slate
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-data-assets-tables
@ue-serialization-savegames

Create the Ability Selection / Upgrade Screen for DungeonForged UE 5.4.
This is the roguelike core: between floors the player picks 1 of 3 abilities.
Follow the Public/Private folder convention:
  .h  → Source/DungeonForged/Public/UI/<FileName>.h
  .cpp → Source/DungeonForged/Private/UI/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── UDFAbilitySelectionSubsystem (WorldSubsystem) ────────────

Properties:
- UDataTable* AbilityTable   ← DT_Abilities reference
- TArray<FName> PlayerAbilityHistory  ← abilities already owned this run

Functions:
- RollAbilityChoices(int32 Count = 3): 
  1. Get all rows from DT_Abilities
  2. Filter out abilities already owned by player
  3. Apply rarity weighting:
     Common=60%, Uncommon=25%, Rare=12%, Epic=3%
  4. Return TArray<FDFAbilityTableRow> of Count unique rolls
- GrantSelectedAbility(FName AbilityRowName, ADFPlayerCharacter* Player):
  1. Add to PlayerAbilityHistory
  2. Grant GameplayAbility to ASC
  3. Assign to next free ability slot (Ability.Slot.1 → .4)
  4. Notify UDFRunManager of the selection
- SkipSelection(): player can skip (roguelike design choice — small gold reward instead)

─── WBP_AbilitySelection (UDFAbilitySelectionWidget) ─────────
Full-screen overlay shown between floors:

Layout:
- Dark translucent background (blur + vignette)
- Title: "Choose Your Power" (animated fade-in)
- Subtitle: "Floor [X] Complete — Pick one ability"
- 3x UDFAbilityCardWidget side by side
- Skip button (bottom right): "Skip for 50 Gold"
- Timer bar (optional): 30s to choose before auto-skip

UDFAbilityCardWidget (one card per choice):
- UImage* AbilityIcon
- UTextBlock* AbilityName
- UTextBlock* RarityLabel   ← colored by rarity (grey/green/blue/purple/orange)
- UTextBlock* Description
- UTextBlock* CooldownText
- UTextBlock* CostText
- UButton* SelectButton
- Hover: scale up 1.05, glow border color by rarity
- On Click: call SubSystem->GrantSelectedAbility, hide widget, resume game

Functions:
- NativeConstruct: call SubSystem->RollAbilityChoices(3), populate cards
- PopulateCard(int32 Index, FDFAbilityTableRow Data)
- OnAbilitySelected(FName RowName): play select SFX/VFX, close with animation
- OnSkipClicked: add gold, close
- StartTimer(): countdown 30s, OnTimerExpired → auto-skip

─── Integration with DungeonManager ──────────────────────────
In ADFDungeonManager::OnFloorCleared():
1. Pause game (SetGlobalTimeDilation(0) except for UI)
2. Add WBP_AbilitySelection to viewport (ZOrder=10)
3. Set input mode: UI Only
4. On selection complete: restore time, restore game input, AdvanceToNextFloor()

Output all .h and .cpp files with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🚪 PROMPT 24 — INTERACTION SYSTEM
## ════════════════════════════════════════

```
@ue-actor-component-archit...
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-data-assets-tables
@ue-physics-collision
@ue-ui-umg-slate

Create the complete Interaction System for DungeonForged UE 5.4.
Follow the Public/Private folder convention:
  .h  → Source/DungeonForged/Public/Interaction/<FileName>.h
  .cpp → Source/DungeonForged/Private/Interaction/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── IDFInteractable (UInterface) ──────────────────────────────
- CanInteract(ACharacter* Interactor): bool
- Interact(ACharacter* Interactor): void
- GetInteractionText(): FText   ← shown in prompt UI ("Open Chest", "Activate Shrine")
- GetInteractionRange(): float

─── ADFInteractableBase (AActor) ──────────────────────────────
Implements IDFInteractable.

Properties:
- USphereComponent* InteractionRange   (radius from GetInteractionRange())
- UStaticMeshComponent* Mesh
- UWidgetComponent* InteractionPromptWidget  ← WBP_InteractionPrompt above actor
- bool bIsInteractable = true
- bool bSingleUse = true

Functions:
- OnRangeBeginOverlap: show WBP_InteractionPrompt, register to UDFInteractionComponent
- OnRangeEndOverlap: hide prompt, unregister
- Interact_Implementation: set bIsInteractable=false if bSingleUse, play interact anim

─── UDFInteractionComponent (ActorComponent on Player) ────────

Properties:
- float InteractTraceRange = 300.f
- TWeakObjectPtr<AActor> CurrentFocusedActor
- TArray<TWeakObjectPtr<AActor>> NearbyInteractables

Functions:
- TickComponent: sphere check + line trace to find best interactable in front
  → set CurrentFocusedActor, update prompt visibility
- TryInteract(): called from IA_Interact binding
  → if CurrentFocusedActor valid → Execute(IDFInteractable::Interact)
- RegisterInteractable(AActor* Actor)
- UnregisterInteractable(AActor* Actor)

─── ADFChest extends ADFInteractableBase ──────────────────────
Property:
- FName LootTableRow   ← from DT_Items, determines drop pool
- EItemRarity MinRarity ← floor-scaled minimum rarity
- UAnimMontage* OpenMontage
- UParticleSystem* OpenVFX
- bool bIsOpen = false

Interact_Implementation:
1. if bIsOpen → return
2. Play OpenMontage on SkeletalMesh
3. Spawn OpenVFX
4. Call UDFLootGeneratorSubsystem::RollLoot (2-4 items)
5. Spawn ADFLootDrop actors with physics scatter impulse
6. bIsOpen = true

─── ADFDoor extends ADFInteractableBase ───────────────────────
Types (EDoorType): KeyDoor, LeverDoor, ExitDoor, BossDoor

Properties:
- EDoorType DoorType
- bool bIsLocked = false
- bool bIsOpen = false
- UTimelineComponent* OpenTimeline   ← smooth door open animation

Functions:
- Interact_Implementation: if !bIsLocked → OpenDoor(); else → show "Locked" hint
- OpenDoor(): play timeline → lerp door mesh rotation 0→90 degrees
- LockDoor() / UnlockDoor(): called by BossTriggerVolume
- OnGameplayEvent("Event.Door.Open"): listens for remote open (boss death → unlock)

─── ADFShrine extends ADFInteractableBase ─────────────────────
Shrine types (EShrineType): Healing, Mana, PowerUp, Mystery

Properties:
- EShrineType ShrineType
- TSubclassOf<UGameplayEffect> ShrineEffect
- float CooldownBetweenUses = 0  ← 0 = single use (bSingleUse=true)
- UNiagaraSystem* ActiveVFX
- UNiagaraSystem* UsedVFX

Interact_Implementation:
1. Apply ShrineEffect to player via ASC
2. Swap ActiveVFX to UsedVFX (shrine dims)
3. For Mystery: roll random effect from: Heal 50% HP | +1 random ability level | random buff 60s

─── WBP_InteractionPrompt ─────────────────────────────────────
UDFInteractionPromptWidget extends UUserWidget:
- UTextBlock* ActionText      ← "[G] Open Chest"
- UTextBlock* InteractorHint  ← small subtitle if needed
- UImage* KeyIcon             ← keyboard key icon
- Animate: bob up/down 5px, fade in on register
- UpdatePrompt(FText ActionText, UTexture2D* KeyIcon)

Output all .h and .cpp files with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## ⚔️ PROMPT 25 — ABILITIES: GUERREIRO (WARRIOR)
## ════════════════════════════════════════

```
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-animation-system
@ue-niagara-effects
@ue-physics-collision

Create all Warrior class abilities for DungeonForged UE 5.4.
All abilities extend UDFGameplayAbility.
Path convention:
  .h  → Source/DungeonForged/Public/GAS/Abilities/Warrior/<FileName>.h
  .cpp → Source/DungeonForged/Private/GAS/Abilities/Warrior/<FileName>.cpp
Always write the full path as a comment at the top of each file.
Already exists: GA_Dodge (Prompt 18), Basic Melee Combo (Prompt 17).
Do NOT recreate those. Create only the abilities below.

─── GA_ShieldBash ─────────────────────────────────────────────
Tag: Ability.Warrior.ShieldBash
Cost: 20 Stamina | Cooldown: 8s
Requirement: Player must have Tag "Equipment.OffHand.Shield"

Flow:
1. CommitAbility
2. PlayMontageAndWait (ShieldBashMontage)
3. On AnimNotify AN_TraceStart:
   - BoxTrace in front of player (50x50x80cm)
   - Hit enemies: apply GE_Damage_Physical (SetByCaller 40 + Strength*0.5)
   - Apply GE_Debuff_Stun (1.5s) to each hit enemy via ASC
   - Apply GE_Buff_Shield (0.5s invulnerability) to self
4. Camera shake (light, 0.3s)
5. EndAbility on montage end

─── GA_WarCry ─────────────────────────────────────────────────
Tag: Ability.Warrior.WarCry
Cost: 30 Mana | Cooldown: 45s
Type: AOE Buff — no damage

Flow:
1. CommitAbility
2. PlayMontageAndWait (WarCryMontage — roar animation)
3. On Montage blend out:
   - Sphere overlap radius=800, filter allies only
   - Apply GE_Buff_DamageUp (Strength +25, duration 10s) to self + allies in range
   - Apply GE_Buff_Speed (+15% speed, duration 10s) to self + allies
   - Spawn WarCry Niagara VFX (gold burst radiating outward)
   - Play WarCry SFX (roar)
4. Broadcast OnWarCryActivated delegate (for AI enemies to react — flee check)
5. EndAbility

─── GA_Whirlwind ──────────────────────────────────────────────
Tag: Ability.Warrior.Whirlwind
Cost: 40 Mana + 20 Stamina | Cooldown: 12s
Type: Channeled AOE melee spin

Flow:
1. CommitAbility
2. Apply GE_Whirlwind_Active (Infinite, grants Tag State.Spinning)
3. PlayMontageAndWait (WhirlwindMontage — looping spin segment)
4. While active (WaitGameplayEvent "Event.Ability.Whirlwind.Tick" from AnimNotify):
   - CapsuleOverlapMulti radius=200 around player
   - Hit each unique enemy once per 0.3s (track with TSet + FTimerHandle per enemy)
   - Apply GE_Damage_Physical (SetByCaller 25 + Strength*0.3) per tick
   - Spawn hit sparks Niagara at each impact point
5. Player movement: allow slow movement during spin (50% speed via CMC override)
6. On Input Released (WaitInputRelease task) OR 3s max duration:
   - RemoveGameplayEffectByHandle (GE_Whirlwind_Active)
   - Play spin-stop blend montage
   - EndAbility

─── GA_IronSkin ───────────────────────────────────────────────
Tag: Ability.Warrior.IronSkin
Cost: 35 Mana | Cooldown: 25s
Type: Defensive cooldown

Flow:
1. CommitAbility
2. Apply GE_IronSkin (HasDuration 6s):
   - Modifier: Armor += 80
   - Modifier: MagicResist += 40
   - GrantedTag: State.Invulnerable (first 0.5s — absorption startup)
   - Period 0.5s: check if Health < 30% → also apply GE_HealthRegen_Burst
3. Spawn IronSkin VFX (metallic shimmer over character, grey/silver tones)
4. Play IronSkin AnimMontage (flex pose, brief)
5. WaitGameplayEffectRemoved (GE_IronSkin handle):
   - Play shatter VFX when buff expires
   - EndAbility

─── GA_Charge ─────────────────────────────────────────────────
Tag: Ability.Warrior.Charge
Cost: 25 Stamina | Cooldown: 14s
Requirement: Target must be 400–2000 units away (checked in CanActivateAbility)

Flow:
1. CanActivateAbility: confirm target via LockOnComponent->CurrentTarget
   AND distance in [400, 2000] range; else fail with GameplayEvent "Ability.Failed.Range"
2. CommitAbility
3. Apply GE_Charge_IFrame (0.5s invulnerable during travel)
4. Get target location, compute direction
5. Launch character: LaunchCharacter(Direction * 2200, true, true)
6. WaitMovementModeChange (landing) OR WaitDelay(0.6s):
   - Sphere overlap at arrival (radius=120)
   - Apply GE_Damage_Physical (SetByCaller 60 + Strength) to all in radius
   - Apply GE_Debuff_Stun (1.2s) to primary target
   - Spawn ground-slam Niagara on landing
   - Camera shake (heavy, 0.5s)
7. Play charge-land montage
8. Remove GE_Charge_IFrame
9. EndAbility

─── GA_Execute ────────────────────────────────────────────────
Tag: Ability.Warrior.Execute
Cost: 50 Mana | Cooldown: 20s
Requirement: Target Health < 25% (checked in CanActivateAbility)

Flow:
1. CanActivateAbility:
   - Must have CurrentTarget via LockOnComponent
   - Target must have Health attribute < 25% of MaxHealth
2. CommitAbility
3. PlayMontageAndWait (ExecuteMontage — dramatic overhead slam)
4. On AnimNotify AN_TraceStart:
   - Single target hit on CurrentTarget
   - Damage = (Strength * 3.0) + (Target.MissingHealth * 0.5) ← executes harder at lower HP
   - Apply via UDFDamageCalculation with custom ExecuteMultiplier SetByCaller
   - If this kills the target:
     → Apply GE_Buff_ExecuteReset: refund 50 Mana, reduce Charge cooldown by 8s
     → Spawn death-blow VFX (blood burst, slow-motion hit-stop 0.1s TimeDialation)
5. EndAbility on montage end

Output all 5 ability .h and .cpp files with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🔮 PROMPT 26 — ABILITIES: MAGO (MAGE)
## ════════════════════════════════════════

```
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-animation-system
@ue-niagara-effects
@ue-physics-collision

Create all Mage class abilities for DungeonForged UE 5.4.
All abilities extend UDFGameplayAbility.
Path convention:
  .h  → Source/DungeonForged/Public/GAS/Abilities/Mage/<FileName>.h
  .cpp → Source/DungeonForged/Private/GAS/Abilities/Mage/<FileName>.cpp
Always write the full path as a comment at the top of each file.
Already exists: GA_Fireball (Prompt 12). Do NOT recreate it.

─── GA_FrostBolt ──────────────────────────────────────────────
Tag: Ability.Mage.FrostBolt
Cost: 25 Mana | Cooldown: 4s
Type: Targeted projectile — slows on hit

Flow:
1. CommitAbility
2. PlayMontageAndWait (FrostBoltCastMontage)
3. On AnimNotify AN_TraceStart:
   - Spawn ADFFrostBoltProjectile at muzzle socket
   - Projectile: speed=2400, homing toward LockOnTarget if active
4. ADFFrostBoltProjectile OnHit:
   - Apply GE_Damage_Magic (Intelligence * 0.8)
   - Apply GE_Debuff_Slow (50% slow, 3s) to target
   - Spawn FrostHit Niagara (ice shatter, blue crystals)
   - If target already has Effect.DoT.Frost tag → apply GE_Freeze (root 1.5s) instead of slow
5. EndAbility on montage end

─── GA_BlizzardStorm ──────────────────────────────────────────
Tag: Ability.Mage.BlizzardStorm
Cost: 80 Mana | Cooldown: 30s
Type: AOE ground-target DoT zone (WoW Blizzard style)

Flow:
1. CommitAbility
2. PlayMontageAndWait (BlizzardCastMontage — arms raise, channeling)
3. GetGroundTargetLocation: decal indicator (8m radius) follows cursor/aim direction
   - Use WaitTargetData task with ADFGroundTargetActor (reticle)
   - On Target confirmed: proceed
4. Spawn ADFBlizzardZone actor at confirmed location:
   - Duration: 8s
   - Period: 0.5s tick
   - Each tick: SphereOverlap radius=400
     → Apply GE_Damage_Magic (Intelligence * 0.4) to each enemy
     → Apply GE_DoT_Frost (1s, stackable) to each enemy
     → Spawn snowflake Niagara burst per tick
   - Zone has UDecalComponent (frost circle on ground)
5. WaitDelay(8s) → EndAbility (zone handles its own destroy)

─── GA_ArcaneBarrage ──────────────────────────────────────────
Tag: Ability.Mage.ArcaneBarrage
Cost: 15 Mana per charge | Cooldown: 1.5s per charge | MaxCharges: 3
Type: Stacking charge ability — rapid fire magic missiles

Setup:
- Use GAS ability charge system: MaxCharges=3, ChargeRegenTime=6s
- Each activation consumes 1 charge

Flow (per cast):
1. CommitAbility (1 charge)
2. PlayMontageAndWait (ArcaneQuickCastMontage — fast hand flick, 0.4s)
3. On AnimNotify AN_TraceStart:
   - Spawn 3 ADFArcaneMissileProjectile with small spread (±5°)
   - Each missile: speed=3000, homing=true (soft homing 60°/s)
4. Each missile OnHit:
   - Apply GE_Damage_Magic (Intelligence * 0.4) per missile
   - Spawn arcane pop Niagara (purple spark)
   - If all 3 hit same target: trigger Arcane Overload
     → Extra GE_Damage_Magic (Intelligence * 1.2) + GE_Debuff_Silence (2s)
5. EndAbility (instant — fast cast feel)

─── GA_TimeWarp ───────────────────────────────────────────────
Tag: Ability.Mage.TimeWarp
Cost: 60 Mana | Cooldown: 60s
Type: Signature ultimate — resets all ability cooldowns

Flow:
1. CommitAbility
2. PlayMontageAndWait (TimeWarpMontage — dramatic rewind gesture, 1.2s)
3. VFX: swirling clock/time Niagara around player, purple+gold palette
4. Post-montage:
   - Get all active cooldown GameplayEffects on self ASC
   - Filter by tag "Ability.Cooldown.*"
   - RemoveActiveGameplayEffect for each → ALL abilities reset
   - Apply GE_Buff_TimeWarpHaste (HasDuration 8s):
     → CooldownReduction += 0.5 (50% CDR for 8s — double-cast window)
     → IntelligenceBonusMultiplier += 0.3
5. Spawn grand VFX pulse (time ripple outward)
6. EndAbility

─── GA_ManaShield ─────────────────────────────────────────────
Tag: Ability.Mage.ManaShield
Cost: 0 Mana (but drains Mana when hit) | Cooldown: 20s
Type: Reactive defense — absorbs damage using Mana instead of Health

Flow:
1. CommitAbility (no cost)
2. Apply GE_ManaShield_Active (Infinite until cancelled):
   - GrantedTag: State.ManaShieldActive
3. Spawn shield bubble VFX around player (transparent blue sphere)
4. In UDFAttributeSet::PostGameplayEffectExecute:
   - If incoming damage AND target HasTag(State.ManaShieldActive):
     → Redirect 70% of damage from Health to Mana drain
     → Remaining 30% hits Health normally
     → If Mana hits 0: force-remove GE_ManaShield_Active + play shield-break VFX
5. WaitGameplayEffectRemoved OR WaitInputAction(IA_Ability toggle):
   - RemoveGameplayEffect if player manually toggles off
   - Hide bubble VFX
6. EndAbility

─── GA_Teleport ───────────────────────────────────────────────
Tag: Ability.Mage.Teleport
Cost: 40 Mana | Cooldown: 10s
Type: Blink — instant repositioning (replaces Dodge for Mage)

Flow:
1. CanActivateAbility: NOT HasTag(State.Dead) AND NOT HasTag(State.Stunned)
2. CommitAbility
3. Determine blink destination:
   - If LockOnTarget active: blink BEHIND target (TargetLocation - TargetForward*150)
   - Else: blink in input direction 700 units (or toward camera forward)
   - Validate with NavigationSystem->ProjectPointToNavigation
4. Apply GE_Teleport_IFrame (0.3s invulnerable)
5. Spawn departure VFX at current location (vanish puff, purple)
6. TeleportTo(DestinationLocation) — instant, no interpolation
7. Spawn arrival VFX at destination (appear puff, purple)
8. Remove GE_Teleport_IFrame
9. EndAbility (immediate — no montage, feels snappy)

Bonus: if Teleport is used while an enemy is casting (target HasTag State.Casting):
- Apply GE_Damage_Magic (Intelligence * 1.5) on arrival — "Spellsteal interrupt"

Output all 6 ability .h and .cpp files with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🗡️ PROMPT 27 — ABILITIES: ASSASSINO (ROGUE)
## ════════════════════════════════════════

```
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-animation-system
@ue-niagara-effects
@ue-physics-collision
@ue-character-movement

Create all Rogue/Assassin class abilities for DungeonForged UE 5.4.
All abilities extend UDFGameplayAbility.
Path convention:
  .h  → Source/DungeonForged/Public/GAS/Abilities/Rogue/<FileName>.h
  .cpp → Source/DungeonForged/Private/GAS/Abilities/Rogue/<FileName>.cpp
Always write the full path as a comment at the top of each file.
Already exists: GA_Dodge (Prompt 18). Do NOT recreate it.

─── Resource: Combo Points System ────────────────────────────
Before the abilities, create UDFComboPointsComponent (ActorComponent):
- int32 CurrentComboPoints = 0 (max 5)
- Broadcast OnComboPointsChanged(int32 New, int32 Max)
- AddComboPoints(int32 Amount): clamp to 5, broadcast
- SpendComboPoints(int32 Amount): returns bool (enough points?)
- ResetComboPoints(): set 0, broadcast
(Combo Points are the Rogue resource — builders add, finishers spend)

─── GA_Backstab ───────────────────────────────────────────────
Tag: Ability.Rogue.Backstab
Cost: 15 Energy (use Stamina as Energy) | Cooldown: 0s (limited by GCD 1s)
Type: Builder — positional melee strike, must be behind target

Flow:
1. CanActivateAbility:
   - Check position: dot(PlayerForward, TargetForward) > 0.5 (player is behind target)
   - If not behind: ability still activates but damage is 50% (front stab)
2. CommitAbility
3. PlayMontageAndWait (BackstabMontage — quick lunge animation, 0.5s)
4. On AN_TraceStart:
   - Single target melee trace
   - IsBehind: Damage = Agility * 2.0 + BaseDamage (full)
   - IsFront:  Damage = Agility * 1.0 + BaseDamage (half)
   - Apply GE_Damage_Physical with SetByCaller
   - Apply GE_DoT_Bleed (3s) always
   - AddComboPoints(1) via UDFComboPointsComponent
   - If IsBehind: AddComboPoints(2) instead (extra point for positional)
5. EndAbility

─── GA_FanOfKnives ────────────────────────────────────────────
Tag: Ability.Rogue.FanOfKnives
Cost: 25 Stamina | Cooldown: 8s
Type: AOE burst — hits all nearby enemies

Flow:
1. CommitAbility
2. PlayMontageAndWait (FanOfKnivesMontage — arms spread wide, 0.6s)
3. On AN_TraceStart:
   - SphereOverlap radius=350 around player
   - For each enemy: spawn ADFKnifeProjectile with random spread angle (360°, N knives = overlap count)
     Each knife: speed=2800, no homing, destroyOnHit=true
4. Each knife OnHit:
   - Apply GE_Damage_Physical (Agility * 0.6) per knife
   - Apply GE_DoT_Poison (3s) to target (stackable)
   - Spawn knife-impact Niagara (blade glint)
5. AddComboPoints(1) regardless of hit count
6. EndAbility on montage end

─── GA_ShadowStep ─────────────────────────────────────────────
Tag: Ability.Rogue.ShadowStep
Cost: 30 Stamina | Cooldown: 12s
Type: Gap closer — teleport directly behind target (offensive blink)

Flow:
1. CanActivateAbility: must have LockOnTarget
2. CommitAbility
3. Apply GE_ShadowStep_Stealth (0.5s):
   - GrantedTag: State.Invisible
   - Modifier: MovementSpeedMultiplier += 0.5
4. Spawn shadow trail Niagara at start position (dark smoke dissipating)
5. TeleportTo(TargetLocation - TargetForward * 120 + FVector(0,0,10))
   (directly behind target, slight Z offset to avoid floor clip)
6. Face target: SetActorRotation toward target
7. Spawn arrival shadow Niagara (dark burst)
8. Remove GE_ShadowStep_Stealth
9. Auto-activate GA_Backstab if target is alive (chained ability):
   - TryActivateAbilityByTag("Ability.Rogue.Backstab") with bAllowRemoteActivation=false
10. AddComboPoints(2)
11. EndAbility

─── GA_Eviscerate ─────────────────────────────────────────────
Tag: Ability.Rogue.Eviscerate
Cost: 0 Stamina (free) | Cooldown: 1s
Type: Finisher — consumes ALL Combo Points for massive bleed

Flow:
1. CanActivateAbility: ComboPointsComponent->CurrentComboPoints >= 1
2. CommitAbility
3. Cache int32 PointsSpent = ComboPointsComponent->CurrentComboPoints
4. ComboPointsComponent->ResetComboPoints()
5. PlayMontageAndWait (EviscerateMontage — vicious slashing sequence, 0.8s)
6. On AN_TraceStart:
   - Single target hit
   - BaseDamage = Agility * 1.5
   - BonusDamage = PointsSpent * Agility * 0.8  ← scales with combo points
   - TotalDamage = BaseDamage + BonusDamage
   - Apply GE_Damage_Physical (TotalDamage)
   - Apply GE_DoT_Bleed:
     → Duration = 2 + (PointsSpent * 1)  ← more points = longer bleed
     → Damage per tick = Agility * 0.3 * PointsSpent
   - Spawn eviscerate Niagara (deep red slash trail, proportional to points)
   - If PointsSpent == 5: trigger Killing Spree bonus
     → Apply GE_Buff_KillingSpree (5s): +40% Agility, +20% attack speed
7. EndAbility

─── GA_Vanish ─────────────────────────────────────────────────
Tag: Ability.Rogue.Vanish
Cost: 50 Stamina | Cooldown: 45s
Type: Escape / reset — full stealth, breaks combat

Flow:
1. CommitAbility
2. Apply GE_Vanish_Stealth (HasDuration 8s OR until combat action):
   - GrantedTag: State.Invisible, State.Stealthed
   - Modifier: MovementSpeedMultiplier += 0.2
   - CancelIfDamaged: bind to Health attribute change → if damage received → RemoveEffect
3. Spawn vanish smoke VFX (character dissolves with smoke puff)
4. Set mesh visibility: 30% opacity via dynamic material (not fully invisible — WoW style)
5. Notify all enemies via GameplayEvent "Event.Stealth.Entered":
   - Enemy AI: clear BB_TargetActor if it was this player
   - Enemies return to Patrol state
6. If player attacks while stealthed (any Ability with Tag Ability.Attack.*):
   - RemoveEffect GE_Vanish_Stealth
   - First attack out of stealth: apply GE_Buff_ShadowStrike (1 hit):
     → Next damage multiplied by 2.0
     → Automatic AddComboPoints(3) (ambush bonus)
7. WaitGameplayEffectRemoved OR WaitDelay(8s):
   - Show character at full opacity
   - EndAbility

─── GA_SmokeScreen ────────────────────────────────────────────
Tag: Ability.Rogue.SmokeScreen
Cost: 20 Stamina | Cooldown: 20s
Type: AOE utility — area denial, blinds enemies

Flow:
1. CommitAbility
2. PlayMontageAndWait (ThrowMontage — quick underarm toss, 0.3s)
3. Spawn ADFSmokeBombActor at aim direction (short arc throw 300 units):
   - Duration: 5s
   - UNiagaraComponent: dense smoke cloud, radius=300
   - Every 0.3s tick: SphereOverlap — for each enemy in smoke:
     → Apply GE_Debuff_Blind (0.5s renewable): 
       AI: set BB_bCanSeeTarget = false even if player is visible
       Reduces enemy accuracy (add GameplayTag Effect.Debuff.Blinded)
4. Player inside smoke: gains GE_Buff_SmokeCover (while overlapping):
   - GrantedTag: State.Concealed
   - Enemy AI will not detect player while in smoke (override perception)
5. ADFSmokeBombActor destroys itself after 5s + spawns clear VFX
6. EndAbility immediately after throw (fire and forget)

Output all 7 files (UDFComboPointsComponent + 6 abilities) with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## ✨ PROMPT 28 — ABILITIES: PASSIVAS E AURAS
## ════════════════════════════════════════

```
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-actor-component-archit...
@ue-niagara-effects

Create all Passive Abilities and Aura system for DungeonForged UE 5.4.
All abilities extend UDFGameplayAbility with ActivationPolicy = Passive.
Path convention:
  .h  → Source/DungeonForged/Public/GAS/Abilities/Passive/<FileName>.h
  .cpp → Source/DungeonForged/Private/GAS/Abilities/Passive/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── Passive Ability base pattern ──────────────────────────────
For ALL passive abilities:
- ActivationPolicy = Passive (auto-activated when granted)
- ActivateAbility: Apply persistent GE_<PassiveName>_Aura (Infinite)
  + store ActiveEffectHandle
- OnRemoveAbility override: RemoveActiveGameplayEffect(ActiveEffectHandle)
- No cost, no cooldown, no input binding

─── GA_Passive_Warrior_Fortitude ──────────────────────────────
Tag: Ability.Passive.Warrior.Fortitude
Class: Warrior only (granted by DT_Classes Warrior row)

Effect GE_Fortitude (Infinite):
- Modifier: MaxHealth += (Strength * 5)     ← scales with Strength
- Modifier: Armor += 10 flat
- Period 5s: if Health < 50% → apply GE_Buff_BattleFury (10s):
  → Strength += 15 (enrage when wounded)

─── GA_Passive_Warrior_Retaliation ────────────────────────────
Tag: Ability.Passive.Warrior.Retaliation

Trigger-based passive (different pattern — use GameplayEvent):
- ActivateAbility: WaitGameplayEvent("Event.Hit.Received") loop
- On each hit received:
  → If damage > (MaxHealth * 0.05): counter-apply GE_Damage_Physical
    to attacker (Strength * 0.3) — automatic riposte
  → Spawn small riposte spark VFX on attacker

─── GA_Passive_Mage_ArcaneMastery ─────────────────────────────
Tag: Ability.Passive.Mage.ArcaneMastery
Class: Mage only

Effect GE_ArcaneMastery (Infinite):
- Modifier: Intelligence += (Level * 3)     ← scales with character level
- Modifier: CooldownReduction += 0.05       ← passive 5% CDR
- Modifier: MaxMana += (Intelligence * 2)

─── GA_Passive_Mage_ManaVortex ────────────────────────────────
Tag: Ability.Passive.Mage.ManaVortex

Trigger-based passive:
- WaitGameplayEvent("Event.Ability.Kill") — fires when player kills with ability
- On kill: apply GE_ManaVortex_Refund (Instant):
  → Mana += 20 flat refund (every kill restores mana)
- Spawn mana orb VFX sucked into player on kill

─── GA_Passive_Rogue_Predator ─────────────────────────────────
Tag: Ability.Passive.Rogue.Predator
Class: Rogue only

Effect GE_Predator (Infinite):
- Modifier: MovementSpeedMultiplier += 0.08  ← always faster
- Modifier: CritChance += 0.10              ← +10% crit base

─── GA_Passive_Rogue_BleedMastery ─────────────────────────────
Tag: Ability.Passive.Rogue.BleedMastery

Trigger-based passive:
- WaitAttributeChange: watch for GE_DoT_Bleed applied to any target by this player
- On Bleed applied to a new target:
  → Enhance the bleed: AddActiveGameplayEffectGrantedTagsForDuration
    → Extra modifier: Bleed tick damage += Agility * 0.2
  → If target already has 2+ bleeds: apply GE_Debuff_ArmorBreak (3s)
    (multiple bleeds = armor destroyed)

─── UDFAuraComponent ──────────────────────────────────────────
ActorComponent for persistent aura zone effects (different from passive GEs):

Properties:
- float AuraRadius = 600.f
- float AuraTickRate = 1.0f
- TSubclassOf<UGameplayEffect> FriendlyAuraEffect
- TSubclassOf<UGameplayEffect> EnemyAuraEffect (optional — debuff aura)
- UNiagaraSystem* AuraVFX
- UNiagaraComponent* ActiveAuraVFX

Functions:
- BeginPlay: spawn AuraVFX, start FTimerHandle TickAura
- TickAura: SphereOverlap(AuraRadius)
  → Apply FriendlyAuraEffect to allies (if set)
  → Apply EnemyAuraEffect to enemies (if set)
  → Only apply if target does NOT already have the effect active
- SetAuraRadius(float NewRadius): update overlap + scale VFX
- Enable/Disable: pause/resume timer, hide/show VFX

Example Auras using UDFAuraComponent (describe setup, not full classes):
- Aura_HolyGround (Paladin future): radius=500, allies regen 2% HP/s
- Aura_FrostPresence (Mage item): radius=400, enemies get GE_Debuff_Slow 20%
- Aura_BattlefieldDread (Boss): radius=800, players get GE_Debuff_Weaken 15%

Output all passive ability files + UDFAuraComponent.h/.cpp with correct paths.
```

---

## ════════════════════════════════════════
## 🌟 PROMPT 29 — ABILITIES: UNIVERSAIS E UTILITÁRIAS
## ════════════════════════════════════════

```
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-animation-system
@ue-niagara-effects
@ue-ui-umg-slate

Create Universal and Utility abilities for DungeonForged UE 5.4.
These are available to ALL classes as drops/pickups during the roguelike run.
All extend UDFGameplayAbility.
Path convention:
  .h  → Source/DungeonForged/Public/GAS/Abilities/Universal/<FileName>.h
  .cpp → Source/DungeonForged/Private/GAS/Abilities/Universal/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── GA_HealthPotion ───────────────────────────────────────────
Tag: Ability.Universal.HealthPotion
Cost: 0 | Cooldown: 30s
Type: Consumable ability (granted by item pickup, limited charges)

Properties (additional):
- int32 Charges = 3        ← set from item row
- int32 HealPercent = 40   ← heals 40% of MaxHealth

Flow:
1. CanActivateAbility: Charges > 0
2. CommitAbility (no resource cost)
3. Charges--
4. PlayMontageAndWait (DrinkPotionMontage — quick sip, 0.4s)
5. Apply GE_HealthPotion_Heal (Instant):
   - SetByCaller Data.Healing = MaxHealth * (HealPercent / 100)
6. Spawn healing VFX (green particles rising from player)
7. Update HUD potion counter: broadcast OnChargesChanged(Charges)
8. If Charges == 0: RemoveAbilityFromASC (auto-removes when empty)
9. EndAbility

─── GA_SecondWind ─────────────────────────────────────────────
Tag: Ability.Universal.SecondWind
Cost: 0 | Cooldown: 120s (long — emergency button)
Type: Cheat death passive trigger (auto-activates)

Flow (passive trigger pattern):
1. ActivationPolicy = Passive
2. ActivateAbility: WaitAttributeChange Health to 0 or below (death threshold)
3. On trigger:
   - BEFORE death broadcast fires: intercept via UDFAttributeSet flag bSecondWindAvailable
   - Apply GE_SecondWind_Revival (Instant):
     → Health = MaxHealth * 0.25 (25% HP restored)
     → Apply GE_Buff_Shield (3s invulnerable recovery window)
   - Apply GE_SecondWind_Cooldown (HasDuration 120s):
     → GrantedTag: Ability.Cooldown.SecondWind (blocks re-trigger)
   - Spawn dramatic revival VFX (golden burst, screen flash white)
   - Camera shake (heavy, 0.8s)
   - Broadcast OnSecondWindActivated (HUD: show "SECOND WIND!" text popup)
4. After revival: re-WaitAttributeChange for next potential trigger
   (but blocked by Ability.Cooldown.SecondWind tag until 120s)

─── GA_BattleHymn ─────────────────────────────────────────────
Tag: Ability.Universal.BattleHymn
Cost: 45 Mana | Cooldown: 35s
Type: Self-buff — short burst of all-stats boost

Flow:
1. CommitAbility
2. PlayMontageAndWait (BattleHymnMontage — shout animation, 0.5s)
3. Apply GE_BattleHymn (HasDuration 12s):
   - Modifier: Strength += 20
   - Modifier: Intelligence += 20
   - Modifier: Agility += 20
   - Modifier: CritChance += 0.15
   - Modifier: CooldownReduction += 0.20
   - GrantedTag: Effect.Buff.BattleHymn
4. Spawn aura VFX (multicolor glow cycling through class colors)
5. WaitGameplayEffectRemoved:
   - Play buff-end audio (fades out)
   - EndAbility

─── GA_Siphon ─────────────────────────────────────────────────
Tag: Ability.Universal.Siphon
Cost: 20 Mana | Cooldown: 6s
Type: Life steal strike — heals player for damage dealt

Flow:
1. CommitAbility
2. PlayMontageAndWait (SiphonMontage — draining grasp animation, 0.6s)
3. On AN_TraceStart:
   - Single target melee/ranged trace (60% Strength + 40% Intelligence hybrid)
   - Apply GE_Damage_True (bypasses armor — soul drain)
   - Calculate DamageDealt via ExecutionCalculation result
   - Apply GE_Siphon_Heal (Instant): Health += DamageDealt * 0.6
   - Spawn life-drain VFX: red tendrils from target to player
   - If target killed: bonus GE_Siphon_Overheal → Health += MaxHealth * 0.1
     (overheal as temporary shield if Health > Max)
4. EndAbility

─── GA_Berserk ────────────────────────────────────────────────
Tag: Ability.Universal.Berserk
Cost: 0 | Cooldown: 90s
Type: Ultimate berserk mode — risk/reward mechanic

Flow:
1. CommitAbility
2. Apply GE_Berserk_Active (HasDuration 10s):
   - Modifier: Strength += 60
   - Modifier: Agility += 40
   - Modifier: MovementSpeedMultiplier += 0.4
   - Modifier: Armor -= 50            ← defense drops!
   - Modifier: MagicResist -= 30      ← magic defense drops!
   - GrantedTag: State.Berserk
   - Period 1s: Apply GE_Damage_True to SELF (Health -= MaxHealth * 0.02)
     (berserk costs health every second — must kill or die)
3. Spawn berserk VFX (red aura, distortion, screen edge vignette red)
4. Override camera: increase FOV from 90 to 100 smoothly
5. Change character material to red emissive overlay
6. WaitGameplayEffectRemoved:
   - Restore FOV to 90
   - Remove material overlay
   - If player survived: apply GE_BerserkExhaustion (5s):
     → Strength -= 20, Speed -= 0.2 (crash after berserk)
   - EndAbility

─── GA_CallLightning ──────────────────────────────────────────
Tag: Ability.Universal.CallLightning
Cost: 55 Mana | Cooldown: 18s
Type: Ground-target instant strike

Flow:
1. CommitAbility
2. PlayMontageAndWait (LightningCastMontage — arm raised, 0.7s)
3. GetAimTargetLocation: line trace from camera forward 2000 units
4. Spawn ADFLightningStrikeActor at hit location:
   - Instant: no projectile travel
   - CapsuleOverlap radius=200, height=600
   - Apply GE_Damage_Magic (Intelligence * 1.8) to all in capsule
   - Apply GE_Debuff_Stun (0.8s) to all hit
   - Spawn lightning bolt Niagara (top-down strike, branching arcs)
   - Camera shake (moderate, 0.4s)
   - If secondary target within 400 units: chain bolt → apply GE_Damage_Magic
     (Intelligence * 0.8) to chain target (1 chain max)
5. EndAbility on montage end

Output all 6 Universal ability .h and .cpp files with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🐉 PROMPT 30 — ABILITIES: BOSS EXCLUSIVAS
## ════════════════════════════════════════

```
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-animation-system
@ue-niagara-effects
@ue-physics-collision
@ue-character-movement

Create Boss-exclusive abilities for DungeonForged UE 5.4.
These are granted ONLY to ADFBossBase derived actors.
All extend UDFGameplayAbility with bSourceObjectMustBeBoss = true (custom check).
Path convention:
  .h  → Source/DungeonForged/Public/GAS/Abilities/Boss/<FileName>.h
  .cpp → Source/DungeonForged/Private/GAS/Abilities/Boss/<FileName>.cpp
Already exists in Prompt 22: GroundSlam, SummonMinions, ChargeAttack. Do NOT recreate.

─── GA_Boss_TerrorShout ───────────────────────────────────────
Tag: Ability.Boss.TerrorShout
Cooldown: 30s | Type: AOE CC + debuff
Phase: Available from Phase 1

Flow:
1. PlayMontageAndWait (BossShoutMontage — head rears back, massive roar)
2. Camera shake (very heavy, 1.5s, radial falloff)
3. SphereOverlap radius=1200:
   - Apply GE_Debuff_TerrorStruck (HasDuration 3s) to player if in range:
     → MovementSpeedMultiplier -= 0.5
     → GrantedTag: Effect.Debuff.Terrified
     → AI override: player input locked (simulates fear — player auto-moves away)
   - Apply GE_Debuff_Weaken (10s) to player: Armor -= 30, MagicResist -= 20
4. Spawn shockwave Niagara (ground ripple outward from boss, dust + debris)
5. EndAbility

─── GA_Boss_MeteorStrike ──────────────────────────────────────
Tag: Ability.Boss.MeteorStrike
Cooldown: 45s | Phase: Phase 2 only
Type: Telegraphed AOE — gives player 2s warning

Flow:
1. Select target location: player's current position + predictive offset (1s ahead)
2. Spawn ADFMeteorWarningDecal at target (red circle, radius=400):
   - Animate: pulsing red → bright red over 2s
   - Spawn warning sound (rumbling)
3. WaitDelay(2.0s) ← player window to escape
4. Spawn ADFMeteorImpactActor:
   - Instant explosion at decal location
   - SphereOverlap radius=400:
     → Apply GE_Damage_True (500 flat — devastating) to player if in zone
     → Apply GE_Debuff_Stun (2s) to player
   - Inner radius=150: GE_Damage_True (1000 — instant kill zone)
   - Spawn massive explosion Niagara (fire, rock debris, crater decal on floor)
   - Camera shake (extreme, 2s)
5. EndAbility after impact

─── GA_Boss_VoidBarrier ───────────────────────────────────────
Tag: Ability.Boss.VoidBarrier
Cooldown: 60s | Phase: Phase 2 only
Type: Defensive phase — boss becomes temporarily unkillable

Flow:
1. Apply GE_VoidBarrier (HasDuration 8s):
   - GrantedTag: State.Invulnerable
   - Modifier: Armor = 99999 (effectively immune)
2. Spawn barrier VFX: swirling void energy sphere around boss (dark purple)
3. While barrier active: boss activates GA_Boss_VoidOrbs:
   - Spawn 4 ADFVoidOrbActor orbiting boss at radius=300
   - Each orb: UProjectileMovementComponent orbiting (custom tick — circular path)
   - Orb OnPlayerOverlap: apply GE_Damage_True (80) + GE_Debuff_Slow (2s)
   - Orbs have health: 150 HP each (player must destroy all 4)
4. Track orbs destroyed via OnOrbDestroyed delegate
5. When all 4 orbs destroyed (or 8s elapsed):
   - RemoveActiveGameplayEffect GE_VoidBarrier
   - Boss becomes vulnerable + briefly stunned 1s (reward for breaking barrier)
   - Destroy remaining orbs
6. EndAbility

─── GA_Boss_PhaseTransition_Slam ──────────────────────────────
Tag: Ability.Boss.PhaseTransitionSlam
Cooldown: N/A (auto-triggered once per phase transition by ADFBossBase)
Type: Unavoidable cinematic ability — screen event

Flow (called by TriggerPhaseTransition in ADFBossBase):
1. Apply GE_Debuff_Stun to PLAYER (1.5s — cinematic lock)
2. PlayMontageAndWait (PhaseTransitionSlamMontage — boss collapses then erupts)
3. At montage midpoint (AnimNotify AN_PhaseErupt):
   - SphereOverlap entire room (radius=9999)
   - Apply GE_Damage_True (MaxHealth * 0.15) to player — always hits for 15% HP
   - This forces the player to manage health before phase transitions
   - Spawn room-wide shockwave Niagara (fills entire dungeon room)
   - Flash screen white for 0.3s via post-process
4. Remove player stun
5. EndAbility (phase transition continues in ADFBossBase)

─── GA_Boss_EnragePulse ───────────────────────────────────────
Tag: Ability.Boss.EnragePulse
Cooldown: 8s | Available only when bIsEnraged == true
Type: Rapid-fire AOE spam during enrage

Flow:
1. CanActivateAbility: Boss->bIsEnraged == true
2. PlayMontageAndWait (EnragePulseMontage — fast pound animation, 0.4s)
3. SphereOverlap radius=600:
   - Apply GE_Damage_Physical (Strength * 1.5) to player
   - Apply GE_Debuff_Slow (1s) to player
   - Spawn kinetic shockwave Niagara (orange/red)
4. EndAbility immediately (fast cycle — 8s CD but enrage = non-stop pressure)

Output all 5 boss ability .h and .cpp files with correct Public/Private paths.
```

---

## 📝 DICAS DE USO NO CURSOR

## 📝 DICAS DE USO NO CURSOR

**Skills disponíveis no projeto** (`.cursor/skills/`):

| Skill | Usada nos Prompts |
|---|---|
| `@ue-project-context` | 0, 15 |
| `@ue-cpp-foundations` | 0, 1, 2, 7, 15 |
| `@ue-module-build-system` | 0, 1, 15 |
| `@ue-gameplay-abilities` | 2,3,4,5,7,8,10,11,12,14,15,17,18,20,22,23,24,25,26,27,28,29,30 |
| `@ue-gameplay-tags` | 2,5,6,8,11,12,14,16,17,18,19,20,21,22,24,25,26,27,28,29,30 |
| `@ue-gameplay-framework` | 3, 4, 9, 13, 16 |
| `@ue-input-system` | 3, 7 |
| `@ue-character-movement` | 3, 18, 21, 27, 30 |
| `@ue-networking-replication` | 4, 8, 15, 18 |
| `@ue-animation-system` | 5,12,17,18,21,22,25,26,27,28,29,30 |
| `@ue-data-assets-tables` | 6, 9, 10, 13, 20, 23, 24 |
| `@ue-asset-manager` | 6, 14 |
| `@ue-ai-navigation` | 8, 19 |
| `@ue-actor-component-archit...` | 8,10,16,17,19,22,24,28 |
| `@ue-procedural-generation` | 9 |
| `@ue-world-level-streaming` | 9 |
| `@ue-physics-collision` | 10,12,17,24,25,26,27,30 |
| `@ue-ui-umg-slate` | 11, 16, 22, 23, 24, 29 |
| `@ue-niagara-effects` | 12,17,22,25,26,27,28,29,30 |
| `@ue-serialization-savegames` | 13, 23 |
| `@ue-game-features` | 13 |
| `@ue-testing-debugging` | 15 |
| `@ue-mass-entity` | — (futuro: sistema de multidão) |
| `@ue-async-threading` | — (futuro: loading assíncrono) |
| `@ue-audio-system` | — (futuro: MetaSounds) |

**Mapa de Abilities por Classe:**

| Prompt | Classe | Abilities |
|---|---|---|
| 12 | 🔮 Mago | Fireball |
| 17 | 🌍 Universal | Melee Combo (3 hits) |
| 18 | 🌍 Universal | Sprint, Dodge |
| 25 | ⚔️ Guerreiro | ShieldBash, WarCry, Whirlwind, IronSkin, Charge, Execute |
| 26 | 🔮 Mago | FrostBolt, BlizzardStorm, ArcaneBarrage, TimeWarp, ManaShield, Teleport |
| 27 | 🗡️ Assassino | ComboPoints, Backstab, FanOfKnives, ShadowStep, Eviscerate, Vanish, SmokeScreen |
| 28 | ✨ Passivas | Fortitude, Retaliation, ArcaneMastery, ManaVortex, Predator, BleedMastery, AuraComponent |
| 29 | 🌟 Universal | HealthPotion, SecondWind, BattleHymn, Siphon, Berserk, CallLightning |
| 30 | 🐉 Boss | TerrorShout, MeteorStrike, VoidBarrier, PhaseTransitionSlam, EnragePulse |

**Ordem recomendada completa:**
```
0 → 1 → 14 → 2 → 4 → 3 → 20 → 5 → 18 → 21 → 17 → 16
→ 6 → 7 → 19 → 8 → 9 → 10 → 24 → 11 → 23
→ 28 → 25 → 26 → 27 → 29 → 12
→ 22 → 30 → 13 → 15
```

**Como usar:**
1. **Abra o projeto inteiro** no Cursor como workspace
2. **Use o modo Agent** para criação de múltiplos arquivos
3. **Cole o Prompt 0 primeiro** — carrega as skills base globais para toda sessão
4. Abilities de classe **dependem do Prompt 5** (base GameplayAbility) — execute-o antes
5. Abilities de boss **dependem do Prompt 22** (BossBase) — execute-o antes do Prompt 30
6. Use `@Public/GAS/Abilities/Warrior/DFAbility_ShieldBash.h` para referenciar abilities já geradas
7. **Para debugar GAS**, adicione ao Prompt 0: `"Always add GAS verbose log tags for debugging"`
8. Skills `@ue-mass-entity`, `@ue-async-threading` e `@ue-audio-system` estão disponíveis para prompts futuros

---

*DungeonForged — Cursor Prompts v4.0 | UE5.4 | GAS | 31 Prompts | 3 Classes + Passivas + Universal + Boss*