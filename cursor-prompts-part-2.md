# 🤖 CURSOR PROMPTS — DungeonForged (UE5.4) — PARTE 2
> Prompts 31–45 | Alta, Média e Baixa Prioridade
> Sempre cole o **Prompt 0** do arquivo CURSOR_PROMPTS.md antes de qualquer prompt deste arquivo.

---

## ══════════════════════════════════════════════
## 🔴 ALTA PRIORIDADE
## ══════════════════════════════════════════════

## ════════════════════════════════════════
## 📈 PROMPT 31 — LEVEL & XP SYSTEM
## ════════════════════════════════════════

```
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-data-assets-tables
@ue-ui-umg-slate
@ue-niagara-effects

Create the Level and XP progression system for DungeonForged UE 5.4.
Path convention:
  .h  → Source/DungeonForged/Public/Progression/<FileName>.h
  .cpp → Source/DungeonForged/Private/Progression/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── FDFLevelTableRow : FTableRowBase ──────────────────────────
File: Public/Progression/DFLevelingData.h

Fields:
- int32 Level
- int32 XPRequired          ← cumulative XP to reach this level
- int32 AttributePointsGranted  ← points to distribute on level up
- TArray<FName> AbilitiesUnlocked ← DT_Abilities rows auto-granted at this level
- float StatScalingMultiplier    ← base stat multiplier applied via GE at this level

─── UDFLevelingComponent (ActorComponent on PlayerState) ──────

Properties (Replicated):
- int32 CurrentLevel = 1
- int32 CurrentXP = 0
- int32 UnspentAttributePoints = 0
- UDataTable* LevelTable        ← DT_Levels reference
- int32 MaxLevel = 30

Functions:
- AddXP(int32 Amount):
  → CurrentXP += Amount
  → CheckLevelUp()
  → Broadcast OnXPChanged(CurrentXP, XPToNextLevel)
- CheckLevelUp():
  → Read FDFLevelTableRow for CurrentLevel + 1
  → While CurrentXP >= XPRequired: call LevelUp()
- LevelUp():
  → CurrentLevel++
  → UnspentAttributePoints += Row.AttributePointsGranted
  → Apply GE_LevelUp_StatScaling (scales base attributes)
  → Grant abilities from Row.AbilitiesUnlocked via ASC
  → Broadcast OnLevelUp(CurrentLevel)
  → Spawn level-up VFX + play fanfare SFX
- SpendAttributePoint(FGameplayAttribute Attribute, int32 Amount):
  → Check UnspentAttributePoints >= Amount
  → Apply GE_AttributeIncrease (SetByCaller: attribute + amount)
  → UnspentAttributePoints -= Amount
  → Broadcast OnAttributePointSpent
- GetXPToNextLevel(): returns int32
- GetXPProgress(): returns float 0..1 for progress bar

─── GE_LevelUp_StatScaling ────────────────────────────────────
HasDuration: Infinite (replaces previous level scaling GE)
- Modifier: MaxHealth *= Row.StatScalingMultiplier
- Modifier: MaxMana   *= Row.StatScalingMultiplier
- Modifier: Strength  += (Level * 2)
- Modifier: Intelligence += (Level * 2)
- Modifier: Agility   += (Level * 1)
- Tag: Character.Level.{N} (one tag per level for checks)
Note: use RemoveGameplayEffectByTag before applying new level GE

─── Integration with EnemyBase ───────────────────────────────
In ADFEnemyBase::OnDeath():
- Get instigator PlayerState → GetLevelingComponent()->AddXP(EnemyRow.ExperienceReward)
- XP scales: EnemyRow.ExperienceReward * (1 + FloorNumber * 0.1) ← floor bonus

─── WBP_LevelUpScreen ─────────────────────────────────────────
UDFLevelUpWidget extends UDFUserWidgetBase:
- Shown after LevelUp broadcast, before WBP_AbilitySelection
- UTextBlock* LevelText       ← "LEVEL 5!"
- TArray<WBP_AttributeRow*>  ← one row per attribute with + button
  Each row: AttributeName | CurrentValue | [+] button (spends 1 point)
- UTextBlock* PointsRemaining
- UButton* ConfirmButton      ← closes screen, passes to ability selection if applicable
- Animate: dramatic slide-in with golden particle burst

─── WBP_XPBar (add to existing HUD) ──────────────────────────
- UProgressBar* XPBar (slim bar below health bar)
- UTextBlock* LevelText       ← "Lv. 5"
- UTextBlock* XPText          ← "1240 / 2000 XP"
- OnLevelUp: play glow animation, briefly expand bar height
- Bind to UDFLevelingComponent::OnXPChanged and OnLevelUp delegates

Output all .h and .cpp files with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🔢 PROMPT 32 — FLOATING COMBAT TEXT
## ════════════════════════════════════════

```
@ue-ui-umg-slate
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-actor-component-archit...

Create the Floating Combat Text system for DungeonForged UE 5.4.
Path convention:
  .h  → Source/DungeonForged/Public/UI/Combat/<FileName>.h
  .cpp → Source/DungeonForged/Private/UI/Combat/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── ECombatTextType (enum) ────────────────────────────────────
File: Public/UI/Combat/DFCombatTextTypes.h
Values:
  Damage_Physical, Damage_Magic, Damage_True
  Damage_Critical         ← larger, yellow, more dramatic float
  Damage_DoT              ← smaller, colored by DoT type
  Heal                    ← green
  Mana_Restore            ← blue
  Miss                    ← grey, italic "MISS"
  Dodge                   ← white, italic "DODGE"
  Block                   ← grey "BLOCK"
  Immune                  ← yellow "IMMUNE"
  LevelUp                 ← gold "LEVEL UP!"
  XPGain                  ← dim yellow "+240 XP"
  GoldGain                ← bright yellow "+50 Gold"
  Status                  ← purple italic (Stunned, Slowed, etc.)

─── UDFCombatTextWidget extends UUserWidget ───────────────────
Single floating text instance (pooled):

Properties:
- UTextBlock* DamageText
- UWidgetAnimation* FloatAnimation  ← moves UP + fades out over 1.2s
- FVector WorldLocation             ← world position to follow

Functions:
- Initialize(FString Text, ECombatTextType Type, FVector WorldPos):
  → Set text content, color, size by type:
    Physical: white, size 28
    Magic: purple, size 28
    Critical: yellow, size 42, bold, slight rotation ±5°
    DoT: orange/green/red (by DoT type), size 22
    Heal: green, size 30
    Miss/Dodge: grey, size 24, italic
    LevelUp: gold, size 52, all caps, stays 2s
  → Add random horizontal scatter (±30px)
  → Play FloatAnimation
  → On animation end: ReturnToPool()
- Tick: ProjectWorldLocationToScreen each frame → SetRenderTranslation

─── UDFCombatTextSubsystem extends UWorldSubsystem ────────────
Object pool manager — avoids widget creation/destruction per hit.

Properties:
- TArray<UDFCombatTextWidget*> WidgetPool
- int32 PoolSize = 30
- TSubclassOf<UDFCombatTextWidget> WidgetClass

Functions:
- Initialize: pre-create PoolSize widgets, add to viewport hidden
- SpawnText(FVector WorldLocation, float Value, ECombatTextType Type):
  → Pop from pool (or create if empty)
  → Compute display string:
    Damage: FString::FromInt(FMath::RoundToInt(Value))
    Critical: "★ " + value + " ★"
    Heal: "+" + value
    Miss: "MISS", Dodge: "DODGE"
  → Call Widget->Initialize(Text, Type, WorldLocation)
- ReturnToPool(UDFCombatTextWidget* Widget): hide + push back to pool

─── Integration points ────────────────────────────────────────
Hook SpawnText into these existing systems:

1. UDFAttributeSet::PostGameplayEffectExecute:
   - On Health decrease: SpawnText at actor location, Damage type + value
   - Detect critical: if GEContext has tag "Effect.Critical" → use Damage_Critical type
   - On Health increase: SpawnText Heal type

2. UDFLevelingComponent::AddXP:
   - SpawnText at player location, XPGain type, "+{Amount} XP"

3. UDFInventoryComponent::AddItem:
   - If item is Gold: SpawnText GoldGain type

4. UDFHitReactionComponent::OnHitReceived:
   - If target has State.Invulnerable: SpawnText Immune type
   - If ability blocked by GAS tag: SpawnText Block type

─── Critical Hit Detection ────────────────────────────────────
Add to UDFDamageCalculation::Execute_Implementation:
- Roll CritChance: if (FMath::FRand() < SourceCritChance)
  → FinalDamage *= CritMultiplier
  → Add GameplayTag "Effect.Critical" to GEContext via AddHitResult
  → This tag is read by AttributeSet to choose combat text type

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🛒 PROMPT 33 — MERCHANT / SHOP SYSTEM
## ════════════════════════════════════════

```
@ue-ui-umg-slate
@ue-data-assets-tables
@ue-gameplay-abilities
@ue-actor-component-archit...
@ue-serialization-savegames

Create the Merchant and Shop system for DungeonForged UE 5.4.
Path convention:
  .h  → Source/DungeonForged/Public/Merchant/<FileName>.h
  .cpp → Source/DungeonForged/Private/Merchant/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── FDFMerchantStockRow : FTableRowBase ───────────────────────
File: Public/Merchant/DFMerchantData.h
Fields:
- FName ItemRowName         ← from DT_Items
- int32 BasePrice
- float PriceVariance       ← ±% randomization per run
- EItemRarity MinRarity
- int32 MinFloorAvailable   ← only appears from this floor onwards
- bool bIsConsumable        ← consumables restock between floors
- int32 StockQuantity       ← -1 = unlimited

─── ADFMerchantActor extends ADFInteractableBase ──────────────
Properties:
- UDataTable* MerchantStockTable   ← DT_MerchantStock
- int32 StockSlots = 6
- float RerollCost = 50.f          ← gold cost, doubles each reroll
- int32 RerollCount = 0
- TArray<FDFMerchantStockEntry> CurrentStock   ← struct: Row + Price + Quantity
- USkeletalMeshComponent* MerchantMesh
- UAnimationAsset* IdleAnimation

Functions:
- BeginPlay: GenerateStock()
- GenerateStock():
  → Filter DT_MerchantStock by CurrentFloor >= MinFloorAvailable
  → Weighted random pick StockSlots items (weight by rarity inverse)
  → Apply price variance: Price = BasePrice * FMath::RandRange(0.8f, 1.2f)
  → Store in CurrentStock
- Interact_Implementation: Open WBP_Shop widget (set input mode UI)
- RerollStock(ADFPlayerCharacter* Buyer):
  → Check Gold >= RerollCost
  → Deduct gold, RerollCost *= 2, RerollCount++
  → GenerateStock() again
- PurchaseItem(int32 SlotIndex, ADFPlayerCharacter* Buyer):
  → Check Gold >= CurrentStock[SlotIndex].Price
  → Deduct gold via RunManager
  → Add item to Inventory via UDFInventoryComponent
  → Decrement Quantity (remove if 0)
  → Broadcast OnPurchaseComplete

─── WBP_Shop extends UDFUserWidgetBase ────────────────────────
Layout:
- Header: Merchant name + portrait image
- Gold display (top right): UImage coin icon + UTextBlock gold amount
- UUniformGridPanel: 3x2 grid of WBP_ShopItemSlot widgets
- UButton* RerollButton: "Reroll Stock — [Cost] Gold" (updates cost label)
- UButton* CloseButton

WBP_ShopItemSlot:
- UImage* ItemIcon
- UTextBlock* ItemName
- UTextBlock* RarityLabel    ← colored by rarity
- UTextBlock* PriceLabel     ← "150 Gold"
- UTextBlock* QuantityLabel  ← "x3" or "∞"
- UButton* BuyButton
- On hover: show tooltip WBP_ItemTooltip (stats preview)
- If insufficient gold: BuyButton grayed + tooltip "Not enough gold"
- On purchase: slot fades out + play coin SFX

WBP_ItemTooltip (reusable across Shop + Inventory):
- UTextBlock* ItemName (colored by rarity)
- UTextBlock* ItemType
- UTextBlock* Description
- TArray<UTextBlock*> StatLines ← "+25 Strength", "+10% CritChance"
- UTextBlock* CompareText ← "▲ +10 vs equipped" (green) or "▼ -5" (red)

─── Gold Resource integration ────────────────────────────────
Add to UDFRunManager:
- int32 CurrentGold = 0
- AddGold(int32 Amount): CurrentGold += Amount, broadcast OnGoldChanged
- SpendGold(int32 Amount): returns bool, deduct if sufficient
- Gold sources: enemy drops (EnemyTableRow.GoldDrop range), chests, shrines, skip rewards
- Gold persists within a run only (roguelike — resets on death)

─── HUD Gold Display ─────────────────────────────────────────
Add to WBP_HUD:
- UImage* CoinIcon
- UTextBlock* GoldText
- Bind to UDFRunManager::OnGoldChanged
- On gold gain: brief scale-up animation on text (coin pop feel)

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🪤 PROMPT 34 — TRAP & HAZARD SYSTEM
## ════════════════════════════════════════

```
@ue-actor-component-archit...
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-physics-collision
@ue-niagara-effects
@ue-animation-system

Create the Trap and Hazard system for DungeonForged UE 5.4.
Path convention:
  .h  → Source/DungeonForged/Public/Dungeon/Traps/<FileName>.h
  .cpp → Source/DungeonForged/Private/Dungeon/Traps/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── ADFTrapBase extends AActor ────────────────────────────────

Properties:
- bool bIsArmed = true
- bool bIsRepeating = true       ← false = single-trigger traps
- float TriggerDelay = 0.f       ← seconds after activation before damage
- float RearmDelay = 2.f         ← seconds before trap resets
- float DamageAmount = 30.f
- TSubclassOf<UGameplayEffect> TrapEffect   ← GE applied on trigger
- UNiagaraSystem* ActiveVFX
- UNiagaraSystem* TriggerVFX
- UNiagaraSystem* DisabledVFX
- FTimerHandle RearmTimer

Functions:
- OnTrapTriggered(AActor* Instigator): override per trap type
  → Apply TrapEffect to Instigator via ASC
  → Spawn TriggerVFX
  → if !bIsRepeating: Disarm()
  → else: start RearmTimer
- Disarm(): bIsArmed=false, swap to DisabledVFX
- Rearm(): bIsArmed=true, swap back to ActiveVFX
- CanBeSeen(): bool — some traps are hidden (floor spikes flush)
- TelegraphActivation(): called TriggerDelay seconds before damage
  → Visual warning (VFX color change, audio warning beep)

─── ADFTrap_SpikePlate extends ADFTrapBase ────────────────────
Trigger: pressure plate (UBoxComponent overlap)

Properties:
- UStaticMeshComponent* PlateMesh     ← flush with floor when disarmed
- UStaticMeshComponent* SpikesMesh    ← spikes emerge on trigger
- bool bIsHidden = true               ← player can't see until close (<300 units)
- UTimelineComponent* SpikeTimeline   ← animates spikes up/down

Behavior:
- OnBeginOverlap: if bIsArmed → TelegraphActivation (0.3s warning — brief)
- TelegraphActivation: play plate-glow VFX (red), play click SFX
- OnTrapTriggered: play SpikeTimeline (spikes emerge in 0.15s)
  → BoxTrace for enemies/player in spike volume
  → Apply GE_Damage_Physical (DamageAmount) + GE_DoT_Bleed (2s)
  → After 1s: retract spikes (reverse timeline)
  → RearmDelay: 1.5s

─── ADFTrap_DartWall extends ADFTrapBase ──────────────────────
Trigger: tripwire (beam sensor across corridor) OR timed interval

Properties:
- TArray<FVector> DartSpawnOffsets   ← positions along wall
- float DartSpeed = 1800.f
- UStaticMeshComponent* WallMesh
- float FireInterval = 3.f           ← for timed variant
- bool bIsTimed = false

Behavior:
- Tripwire variant: UBoxComponent thin trigger across corridor
  → On overlap: fire all darts (ADFDartProjectile actors)
  → TelegraphActivation (0.2s): wall clicks, light flash
- Timed variant: FTimerHandle fires every FireInterval
  → No telegraph — purely timing-based
- ADFDartProjectile: UProjectileMovementComponent speed=DartSpeed
  → OnHit: apply GE_Damage_Physical (DamageAmount) + GE_Debuff_Slow (2s)
  → Sticks in walls (disable physics, keep mesh)

─── ADFTrap_FireJet extends ADFTrapBase ───────────────────────
Trigger: timed only (predictable pattern)

Properties:
- float ActiveDuration = 1.5s
- float InactiveDuration = 2.0s    ← player can time the gap
- UCapsuleComponent* DamageVolume  ← fire jet area
- UNiagaraComponent* FireJetNiagara

Behavior:
- Start: alternate Active/Inactive on loop timers
- ActiveDuration: FireJetNiagara active, DamageVolume enabled
  → Every 0.3s tick: apply GE_Damage_True (DamageAmount) + GE_DoT_Fire (3s)
  → Screen vignette orange while player inside
- InactiveDuration: Niagara fades, DamageVolume disabled
  → 0.5s before re-activation: play pre-ignite sound + brief flame flicker
    (telegraph for skilled players to recognize timing)
- Cannot be disarmed — always repeating

─── ADFTrap_PoisonVent extends ADFTrapBase ────────────────────
Trigger: proximity (sensor) + timed

Properties:
- float CloudRadius = 300.f
- float CloudDuration = 5.f
- USphereComponent* DetectionRange  ← triggers when player within 500 units

Behavior:
- On player detected: WaitDelay(1s) → Activate vent
- Spawn ADFPoisonCloudActor (lifespan=CloudDuration):
  → USphereComponent radius=CloudRadius
  → UNiagaraComponent green toxic cloud
  → Every 0.5s: SphereOverlap → apply GE_DoT_Poison (3s) to all in cloud
  → Obscures vision slightly (post-process desaturation inside cloud)
- Vent rearms after CloudDuration + 4s (rearm delay)

─── ADFTrap_CollapsingFloor extends ADFTrapBase ───────────────
Trigger: weight (player walks on) — one-shot environmental

Properties:
- TArray<UStaticMeshComponent*> FloorTiles   ← collapsing sections
- float CollapseDelay = 0.8s                 ← cracks first, then falls
- float FallDamage = 50.f                    ← if player doesn't dodge

Behavior:
- On player overlap: play crack VFX + rumble SFX (CollapseDelay warning)
- After delay: all FloorTiles disable collision + play fall animation downward
- Player on tiles: apply GE_Damage_True (FallDamage) + GE_Debuff_Stun (0.5s)
- Tiles fall to lower level or are destroyed (ADestructibleActor approach)
- Not repeating (bIsRepeating=false, bIsArmed=false after trigger)

─── UDFTrapDetectionComponent (on PlayerCharacter) ───────────────
Detects nearby hidden traps:

Properties:
- float DetectionRadius = 250.f
- bool bTrapHighlightEnabled = true   ← toggled by skill/item

Functions:
- TickDetection (every 0.5s): SphereOverlap for ADFTrapBase actors
  → For each trap with bIsHidden=true AND distance < DetectionRadius:
    → Set trap mesh render CustomDepth (outline highlight, yellow)
    → Show WBP_TrapIndicator (! icon above trap)
- HideTrapHighlight(ADFTrapBase* Trap): remove outline

Output all .h and .cpp with correct Public/Private paths.
```

---

## ══════════════════════════════════════════════
## 🟡 MÉDIA PRIORIDADE
## ══════════════════════════════════════════════

## ════════════════════════════════════════
## 🎵 PROMPT 35 — AUDIO SYSTEM (METASOUNDS)
## ════════════════════════════════════════

```
@ue-audio-system
@ue-gameplay-tags
@ue-actor-component-archit...
@ue-animation-system

Create the complete Audio System for DungeonForged UE 5.4 using MetaSounds.
Path convention:
  .h  → Source/DungeonForged/Public/Audio/<FileName>.h
  .cpp → Source/DungeonForged/Private/Audio/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── UDFAudioComponent extends UAudioComponent ─────────────────

Properties:
- USoundConcurrency* AbilitySFXConcurrency
- USoundConcurrency* FootstepConcurrency
- USoundAttenuation* DefaultAttenuation3D
- TMap<FGameplayTag, USoundBase*> AbilitySoundMap  ← tag → sound asset
- TMap<EPhysicalSurface, USoundBase*> FootstepMap  ← surface → footstep

Functions:
- PlayAbilitySound(FGameplayTag AbilityTag, FVector Location):
  → Lookup AbilitySoundMap, play at location with 3D attenuation
- PlayFootstep(EPhysicalSurface Surface, FVector Location):
  → Lookup FootstepMap, play with FootstepConcurrency (max 3 concurrent)
- PlayImpactSound(EPhysicalSurface Surface, FVector Location, float Force):
  → Scale volume by Force, pick variation by surface
- PlayUISound(USoundBase* Sound): 2D, no attenuation

─── UDFMusicManagerSubsystem extends UWorldSubsystem ──────────
Adaptive music system — transitions between states.

Music States (EMusicState):
  MainMenu, Exploration, Combat, Elite, Boss, Victory, Death

Properties:
- EMusicState CurrentState = Exploration
- UAudioComponent* MusicLayerBase      ← always playing (ambient layer)
- UAudioComponent* MusicLayerCombat    ← fades in during combat
- UAudioComponent* MusicLayerBoss      ← boss music layer
- float CrossfadeDuration = 2.0f

Functions:
- SetMusicState(EMusicState NewState):
  → If same state: return
  → CrossfadeToState(NewState) using FTimerHandle per-layer volume lerp
- CrossfadeToState(EMusicState Target):
  → Exploration: fade out Combat layer (2s), keep Base layer
  → Combat: fade in Combat layer (1s)
  → Boss: fade out all, fade in Boss layer (1.5s)
  → Death: immediate stop all, play death sting
- OnCombatStateChanged: bind to GameplayTag State.InCombat changes on player ASC
  → Added: SetMusicState(Combat) | Removed: SetMusicState(Exploration) after 5s delay
- OnBossEncounterStarted: SetMusicState(Boss)
- OnBossDefeated: SetMusicState(Victory) for 8s → then Exploration

─── Footstep System integration ───────────────────────────────
UDFAnimNotify_FootStep (from Prompt 21) full implementation:
- Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation):
  → Line trace downward from foot socket 50 units
  → Get hit PhysicalMaterial → EPhysicalSurface
  → Get UDFAudioComponent from owner
  → PlayFootstep(Surface, HitLocation)
  → Spawn optional footstep dust Niagara by surface (dirt=dust, stone=spark)

─── MetaSound Source descriptions ────────────────────────────
Describe setup for these MetaSound assets (C++ cannot create MS assets,
but describe the MetaSound graph nodes needed for each):

MS_Ability_Fireball:
- Input: float Intensity (0-1)
- Nodes: WavePlayer(FireWhoosh) → Envelope(Attack=0.01, Release=0.3)
  → Spatializer → Output
- On impact: trigger MS_Impact_Fire

MS_Ability_FrostBolt:
- WavePlayer(IceProjectile) + WavePlayer(IceHum) blended
- On impact: MS_Impact_Ice (crystal shatter + reverb tail)

MS_Combat_HitFlesh:
- Random WavePlayer from pool of 4 flesh hit sounds
- Pitch randomization ±5%

MS_UI_Purchase: simple 2D coin jingle
MS_UI_LevelUp: fanfare ascending arpeggio
MS_UI_AbilitySelect: whoosh + confirmation tone

─── Sound Effect Hooks table ──────────────────────────────────
Create UDFSoundLibrary (UDataAsset):
- TMap<FGameplayTag, USoundBase*> TaggedSounds
  Populate with tags:
  SFX.Ability.Fireball.Cast
  SFX.Ability.Fireball.Impact
  SFX.Ability.FrostBolt.Cast
  SFX.Ability.Melee.Swing
  SFX.Ability.Melee.Hit
  SFX.Ability.Dodge
  SFX.UI.Purchase
  SFX.UI.LevelUp
  SFX.Enemy.Death.Normal
  SFX.Enemy.Death.Boss
  SFX.Dungeon.DoorOpen
  SFX.Dungeon.ChestOpen
  SFX.Dungeon.TrapTrigger

- static USoundBase* GetSound(FGameplayTag Tag): lookup helper

Output UDFAudioComponent.h/.cpp, UDFMusicManagerSubsystem.h/.cpp,
UDFSoundLibrary.h/.cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🔰 PROMPT 36 — STATUS EFFECT HUD
## ════════════════════════════════════════

```
@ue-ui-umg-slate
@ue-gameplay-abilities
@ue-gameplay-tags

Create the Status Effect HUD system for DungeonForged UE 5.4.
Path convention:
  .h  → Source/DungeonForged/Public/UI/Status/<FileName>.h
  .cpp → Source/DungeonForged/Private/UI/Status/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── FDFStatusEffectDisplayData (struct) ───────────────────────
File: Public/UI/Status/DFStatusEffectData.h
Fields:
- FGameplayTag EffectTag
- UTexture2D* Icon
- FLinearColor BorderColor    ← buff=gold, debuff=red, DoT=orange, cc=purple
- FText DisplayName
- FText Description
- bool bShowDuration          ← false for permanent passives
- bool bIsDebuff

─── UDFStatusEffectIconWidget extends UUserWidget ─────────────
Single status icon widget (pooled):

Properties:
- UImage* IconImage
- UImage* BorderImage         ← color-coded
- UTextBlock* DurationText    ← countdown in seconds
- UProgressBar* DurationBar   ← circular or linear progress draining
- UButton* HoverArea          ← for tooltip

Functions:
- Initialize(FActiveGameplayEffectHandle Handle, FDFStatusEffectDisplayData Data):
  → Set icon, border color, name
  → Start FTimerHandle to update DurationText every 0.1s
- UpdateDuration(float RemainingTime, float TotalDuration):
  → DurationText = FString::SanitizeFloat(RemainingTime, 1) + "s"
  → DurationBar percent = RemainingTime / TotalDuration
  → If RemainingTime < 3s: pulse DurationText red (warning)
- OnHovered: show WBP_StatusTooltip (name + description + remaining time)
- OnEffectExpired: play fade-out animation → ReturnToPool()

─── WBP_StatusEffectBar extends UDFUserWidgetBase ─────────────
Container for all active status icons — added to main HUD.

Layout:
- Two rows:
  BuffRow  (HorizontalBox, left-to-right): player buffs
  DebuffRow (HorizontalBox, left-to-right): debuffs and DoTs
- Icons: 32x32px with 4px gap

Functions:
- NativeConstruct: subscribe to ASC->OnActiveGameplayEffectAddedDelegateToSelf
  AND ASC->OnAnyGameplayEffectRemovedDelegate
- OnEffectAdded(FActiveGameplayEffectHandle Handle):
  → Read GE tags, look up FDFStatusEffectDisplayData from UDFStatusLibrary
  → If no data found: skip (internal/hidden effects)
  → Pop icon from pool, Initialize, add to correct row
  → Play add-animation (scale from 0 to 1, bounce)
- OnEffectRemoved(FActiveGameplayEffectHandle Handle):
  → Find icon by Handle, call OnEffectExpired
- SortIcons(): buffs by remaining time descending, debuffs by severity

─── UDFStatusLibrary extends UDataAsset ───────────────────────
Maps GameplayTags → FDFStatusEffectDisplayData:

Static function: GetStatusData(FGameplayTag Tag) → const FDFStatusEffectDisplayData*

Pre-populate for all tags from Prompt 20:
- Effect.Buff.Speed → green border, speed icon
- Effect.Buff.DamageUp → orange border, sword icon
- Effect.Debuff.Slow → blue border, snail icon
- State.Stunned → yellow border, star icon
- State.Berserk → red border, rage icon
- Effect.DoT.Fire → orange border, flame icon
- Effect.DoT.Poison → green border, skull icon
- Effect.DoT.Bleed → red border, drop icon
- State.ManaShieldActive → blue border, shield icon
- State.Invisible → grey border, eye-slash icon
(Add all tags from Prompt 20 GE Library)

─── Enemy Status Bar integration ──────────────────────────────
Extend ADFEnemyBase WBP_EnemyStatusBar (above healthbar):
- Show max 3 debuff icons on enemy (most severe)
- Same UDFStatusEffectIconWidget pool, smaller size (24x24)
- Only show debuffs the PLAYER applied (filter by source ASC)

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 👗 PROMPT 37 — EQUIPMENT VISUAL SYSTEM
## ════════════════════════════════════════

```
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-data-assets-tables
@ue-actor-component-archit...
@ue-animation-system

Create the Equipment Visual System for DungeonForged UE 5.4.
Path convention:
  .h  → Source/DungeonForged/Public/Equipment/<FileName>.h
  .cpp → Source/DungeonForged/Private/Equipment/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── EEquipmentSlot (enum) ─────────────────────────────────────
Values: Weapon, OffHand, Helmet, Chest, Legs, Boots, Gloves, Ring1, Ring2, Amulet

─── UDFEquipmentComponent extends UActorComponent ─────────────
Owner: ADFPlayerCharacter

Properties:
- TMap<EEquipmentSlot, FName> EquippedItems    ← slot → DT_Items RowName
- TMap<EEquipmentSlot, FActiveGameplayEffectHandle> EquipEffectHandles
- TMap<EEquipmentSlot, USkeletalMeshComponent*> SlotMeshComponents
  ← visual mesh per slot (helmet, chest, etc.)

Functions:
- EquipItem(FName ItemRowName, EEquipmentSlot Slot):
  → Read FDFItemTableRow from DT_Items
  → UnequipSlot(Slot) first if occupied
  → Apply item's OnEquipEffect to ASC → store handle
  → If item has mesh: SwapSlotMesh(Slot, ItemRow.ItemMesh)
  → Store in EquippedItems
  → Broadcast OnEquipmentChanged(Slot, ItemRowName)
- UnequipSlot(EEquipmentSlot Slot):
  → RemoveActiveGameplayEffect(EquipEffectHandles[Slot])
  → RestoreDefaultMesh(Slot)
  → Remove from EquippedItems
  → Add item back to UDFInventoryComponent
- SwapSlotMesh(EEquipmentSlot Slot, USkeletalMesh* NewMesh):
  → Get USkeletalMeshComponent for slot
  → SetSkeletalMesh(NewMesh)
  → SetLeaderPoseComponent(CharacterBaseMesh) ← UE5 master pose
- GetEquippedItem(EEquipmentSlot Slot): returns FDFItemTableRow*
- GetTotalStatBonus(FGameplayAttribute Attribute): float — sum all equipped items

─── Modular Character Mesh Setup ─────────────────────────────
In ADFPlayerCharacter, replace single SkeletalMeshComponent with:
- USkeletalMeshComponent* Mesh_Base      ← body/skin (Leader Pose)
- USkeletalMeshComponent* Mesh_Helmet    ← SetLeaderPoseComponent(Mesh_Base)
- USkeletalMeshComponent* Mesh_Chest     ← SetLeaderPoseComponent(Mesh_Base)
- USkeletalMeshComponent* Mesh_Legs      ← SetLeaderPoseComponent(Mesh_Base)
- USkeletalMeshComponent* Mesh_Boots     ← SetLeaderPoseComponent(Mesh_Base)
- USkeletalMeshComponent* Mesh_Gloves    ← SetLeaderPoseComponent(Mesh_Base)
- USkeletalMeshComponent* Mesh_Weapon    ← attached to "weapon_r" socket
- USkeletalMeshComponent* Mesh_OffHand   ← attached to "weapon_l" socket

All slave meshes: SetCollisionEnabled(NoCollision), tick disabled,
bReceivesDecals=true, bCastShadow=true

─── WBP_CharacterScreen extends UDFUserWidgetBase ─────────────
Paper-doll equipment screen:

Layout:
- Character preview: USceneCaptureComponent2D renders player to RenderTarget
  (rotate with mouse drag, zoom with scroll)
- Equipment slots arranged around preview (WoW paper doll layout):
  Top: Helmet | Left col: Amulet, Chest, Ring1, Gloves | Right col: Ring2, Legs, Boots | Bottom: Weapon, OffHand
- Each slot: WBP_EquipmentSlot widget

WBP_EquipmentSlot:
- UImage* SlotIcon (item icon or grey slot silhouette if empty)
- UImage* RarityBorder
- UTextBlock* ItemLevelText
- On Click: open WBP_ItemTooltip with Unequip button option
- On DragDrop from Inventory: call EquipmentComponent->EquipItem

─── UDFPreviewCaptureComponent ────────────────────────────────
Renders player character to UTextureRenderTarget2D for paper-doll:
- USceneCaptureComponent2D* CaptureComp (150mm FOV, orthographic)
- Position: offset 200 units in front of character, elevated 80 units
- Capture only character mesh layers (custom depth stencil)
- Update only when WBP_CharacterScreen is open (disable when closed)

─── Item stat comparison ─────────────────────────────────────
Extend WBP_ItemTooltip (from Prompt 33):
- GetCurrentlyEquipped(EEquipmentSlot): read UDFEquipmentComponent
- For each attribute modifier in hovered item:
  → Compare vs currently equipped item's same attribute
  → Show "▲ +15 Strength (vs equipped)" in green
  → Show "▼ -5 Agility (vs equipped)" in red
  → Show "NEW" if slot is empty

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 💥 PROMPT 38 — HIT STOP & SCREEN FX
## ════════════════════════════════════════

```
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-actor-component-archit...
@ue-niagara-effects

Create the Hit Stop and Screen Effects system for DungeonForged UE 5.4.
Path convention:
  .h  → Source/DungeonForged/Public/FX/<FileName>.h
  .cpp → Source/DungeonForged/Private/FX/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── UDFHitStopSubsystem extends UWorldSubsystem ───────────────

Functions:
- TriggerHitStop(float Duration, float TimeDilation, AActor* ExcludeActor):
  → UGameplayStatics::SetGlobalTimeDilation(World, TimeDilation)
  → ExcludeActor: set CustomTimeDilation = 1.0 / TimeDilation (stays real-time)
  → FTimerHandle (real-time timer): restore TimeDilation after Duration
  → Do NOT stack: if already in hit stop, extend only if new Duration longer

Hit Stop presets (call these from abilities and damage calculation):
- LightHit():   TriggerHitStop(0.06s, 0.05f)   ← normal melee
- HeavyHit():   TriggerHitStop(0.10s, 0.02f)   ← heavy attacks, charge landing
- CriticalHit():TriggerHitStop(0.14s, 0.01f)   ← critical strikes
- BossSlam():   TriggerHitStop(0.20s, 0.00f)   ← complete freeze, boss abilities

─── UDFScreenEffectsComponent extends UActorComponent ────────
On PlayerCharacter — controls all post-process effects dynamically.

Properties:
- UPostProcessComponent* PostProcessComp
- UMaterialInstanceDynamic* ScreenEffectMaterial  ← master screen FX material
  Parameters: VignetteIntensity, VignetteColor, ChromaticAberration,
              BlurAmount, SaturationMult, FlashIntensity, FlashColor

Functions:

DamageReceived(float DamagePercent):
  → FlashScreen(FLinearColor::Red, 0.15s, DamagePercent * 0.8)
  → If DamagePercent > 0.3: CameraShake(HeavyHitShake)
  → ChromaticAberrationPulse(0.3s, DamagePercent * 2.0)

LowHealthPulse():
  → Continuously pulsing red vignette when Health < 25%
  → Bind to Health attribute: if Health/MaxHealth < 0.25 → start pulse timer
  → Pulse: lerp VignetteIntensity 0.3→0.7→0.3 over 1.5s loop
  → Stops when Health > 25%

HealReceived(float HealPercent):
  → FlashScreen(FLinearColor::Green, 0.2s, HealPercent * 0.5)
  → Brief desaturation→oversaturation snap (health surge feel)

BerserkActive():
  → Red vignette constant (0.5 intensity)
  → Slight grain/noise overlay
  → FOV lerp to 100 (call PlayerCamera)

OnDeath():
  → FullDesaturation over 2s (black and white)
  → Slow vignette close
  → SlowMotion: SetGlobalTimeDilation(0.2) over 3s (death slow-mo)

Teleport/Blink():
  → Brief white flash (0.05s) + chromatic aberration spike

FlashScreen(FLinearColor Color, float Duration, float Intensity):
  → Set FlashColor + FlashIntensity → lerp back to 0 over Duration

ChromaticAberrationPulse(float Duration, float Intensity):
  → Set ChromaticAberration → lerp to 0

─── UDFCameraShakeLibrary ─────────────────────────────────────
Define C++ camera shake classes (extend UCameraShakeBase):

UDFCameraShake_LightHit:
  - OscillationDuration=0.2s, OscillationBlendInTime=0.01s
  - RotOscillation: Pitch Amplitude=0.8, Frequency=30

UDFCameraShake_HeavyHit:
  - Duration=0.35s
  - RotOscillation: Pitch+Roll Amplitude=2.0, Frequency=20
  - LocOscillation: Y Amplitude=3.0, Frequency=25

UDFCameraShake_BossSlam:
  - Duration=0.6s
  - RotOscillation: all axes Amplitude=4.0, Frequency=15
  - LocOscillation: Z Amplitude=8.0, Frequency=10

UDFCameraShake_Explosion:
  - Duration=0.8s, Amplitude=5.0 all axes, radial falloff by distance

Integration in UDFHitReactionComponent::OnHitReceived:
  → Light damage: LightHit() + UDFHitStopSubsystem::LightHit()
  → Heavy damage: HeavyHit() + HeavyHitShake + DamageReceived(percent)
  → Critical: CriticalHit() + HeavyHitShake + ChromaticAberrationPulse

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🗺️ PROMPT 39 — MINIMAP SYSTEM
## ════════════════════════════════════════

```
@ue-ui-umg-slate
@ue-gameplay-framework
@ue-actor-component-archit...

Create the Minimap system for DungeonForged UE 5.4.
Path convention:
  .h  → Source/DungeonForged/Public/UI/Minimap/<FileName>.h
  .cpp → Source/DungeonForged/Private/UI/Minimap/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── UDFMinimapCaptureComponent ────────────────────────────────
Placed in level — captures top-down view for minimap texture.

Properties:
- USceneCaptureComponent2D* CaptureComp
  → ProjectionType: Orthographic
  → OrthoWidth: 3000 (covers one dungeon room)
  → CaptureSource: FinalColorHDR
  → ShowFlags: disable dynamic shadows, fog, reflections (performance)
  → ShowOnlyActors: walls, floors — use custom depth layer
- UTextureRenderTarget2D* MinimapRenderTarget  ← 256x256
- float UpdateInterval = 0.1s  ← don't update every frame

Functions:
- UpdatePosition(FVector PlayerLocation): move capture to above player
- SetOrthoWidth(float Width): zoom minimap in/out (call from UI zoom controls)

─── ADFMinimapRoom ────────────────────────────────────────────
Actor placed in each dungeon room by DungeonManager:

Properties:
- ERoomType RoomType  ← Normal, Elite, Boss, Treasure, Merchant, Start, Exit
- bool bIsRevealed = false
- bool bIsVisited = false
- FVector RoomCenter
- FVector2D RoomSize

Functions:
- RevealRoom(): bIsRevealed=true, notify WBP_Minimap
- VisitRoom(): bIsVisited=true (player entered)
- GetIconTexture(): return icon by RoomType (boss skull, treasure chest, etc.)

─── WBP_Minimap extends UDFUserWidgetBase ─────────────────────
Minimap widget in HUD corner (200x200px circle mask):

Properties:
- UImage* MinimapTexture          ← UTextureRenderTarget2D displayed here
- UImage* PlayerDot               ← always center (white arrow)
- UCanvasPanel* IconLayer         ← room icons drawn here
- float ZoomLevel = 1.0f
- bool bIsExpanded = false        ← toggle for fullscreen map (M key)
- TArray<ADFMinimapRoom*> KnownRooms

Functions:
- NativeConstruct: bind to UDFDungeonManager::OnRoomRevealed
- UpdateMinimapTexture(): set UTextureRenderTarget2D on MinimapTexture
- OnRoomRevealed(ADFMinimapRoom* Room):
  → Add UDFMinimapIconWidget at projected 2D position
  → Icon type/color by RoomType:
    Normal=grey, Elite=orange, Boss=red, Treasure=yellow, Merchant=green
- UpdatePlayerRotation(float Yaw): rotate PlayerDot arrow
- OnToggleExpanded(): lerp size 200→500, show room labels

UDFMinimapIconWidget (small icon per room on canvas):
- UImage* RoomIcon  (32x32)
- UImage* VisitedOverlay  (dim if not yet visited)
- UTextBlock* RoomLabel   (hidden until expanded)
- Animate: pulse if it's the current room

─── Fog of War ────────────────────────────────────────────────
UDFMinimapFogComponent (on player):
- Each dungeon room starts hidden on minimap (icon invisible)
- On room enter (BeginOverlap with ADFMinimapRoom trigger):
  → Call Room->RevealRoom() → icon appears on minimap
  → Room->VisitRoom() on exit overlap (fully cleared)
- Unexplored rooms: shown as faded silhouette if adjacent to revealed room
  (one-room lookahead via ADFMinimapRoom neighbor array)

Output all .h and .cpp with correct Public/Private paths.
```

---

## ══════════════════════════════════════════════
## 🟢 BAIXA PRIORIDADE
## ══════════════════════════════════════════════

## ════════════════════════════════════════
## 🌊 PROMPT 40 — ELEMENTAL AFFINITY SYSTEM
## ════════════════════════════════════════

```
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-data-assets-tables
@ue-niagara-effects

Create the Elemental Affinity and Weakness system for DungeonForged UE 5.4.
Path convention:
  .h  → Source/DungeonForged/Public/GAS/Elemental/<FileName>.h
  .cpp → Source/DungeonForged/Private/GAS/Elemental/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── EElementType (enum) ───────────────────────────────────────
Values: None, Fire, Ice, Lightning, Earth, Arcane, Physical, True

─── Elemental Advantage Matrix ────────────────────────────────
Define as static constexpr TMap or 2D array in UDFElementalLibrary:
Fire     → strong vs Ice    (1.5x), weak vs Water/Earth (0.7x)
Ice      → strong vs Earth  (1.5x), weak vs Fire (0.7x)
Lightning→ strong vs Water  (1.5x), weak vs Earth (0.7x)
Earth    → strong vs Lightning (1.5x), weak vs Ice (0.7x)
Arcane   → neutral vs all (1.0x) but bypasses resistances

─── FDFElementalAffinityRow : FTableRowBase ───────────────────
File: Public/GAS/Elemental/DFElementalData.h
Per enemy row in DT_EnemyElemental:
- EElementType PrimaryElement       ← enemy's own element
- TMap<EElementType, float> Resistances   ← element → multiplier (0.5=resist, 2.0=weak)
- TMap<EElementType, FGameplayTag> ReactionTags
  ← element → reaction effect tag (Fire+Ice=Frozen, Fire+Earth=Burn+Knockback)

─── UDFElementalReactionSubsystem extends UWorldSubsystem ─────

Functions:
- GetDamageMultiplier(EElementType AttackElement, EElementType TargetElement,
  const FDFElementalAffinityRow& TargetData): float
  → Matrix lookup + TargetData.Resistances override

- CheckElementalReaction(AActor* Target, EElementType IncomingElement):
  → Check target's active GameplayTags for existing elements
  → Fire hit + target has Effect.DoT.Frost tag → Reaction: Melt
    (remove frost, apply GE_Reaction_Melt: 2x fire damage + remove ice)
  → Lightning hit + target has Effect.Debuff.Slow (wet/water) → Reaction: Electrocute
    (apply GE_Reaction_Electrocute: stun 2s + chain to nearby)
  → Ice hit + target has Effect.DoT.Fire → Reaction: Steam
    (apply GE_Reaction_Steam: blind 1s + AOE damage small)
  → Apply ReactionTag from table
  → Spawn reaction Niagara VFX by reaction type

- ApplyElementalDamage(FGameplayEffectSpecHandle& Spec,
  EElementType Element, AActor* Target):
  → Get multiplier
  → Modify Spec SetByCaller(Data.Damage) by multiplier
  → Add element GameplayTag to Spec: Effect.Element.Fire etc.
  → Call CheckElementalReaction

─── UDFElementalComponent extends UActorComponent ────────────
On enemies — stores their elemental profile.

Properties:
- EElementType PrimaryElement
- TMap<EElementType, float> Resistances   ← loaded from DT_EnemyElemental
- FGameplayTag CurrentReactionTag

Functions:
- InitFromTable(FName EnemyRowName): load FDFElementalAffinityRow
- GetResistance(EElementType Element): float
- OnElementalHit(EElementType IncomingElement):
  → Call UDFElementalReactionSubsystem::ApplyElementalDamage + reaction
  → Update CurrentReactionTag
  → Show floating element indicator (via UDFCombatTextSubsystem with element icon)

─── Visual Feedback ───────────────────────────────────────────
Extend UDFFloatingCombatText with element type icons:
- Each element: small icon sprite before damage number
  🔥 Fire: flame icon | ❄️ Ice: snowflake | ⚡ Lightning: bolt
- Weakness hit (multiplier > 1.0): show "WEAK!" text in yellow above number
- Resistance hit (multiplier < 1.0): show "RESIST" in grey
- Reaction hit: show reaction name "FROZEN!" / "ELECTROCUTED!" in element color

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🎲 PROMPT 41 — RANDOM EVENT SYSTEM
## ════════════════════════════════════════

```
@ue-ui-umg-slate
@ue-data-assets-tables
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-serialization-savegames

Create the Random Event system for DungeonForged UE 5.4.
Roguelike narrative events appear between floors — text cards with choices.
Path convention:
  .h  → Source/DungeonForged/Public/Events/<FileName>.h
  .cpp → Source/DungeonForged/Private/Events/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── FDFEventChoice (struct) ───────────────────────────────────
File: Public/Events/DFEventData.h
Fields:
- FText ChoiceText                  ← "Drink from the well"
- FText OutcomeText                 ← shown after choice
- EEventOutcomeType OutcomeType     ← Heal, Damage, Gold, AddAbility, RemoveAbility,
                                       AddEffect, Nothing, RandomGood, RandomBad
- float OutcomeValue                ← amount/percent
- FName AbilityRowName              ← if OutcomeType = AddAbility
- TSubclassOf<UGameplayEffect> EffectClass
- float ChoiceWeight                ← for AI hint (not shown to player)
- FGameplayTagContainer RequiredTags ← only show if player has these tags

─── FDFRandomEventRow : FTableRowBase ─────────────────────────
Fields:
- FText EventTitle
- FText EventDescription
- UTexture2D* EventIllustration     ← atmospheric art
- TArray<FDFEventChoice> Choices    ← 2-4 choices
- int32 MinFloor                    ← earliest floor this can appear
- float Weight                      ← probability weight in pool
- bool bCanRepeat = false           ← roguelike variety
- FGameplayTagContainer EventTags   ← "Event.Dungeon", "Event.Mystical", etc.

─── Pre-defined events (describe in DT_RandomEvents): ────────

Row "MysteriousAltar":
- Title: "Altar das Sombras"
- Choices: A) Sacrifique 25% HP → ganhe ability épica random
           B) Ignore → nada acontece
           C) Destrua o altar → 50 Gold + inimigos ficam agressivos (buff enemies +20% dmg)

Row "WanderingMerchant":
- Title: "Mercador Errante"
- Choices: A) Compre item aleatório por 30 Gold
           B) Roube (Agility check: if Agility>40 → free item; else → 80 dmg + curse)
           C) Troque uma habilidade por outra random

Row "AncientFountain":
- Title: "Fonte Antiga"
- Choices: A) Beba → 50% chance: cura total HP | 50% chance: receba 40% HP de dano
           B) Encha poção → +1 carga de HealthPotion
           C) Passe → nada

Row "FallenHero":
- Title: "Herói Caído"
- Choices: A) Pegue a arma → equipar arma aleatória épica
           B) Honre o guerreiro → receba buff "Blessed" (+15 all stats, 3 floors)
           C) Saque os pertences → 100 Gold mas perde 1 ability slot temporariamente

Row "DemonPact":
- Title: "Pacto Demoníaco"
- Choices: A) Aceite → +80 a todos atributos, -50% HP máximo permanente na run
           B) Recuse → nada
           C) Ataque o demônio → miniboss fight spawns

─── UDFRandomEventSubsystem extends UWorldSubsystem ───────────

Properties:
- UDataTable* EventTable
- TArray<FName> UsedEvents    ← prevent repeats (for bCanRepeat=false)
- float EventChancePerFloor = 0.4f  ← 40% chance between floors

Functions:
- ShouldTriggerEvent(): FMath::FRand() < EventChancePerFloor
- RollEvent(int32 CurrentFloor): 
  → Filter by MinFloor, bCanRepeat, used list
  → Weighted random pick
  → Return FDFRandomEventRow*
- ExecuteChoice(const FDFEventChoice& Choice, ADFPlayerCharacter* Player):
  → Switch on OutcomeType:
    Heal: apply GE heal (OutcomeValue% of MaxHealth)
    Damage: apply GE_Damage_True
    Gold: RunManager->AddGold
    AddAbility: AbilitySelectionSubsystem->GrantSelectedAbility
    RemoveAbility: remove random ability from ASC (brutal roguelike)
    AddEffect: apply EffectClass Infinite to player
    RandomGood/Bad: roll from pool of outcomes
  → Show WBP_EventOutcome (outcome text animation)

─── WBP_RandomEvent extends UDFUserWidgetBase ─────────────────
Full-screen card (similar to WBP_AbilitySelection blocking):

Layout:
- Atmospheric illustration (UImage*, top 40% of card)
- Dark parchment background texture
- UTextBlock* EventTitle (stylized font, center)
- UTextBlock* EventDescription (body text, 3-5 lines)
- TArray<WBP_EventChoiceButton*> (2-4 choices, vertical stack)
  Each button: choice text + hover description of consequence (if player has enough tags/stats)
- No timer — player takes their time (roguelike decision weight)

WBP_EventChoiceButton:
- UButton + UTextBlock* ChoiceText
- UTextBlock* OutcomeHint   ← visible only on hover: "Requires 40 Agility" or "Risky"
- On Click: ExecuteChoice → show WBP_EventOutcome → close after 2s

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🐛 PROMPT 42 — DEBUG & CHEAT SYSTEM
## ════════════════════════════════════════

```
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-ui-umg-slate
@ue-cpp-foundations

Create the Debug and Cheat system for DungeonForged UE 5.4.
All debug code wrapped in #if !UE_BUILD_SHIPPING ... #endif.
Path convention:
  .h  → Source/DungeonForged/Public/Debug/<FileName>.h
  .cpp → Source/DungeonForged/Private/Debug/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── UDFCheatManager extends UCheatManager ─────────────────────
Registered in ADFPlayerController via CheatClass = UDFCheatManager::StaticClass()

Console Commands (UFUNCTION(Exec)):

Player cheats:
- df.god [0|1]      → toggle State.Invulnerable permanent tag on player ASC
- df.levelup [N]    → call LevelingComponent->LevelUp() N times (default 1)
- df.setlevel [N]   → set player to level N, recompute all stats
- df.addxp [N]      → LevelingComponent->AddXP(N)
- df.fullheal       → set Health = MaxHealth via GE instant
- df.fullmana       → set Mana = MaxMana
- df.addgold [N]    → RunManager->AddGold(N)
- df.giveitem [RowName] → UDFInventoryComponent->AddItem(RowName)
- df.giveability [RowName] → grant ability from DT_Abilities row

Dungeon cheats:
- df.nextfloor      → DungeonManager->OnFloorCleared() immediately
- df.skipboss       → destroy all boss actors, trigger victory
- df.spawnboss [RowName] → spawn boss at player location from DT_Enemies
- df.spawnenemy [RowName] [Count] → spawn N enemies around player
- df.killall        → apply GE_Damage_True(99999) to all enemies in level
- df.revealminimap  → reveal all minimap rooms

GAS Debug:
- df.showtags       → toggle WBP_GASDebugOverlay on screen
- df.showattributes → log all attributes to screen (GEngine->AddOnScreenDebugMessage)
- df.granteffect [GEClassName] [Duration] → apply any GE by class name
- df.removeeffect [GEClassName] → remove active GE
- df.clearcd        → remove all Ability.Cooldown.* tags (reset all cooldowns)

─── WBP_GASDebugOverlay extends UUserWidget ───────────────────
In-game debug panel (toggle with df.showtags):

Sections:
1. Active GameplayTags (scrollbox):
   - List all tags on player ASC, color-coded:
     State.*=yellow, Ability.*=blue, Effect.Buff.*=green, Effect.Debuff.*=red
   - Update every 0.2s

2. Attributes panel:
   - Two columns: Attribute Name | Current / Max
   - Highlight in red if below 20% max
   - Health, Mana, Stamina, Strength, Intelligence, Agility, Armor, MagicResist, CritChance, CDR

3. Active Effects (scrollbox):
   - Name | Remaining Duration | Stacks | Source
   - Color: buff=green, debuff=red

4. Ability Slots:
   - Each slot: Tag | Cooldown remaining | Charges

5. Performance stats (top-right corner always visible in non-shipping):
   - FPS, Frame time, Draw calls, Active particles
   - Via UEngine stats

─── UDFDebugComponent extends UActorComponent ─────────────────
On PlayerCharacter — helper for runtime debugging:

Functions:
- DrawAttributeDebug(): GEngine->AddOnScreenDebugMessage all attributes
- DrawAbilityDebug(): log all active abilities + cooldowns
- DrawAIDebug(float Radius): draw debug spheres around all enemies in range
  showing their blackboard state (DRAW_SPHERE + DrawDebugString)
- LogGASEvent(FString Event): UE_LOG to LogDungeonForged category with timestamp

─── Custom Log Category ───────────────────────────────────────
In DungeonForgedModule.h:
DECLARE_LOG_CATEGORY_EXTERN(LogDungeonForged, Log, All)

In DungeonForgedModule.cpp:
DEFINE_LOG_CATEGORY(LogDungeonForged)

Macros for use throughout project:
#define DF_LOG(Verbosity, Format, ...) UE_LOG(LogDungeonForged, Verbosity, TEXT(Format), ##__VA_ARGS__)
#define DF_SCREEN(Color, Duration, Format, ...) GEngine->AddOnScreenDebugMessage(-1, Duration, Color, FString::Printf(TEXT(Format), ##__VA_ARGS__))

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🌐 PROMPT 43 — NETWORKING & REPLICATION AUDIT
## ════════════════════════════════════════

```
@ue-networking-replication
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-gameplay-framework

Perform a complete Networking and Replication audit for DungeonForged UE 5.4.
NOTE: This project targets single-player + optional co-op (2 players).
Path convention:
  .h  → Source/DungeonForged/Public/Network/<FileName>.h
  .cpp → Source/DungeonForged/Private/Network/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── GAS Replication Mode Audit ────────────────────────────────
Review and confirm correct ReplicationMode for each actor:

ADFPlayerState (owns player ASC):
- ASC->SetReplicationMode(EGameplayEffectReplicationMode::Mixed)
  → Server: full GE data | Clients: minimal + tags for UI

ADFEnemyBase (owns enemy ASC):
- ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal)
  → Only gameplay tags replicated to all clients (sufficient for visuals)

ADFBossBase:
- ASC->SetReplicationMode(EGameplayEffectReplicationMode::Full)
  → Boss state fully replicated (all clients see boss phase, effects)

─── DOREPLIFETIME audit ────────────────────────────────────────
Create UDFReplicationAudit.h documenting ALL replicated properties:

ADFPlayerCharacter:
- DOREPLIFETIME(ADFPlayerCharacter, CurrentAbilitySlots)    ← for co-op UI

ADFPlayerState:
- DOREPLIFETIME(ADFPlayerState, AbilitySystemComponent)
- DOREPLIFETIME(ADFPlayerState, AttributeSet)

UDFLevelingComponent:
- DOREPLIFETIME_CONDITION(UDFLevelingComponent, CurrentLevel, COND_OwnerOnly)
- DOREPLIFETIME_CONDITION(UDFLevelingComponent, CurrentXP, COND_OwnerOnly)

UDFInventoryComponent:
- DOREPLIFETIME_CONDITION(UDFInventoryComponent, Items, COND_OwnerOnly)

ADFEnemyBase:
- DOREPLIFETIME(ADFEnemyBase, bIsDead) ← all clients need this

ADFDungeonManager:
- DOREPLIFETIME(ADFDungeonManager, CurrentFloor)
- DOREPLIFETIME(ADFDungeonManager, EnemiesRemaining)

─── RPC Patterns ──────────────────────────────────────────────
Create UDFNetworkLibrary (UBlueprintFunctionLibrary):

Correct RPC usage examples for the project:

Server RPCs (client calls → executes on server):
- Server_RequestEquipItem(FName ItemRow, EEquipmentSlot Slot)
  → WithValidation: check item exists in client inventory
- Server_RequestPurchase(int32 ShopSlotIndex)
  → Validate: player has gold, slot valid
- Server_TriggerAbility(FGameplayTag AbilityTag)
  → GAS handles this natively via ASC->AbilityLocalInputPressed

Client RPCs (server calls → executes on owning client):
- Client_ShowEventCard(FName EventRow)
- Client_ShowLevelUpScreen()
- Client_PlayVictorySequence()

NetMulticast RPCs (server calls → all clients):
- Multicast_SpawnHitVFX(FVector Location, FRotator Normal, FGameplayTag DamageType)
  → Cosmetic only — not gameplay critical
- Multicast_PlayBossRoar(FVector BossLocation)
- Multicast_TriggerPhaseTransitionFX(int32 Phase)

─── Anti-cheat patterns ───────────────────────────────────────
UDFNetworkValidator (UObject on server only):

Rules:
- All ability activations: validate on server before applying effects
  (GAS ASC->ServerActivateAbility handles this natively if set up correctly)
- Gold transactions: only modified on server, replicated to owner
- Damage values: calculated server-side only via GameplayEffectExecution
  (never trust client-sent damage values)
- Position validation: server periodically checks client position vs nav mesh
  → If delta > 500 units in one tick: teleport back (anti-speed-hack)

─── UDFGameInstance extends UGameInstance ─────────────────────
For session management (single-player + future co-op):

Properties:
- bool bIsOnlineSession = false
- FString ServerAddress
- int32 MaxPlayers = 2          ← co-op cap

Functions:
- HostSession(): CreateSession (LAN for now)
- JoinSession(FString Address): ClientTravel
- LeaveSession(): DestroySession + MainMenu travel

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## ⚙️ PROMPT 44 — PERFORMANCE & OPTIMIZATION
## ════════════════════════════════════════

```
@ue-async-threading
@ue-actor-component-archit...
@ue-niagara-effects
@ue-world-level-streaming

Create the Performance and Optimization systems for DungeonForged UE 5.4.
Path convention:
  .h  → Source/DungeonForged/Public/Performance/<FileName>.h
  .cpp → Source/DungeonForged/Private/Performance/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── UDFObjectPoolSubsystem extends UWorldSubsystem ────────────
Generic object pool for frequently spawned/destroyed actors.

Template: TDFObjectPool<T extends AActor>

Properties per pool:
- TArray<T*> AvailableObjects
- TArray<T*> ActiveObjects
- int32 PoolSize
- TSubclassOf<T> ObjectClass

Functions:
- Initialize(TSubclassOf<T> Class, int32 Size): pre-spawn Size inactive objects
- Acquire(FVector Location, FRotator Rotation): T*
  → Pop from AvailableObjects, SetActorLocationAndRotation, SetActorHidden(false)
  → Move to ActiveObjects
- Release(T* Object): SetActorHidden(true), disable collision, push to Available
- PrewarmPool(int32 Count): spawn additional objects ahead of time

Pools to create (register in UDFPerformanceSubsystem):
- ProjectilePool<ADFFireballProjectile> size=20
- ProjectilePool<ADFFrostBoltProjectile> size=20
- ProjectilePool<ADFArcaneMissileProjectile> size=40
- LootDropPool<ADFLootDrop> size=30
- CombatTextPool<UDFCombatTextWidget> size=30 (already in Prompt 32)

─── UDFPerformanceSubsystem extends UWorldSubsystem ───────────

Properties:
- TMap<FName, UDFObjectPoolSubsystem*> PoolRegistry
- float LastProfilingTime = 0.f
- float ProfilingInterval = 5.f

Functions:
- RegisterPool(FName PoolName, ...): add to registry
- GetPool(FName PoolName): retrieve
- TickProfiling(float DeltaTime): every ProfilingInterval, log:
  → Active particle systems count
  → Active enemies count vs pool usage
  → Memory stats: UE_LOG or AddOnScreenDebugMessage in non-shipping

─── Room Culling System ───────────────────────────────────────
UDFRoomCullingComponent (on each dungeon room actor):

Properties:
- TArray<AActor*> RoomActors    ← all actors within this room bounds
- bool bIsVisible = false
- float CullDistance = 3000.f  ← rooms beyond this distance are culled

Functions:
- SetRoomVisible(bool bVisible):
  → For each actor in RoomActors:
    SetActorHiddenInGame(!bVisible)
    SetActorEnableCollision(bVisible)
    SetActorTickEnabled(bVisible)
  → Enemies in hidden rooms: pause Behavior Tree
  → Niagara systems in hidden rooms: DeactivateImmediate

Player proximity detection (in ADFDungeonManager::TickDungeon):
- Every 1s: check all rooms vs player distance
- Rooms within CullDistance * 1.5: SetRoomVisible(true) (preload buffer)
- Rooms beyond: SetRoomVisible(false)

─── Async Asset Loading ───────────────────────────────────────
UDFAssetLoaderSubsystem extends UWorldSubsystem:

Functions:
- PreloadFloorAssets(int32 FloorNumber):
  → Read DT_Dungeon for next floor
  → Collect all asset references (enemy meshes, ability VFX, room meshes)
  → UAssetManager::Get().LoadPrimaryAssets(AssetIds, {}, OnLoadComplete)
  → Called DURING current floor (background loading while player fights)

- PreloadAbilityAssets(TArray<FName> AbilityRows):
  → Load ability montages, Niagara systems, SFX async
  → Callback: broadcast OnAbilityAssetsReady

─── CPU Profiler Scopes ───────────────────────────────────────
Add TRACE_CPUPROFILER_EVENT_SCOPE macros to heavy functions:

In UDFDungeonManager::GenerateDungeon():
  TRACE_CPUPROFILER_EVENT_SCOPE(DungeonForged.GenerateDungeon)

In UDFLootGeneratorSubsystem::RollLoot():
  TRACE_CPUPROFILER_EVENT_SCOPE(DungeonForged.RollLoot)

In UDFAttributeSet::PostGameplayEffectExecute():
  TRACE_CPUPROFILER_EVENT_SCOPE(DungeonForged.AttributeExecution)

In ADFBTService_UpdateTarget::TickNode():
  TRACE_CPUPROFILER_EVENT_SCOPE(DungeonForged.AIUpdateTarget)

─── Niagara Performance ───────────────────────────────────────
UDFNiagaraPoolComponent (on frequently triggered VFX actors):
- Pre-instantiate Niagara system at level load
- On trigger: Activate() instead of SpawnSystemAtLocation (avoid re-creation)
- On complete: Deactivate() (keep component alive for re-use)
- Max concurrent: UNiagaraComponent::SetMaxSimTime to auto-kill long effects

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🌍 PROMPT 45 — LOCALIZATION & ACCESSIBILITY
## ════════════════════════════════════════

```
@ue-cpp-foundations
@ue-ui-umg-slate
@ue-serialization-savegames
@ue-input-system

Create the Localization and Accessibility system for DungeonForged UE 5.4.
Path convention:
  .h  → Source/DungeonForged/Public/Localization/<FileName>.h
  .cpp → Source/DungeonForged/Private/Localization/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── Supported Languages ───────────────────────────────────────
- pt-BR (Portuguese Brazil — default)
- en (English)
- es (Spanish)
- fr (French)

─── StringTable Setup ─────────────────────────────────────────
Create these StringTable assets (describe in code):

ST_UI:          all HUD, menu, button labels
ST_Abilities:   ability names and descriptions
ST_Items:       item names and descriptions
ST_Enemies:     enemy names
ST_Events:      random event text and choices
ST_Tutorials:   hint/tutorial messages

Usage in C++:
- Replace all raw FText::FromString() with LOCTABLE("ST_UI", "Key")
- Example: LOCTABLE("ST_Abilities", "Fireball_Name") → "Bola de Fogo" / "Fireball"
- In DataTable structs: FText fields auto-localized when using StringTable refs

─── UDFLocalizationSubsystem extends UGameInstanceSubsystem ───

Properties:
- ELanguage CurrentLanguage = Portuguese
- FString CurrentCultureCode = "pt-BR"

Functions:
- SetLanguage(ELanguage Language):
  → Map enum to FString culture code
  → FInternationalization::Get().SetCurrentCulture(Code)
  → Save to UDFSaveGame::PreferredLanguage
  → Broadcast OnLanguageChanged → all bound FText widgets auto-refresh
- GetCurrentLanguage(): ELanguage
- GetAvailableLanguages(): TArray<FText> (localized language names)
- Initialize: load language from SaveGame on startup

─── FDFAccessibilitySettings (struct) ─────────────────────────
File: Public/Localization/DFAccessibilityData.h
Fields:
- float UIFontScale = 1.0f          ← 0.8 to 2.0
- bool bHighContrast = false        ← darker backgrounds, thicker UI borders
- bool bReduceMotion = false        ← disable screen shake, reduce VFX intensity
- bool bColorBlindMode = false
- EColorBlindMode ColorBlindType    ← Protanopia, Deuteranopia, Tritanopia
- float MasterVolume = 1.0f
- float MusicVolume = 0.8f
- float SFXVolume = 1.0f

─── UDFAccessibilitySubsystem extends UGameInstanceSubsystem ──

Properties:
- FDFAccessibilitySettings CurrentSettings

Functions:
- ApplySettings(FDFAccessibilitySettings Settings):
  → FontScale: set on UUserInterfaceSettings (global)
  → HighContrast: change UMG global style asset (swap color palette)
  → ReduceMotion:
    → UDFHitStopSubsystem: disable hit stop
    → UDFScreenEffectsComponent: disable chromatic aberration, reduce shake
    → All camera shake: multiply amplitude by 0.1f
  → ColorBlind: apply post-process material with color correction LUT
  → Volume: set in AudioDevice MasterVolume
- SaveSettings(): serialize to UDFSaveGame::AccessibilitySettings
- LoadSettings(): restore on game launch
- GetSettings(): const ref

─── Key Remapping via Enhanced Input ──────────────────────────
UDFInputRemappingSubsystem extends UGameInstanceSubsystem:

Properties:
- TMap<FName, FKey> RemappedKeys   ← IA name → player-set key
- UInputMappingContext* CurrentIMC

Functions:
- RemapKey(FName InputActionName, FKey NewKey):
  → Get UEnhancedInputLocalPlayerSubsystem
  → IMC->MapKey(InputAction, NewKey)
  → RemappedKeys[InputActionName] = NewKey
  → SaveRemapping()
- ResetToDefaults(): reload original IMC mappings
- SaveRemapping(): serialize RemappedKeys to UDFSaveGame::KeyBindings
- LoadRemapping(): restore on game start

─── WBP_OptionsScreen extends UDFUserWidgetBase ───────────────
Full options menu:

Tabs: Audio | Graphics | Controls | Accessibility | Language

Audio tab:
- Sliders: Master, Music, SFX, Voice volumes
- Bind to UDFAccessibilitySubsystem

Controls tab:
- TArray<WBP_KeyBindRow*>: one per InputAction
  Each row: Action label | Current key button (click to remap)
  → On click: enter "listening mode" → detect next key press → RemapKey()
- Reset All button

Accessibility tab:
- Slider: UI Font Size (0.8x → 2.0x, live preview)
- Toggle: High Contrast Mode
- Toggle: Reduce Motion (disables screen shake/hit stop)
- Dropdown: Color Blind Mode (Off / Protanopia / Deuteranopia / Tritanopia)
- Apply + Save button → call UDFAccessibilitySubsystem::ApplySettings + SaveSettings

Language tab:
- TileView of language options with flag icons
- On select: UDFLocalizationSubsystem::SetLanguage
- Preview: show sample text in selected language before confirming

Output all .h and .cpp with correct Public/Private paths.
```

---

## ══════════════════════════════════════════════
## 🎮 ESTRUTURA DE MUNDO & GAMEMODES
## ══════════════════════════════════════════════

## ════════════════════════════════════════
## ⚔️ PROMPT 46 — RUN GAMEMODE & GAMESTATE
## ════════════════════════════════════════

```
@ue-gameplay-framework
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-ui-umg-slate
@ue-networking-replication
@ue-serialization-savegames

Create the Run GameMode and GameState for DungeonForged UE 5.4.
This is the GameMode active DURING a dungeon run (not the Nexus hub).
Path convention:
  .h  → Source/DungeonForged/Public/GameModes/Run/<FileName>.h
  .cpp → Source/DungeonForged/Private/GameModes/Run/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── ADFRunGameMode extends AGameModeBase ──────────────────────

Properties:
- TSubclassOf<ADFPlayerCharacter>  DefaultPlayerClass
- TSubclassOf<ADFRunGameState>     GameStateClass
- TSubclassOf<ADFRunHUD>           HUDClass
- TSubclassOf<ADFRunPlayerController> PlayerControllerClass
- UDataTable* DungeonFloorTable    ← DT_Dungeon
- float RunTimeLimit = 0.f         ← 0 = no time limit

Initialization flow:
- InitGame: initialize UDFRunManager via GameInstance
- PostLogin(APlayerController* PC):
  → Get ETravelReason from UDFRunManager::GetArrivalReason()
  → If NewRun: InitializePlayerFromClass(PC, PendingClass)
  → If NextFloor: UDFRunManager::RestoreRunState(Player)
  → Call ADFDungeonManager::StartFloor(CurrentFloor)
- InitializePlayerFromClass(APlayerController* PC, FName ClassName):
  → Read FDFClassTableRow from DT_Classes
  → Apply FDFClassTableRow.BaseAttributeValues via GE_ClassBaseStats (Instant)
  → Grant FDFClassTableRow.StartingAbilities to ASC
  → Swap player mesh to ClassRow.CharacterMesh
  → Set UDFRunManager::SelectedClass

Victory / Defeat conditions:
- OnFloorCleared (bound to ADFDungeonManager::OnFloorCleared):
  → If CurrentFloor == MaxFloor (10): TriggerVictory()
  → Else: TriggerBetweenFloorSequence()
- OnPlayerDied (bound to UDFAttributeSet death delegate):
  → WaitDelay(3s — death animation)
  → TriggerDefeat()
- TriggerVictory():
  → SetGlobalTimeDilation(0.3) — slow motion celebration
  → Broadcast GameState->SetPhase(Victory)
  → Open WBP_VictoryScreen (ZOrder=20)
  → WaitDelay(5s) → UDFWorldTransitionSubsystem::TravelToNexus(Victory)
- TriggerDefeat():
  → Broadcast GameState->SetPhase(Defeat)
  → Open WBP_DefeatScreen
  → WaitDelay(5s) → UDFWorldTransitionSubsystem::TravelToNexus(Defeat)

Between-floor sequence:
1. GameState->SetPhase(BetweenFloors)
2. UDFRunManager::CaptureRunState()
3. SetGlobalTimeDilation(0)
4. Show WBP_LevelUpScreen (if pending points)
5. Show WBP_AbilitySelection (Prompt 23)
6. Roll UDFRandomEventSubsystem::ShouldTriggerEvent() → show WBP_RandomEvent
7. Show WBP_FloorTransition card ("ANDAR 4")
8. UDFWorldTransitionSubsystem::TravelToNextFloor(CurrentFloor + 1)

─── ADFRunGameState extends AGameStateBase ────────────────────

Replicated:
- int32 CurrentFloor
- float ElapsedRunTime
- int32 TotalKills
- int32 TotalGoldCollected
- ERunPhase CurrentPhase
  (Enum: PreRun, InCombat, BetweenFloors, BossEncounter, Victory, Defeat)

Functions:
- IncrementKills(): TotalKills++, broadcast OnKillsChanged
- AddGold(int32 Amount): TotalGoldCollected += Amount
- SetPhase(ERunPhase Phase): CurrentPhase = Phase, broadcast OnPhaseChanged
- GetRunSummary(): FDFRunSummary
  → FloorReached, Kills, Gold, Time, ClassName, AbilitiesCollected

─── ADFRunPlayerController extends APlayerController ──────────
- SetupInputModeGameplay(): Enhanced Input, hide cursor
- SetupInputModeUI(): show cursor, pause game
- ToggleInventory(): SetupInputModeUI + WBP_CharacterScreen
- OnPause(): WBP_PauseMenu

─── ADFRunHUD extends AHUD ────────────────────────────────────
Widgets:
- WBP_HUD (health/mana/stamina, ability slots, XP bar, gold)
- WBP_Minimap (corner)
- WBP_StatusEffectBar (buffs/debuffs)
- WBP_BossHealthBar (boss floors only)
- WBP_LockOnIndicator (when locked on)
- WBP_FloorCounter (top center: "Andar 3 / 10")
- WBP_KillCounter ("47 abates")

- OnPhaseChanged(ERunPhase Phase):
  → BetweenFloors: hide combat widgets
  → BossEncounter: ShowBossHUD()
  → InCombat: show all

─── WBP_VictoryScreen ─────────────────────────────────────────
- UTextBlock* TitleText          ← "VITÓRIA!" entrada animada
- UTextBlock* TimeText           ← "Tempo: 23:47"
- UTextBlock* KillsText          ← "Inimigos: 147"
- UTextBlock* GoldText           ← "Ouro: 1.240"
- UScrollBox* AbilitiesCollected ← ícones das abilities da run
- UTextBlock* UnlocksEarned      ← "Nova classe: Paladino!"
- UProgressBar* MetaXPBar        ← XP permanente ganho nessa run
- UButton* ReturnNexus + UButton* PlayAgain
- Spawn chuva de partículas douradas Niagara
- UDFMusicManagerSubsystem::SetMusicState(Victory)

─── WBP_DefeatScreen ──────────────────────────────────────────
- UImage* BackgroundArt          ← arte escura desaturada
- UTextBlock* TitleText          ← "VOCÊ MORREU"
- UTextBlock* FloorText          ← "Chegou ao Andar 4"
- UTextBlock* CauseText          ← "Derrotado por: {EnemyName}"
- UProgressBar* MetaXPBar        ← XP permanente ganho mesmo na derrota
- UTextBlock* TipText            ← dica aleatória de DT_Tips
- UButton* ReturnNexus + UButton* PlayAgain
- Post-process desaturado no background
- UDFMusicManagerSubsystem::SetMusicState(Death)

─── WBP_PauseMenu ─────────────────────────────────────────────
- UButton* Resume
- UButton* Options → WBP_OptionsScreen
- UButton* AbandonRun → confirm dialog → TravelToNexus(Abandon)
- Stats da run atual (andar, kills, gold, tempo)
- Fundo com blur material

Output all .h and .cpp files with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🏛️ PROMPT 47 — NEXUS HUB (GAMEMODE + WORLD)
## ════════════════════════════════════════

```
@ue-gameplay-framework
@ue-gameplay-tags
@ue-ui-umg-slate
@ue-data-assets-tables
@ue-serialization-savegames
@ue-actor-component-archit...

Create the Nexus Hub GameMode and world for DungeonForged UE 5.4.
The Nexus is the safe hub between runs — no combat, persistent NPCs,
permanent upgrades, class selection, and dungeon entry portal.
Path convention:
  .h  → Source/DungeonForged/Public/GameModes/Nexus/<FileName>.h
  .cpp → Source/DungeonForged/Private/GameModes/Nexus/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── ADFNexusGameMode extends AGameModeBase ────────────────────

Properties:
- TSubclassOf<ADFNexusPlayerController> PlayerControllerClass
- TSubclassOf<ADFNexusHUD>              HUDClass
- TSubclassOf<APawn>                    NexusPawnClass

Initialization:
- PostLogin:
  → Load UDFSaveGame via UDFRunManager
  → Get ETravelReason from UDFRunManager::GetArrivalReason()
  → Spawn player at correct spawn point:
    NewRun/FirstLaunch: entrance spawnpoint
    Victory: center plaza spawnpoint
    Defeat/Abandon: entrance spawnpoint
  → Play arrival animation/camera via ETravelReason
  → ProcessPendingUnlocks(Save)
- ProcessPendingUnlocks(UDFSaveGame* Save):
  → For each entry in Save->PendingUnlocks:
    UnlockClass: add to Save->UnlockedClasses, reveal NPC
    UnlockNPC: set ADFNexusNPCBase::bIsUnlocked = true
    UnlockUpgrade: mark in CompletedUpgrades
  → Clear PendingUnlocks, SaveGame
  → Queue WBP_UnlockNotification for each unlock

─── ADFNexusGameState extends AGameStateBase ──────────────────

Properties (loaded from SaveGame on init):
- int32 TotalRunsCompleted
- int32 TotalRunsWon
- int32 MetaXP
- int32 MetaLevel
- TArray<FName> UnlockedClasses
- TArray<FName> UnlockedNPCs
- TArray<FName> CompletedUpgrades

Functions:
- AddMetaXP(int32 Amount): MetaXP += Amount, CheckMetaLevelUp(), SaveGame
- CheckMetaLevelUp():
  → Read DT_NexusLevels for next threshold
  → If MetaXP >= required: MetaLevel++, broadcast OnMetaLevelUp
    → ProcessPendingUnlocks triggered by level-up rewards

─── ADFNexusPlayerController ──────────────────────────────────
- Input mode: GameAndUI (cursor visible, character moves freely)
- Bindings: IA_Move, IA_Look, IA_Interact only
- No ability, dodge, sprint, combat input
- IA_Interact → talk to nearest NPC or activate Portal

─── ADFNexusHUD extends AHUD ──────────────────────────────────
- WBP_NexusHUD (MetaXP bar, MetaLevel, run history stats, gold)
- WBP_InteractionPrompt (Prompt 24)
- WBP_UnlockNotification queue

─── ADFNexusNPCBase extends ACharacter ────────────────────────

Properties:
- FText NPCName, NPCDescription
- UAnimMontage* IdleMontage, TalkMontage
- bool bIsUnlocked = false
- FName UnlockConditionRow     ← from DT_NexusUnlockConditions
- UWidgetComponent* NameplateWidget
- TSubclassOf<UUserWidget> ServiceWidgetClass

Functions:
- BeginPlay: SetActorHiddenInGame(!bIsUnlocked)
- OnInteract: face player, PlayMontage(TalkMontage), create ServiceWidget
- CheckUnlockCondition(UDFSaveGame* Save): bool
  → Reads condition: RunsCompleted >= N, WinsCompleted >= N, MetaLevel >= N

NPC Implementations:

1. ADFNexusNPC_Blacksmith (Ferreiro — sempre desbloqueado):
   WBP_Blacksmith:
   - Upgrade de dano de arma (permanente por run): gasta MetaXP + Gold
   - Reforge de raridade de item: gamble com Gold para subir tier
   - DataTable: DT_BlacksmithUpgrades (RowName, MetaXPCost, GoldCost, GE aplicado)

2. ADFNexusNPC_Sage (Sábio — unlock: 3 runs completas):
   WBP_AbilityShrine:
   - Aprender habilidade passiva permanente (aparece em toda run)
   - Desbloquear nova branch de habilidades no DT_Abilities
   - DataTable: DT_SageUnlocks

3. ADFNexusNPC_Alchemist (Alquimista — unlock: 1 vitória):
   WBP_Alchemy:
   - Criar poções permanentes (começa cada run com X cargas)
   - Combinar itens para meta-upgrades
   - DataTable: DT_AlchemyRecipes (ingredientes → resultado)

4. ADFNexusNPC_Chronicler (Cronista — sempre desbloqueado):
   WBP_RunHistory:
   - Histórico de todas as runs (de UDFSaveGame::RunHistory)
   - Bestiário (inimigos encontrados, fraquezas descobertas)
   - Estatísticas totais: kills lifetime, mortes, tempo jogado, ouro total

5. ADFNexusNPC_Merchant (Mercador Permanente — unlock: 5 runs):
   WBP_NexusMerchant (reutiliza WBP_Shop do Prompt 33):
   - Estoque refresca a cada 3 runs
   - Aceita MetaXP OU Gold
   - Vende itens passivos permanentes (persistem entre runs)

─── ADFRunPortal extends ADFInteractableBase ──────────────────

Properties:
- UNiagaraComponent* PortalVFX    ← sempre ativo, swirling portal
- UPointLightComponent* PortalLight (cor muda por MetaLevel)

Interact_Implementation:
1. Open WBP_ClassSelection (fullscreen)
2. Player selects class + reviews meta-upgrades
3. Confirm → UDFWorldTransitionSubsystem::TravelToRun(SelectedClass)

─── WBP_ClassSelection ────────────────────────────────────────

Layout:
- Left: TileView de classes desbloqueadas (WBP_ClassCard por classe)
- Center: SceneCapture 3D preview do personagem selecionado
- Right: stats base + passivas da classe + abilities iniciais
- Bottom: "Iniciar Run" (disabled até escolher) + "Ver Upgrades"

WBP_ClassCard:
- UImage* ClassArt
- UTextBlock* ClassName + breve descrição
- UProgressBar* Bars de stats (Força, Int, Agi, Defesa)
- Lock overlay se não desbloqueada (mostra condição de unlock)

─── WBP_NexusHUD ──────────────────────────────────────────────
- UTextBlock* MetaLevelText     ← "Nexus Nv. 7"
- UProgressBar* MetaXPBar
- UTextBlock* RunStats          ← "Runs: 23 | Vitórias: 8"
- Área de notificações (WBP_UnlockNotification)

─── WBP_UnlockNotification ────────────────────────────────────
- UImage* UnlockIcon
- UTextBlock* Title             ← "Nova Classe Desbloqueada!"
- UTextBlock* Name              ← "Paladino"
- Slide-in da direita, auto-dismiss após 4s
- Fila: mostra um por vez

─── FDFNexusLevelRow : FTableRowBase ──────────────────────────
File: Public/GameModes/Nexus/DFNexusLevelData.h
- int32 NexusLevel
- int32 MetaXPRequired
- FText UnlockDescription
- FName UnlockNPCRow
- FName UnlockClassRow
- TArray<FName> UnlockUpgradeRows

Output all .h and .cpp files with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🌀 PROMPT 48 — WORLD TRANSITION SYSTEM
## ════════════════════════════════════════

```
@ue-gameplay-framework
@ue-world-level-streaming
@ue-serialization-savegames
@ue-ui-umg-slate
@ue-gameplay-tags
@ue-async-threading

Create the World Transition System for DungeonForged UE 5.4.
Handles all travel between Nexus ↔ Run, floor-to-floor checkpoints,
loading screens temáticas e preservação de estado via GameInstance.
Path convention:
  .h  → Source/DungeonForged/Public/World/<FileName>.h
  .cpp → Source/DungeonForged/Private/World/<FileName>.cpp
Always write the full path as a comment at the top of each file.

─── ETravelReason (enum) ──────────────────────────────────────
File: Public/World/DFWorldTypes.h
Values: NewRun, NextFloor, Victory, Defeat, AbandonRun, FirstLaunch

─── UDFWorldTransitionSubsystem extends UGameInstanceSubsystem ─

Properties:
- ETravelReason PendingReason
- FName         PendingClass
- bool          bIsTransitioning = false
- FString       NexusMapName = "Nexus"
- FString       RunMapName   = "DungeonRun"

Functions:

TravelToRun(FName SelectedClass):
  → Guard: if bIsTransitioning return
  → bIsTransitioning = true
  → PendingReason = NewRun, PendingClass = SelectedClass
  → UDFRunManager::CaptureRunState() (fresh state)
  → SaveCheckpoint(RunStart)
  → UDFLoadingScreenSubsystem::ShowLoadingScreen(NewRun)
  → UGameplayStatics::OpenLevel(World, RunMapName)

TravelToNextFloor(int32 NextFloor):
  → PendingReason = NextFloor
  → UDFRunManager::CaptureRunState()
  → SaveCheckpoint(FloorComplete)
  → UDFLoadingScreenSubsystem::ShowLoadingScreen(NextFloor)
  → OpenLevel(RunMapName) ← mesma map, GameMode reinicia o floor

TravelToNexus(ETravelReason Reason):
  → PendingReason = Reason
  → FinalizeRunData(Reason)
  → SaveCheckpoint(RunEnd)
  → UDFLoadingScreenSubsystem::ShowLoadingScreen(Reason)
  → OpenLevel(NexusMapName)

FinalizeRunData(ETravelReason Reason):
  → Get FDFRunSummary from ADFRunGameState
  → Compute MetaXP:
    Victory:  500 + (Floor * 50) + (Kills * 2)
    Defeat:   100 + (Floor * 20) + (Kills * 1)
    Abandon:  25  + (Floor * 5)
  → UDFSaveGame: increment TotalRuns, update BestFloor, BestKills, TotalTimePlayed
  → Add earned MetaXP to Save->MetaXP
  → If Victory: evaluate unlock conditions → append to PendingUnlocks
  → Serialize and write SaveGame to disk

SaveCheckpoint(ECheckpointType Type):
  → Serialize UDFRunManager::CurrentRunState to UDFSaveGame::LastCheckpoint
  → Allows resume if game crashes between floors

─── UDFLoadingScreenSubsystem extends UGameInstanceSubsystem ──

Properties:
- TSubclassOf<UUserWidget> LoadingScreenClass
- UUserWidget* ActiveLoadingScreen
- float MinLoadingTime = 2.0f
- float LoadingStartTime

Functions:
- ShowLoadingScreen(ETravelReason Reason):
  → Create widget (ZOrder=100), configure by reason:
    NewRun:    título "Gerando Dungeon..." + lore da classe + dica
    NextFloor: título "Andar {N}..." + ícones dos inimigos do próximo andar
    Victory:   título "Retornando ao Nexus..." + música de vitória
    Defeat:    título "Retornando ao Nexus..." + música sombria + frase motivacional
  → Fake progress bar: lerp 0→0.9 em 1.5s, snap 1.0 no PostLoadMap
  → Register FCoreUObjectDelegates::PostLoadMapWithWorld → HideLoadingScreen

- HideLoadingScreen():
  → Enforce MinLoadingTime: WaitDelay se necessário
  → Fade out 0.5s → RemoveFromParent
  → bIsTransitioning = false

─── WBP_LoadingScreen extends UUserWidget ─────────────────────

Layout base (todos os tipos):
- UImage* BackgroundArt        ← arte temática (dungeon/nexus) por Reason
- UImage* LogoImage            ← logo DungeonForged
- UTextBlock* LoadingTitle     ← "Gerando Dungeon..." / "VITÓRIA!" etc.
- UTextBlock* FlavorText       ← lore da classe ou lore do dungeon
- UProgressBar* LoadingBar     ← smooth fill com stutter realista aos 85%
- UTextBlock* TipLabel + UTextBlock* TipText ← dica aleatória de DT_Tips

Variante NextFloor (adicional):
- UHorizontalBox* EnemyPreview ← 3 ícones de inimigos do próximo andar
- UTextBlock* FloorNumber      ← "ANDAR 4 / 10"
- UTextBlock* FloorDifficulty  ← "⚠ Andar Élite" / "💀 BOSS"
- UProgressBar* RunProgress    ← barra de progresso geral da run

Animações:
- Entrada: fade in 0.3s
- LoadingBar: fill suave, pausa em 85% (stutter realista)
- Logo: breathing scale animation sutil
- Saída: fade out 0.5s

─── UDFRunManager — extensão para GameInstance ────────────────
Extend UDFRunManager (Prompt 13):

New Properties:
- ETravelReason LastTravelReason
- FName         PendingSelectedClass
- FDFRunState   RestoredRunState    ← snapshot completo da run

New Functions:
- CaptureRunState():
  → Serialize: CurrentFloor, Gold, XP, Level, EquippedItems,
    GrantedAbilities, Health%, Mana%, ComboPoints, AbilityHistory
- RestoreRunState(ADFPlayerCharacter* Player):
  → Called by ADFRunGameMode::PostLogin on NextFloor travel
  → Re-apply: Health/Mana via GE, gold via RunManager, re-grant abilities
  → Skip restore if reason == NewRun
- GetArrivalReason(): ETravelReason
- SetPendingClass(FName ClassName)

─── Map World Settings ────────────────────────────────────────
Document as comments in UDFWorldTransitionSubsystem:

Map "Nexus":
  GameMode:             ADFNexusGameMode
  DefaultPawnClass:     ADFNexusPawn (sem combate, animações relaxadas)
  PlayerControllerClass: ADFNexusPlayerController
  Sky:                  sempre entardecer (eternal golden hour)
  Ambient:              MetaSound ambiente místico/tranquilo

Map "DungeonRun":
  GameMode:             ADFRunGameMode
  DefaultPawnClass:     ADFPlayerCharacter
  PlayerControllerClass: ADFRunPlayerController
  NavMesh:              regenerado após PCG geração do dungeon
  Sky:                  nenhum (dungeon subterrâneo, só luz artificial)
  Ambient:              MetaSound dungeon (água pingando, ecos distantes)

─── Fluxo Completo do Jogo (State Machine) ────────────────────
Document as block comment in UDFWorldTransitionSubsystem.cpp:

/*
  PRIMEIRO LAUNCH
        ↓
  [Nexus — ETravelReason::FirstLaunch]
  ADFNexusGameMode → Ferreiro + Cronista desbloqueados
        ↓  (Portal → WBP_ClassSelection → Confirmar)
  TravelToRun(ClassName) → LoadingScreen "Gerando Dungeon..."
        ↓
  [DungeonRun — Floor 1]
  ADFRunGameMode::PostLogin → InitializePlayerFromClass
        ↓
  Combate → Loot → Floor Cleared
        ↓
  BetweenFloorSequence:
    LevelUp → AbilitySelection → RandomEvent → FloorTransition card
        ↓
  TravelToNextFloor(2) → LoadingScreen "Andar 2..."
        ↓
  ... Andares 2-9 ...
        ↓
  [Floor 10 — Boss]
  ADFBossTriggerVolume → Boss encounter
        ↓
  ┌─── Boss derrotado ────────────────┬─── Jogador morreu ───────────────┐
  TriggerVictory()              TriggerDefeat()                AbandonRun()
  WBP_VictoryScreen             WBP_DefeatScreen               WBP_PauseMenu
  WaitDelay(5s)                 WaitDelay(5s)                  Confirm
        ↓                             ↓                               ↓
  FinalizeRunData(Victory)      FinalizeRunData(Defeat)    FinalizeRunData(Abandon)
  MetaXP: 500 + bonuses         MetaXP: 100 + bonuses      MetaXP: 25 + bonuses
  Check unlocks → Pending       Sem unlocks de vitória      Sem unlocks
        ↓                             ↓                               ↓
  LoadingScreen "Vitória!"      LoadingScreen "Retornando..." LoadingScreen
        ↓                             ↓                               ↓
  [Nexus — ETravelReason::Victory/Defeat/Abandon]
  ADFNexusGameMode::PostLogin
  → ProcessPendingUnlocks()
  → SpawnAtCorrectPoint()
  → WBP_UnlockNotification queue
        ↓
  Player explora Nexus:
  Ferreiro → Sábio → Alquimista → Mercador → Cronista → Portal
        ↓
  Loop → Nova run
*/

Output all .h and .cpp files with correct Public/Private paths.
```

---

## 📊 ÍNDICE COMPLETO — PARTE 2

| Prompt | Sistema | Prioridade | Pasta |
|---|---|---|---|
| 31 | Level & XP System | 🔴 Alta | `Progression/` |
| 32 | Floating Combat Text | 🔴 Alta | `UI/Combat/` |
| 33 | Merchant / Shop | 🔴 Alta | `Merchant/` |
| 34 | Trap & Hazard | 🔴 Alta | `Dungeon/Traps/` |
| 35 | Audio (MetaSounds) | 🟡 Média | `Audio/` |
| 36 | Status Effect HUD | 🟡 Média | `UI/Status/` |
| 37 | Equipment Visual | 🟡 Média | `Equipment/` |
| 38 | Hit Stop & Screen FX | 🟡 Média | `FX/` |
| 39 | Minimap System | 🟡 Média | `UI/Minimap/` |
| 40 | Elemental Affinity | 🟢 Baixa | `GAS/Elemental/` |
| 41 | Random Event System | 🟢 Baixa | `Events/` |
| 42 | Debug & Cheat System | 🟢 Baixa | `Debug/` |
| 43 | Networking & Replication | 🟢 Baixa | `Network/` |
| 44 | Performance & Optimization | 🟢 Baixa | `Performance/` |
| 45 | Localization & Accessibility | 🟢 Baixa | `Localization/` |
| **46** | **Run GameMode & GameState** | **🔴 Alta** | `GameModes/Run/` |
| **47** | **Nexus Hub GameMode + World** | **🔴 Alta** | `GameModes/Nexus/` |
| **48** | **World Transition System** | **🔴 Alta** | `World/` |

**Ordem recomendada — Parte 2 completa:**
```
48 → 46 → 47 → 31 → 32 → 33 → 34 → 38 → 35 → 36 → 37 → 39 → 40 → 41 → 42 → 43 → 44 → 45
```
> ⚠️ Prompts **46, 47, 48** devem ser executados **antes** de todos os outros da Parte 2,
> pois definem a estrutura de GameMode que os sistemas dependem.

> ⚠️ Sempre execute o **Prompt 0** (arquivo Parte 1) antes de qualquer prompt deste arquivo.

---

*DungeonForged — Cursor Prompts PARTE 2 | v2.0 | Prompts 31–48 | GameModes + Alta + Média + Baixa*