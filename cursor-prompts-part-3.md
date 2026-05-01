# 🤖 CURSOR PROMPTS — DungeonForged (UE5.4) — PARTE 3
> Prompts 49–70 | Novas Classes · Progressão · Narrativa · Polish · Meta
> Sempre cole o **Prompt 0** do arquivo CURSOR_PROMPTS.md antes de qualquer prompt deste arquivo.

---

## ══════════════════════════════════════════════
## 🧙 NOVAS CLASSES
## ══════════════════════════════════════════════

## ════════════════════════════════════════
## ⚜️ PROMPT 49 — 4ª CLASSE: PALADINO
## ════════════════════════════════════════

```
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-animation-system
@ue-niagara-effects
@ue-actor-component-archit...

Create all Paladin class abilities for DungeonForged UE 5.4.
The Paladin is a hybrid tank/support — holy damage, shields, and heals.
Resource: Holy Power (FGameplayAttribute, 0-5 stacks — builder/spender like Rogue Combo Points).
All abilities extend UDFGameplayAbility.
Path convention:
  .h  → Source/DungeonForged/Public/GAS/Abilities/Paladin/<FileName>.h
  .cpp → Source/DungeonForged/Private/GAS/Abilities/Paladin/<FileName>.cpp
Always write full path as comment at top of each file.

─── UDFHolyPowerComponent extends UActorComponent ─────────────
File: Public/GAS/Abilities/Paladin/DFHolyPowerComponent.h
Same pattern as UDFComboPointsComponent (Prompt 27):
- int32 CurrentHolyPower = 0, Max = 5
- AddHolyPower(int32), SpendHolyPower(int32): bool, Reset
- Broadcast OnHolyPowerChanged(int32 New, int32 Max)
- WBP_HolyPowerBar: 5 pip icons that glow gold when filled

─── GA_Paladin_Judgment ───────────────────────────────────────
Tag: Ability.Paladin.Judgment
Cost: 20 Mana | Cooldown: 6s
Type: Melee strike with holy radiance — main builder

Flow:
1. CommitAbility
2. PlayMontageAndWait (JudgmentMontage — overhead hammer smash, 0.7s)
3. On AN_TraceStart:
   - Melee trace in front (BoxTrace 80x80x100cm)
   - Damage = Strength * 0.8 + Intelligence * 0.6 (hybrid holy damage)
   - Apply GE_Damage_Holy (new damage type — bypasses physical armor, uses MagicResist)
   - AddHolyPower(1)
   - If target has Effect.DoT.Fire or Effect.Debuff.*: AddHolyPower(2) instead
     (punishing weakened enemies generates more holy power)
   - Spawn golden impact Niagara (holy cross flash)
4. EndAbility

─── GA_Paladin_HolyShield ─────────────────────────────────────
Tag: Ability.Paladin.HolyShield
Cost: 30 Mana | Cooldown: 20s
Type: AOE aura shield — protects player + nearby allies

Flow:
1. CommitAbility
2. PlayMontageAndWait (ShieldRaiseMontage — arm raised, divine light, 0.6s)
3. Apply GE_HolyShield (HasDuration 8s):
   - Modifier: Armor += 60, MagicResist += 40
   - GrantedTag: State.HolyShielded
   - Period 1s: SphereOverlap radius=500
     → Apply GE_Buff_HolyAura (1.5s renewable) to all allies:
       Armor += 20, health regen tick
4. Spawn radiant bubble Niagara around player (golden translucent sphere)
5. WaitGameplayEffectRemoved:
   - Burst VFX on expiry
   - EndAbility

─── GA_Paladin_HammerOfJustice ────────────────────────────────
Tag: Ability.Paladin.HammerOfJustice
Cost: 3 Holy Power (Spender) | Cooldown: 2s
Type: Finisher — thrown hammer, AOE stun on return

Flow:
1. CanActivateAbility: HolyPowerComponent->CurrentHolyPower >= 3
2. CommitAbility, HolyPowerComponent->SpendHolyPower(3)
3. PlayMontageAndWait (HammerThrowMontage — spinning throw, 0.8s)
4. Spawn ADFHammerProjectile:
   - UProjectileMovementComponent: speed=1800, range=1200
   - On reach max range: reverse direction (boomerang return)
   - OnHit (outbound): apply GE_Damage_Holy (Intelligence * 1.5) + GE_Debuff_Stun (1.5s)
   - OnHit (return): apply GE_Damage_Holy (Intelligence * 0.8) (hits again on return)
   - On return to player: AddHolyPower(1) (partial refund)
   - Spawn spinning golden hammer Niagara trail
5. EndAbility after hammer returns (or 3s timeout)

─── GA_Paladin_HolyWord ───────────────────────────────────────
Tag: Ability.Paladin.HolyWord
Cost: 40 Mana | Cooldown: 12s
Type: Targeted heal (self or ally via LockOn)

Flow:
1. CommitAbility
2. PlayMontageAndWait (HealingCastMontage — hands glow, prayer pose, 1.0s)
3. Determine target: LockOnComponent->CurrentTarget if friendly, else self
4. Apply GE_HolyWord_Heal (Instant):
   - SetByCaller Data.Healing = MaxHealth * 0.35 + (Intelligence * 3)
   - If target below 30% HP: bonus GE_HolyWord_CritHeal (additional 20% MaxHealth)
5. Apply GE_Buff_HolyBlessing (HasDuration 6s) to target:
   - Modifier: HealthRegen += MaxHealth * 0.03 per second
   - GrantedTag: Effect.Buff.Blessed
6. Spawn healing beam Niagara from caster to target (golden rays)
7. AddHolyPower(1)
8. EndAbility

─── GA_Paladin_Redemption ─────────────────────────────────────
Tag: Ability.Paladin.Redemption
Cost: 5 Holy Power (full bar) | Cooldown: 90s
Type: Ultimate — cheat death OR revive effect

Flow:
1. CanActivateAbility: HolyPowerComponent->CurrentHolyPower == 5
2. CommitAbility, HolyPowerComponent->Reset()
3. PlayMontageAndWait (RedemptionMontage — arms spread wide, divine explosion, 1.5s)
4. AOE burst radius=600:
   - Apply GE_Damage_Holy (Intelligence * 4.0) to all enemies
   - Apply GE_Debuff_Stun (2s) to all enemies hit
5. Apply GE_Redemption_Blessing to self (HasDuration 15s):
   - GrantedTag: State.Redeemed
   - Inner passive: if Health would drop to 0 while Redeemed:
     → Intercept death (same pattern as GA_SecondWind Prompt 29)
     → Restore Health to 50% MaxHealth
     → Remove State.Redeemed (one use)
     → Spawn massive holy burst VFX + camera shake
6. Spawn pillar of light Niagara at player position
7. EndAbility

─── GA_Paladin_Consecration ───────────────────────────────────
Tag: Ability.Paladin.Consecration
Cost: 50 Mana | Cooldown: 18s
Type: Ground AOE — persistent holy zone

Flow:
1. CommitAbility
2. PlayMontageAndWait (ConsecrationMontage — kneel + slam ground, 0.8s)
3. Spawn ADFConsecrationZone at player feet:
   - Radius=400, Duration=8s
   - UDecalComponent: golden glowing circle on floor
   - UNiagaraComponent: rising holy light columns
   - Period 0.5s tick:
     → SphereOverlap enemies: apply GE_Damage_Holy (Intelligence * 0.5)
     → SphereOverlap player/allies: apply GE_Buff_Consecrated (1s renewable):
       Armor += 15, auto-AddHolyPower(1) every 2s while standing in zone
4. EndAbility immediately (zone self-manages, destroyed after Duration)

─── GA_Passive_Paladin_DivineAura ─────────────────────────────
Tag: Ability.Passive.Paladin.DivineAura
Pattern: Passive (Prompt 28 pattern)

GE_DivineAura (Infinite):
- Modifier: MaxHealth += (Intelligence * 8) ← Paladin scales HP with INT
- Modifier: Armor += 15 flat
- Period 3s: if ally within 600 units has Health < 50%
  → auto-apply GE_Buff_SmallHeal (Instant, +5% MaxHealth) to that ally

Output all 7 files (HolyPowerComponent + 6 abilities) with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 💀 PROMPT 50 — 5ª CLASSE: NECROMANTE
## ════════════════════════════════════════

```
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-animation-system
@ue-niagara-effects
@ue-physics-collision
@ue-actor-component-archit...

Create all Necromancer class abilities for DungeonForged UE 5.4.
The Necromancer is a summoner/dark mage — controls undead, consumes corpses.
Resource: Essences (collected from enemy corpses — unique mechanic).
All abilities extend UDFGameplayAbility.
Path convention:
  .h  → Source/DungeonForged/Public/GAS/Abilities/Necromancer/<FileName>.h
  .cpp → Source/DungeonForged/Private/GAS/Abilities/Necromancer/<FileName>.cpp
Always write full path as comment at top of each file.

─── UDFEssenceComponent extends UActorComponent ───────────────
File: Public/GAS/Abilities/Necromancer/DFEssenceComponent.h

Properties:
- int32 CurrentEssences = 0, MaxEssences = 10
- TArray<TWeakObjectPtr<AActor>> NearbyCorpses ← tracked dead enemies
- float CorpseDecayTime = 30.f

Functions:
- OnEnemyDied(AActor* Enemy, FVector Location):
  → Mark enemy as Corpse (add GameplayTag State.IsCorpse to dead enemy ASC)
  → Register in NearbyCorpses
  → Start decay timer per corpse
- HarvestCorpse(AActor* Corpse): AddEssences(2), remove corpse tag, destroy actor
- AddEssences(int32 Amount): clamp to MaxEssences, broadcast OnEssencesChanged
- SpendEssences(int32 Amount): bool
- GetNearestCorpse(float MaxRange=800): AActor* ← for auto-harvest abilities
- WBP_EssenceBar: dark purple orb counter (10 orbs)

─── ADFSkeletonMinion extends ACharacter ──────────────────────
Summoned minion actor (not a full enemy, simplified AI):

Properties:
- TWeakObjectPtr<AActor> NecromancerOwner
- float LifeSpan = 60.f               ← minions expire
- float MinionDamage = 20.f
- UBehaviorTreeComponent* BTComp      ← simplified BT: find nearest enemy → attack

Functions:
- InitMinion(AActor* Owner, float DmgMultiplier):
  → Set owner reference, scale damage by Necromancer Intelligence
  → Start lifespan timer
- OnMinionDied: notify UDFMinionManagerComponent, spawn bone pile VFX

─── UDFMinionManagerComponent extends UActorComponent ─────────
On Necromancer PlayerCharacter:

Properties:
- TArray<TWeakObjectPtr<ADFSkeletonMinion>> ActiveMinions
- int32 MaxMinions = 6

Functions:
- SpawnMinion(FVector Location, float DmgMultiplier): ADFSkeletonMinion*
  → if ActiveMinions.Num() >= MaxMinions: destroy oldest minion first
  → SpawnActor ADFSkeletonMinion, InitMinion, add to array
- OnMinionDied(ADFSkeletonMinion* Minion): remove from array
- GetMinionCount(): int32
- CommandMinions(AActor* Target): all minions set new BT target
- DismissAllMinions(): destroy all active minions instantly

─── GA_Necromancer_RaiseSkeleton ──────────────────────────────
Tag: Ability.Necromancer.RaiseSkeleton
Cost: 3 Essences | Cooldown: 2s
Type: Summon — raises skeleton from nearest corpse

Flow:
1. CanActivateAbility: EssenceComponent->CurrentEssences >= 3
   AND EssenceComponent->GetNearestCorpse(800) != nullptr
2. CommitAbility, SpendEssences(3)
3. Get nearest corpse location
4. PlayMontageAndWait (SummonMontage — dark gesture, 0.5s)
5. EssenceComponent->HarvestCorpse(Corpse)
6. MinionManagerComponent->SpawnMinion(CorpseLocation, Intelligence * 0.3)
7. Spawn bone-rise Niagara at corpse location (skeleton emerging from ground)
8. Skeleton plays rise animation, enters AI combat
9. AddEssences(0) — no refund but corpse was worth the investment
10. EndAbility

─── GA_Necromancer_CorpseExplosion ────────────────────────────
Tag: Ability.Necromancer.CorpseExplosion
Cost: 2 Essences | Cooldown: 3s
Type: AOE burst — detonates nearest corpse

Flow:
1. CanActivateAbility: EssenceComponent->GetNearestCorpse(1200) valid
2. CommitAbility, SpendEssences(2)
3. Get target corpse (nearest OR locked-on dead enemy if valid)
4. PlayMontageAndWait (ExplosionCastMontage — pointing gesture, 0.4s)
5. Apply visual on corpse: brief dark glow (0.4s telegraph)
6. Destroy corpse actor
7. SphereOverlap radius=350 at corpse location:
   - Apply GE_Damage_True (Intelligence * 2.0 + MinionManagerComponent->GetMinionCount() * 20)
     ← more minions = stronger explosion (bone shrapnel from army)
   - Apply GE_DoT_Poison (3s) to all hit
   - Spawn massive gore/dark explosion Niagara
   - Camera shake (moderate)
8. AddEssences(1) ← small refund from the blast essence
9. EndAbility

─── GA_Necromancer_Curse ──────────────────────────────────────
Tag: Ability.Necromancer.Curse
Cost: 20 Mana | Cooldown: 8s
Type: Single target debuff — weakens and marks for death

Flow:
1. CommitAbility
2. PlayMontageAndWait (CurseMontage — sinister finger point, 0.5s)
3. Determine target: LockOnTarget or nearest enemy in 800 range
4. Apply GE_Curse (HasDuration 12s) to target:
   - Modifier: Armor -= 40 (armor shattered)
   - Modifier: MagicResist -= 30
   - GrantedTag: Effect.Debuff.Cursed, Effect.Debuff.Marked
   - Period 2s: apply GE_DoT_Decay (dark damage tick, Intelligence * 0.4)
   - On death while cursed: auto-generate Essence (+2) + corpse persists longer (60s)
5. Spawn dark hex symbol Niagara above target (rotating runes)
6. AddEssences(1) ← anticipation bonus
7. EndAbility

─── GA_Necromancer_DrainLife ──────────────────────────────────
Tag: Ability.Necromancer.DrainLife
Cost: 15 Mana per second (channeled) | Cooldown: 6s
Type: Channeled drain beam

Flow:
1. CommitAbility
2. PlayMontageAndWait looping (DrainMontage — sustained dark beam gesture)
3. Spawn ADFDrainBeamActor between player and target:
   - UNiagaraComponent: dark purple/red energy beam
   - Updates position each tick to follow player hand socket → target
4. WaitGameplayEvent loop (0.3s ticks via AnimNotify):
   - Each tick: apply GE_DrainTick (Instant) to target:
     → Target: Health -= Intelligence * 0.5 (true damage)
     → Self: Health += Intelligence * 0.3 (60% lifesteal)
   - Drain 15 Mana each tick (via GE_Cost_DrainTick)
   - If target dies mid-channel: AddEssences(3) bonus (drained to death)
   - If Mana hits 0 or WaitInputRelease: EndDrain
5. On EndDrain: destroy ADFDrainBeamActor
6. EndAbility

─── GA_Necromancer_UndeadArmy ─────────────────────────────────
Tag: Ability.Necromancer.UndeadArmy
Cost: 8 Essences (max spend) | Cooldown: 60s
Type: Ultimate — mass summon wave

Flow:
1. CanActivateAbility: EssenceComponent->CurrentEssences >= 4 (flexible cost)
2. CommitAbility
3. Cache int32 EssencesSpent = min(CurrentEssences, 8)
4. SpendEssences(EssencesSpent)
5. PlayMontageAndWait (ArmyRiseMontage — both arms raised, 1.5s epic cast)
6. Spawn VFX: dark energy pillar, screen darkens, wind effect
7. For i = 0 to EssencesSpent - 1:
   - Find spawn location in arc around player (evenly spaced 360°)
   - WaitDelay(i * 0.15s) ← staggered rise for dramatic effect
   - MinionManagerComponent->SpawnMinion(SpawnLoc, Intelligence * 0.5)
   - Each skeleton rises with bone-emerge Niagara
8. Apply GE_UndeadArmy_Aura (HasDuration 20s, while army lives):
   - All minions: +50% damage, +100% HP
   - GrantedTag: State.UndeadArmyActive
9. EndAbility after all spawned

─── GA_Passive_Necromancer_DeathMastery ───────────────────────
Tag: Ability.Passive.Necromancer.DeathMastery

GE_DeathMastery (Infinite):
- Modifier: Intelligence += (ActiveMinions * 5) ← more minions = smarter mage
- Modifier: MaxEssences += 2 (passive extends resource cap)
- Period 5s: foreach active minion with HP < 30%:
  → auto-apply GE_Buff_MinionHeal (Instant, +20% max HP) keeping them alive longer

Output all 8 files (EssenceComponent + MinionManager + ADFSkeletonMinion + 5 abilities + passive)
with correct Public/Private paths.
```

---

## ══════════════════════════════════════════════
## 🌳 PROGRESSÃO VISUAL & DESAFIOS
## ══════════════════════════════════════════════

## ════════════════════════════════════════
## 🌳 PROMPT 51 — SKILL TREE VISUAL (META-UPGRADES)
## ════════════════════════════════════════

```
@ue-ui-umg-slate
@ue-data-assets-tables
@ue-serialization-savegames
@ue-gameplay-abilities
@ue-gameplay-tags

Create the visual Skill Tree for permanent meta-upgrades in DungeonForged UE 5.4.
Located in the Nexus — accessed via Sábio NPC (Prompt 47).
Path convention:
  .h  → Source/DungeonForged/Public/UI/SkillTree/<FileName>.h
  .cpp → Source/DungeonForged/Private/UI/SkillTree/<FileName>.cpp
Always write full path as comment at top of each file.

─── FDFSkillTreeNode : FTableRowBase ──────────────────────────
File: Public/UI/SkillTree/DFSkillTreeData.h
Fields:
- FText NodeName, NodeDescription
- UTexture2D* NodeIcon
- ESkillTreeBranch Branch         ← Offensive, Defensive, Utility, Universal
- FName ClassName                 ← empty = Universal (all classes)
- int32 MetaXPCost
- int32 Tier                      ← 1-5 (visual row position in tree)
- TArray<FName> RequiredNodes     ← prerequisite rows (dependencies)
- TSubclassOf<UGameplayEffect> PermanentEffect ← applied at run start always
- FText EffectPreviewText         ← "+15 Strength every run"
- bool bIsUnlocked (runtime, not stored in DT)

─── UDFSkillTreeSubsystem extends UGameInstanceSubsystem ──────

Properties:
- UDataTable* SkillTreeTable      ← DT_SkillTreeNodes
- TArray<FName> UnlockedNodes     ← persisted in UDFSaveGame

Functions:
- CanUnlockNode(FName NodeRow, UDFSaveGame* Save): bool
  → Check MetaXP >= Cost
  → Check all RequiredNodes are in UnlockedNodes
  → Check class restriction matches or is Universal
- UnlockNode(FName NodeRow, APlayerController* PC):
  → CanUnlockNode check
  → Deduct MetaXP via GameState->AddMetaXP(-Cost)
  → Add to UnlockedNodes, Save to UDFSaveGame
  → Broadcast OnNodeUnlocked(NodeRow)
- ApplyUnlockedNodesToPlayer(ADFPlayerCharacter* Player):
  → Called by ADFRunGameMode::PostLogin after class init
  → For each UnlockedNode: apply PermanentEffect (Infinite) to player ASC
- GetNodeData(FName Row): FDFSkillTreeNode*
- GetUnlockedNodes(): TArray<FName>

─── WBP_SkillTree extends UDFUserWidgetBase ───────────────────
Main skill tree screen — opened from Nexus Sábio NPC.

Layout:
- UCanvasPanel* TreeCanvas        ← nodes positioned at (Tier * 180, BranchOffset)
- Tab row: [Universal] [Guerreiro] [Mago] [Assassino] [Paladino] [Necromante]
- Bottom bar: MetaXP display + "X disponível"
- UScrollBox wrapper for large trees

Node positioning algorithm:
- Tier 1 = top row (Y=0), Tier 5 = bottom (Y=720)
- Branch columns: Offensive=left, Defensive=center-left, Utility=center-right
- Universal spans full width (centered)

UDFSkillTreeNodeWidget (one per node):
- UImage* NodeBackground         ← locked=dark, unlocked=glowing, available=highlighted
- UImage* NodeIcon
- UTextBlock* NodeName (small)
- UImage* LockIcon               ← visible when locked
- UButton* NodeButton
- On Hover: show WBP_NodeTooltip (name, description, cost, requirements, effect preview)
- On Click (if available): show confirm dialog → UnlockNode()
- Visual states:
  Locked+unavailable: greyed out, 40% opacity
  Locked+available:   full color, pulsing gold border
  Unlocked:           bright glow, checkmark overlay

UDFSkillTreeConnectionWidget:
- Draws lines between prerequisite nodes and their children
- Uses UImage with line material (thin gold line)
- Line color: grey if child locked, gold if child available, bright if both unlocked

WBP_NodeTooltip:
- NodeName (colored by branch)
- Description
- EffectPreviewText in green (+15 Strength every run)
- RequiredNodes list (grey if not met, green if met)
- MetaXP Cost
- [DESBLOQUEADO] badge if already owned

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🎯 PROMPT 52 — CHALLENGE & DAILY RUN SYSTEM
## ════════════════════════════════════════

```
@ue-serialization-savegames
@ue-data-assets-tables
@ue-gameplay-tags
@ue-ui-umg-slate

Create the Challenge and Daily Run system for DungeonForged UE 5.4.
Path convention:
  .h  → Source/DungeonForged/Public/Challenge/<FileName>.h
  .cpp → Source/DungeonForged/Private/Challenge/<FileName>.cpp
Always write full path as comment at top of each file.

─── EChallengeType (enum) ─────────────────────────────────────
Values: Daily, Weekly, Permanent, Roguelike (single run modifier)

─── FDFChallengeModifier (struct) ─────────────────────────────
File: Public/Challenge/DFChallengeData.h
Fields:
- FText ModifierName, ModifierDescription
- EModifierType Type             ← Buff (for player), Debuff (for player), EnemyBuff
- TSubclassOf<UGameplayEffect> EffectToApply
- float Magnitude

─── FDFChallengeRow : FTableRowBase ───────────────────────────
Fields:
- FText ChallengeName, ChallengeDescription
- EChallengeType ChallengeType
- TArray<FDFChallengeModifier> RunModifiers ← applied to entire run
  Examples:
  "Iron Man":     EnemyHP += 50%, Player cannot heal
  "Glass Cannon": Player damage +100%, Player HP -60%
  "No Potions":   HealthPotion ability blocked (GameplayTag block)
  "Speed Run":    15 min time limit, bonus MetaXP scaling with time left
  "Berserker":    All enemies enraged from floor 1
  "Pacifist":     No combat abilities (stealth/utility only)
  "Hardcore":     Death is permanent for this run (no SaveGame checkpoint)
- int32 MetaXPReward             ← bonus MetaXP on completion
- int32 SeedOverride             ← 0 = random, non-0 = seeded run (same layout for all)
- FString ExpiryDate             ← "2025-04-25" for daily/weekly, empty for permanent
- bool bRequiresSpecificClass
- FName RequiredClass

─── UDFChallengeSubsystem extends UGameInstanceSubsystem ──────

Properties:
- UDataTable* ChallengeTable
- TArray<FName> ActiveChallenges     ← today's daily + this week's weekly
- TArray<FName> CompletedChallenges  ← from SaveGame
- FName SelectedChallenge            ← chosen for current run (optional)
- int32 CurrentRunSeed = 0

Functions:
- Initialize:
  → RefreshDailyChallenges() if new day (compare date with SaveGame::LastChallengeDate)
  → RefreshWeeklyChallenges() if new week
- RefreshDailyChallenges():
  → Use FDateTime::Today as seed for FMath::RandInit
  → Pick 3 daily challenges from DT_Challenges filtered by EChallengeType::Daily
  → Store in ActiveChallenges, update SaveGame::LastChallengeDate
- GetActiveChallenges(): TArray<FDFChallengeRow*>
- SelectChallengeForRun(FName ChallengeRow):
  → SelectedChallenge = Row
  → If SeedOverride != 0: CurrentRunSeed = Row.SeedOverride (seeded dungeon)
  → Store in UDFRunManager for ADFRunGameMode to pick up
- ApplyChallengeModifiers(ADFPlayerCharacter* Player):
  → Called by ADFRunGameMode::PostLogin after class init
  → For each modifier: apply EffectToApply to player/enemies
- OnChallengeCompleted(FName ChallengeRow):
  → Add to CompletedChallenges, SaveGame
  → Reward MetaXP + show WBP_ChallengeComplete notification
- IsChallengeCompleted(FName Row): bool

─── WBP_ChallengeBoard extends UDFUserWidgetBase ──────────────
Displayed in Nexus (pin board near portal or Cronista NPC):

Layout:
- Header: "Desafios" + "Atualiza em: 14:32:05" (countdown timer)
- Tab: [Diários] [Semanais] [Permanentes]
- TileView of WBP_ChallengeCard per challenge

WBP_ChallengeCard:
- UImage* ChallengeThumb (art per modifier type)
- UTextBlock* ChallengeName
- UTextBlock* ModifiersSummary (1-line summary of modifiers)
- UTextBlock* RewardText       ← "+250 MetaXP"
- UTextBlock* SeedText         ← "Seed: #4821" if seeded
- UImage* CompletedBadge       ← visible if done
- UButton* SelectButton → SelectChallengeForRun + open WBP_ClassSelection
- Difficulty indicator: skull icons (1-3) based on modifier severity

WBP_ChallengeComplete (notification):
- Slide in from top, gold border
- "Desafio Concluído! +{X} MetaXP"
- Challenge name + icon

Output all .h and .cpp with correct Public/Private paths.
```

---

## ══════════════════════════════════════════════
## 🎓 UX & ONBOARDING
## ══════════════════════════════════════════════

## ════════════════════════════════════════
## 📖 PROMPT 53 — TUTORIAL & ONBOARDING SYSTEM
## ════════════════════════════════════════

```
@ue-ui-umg-slate
@ue-gameplay-tags
@ue-serialization-savegames
@ue-actor-component-archit...

Create the Tutorial and Onboarding system for DungeonForged UE 5.4.
Contextual, non-intrusive — triggers once, skippable, persisted in SaveGame.
Path convention:
  .h  → Source/DungeonForged/Public/Tutorial/<FileName>.h
  .cpp → Source/DungeonForged/Private/Tutorial/<FileName>.cpp
Always write full path as comment at top of each file.

─── ETutorialStep (enum) ──────────────────────────────────────
Values (in order):
  Movement, Camera, Sprint, Dodge, BasicAttack, ComboSystem,
  Abilities, LockOn, Interact, Inventory, Equipment,
  Shop, LevelUp, AbilitySelection, DungeonCleared,
  NexusArrival, NPCInteraction, SkillTree, Portal

─── FDFTutorialEntry (struct) ─────────────────────────────────
File: Public/Tutorial/DFTutorialData.h
Fields:
- ETutorialStep Step
- FText HeaderText              ← "Movimento"
- FText BodyText                ← "Use WASD / analógico esquerdo para mover."
- UTexture2D* IconImage         ← controller or keyboard icon
- EArrowDirection ArrowDir      ← points to relevant UI element
- FVector2D TargetScreenPercent ← 0..1 normalized position of arrow tip
- float DisplayDuration         ← 0 = manual dismiss only
- bool bPauseGame               ← true for critical steps (first ability use)
- FGameplayTag TriggerTag       ← tag on player ASC that triggers this step
- FGameplayTag CompletionTag    ← tag added when player performs the action

─── UDFTutorialSubsystem extends UWorldSubsystem ──────────────

Properties:
- TArray<ETutorialStep> CompletedSteps   ← from SaveGame
- ETutorialStep CurrentStep = None
- bool bTutorialEnabled = true
- FTimerHandle AutoDismissTimer

Functions:
- Initialize: load CompletedSteps from SaveGame, subscribe to player ASC tags
- TryTriggerStep(ETutorialStep Step):
  → If CompletedSteps.Contains(Step): return (already seen)
  → If !bTutorialEnabled: return (player disabled tutorials)
  → CurrentStep = Step
  → Get FDFTutorialEntry for step
  → if bPauseGame: SetGlobalTimeDilation(0)
  → Show WBP_TutorialTooltip
  → If DisplayDuration > 0: start AutoDismissTimer
- CompleteStep(ETutorialStep Step):
  → Add to CompletedSteps, SaveGame
  → Hide WBP_TutorialTooltip
  → Restore time if was paused
  → CurrentStep = None
- SkipAllTutorials(): CompletedSteps = all steps, SaveGame

Trigger points (subscribe to these events):
- Movement: player first moves (velocity > 10)
- BasicAttack: first IA_Attack input
- ComboSystem: player completes first 2-hit combo
- LockOn: first IA_LockOn input
- Shop: WBP_Shop opened for first time
- AbilitySelection: WBP_AbilitySelection opened for first time

─── WBP_TutorialTooltip extends UUserWidget ───────────────────
Non-intrusive overlay (NOT fullscreen — appears at screen edge):

Layout:
- UImage* BackgroundPanel       ← semi-transparent dark card, 380x200px
- UImage* IconImage             ← input icon (key or controller button)
- UTextBlock* HeaderText        ← bold, 18pt
- UTextBlock* BodyText          ← regular, 14pt, max 3 lines
- UImage* ArrowIndicator        ← animated bouncing arrow pointing to UI element
- UButton* DismissButton        ← "OK" or press any key to dismiss
- UButton* SkipAllButton        ← "Pular Tutoriais" (shows confirm dialog)

Position logic:
- Place card at screen edge opposite of TargetScreenPercent
  (if arrow points right, card appears on left side)
- Arrow extends from card edge toward target
- Animate card: slide in from edge, bob slightly

WBP_TutorialArrow:
- UImage* ArrowImage (pointing in EArrowDirection)
- UWidgetAnimation* BobAnimation (loops while visible)
- UpdateTarget(FVector2D ScreenPosition): animate arrow tip position

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🎮 PROMPT 56 — GAMEPAD & RADIAL ABILITY MENU
## ════════════════════════════════════════

```
@ue-input-system
@ue-ui-umg-slate
@ue-gameplay-abilities
@ue-gameplay-tags

Create Gamepad enhancements and Radial Ability Menu for DungeonForged UE 5.4.
Path convention:
  .h  → Source/DungeonForged/Public/Input/Gamepad/<FileName>.h
  .cpp → Source/DungeonForged/Private/Input/Gamepad/<FileName>.cpp
Always write full path as comment at top of each file.

─── UDFHapticFeedbackLibrary ──────────────────────────────────
File: Public/Input/Gamepad/DFHapticFeedbackLibrary.h
UBlueprintFunctionLibrary with static helpers:

- TriggerHaptic_LightHit(APlayerController* PC):
  → IInputInterface::SetHapticFeedbackValues(0, 0, FHapticFeedbackValues(0.3, 0.3))
  → Duration via FTimerHandle 0.08s then clear
- TriggerHaptic_HeavyHit(APlayerController* PC):   0.8 intensity, 0.15s
- TriggerHaptic_CriticalHit(APlayerController* PC): 1.0, 0.2s, both motors
- TriggerHaptic_BossSlam(APlayerController* PC):    1.0, 0.4s, rumble pattern
- TriggerHaptic_Heal(APlayerController* PC):        0.2 left only, 0.1s (soft pulse)
- TriggerHaptic_AbilityActivate(APlayerController* PC): 0.4, 0.05s (click feel)
- TriggerHaptic_LevelUp(APlayerController* PC):     0.6 rising, 0.3s
- TriggerHaptic_Death(APlayerController* PC):       long fade out 1.0→0.0 over 1s

Integration points (add calls to existing systems):
- UDFHitReactionComponent::OnHitReceived → LightHit or HeavyHit
- UDFDamageCalculation: if Critical → CriticalHit haptic
- ADFBossBase::TriggerPhaseTransition → BossSlam haptic
- UDFAttributeSet: on Health increase → Heal haptic
- UDFLevelingComponent::LevelUp → LevelUp haptic
- ADFPlayerCharacter death → Death haptic

─── UDFRadialMenuComponent extends UActorComponent ────────────
On PlayerCharacter — manages the radial ability selector.

Properties:
- bool bIsRadialOpen = false
- int32 SelectedSlot = -1        ← -1 = none
- TArray<FName> AbilitySlots     ← 4 slots: Ability.Slot.1 through .4
- FVector2D LastStickInput       ← right stick input while radial open

Functions:
- OpenRadialMenu():
  → bIsRadialOpen = true
  → SetGlobalTimeDilation(0.2) ← bullet time while choosing
  → Show WBP_RadialMenu
  → Subscribe to right stick axis input
- CloseRadialMenu(bool bConfirm):
  → bIsRadialOpen = false
  → Restore TimeDilation
  → Hide WBP_RadialMenu
  → If bConfirm AND SelectedSlot >= 0:
    TryActivateAbilityByTag(Ability.Slot.[SelectedSlot+1])
- UpdateStickInput(FVector2D StickInput):
  → If magnitude < 0.3: SelectedSlot = -1 (center = no selection)
  → Else: compute angle → map to 4 quadrants → SelectedSlot = quadrant index
  → Update WBP_RadialMenu highlight

Input binding (add to ADFRunPlayerController):
- IA_RadialMenu_Hold (L1/LB): Started → OpenRadialMenu, Completed → CloseRadialMenu(true)
- IA_RadialMenu_Look (right stick): triggers UpdateStickInput while radial open
- Override: while radial open, suppress all other ability inputs

─── WBP_RadialMenu extends UUserWidget ────────────────────────
Circular ability selector — appears centered on screen:

Layout:
- UImage* DarkOverlay (full screen, 60% opacity, behind radial)
- UCanvasPanel* RadialCanvas (center of screen, 400x400px)
- 4x WBP_RadialSlot at 90° intervals (top/right/bottom/left)
- UImage* CenterDot (stick deadzone indicator)
- UImage* SelectionHighlight (arc segment highlights selected quadrant)

WBP_RadialSlot (one per ability slot):
- UImage* AbilityIcon
- UTextBlock* AbilityName (small, below icon)
- UTextBlock* CooldownText (if on cooldown: red timer)
- UImage* CooldownOverlay (darkens icon if on cooldown)
- UImage* SlotBorder (highlights when selected)
- Position: top=Slot1, right=Slot2, bottom=Slot3, left=Slot4

Animations:
- Open: scale from 0→1 with elastic ease (0.15s)
- Selection: highlight arc sweeps to selected slot (0.05s)
- Close: scale 1→0 (0.1s)
- Bullet time visual: slight desaturation + vignette while radial open

Output all .h and .cpp with correct Public/Private paths.
```

---

## ══════════════════════════════════════════════
## 🎬 APRESENTAÇÃO & CINEMÁTICA
## ══════════════════════════════════════════════

## ════════════════════════════════════════
## 🎬 PROMPT 54 — CINEMATIC & LEVEL SEQUENCE SYSTEM
## ════════════════════════════════════════

```
@ue-gameplay-framework
@ue-gameplay-tags
@ue-ui-umg-slate
@ue-animation-system
@ue-niagara-effects

Create the Cinematic and Level Sequence system for DungeonForged UE 5.4.
Path convention:
  .h  → Source/DungeonForged/Public/Cinematics/<FileName>.h
  .cpp → Source/DungeonForged/Private/Cinematics/<FileName>.cpp
Always write full path as comment at top of each file.

─── UDFCinematicSubsystem extends UWorldSubsystem ─────────────

Properties:
- bool bCinematicActive = false
- ULevelSequencePlayer* ActivePlayer
- ALevelSequenceActor* ActiveSequenceActor
- FTimerHandle SkipCooldown     ← prevent accidental skip (0.5s lock)

Functions:
- PlayCinematic(ULevelSequence* Sequence, bool bSkippable=true,
  TFunction<void()> OnComplete = nullptr):
  → bCinematicActive = true
  → Disable player input: apply GameplayTag UI.CinematicActive to player
  → Create ALevelSequenceActor, bind sequence
  → ULevelSequencePlayer::Play()
  → If bSkippable: start SkipCooldown timer (0.5s), then listen for any input
  → On complete/skip: call OnComplete lambda, re-enable input, remove tag

- SkipCinematic():
  → ActivePlayer->Stop()
  → Jump to end (apply final state manually if needed)
  → EndCinematic()

- EndCinematic():
  → bCinematicActive = false
  → Remove UI.CinematicActive tag
  → Restore player input
  → Destroy ActiveSequenceActor

─── Cinematic descriptions (describe Level Sequence setups) ───

CineSeq_DungeonEntrance (plays when run starts — floor 1):
- Duration: 8s, skippable after 1s
- Shot 1 (0-2s): close-up of portal entrance, player walks through
- Shot 2 (2-4s): camera pulls back revealing dungeon scale, torches
- Shot 3 (4-6s): camera sweeps to show first room, enemies patrolling
- Shot 4 (6-8s): camera returns behind player (normal gameplay cam)
- Audio: epic dungeon entrance sting
- Triggers: ADFRunGameMode::PostLogin (first floor only)

CineSeq_BossIntro (plays when entering boss room):
- Duration: 6s, skippable after 1.5s
- Shot 1 (0-1.5s): door opens, light pours through
- Shot 2 (1.5-3.5s): boss revealed — camera orbits boss slowly
- Shot 3 (3.5-5s): boss nameplate fades in (WBP_BossTitle overlay)
- Shot 4 (5-6s): cut to player face, then behind player facing boss
- Audio: boss theme intro sting
- Triggers: ADFBossTriggerVolume::OnPlayerEnter

CineSeq_VictoryFinal (plays after final boss death):
- Duration: 10s, skippable after 3s
- Shot 1 (0-3s): boss falls in slow motion (SetGlobalTimeDilation(0.3))
- Shot 2 (3-6s): camera rises above dungeon, golden light floods in
- Shot 3 (6-8s): player stands victorious, cape/hair physics
- Shot 4 (8-10s): fade to white → WBP_VictoryScreen
- Audio: victory fanfare full length

CineSeq_DeathCam (plays on player death — no skip):
- Duration: 5s
- Shot 1 (0-2s): camera orbits falling player (ragdoll), slow motion
- Shot 2 (2-4s): enemy walks toward camera, menacing
- Shot 3 (4-5s): fade to black → WBP_DefeatScreen
- SetGlobalTimeDilation(0.2) during entire sequence

─── WBP_BossTitle (overlay during boss intro cinematic) ───────
- UTextBlock* BossName     ← dramatic font, center screen, gold
- UTextBlock* BossSubtitle ← "Guardião do Andar Final"
- UWidgetAnimation* FadeInOut (2s in, hold 2s, 1s out)
- Shown during CineSeq_BossIntro shot 3

─── ADFCinematicTriggerVolume ─────────────────────────────────
Box volume — plays cinematic when player enters:
- ULevelSequence* LinkedSequence
- bool bOneShot = true (never replays)
- bool bSkippable = true
- OnBeginOverlap → UDFCinematicSubsystem::PlayCinematic(LinkedSequence)

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 📸 PROMPT 65 — PHOTO MODE
## ════════════════════════════════════════

```
@ue-gameplay-framework
@ue-ui-umg-slate
@ue-actor-component-archit...

Create Photo Mode for DungeonForged UE 5.4.
Activated from pause menu — pauses game, frees camera, screenshot tools.
Path convention:
  .h  → Source/DungeonForged/Public/PhotoMode/<FileName>.h
  .cpp → Source/DungeonForged/Private/PhotoMode/<FileName>.cpp
Always write full path as comment at top of each file.

─── UDFPhotoModeSubsystem extends UWorldSubsystem ─────────────

Properties:
- bool bIsActive = false
- APawn* FreeCameraActor          ← spawned spectator pawn
- APlayerController* OwnerPC
- FVector SavedCameraLocation
- FRotator SavedCameraRotation
- FPhotoModeSettings CurrentSettings

FPhotoModeSettings struct:
- float FOV = 90.f               (range 20-150)
- float ApertureSize = 4.0f      (depth of field f-stop)
- float FocusDistance = 500.f
- EPhotoFilter Filter            (None, Vintage, Noir, Vivid, Warm, Cold)
- float FilterIntensity = 1.0f
- float Brightness = 0.f         (-1 to 1)
- float Saturation = 1.0f        (0-2)
- bool bHideUI = true
- bool bHidePlayer = false

Functions:
- EnterPhotoMode(APlayerController* PC):
  → bIsActive = true
  → SetGlobalTimeDilation(0) — full pause
  → Save camera transform
  → Spawn ASpectatorPawn at current camera location
  → Possess spectator pawn (free fly camera)
  → Show WBP_PhotoModeUI
  → If bHideUI: hide all HUD widgets
- ExitPhotoMode():
  → Unpossess spectator → re-possess original character
  → Restore TimeDilation
  → Hide WBP_PhotoModeUI, restore HUD
  → Destroy spectator pawn
  → bIsActive = false
- ApplySettings(FPhotoModeSettings Settings):
  → Update PostProcessVolume parameters:
    DOF: enable, FocalDistance=Settings.FocusDistance, Aperture=Settings.ApertureSize
    Color grading: Saturation, Brightness via FColorGradingSettings
    Filter: apply LUT texture by EPhotoFilter enum
  → Update player camera FOV
- TakeScreenshot():
  → FHighResScreenshotConfig& Config = GetHighResScreenshotConfig()
  → Config.SetFilename("DungeonForged_Photo_" + FDateTime::Now())
  → Config.ResolutionMultiplier = 2.0f (2x resolution)
  → FScreenshotRequest::RequestScreenshot(true)
  → Show WBP_ScreenshotFlash (brief white flash + shutter sound)

─── WBP_PhotoModeUI extends UUserWidget ───────────────────────
Minimal side panel (right side, 280px wide):

Sections:
- Camera: FOV slider (20-150), Focus Distance slider
- Depth of Field: Aperture (f/1.4 to f/22), toggle enable
- Color: Brightness (-1 to 1), Saturation (0 to 2)
- Filters: HorizontalBox of filter thumbnails (click to select)
- Visibility: toggles HideUI, HidePlayer
- [📷 Tirar Foto] button → TakeScreenshot()
- [✕ Sair] button → ExitPhotoMode()

All sliders: USlider + UTextBlock showing current value
Filter thumbnails: UButton with 60x40 preview UImage

WBP_ScreenshotFlash:
- Full-screen white UImage, fade from opaque to transparent in 0.3s
- Plays shutter click SFX via UDFAudioComponent

Output all .h and .cpp with correct Public/Private paths.
```

---

## ══════════════════════════════════════════════
## 📖 NARRATIVA & MUNDO
## ══════════════════════════════════════════════

## ════════════════════════════════════════
## 💬 PROMPT 55 — DIALOGUE SYSTEM
## ════════════════════════════════════════

```
@ue-ui-umg-slate
@ue-data-assets-tables
@ue-serialization-savegames
@ue-actor-component-archit...

Create the Dialogue System for Nexus NPCs in DungeonForged UE 5.4.
NPCs have personality, remember run history, and react to MetaLevel.
Path convention:
  .h  → Source/DungeonForged/Public/Dialogue/<FileName>.h
  .cpp → Source/DungeonForged/Private/Dialogue/<FileName>.cpp
Always write full path as comment at top of each file.

─── FDFDialogueNode (struct) ──────────────────────────────────
File: Public/Dialogue/DFDialogueData.h
Fields:
- FName NodeID
- FText SpeakerText              ← what NPC says
- TArray<FDFDialogueChoice> Choices ← player responses
- FName NextNodeID               ← auto-advance if no choices (linear)
- FGameplayTagContainer RequiredTags ← conditions to show this node
  (e.g. Tag "Meta.Won.AtLeastOnce" shows different ferreiro dialogue)
- bool bEndsDialogue = false
- USoundBase* VoiceLine          ← optional VO clip

FDFDialogueChoice struct:
- FText ChoiceText
- FName NextNodeID
- FGameplayTagContainer RequiredTags ← only show choice if player has tags

─── FDFDialogueTreeRow : FTableRowBase ────────────────────────
Fields:
- FName NPCName
- TArray<FDFDialogueNode> Nodes
- FName EntryNodeID              ← first node to show

─── UDFDialogueComponent extends UActorComponent ──────────────
On each ADFNexusNPCBase:

Properties:
- UDataTable* DialogueTable      ← DT_Dialogues
- FName DialogueTreeRow          ← which row to use
- FName CurrentNodeID
- bool bIsDialogueActive = false

Functions:
- StartDialogue(APlayerController* PC):
  → Load FDFDialogueTreeRow from table
  → CurrentNodeID = EntryNodeID
  → bIsDialogueActive = true
  → Open WBP_DialogueBox, show first node
  → Lock player input (GameplayTag UI.DialogueActive)
- AdvanceToNode(FName NodeID, APlayerController* PC):
  → Find node in tree by ID
  → Filter choices by RequiredTags vs player ASC
  → Update WBP_DialogueBox
  → If bEndsDialogue: EndDialogue()
- OnChoiceSelected(int32 ChoiceIndex):
  → AdvanceToNode(Choices[ChoiceIndex].NextNodeID)
- EndDialogue():
  → Close WBP_DialogueBox
  → Remove UI.DialogueActive tag
  → Restore input

─── WBP_DialogueBox extends UUserWidget ───────────────────────
Classic RPG dialogue box (bottom of screen):

Layout:
- UImage* BackgroundPanel (dark, 80% opacity, full width, 180px tall, bottom)
- UImage* NPCPortrait (80x80, left side, circular mask)
- UTextBlock* NPCNameText (bold, above portrait)
- UTextBlock* DialogueText (typewriter animation, 3 lines max)
- UVerticalBox* ChoicesBox (choices appear after text completes)
  Each choice: WBP_DialogueChoiceButton (UButton + UTextBlock)
- UTextBlock* AdvanceHint (right corner: "▶ Continuar" or "Press any key")
- UWidgetAnimation* TypewriterAnim (reveals text char by char at 40 chars/sec)

Functions:
- ShowNode(FDFDialogueNode Node):
  → Set NPCNameText, NPCPortrait
  → Clear ChoicesBox
  → StartTypewriter(Node.SpeakerText)
  → If voice line: play audio
- StartTypewriter(FText FullText):
  → Reveal text character by character via FTimerHandle (25ms per char)
  → On complete: show choices or advance hint
- OnAdvanceClicked:
  → If typewriter still running: jump to full text (skip animation)
  → Else: AdvanceToNode (auto or choice)

NPC-specific dialogues (describe 3 examples in DT_Dialogues):

Ferreiro — first visit:
  "Outro aventureiro... Espero que você seja mais resistente que os últimos.
   Dizem que o dungeon muda a cada visita. Eu existo aqui há séculos — ele sempre muda, você muda menos."
  Choices: [O que você pode fazer por mim?] [Quem é você?] [Até logo.]

Ferreiro — after first victory (RequiredTag: Meta.Won.AtLeastOnce):
  "Você voltou VIVO. Genuinamente impressionante. A maioria não volta do décimo andar.
   Minha bigorna te saúda."

Sábio — after 10 runs (RequiredTag: Meta.Runs.10Plus):
  "Você já trilhou este caminho dez vezes. Percebo mudanças em você. Não só no poder —
   na determinação. O dungeon testa mais que força."

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 📚 PROMPT 58 — ENVIRONMENTAL STORYTELLING & LORE
## ════════════════════════════════════════

```
@ue-actor-component-archit...
@ue-ui-umg-slate
@ue-data-assets-tables
@ue-serialization-savegames

Create Environmental Storytelling and Lore system for DungeonForged UE 5.4.
Path convention:
  .h  → Source/DungeonForged/Public/Lore/<FileName>.h
  .cpp → Source/DungeonForged/Private/Lore/<FileName>.cpp
Always write full path as comment at top of each file.

─── ELoreCategory (enum) ──────────────────────────────────────
Values: DungeonHistory, Monsters, Nexus, Classes, Artifacts, Unknown

─── FDFLoreEntry : FTableRowBase ──────────────────────────────
File: Public/Lore/DFLoreData.h
Fields:
- FText LoreTitle
- FText LoreBody                 ← 2-5 paragraphs of lore text
- UTexture2D* LoreIllustration   ← atmospheric concept art
- ELoreCategory Category
- bool bIsDiscovered = false     ← runtime, persisted in SaveGame
- FText HintText                 ← shown before discovery "????? — Objeto Antigo"

─── ADFLoreObject extends ADFInteractableBase ─────────────────
Interactable lore actor placed in dungeon rooms:

Properties:
- UStaticMeshComponent* PropMesh  ← book, inscription, statue, skull pile
- FName LoreRowName               ← from DT_LoreEntries
- bool bGlowWhenNear = true       ← custom depth outline when player close
- UPointLightComponent* GlowLight ← soft ambient glow

Functions:
- Interact_Implementation:
  → UDFLoreSubsystem::DiscoverLore(LoreRowName)
  → Open WBP_LoreViewer with entry data
  → bSingleUse = false (can re-read, but discovery happens once)
- OnPlayerNear (DetectionRange overlap):
  → if bGlowWhenNear: set mesh CustomDepthStencilValue (yellow outline)
  → Show interaction prompt "📖 Examinar"

─── UDFLoreSubsystem extends UWorldSubsystem ──────────────────

Properties:
- UDataTable* LoreTable
- TArray<FName> DiscoveredLore   ← from SaveGame

Functions:
- DiscoverLore(FName RowName):
  → If already discovered: return (no duplicate)
  → Add to DiscoveredLore, SaveGame
  → Add GameplayTag "Lore.Discovered.{RowName}" to player
  → Broadcast OnLoreDiscovered(RowName)
  → Show WBP_LoreDiscoveredNotification ("Lore Descoberto: {Title}")
- GetDiscoveredEntries(): TArray<FDFLoreEntry*>
- GetEntriesByCategory(ELoreCategory): filtered array
- GetDiscoveryProgress(): float (discovered/total)

─── WBP_LoreViewer extends UUserWidget ────────────────────────
Opens when player interacts with lore object:

Layout:
- UImage* IllustrationImage      ← full left panel, 50% width
- UTextBlock* TitleText          ← stylized header font
- ELoreCategory badge            ← small colored tag
- UScrollBox* LoreTextScroll:
  UTextBlock* LoreBody           ← readable font, line height 1.5
- UButton* CloseButton
- UButton* AddToCodex            ← "Adicionar ao Códex" (already auto-added)
- Backdrop: parchment texture + ink border
- Animate: scroll unfurls from top

─── WBP_LoreCodex extends UDFUserWidgetBase ───────────────────
Full lore library — accessed via Cronista NPC in Nexus:

Layout:
- Left panel: category list (all ELoreCategory values)
  Each: count "Dungeon: 4/12 descobertos"
- Right panel: scrollable grid of discovered entries
  Undiscovered shown as locked silhouette "???"
- UProgressBar* OverallProgress ← total lore completion %
- On entry click: open WBP_LoreViewer

Output all .h and .cpp with correct Public/Private paths.
```

---

## ══════════════════════════════════════════════
## 🎨 VISUAL & POLISH
## ══════════════════════════════════════════════

## ════════════════════════════════════════
## 🌈 PROMPT 59 — STATUS VISUAL ON CHARACTER MESH
## ════════════════════════════════════════

```
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-niagara-effects
@ue-actor-component-archit...
@ue-animation-system

Create the Status Visual system for character meshes in DungeonForged UE 5.4.
Visual feedback for GAS effects directly on character models.
Path convention:
  .h  → Source/DungeonForged/Public/FX/StatusVisuals/<FileName>.h
  .cpp → Source/DungeonForged/Private/FX/StatusVisuals/<FileName>.cpp
Always write full path as comment at top of each file.

─── UDFStatusVisualsComponent extends UActorComponent ─────────
On both PlayerCharacter and EnemyBase:

Properties:
- UMaterialInstanceDynamic* StatusMID   ← dynamic instance of character material
- TMap<FGameplayTag, UNiagaraComponent*> ActiveStatusVFX
- TMap<FGameplayTag, FActiveGameplayEffectHandle> WatchedEffects

Material Parameters (set via StatusMID->SetScalarParameterValue):
- BurnIntensity      (0-1) → orange emissive overlay + heat distortion
- FrozenAmount       (0-1) → blue-white overlay + crystalline surface
- PoisonAmount       (0-1) → green pulsing emissive + sickly tint
- BleedAmount        (0-1) → red dripping texture overlay
- ElectroAmount      (0-1) → blue-white crackle emissive + arc pattern
- StunAmount         (0-1) → grey desaturation + dazed effect
- HolyAmount         (0-1) → gold radiance emissive (Paladin Consecration)
- CurseAmount        (0-1) → dark purple wisps on mesh
- BerserkAmount      (0-1) → red emissive + vein pattern
- InvisibilityAmount (0-1) → mesh opacity reduction (0.3 when stealthed)

Functions:
- Initialize: get SkeletalMeshComponent, create MID from base material
- SubscribeToASC(UAbilitySystemComponent* ASC):
  → Listen OnActiveGameplayEffectAddedDelegate
  → Listen OnAnyGameplayEffectRemovedDelegate
- OnEffectAdded(FActiveGameplayEffectHandle Handle):
  → Read GE tags → determine which parameter to drive
  → LerpParameter(ParamName, 0→1, BlendInTime=0.3s)
  → SpawnStatusNiagara(tag)
- OnEffectRemoved(FActiveGameplayEffectHandle Handle):
  → LerpParameter(ParamName, current→0, BlendOutTime=0.5s)
  → DeactivateStatusNiagara(tag)
- LerpParameter(FName Param, float From, float To, float Duration):
  → FTimerHandle lerp loop updating MID each tick

─── Status Niagara VFX per tag ────────────────────────────────
SpawnStatusNiagara creates these UNiagaraComponent (attached to mesh):

Effect.DoT.Fire → NiagaraSystem NS_StatusBurn:
  - Flame particles rising from mesh surface
  - Orange embers floating upward
  - Attached to mesh root, follows character

Effect.DoT.Poison → NS_StatusPoison:
  - Green bubble particles rising slowly
  - Sickly green mist at feet

Effect.DoT.Bleed → NS_StatusBleed:
  - Red droplet particles falling from wounds
  - Impact decals via DecalComponent at hit socket

Effect.DoT.Frost → NS_StatusFrost:
  - Ice crystal particles accumulating on mesh
  - Breath mist from mouth socket

State.Stunned → NS_StatusStun:
  - Classic cartoon stars orbiting head (3 star meshes, URotatingMovementComponent)
  - Position: above head socket + 50 Z

State.Electrocuted → NS_StatusElectro:
  - Blue-white arc particles between random body sockets
  - Occasional full-body crackle burst

Effect.Buff.Speed → NS_StatusSpeed:
  - Motion blur trail behind character (ribbon Niagara)
  - Wind particles at feet

State.Berserk → NS_StatusBerserk:
  - Red energy aura surrounding character
  - Vein-like crack particles on skin

State.Invisible → no Niagara (mesh opacity reduced via MID InvisibilityAmount)
  - Subtle shimmer distortion effect via refraction material parameter

─── Stun Stars implementation ─────────────────────────────────
ADFStunStarsActor (spawned on character on Stun):
- 3x UStaticMeshComponent (star shape) rotating around attachment point
- URotatingMovementComponent: RotationRate Z = 180°/s
- Attached to head socket with +80 Z offset
- Lifespan bound to GE_Debuff_Stun duration via delegate

Output all .h and .cpp with correct Public/Private paths.
```

---

## ══════════════════════════════════════════════
## 🖥️ UI & QoL
## ══════════════════════════════════════════════

## ════════════════════════════════════════
## 🗂️ PROMPT 60 — INVENTORY SORTING & FILTERING
## ════════════════════════════════════════

```
@ue-ui-umg-slate
@ue-data-assets-tables
@ue-actor-component-archit...

Create Inventory Sorting, Filtering and advanced UX for DungeonForged UE 5.4.
Extends the UDFInventoryComponent (Prompt 10) and WBP_CharacterScreen (Prompt 37).
Path convention:
  .h  → Source/DungeonForged/Public/UI/Inventory/<FileName>.h
  .cpp → Source/DungeonForged/Private/UI/Inventory/<FileName>.cpp
Always write full path as comment at top of each file.

─── ESortMode (enum) ──────────────────────────────────────────
Values: Rarity_Desc, Rarity_Asc, Type, Name_AZ, Value_Desc, Recent

─── UDFInventorySortFilterComponent extends UActorComponent ───
Extends UDFInventoryComponent with sorting/filtering:

Properties:
- ESortMode CurrentSort = Rarity_Desc
- TSet<EItemType> ActiveTypeFilters   ← empty = show all
- FString SearchText

Functions:
- GetFilteredSortedItems(): TArray<FDFInventorySlot>
  → Filter by ActiveTypeFilters (if not empty)
  → Filter by SearchText (FString::Contains on item name)
  → Sort by CurrentSort:
    Rarity_Desc: sort by EItemRarity enum value descending
    Type: group by EItemType then by rarity
    Name_AZ: alphabetical on FText
    Value_Desc: sort by BasePrice from DT_Items descending
    Recent: reverse insertion order (most recently acquired first)
- SetSort(ESortMode Mode): CurrentSort=Mode, broadcast OnInventoryDisplayChanged
- ToggleTypeFilter(EItemType Type): add/remove from ActiveTypeFilters, broadcast
- SetSearch(FString Text): SearchText=Text, broadcast

─── WBP_Inventory extends UDFUserWidgetBase ───────────────────
Full inventory panel with sorting/filtering controls:

Layout:
- Top bar:
  UEditableTextBox* SearchBox (🔍 "Buscar item...")
  Sort dropdown: UComboBoxString with ESortMode options
- Filter row: UHorizontalBox with EItemType toggle buttons
  Each: UButton with type icon (sword icon, shield icon, etc.) + UTextBlock count
  Active filter: highlighted border, inactive: grey
- UUniformGridPanel ItemGrid: 5x4 = 20 slots of WBP_InventorySlot
  Paginated if > 20 items (prev/next buttons)
- Bottom: "X / 20 itens" counter

WBP_InventorySlot:
- UImage* ItemIcon (rarity-colored border)
- UTextBlock* QuantityBadge (top-right, small: "x3")
- UImage* EquippedBadge (bottom-right: checkmark if equipped)
- On Click: select slot (highlight) → show action panel
- On Double-Click: equip/use immediately
- On Right-Click: context menu (Equip / Usar / Descartar / Comparar)
- On Drag: start drag-drop (DragDropOperation with slot data)
- On Drop to equipment slot: EquipItem
- Tooltip on hover: WBP_ItemTooltip (Prompt 33) with compare data

Stack splitting:
- Shift+Click on stackable item (consumables): open WBP_StackSplitter
  WBP_StackSplitter: USlider (1 to max-1), UTextBlock amount, Confirm/Cancel
  On confirm: split stack → creates new inventory slot with split amount

Drag and Drop:
- UDFItemDragDropOperation extends UDragDropOperation:
  → Payload: FDFInventorySlot data + source slot index
  → DragVisual: WBP_ItemDragVisual (item icon + name, follows cursor)
- Equipment slots in WBP_CharacterScreen accept drops from inventory:
  → OnDrop: call UDFEquipmentComponent::EquipItem(Payload.RowName, TargetSlot)

Output all .h and .cpp with correct Public/Private paths.
```

---

## ══════════════════════════════════════════════
## 🏆 META & SOCIAL
## ══════════════════════════════════════════════

## ════════════════════════════════════════
## 🏅 PROMPT 61 — ACHIEVEMENT SYSTEM
## ════════════════════════════════════════

```
@ue-serialization-savegames
@ue-data-assets-tables
@ue-ui-umg-slate
@ue-gameplay-tags

Create the Achievement system for DungeonForged UE 5.4.
Path convention:
  .h  → Source/DungeonForged/Public/Achievements/<FileName>.h
  .cpp → Source/DungeonForged/Private/Achievements/<FileName>.cpp
Always write full path as comment at top of each file.

─── EAchievementCategory (enum) ───────────────────────────────
Values: Combat, Exploration, Progression, Mastery, Secret, Challenge

─── FDFAchievementRow : FTableRowBase ─────────────────────────
File: Public/Achievements/DFAchievementData.h
Fields:
- FText AchievementName, Description
- UTexture2D* Icon
- EAchievementCategory Category
- bool bIsSecret                ← hidden until earned
- EAchievementTrackerType TrackerType  ← Counter, Boolean, Milestone
- float TargetValue             ← for Counter: 1000 kills; Milestone: floor 10
- FGameplayTag TrackerTag       ← stat tag to monitor (Stat.TotalKills etc.)
- FText UnlockRewardText        ← "Desbloqueia: Skin Guerreiro Sombrio"
- FName CosmeticUnlockRow       ← optional cosmetic from DT_Cosmetics

─── UDFAchievementSubsystem extends UGameInstanceSubsystem ────

Properties:
- UDataTable* AchievementTable
- TMap<FName, float> AchievementProgress   ← row → current value
- TArray<FName> UnlockedAchievements
- TQueue<FName> NotificationQueue

Functions:
- Initialize: load from SaveGame, subscribe to stat tags
- TrackProgress(FGameplayTag StatTag, float Delta):
  → Find all achievements with TrackerTag == StatTag
  → AchievementProgress[Row] += Delta
  → If progress >= TargetValue AND not yet unlocked: UnlockAchievement(Row)
- UnlockAchievement(FName Row):
  → Add to UnlockedAchievements, SaveGame
  → NotificationQueue.Enqueue(Row)
  → ProcessNotificationQueue()
  → If CosmeticUnlockRow: notify UDFCosmeticsComponent
- ProcessNotificationQueue():
  → If WBP_AchievementPopup not showing: dequeue + show
  → Auto-chain next after 4s
- GetProgress(FName Row): float 0..1

Stat tracking hooks (add TrackProgress calls to):
- UDFAttributeSet::PostGE: Stat.TotalDamageDone, Stat.TotalHealing
- ADFEnemyBase::OnDeath: Stat.TotalKills, Stat.BossKills
- UDFLevelingComponent::LevelUp: Stat.LevelsGained
- ADFDungeonManager::OnFloorCleared: Stat.FloorsCleared
- ADFRunGameMode::TriggerVictory: Stat.RunsWon
- UDFInventoryComponent::AddItem by rarity: Stat.LegendaryItems

─── Pre-defined achievements (describe for DT_Achievements) ───
"Primeiro Sangue":   1 kill → unlock: nothing (tutorial)
"Matador":           1000 kills total
"Sem Arranhões":     complete a floor without taking damage
"Velocista":         win a run in under 20 minutes
"Colecionador":      collect 1 Legendary item
"Imortal":           activate SecondWind passive (survive death)
"Lendário":          reach MetaLevel 10
"Caçador de Chefes": kill 50 bosses total
"Explorador":        discover all 12 lore entries in one run
"Pacto Selado":      accept the Demon Pact random event and win
"Mestre das Sombras":complete a run using only Rogue abilities (no universals)
Secret: "???":       [hidden] — win with all 5 classes (true mastery)

─── WBP_AchievementPopup ──────────────────────────────────────
Slide in from top-right corner, 4s display:
- UImage* AchievementIcon
- UTextBlock* "Conquista Desbloqueada!"
- UTextBlock* AchievementName (bold)
- UTextBlock* RewardText (small, if any)
- Gold border animation, achievement unlock SFX

─── WBP_AchievementList (in Nexus Cronista) ───────────────────
- Category tabs (EAchievementCategory)
- WBP_AchievementCard per achievement:
  Unlocked: full color icon + name + description + date earned
  Locked (non-secret): greyed + description visible + progress bar
  Secret locked: "???" icon + "???" name + "Desbloqueio secreto"
- Overall progress: "47 / 63 Conquistas"

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 💅 PROMPT 62 — CHARACTER COSMETICS SYSTEM
## ════════════════════════════════════════

```
@ue-actor-component-archit...
@ue-data-assets-tables
@ue-serialization-savegames
@ue-ui-umg-slate
@ue-niagara-effects

Create the Character Cosmetics system for DungeonForged UE 5.4.
Purely visual — no gameplay impact. Unlocked via achievements and MetaLevel.
Path convention:
  .h  → Source/DungeonForged/Public/Cosmetics/<FileName>.h
  .cpp → Source/DungeonForged/Private/Cosmetics/<FileName>.cpp
Always write full path as comment at top of each file.

─── ECosmeticSlot (enum) ──────────────────────────────────────
Values: AbilityAuraColor, MovementTrail, DeathEffect, WeaponSkin,
        CharacterTint, TitleCard, PortraitFrame

─── FDFCosmeticRow : FTableRowBase ────────────────────────────
File: Public/Cosmetics/DFCosmeticsData.h
Fields:
- FText CosmeticName, Description
- ECosmeticSlot Slot
- UTexture2D* PreviewThumbnail
- EItemRarity Rarity            ← visual quality of cosmetic itself
- FLinearColor AuraColor        ← for AbilityAuraColor slot
- UNiagaraSystem* TrailVFX      ← for MovementTrail slot
- UNiagaraSystem* DeathVFX      ← for DeathEffect slot
- UMaterialInterface* WeaponMaterial ← for WeaponSkin slot
- FLinearColor TintColor        ← for CharacterTint slot
- UTexture2D* TitleCardBG       ← for TitleCard slot (shown in UI/loading)
- FName UnlockAchievementRow    ← which achievement unlocks this
- int32 UnlockMetaLevel         ← or via MetaLevel

─── UDFCosmeticsComponent extends UActorComponent ─────────────
On PlayerCharacter:

Properties:
- TMap<ECosmeticSlot, FName> EquippedCosmetics   ← slot → DT_Cosmetics RowName

Functions:
- EquipCosmetic(ECosmeticSlot Slot, FName RowName):
  → Validate unlocked via SaveGame
  → UnequipSlot(Slot) first
  → Apply cosmetic effect by slot type:
    AbilityAuraColor: update all ability Niagara systems' ColorParameter
    MovementTrail:   attach NS_Trail to player feet socket
    DeathEffect:     store for on-death spawn
    WeaponSkin:      apply material override to Mesh_Weapon
    CharacterTint:   set MID TintColor parameter on all mesh components
  → Store in EquippedCosmetics, SaveGame
- UnequipSlot(Slot): restore defaults for that slot
- ApplyEquippedCosmetics(): called on player spawn to restore all

─── WBP_CosmeticsScreen extends UDFUserWidgetBase ─────────────
Cosmetics showcase — opened from Nexus:

Layout:
- Left: slot category tabs (AbilityAura, Trail, Death, etc.)
- Center: SceneCapture preview of player with selected cosmetic applied live
- Right: grid of cosmetics for selected slot
  WBP_CosmeticCard per item:
  - UImage* Thumbnail
  - UTextBlock* Name + Rarity colored border
  - UImage* LockIcon if not unlocked
  - UTextBlock* UnlockHint ("Conquista: Matador" or "Nexus Nv. 5")
  - On hover: apply to SceneCapture preview (non-destructive preview)
  - On click (if unlocked): EquipCosmetic

- Equip button: "Equipar" (confirms selection)
- Preview rotates via mouse drag on SceneCapture render target
- Unlocked count: "12 / 28 cosméticos"

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 💥 PROMPT 64 — KILL STREAK & COMBO CHAIN SYSTEM
## ════════════════════════════════════════

```
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-ui-umg-slate
@ue-niagara-effects
@ue-actor-component-archit...

Create the Kill Streak and Combo Chain system for DungeonForged UE 5.4.
Rewards fast, aggressive play with escalating bonuses.
Path convention:
  .h  → Source/DungeonForged/Public/Combat/KillStreak/<FileName>.h
  .cpp → Source/DungeonForged/Private/Combat/KillStreak/<FileName>.cpp
Always write full path as comment at top of each file.

─── UDFKillStreakComponent extends UActorComponent ────────────
On PlayerCharacter:

Properties:
- int32 CurrentStreak = 0
- float StreakWindowDuration = 5.0f  ← time between kills before streak breaks
- FTimerHandle StreakResetTimer
- TArray<FDFStreakTier> StreakTiers  ← configured in BP/DataAsset

FDFStreakTier struct:
- int32 KillsRequired
- FText StreakName              ← "Em Chamas!", "Devastador!", "Lendário!"
- FLinearColor DisplayColor
- TSubclassOf<UGameplayEffect> BonusEffect   ← applied at this tier
- UNiagaraSystem* StreakVFX     ← attached to player at this tier
- int32 BonusGoldPerKill        ← extra gold while at this tier

Tiers (configure in DataAsset):
  3 kills  → "Em Chamas"    → GE: Damage +10%, Speed +5%
  6 kills  → "Devastador"   → GE: Damage +20%, Speed +10%, gold +5/kill
  10 kills → "Imparável"    → GE: Damage +35%, Speed +15%, gold +10/kill
  15 kills → "LENDÁRIO"     → GE: Damage +50%, AoE on every kill (small explosion)

Functions:
- OnEnemyKilled(AActor* Enemy):
  → CurrentStreak++
  → RunManager->AddGold(CurrentTier.BonusGoldPerKill)
  → AchievementSubsystem->TrackProgress("Stat.TotalKills", 1)
  → CheckTierUpgrade()
  → ResetStreakTimer()
  → UDFCombatTextSubsystem->SpawnText(EnemyLocation, 0, ECombatTextType::XPGain)
    with "🔥 x{CurrentStreak}" text
- CheckTierUpgrade():
  → Find highest tier where KillsRequired <= CurrentStreak
  → If new tier > current: apply tier GE, spawn tier VFX, show WBP_StreakAnnounce
- ResetStreakTimer():
  → Clear + restart FTimerHandle StreakWindowDuration
- OnStreakExpired():
  → Remove current tier GE
  → Remove tier Niagara VFX
  → CurrentStreak = 0
  → Show streak summary if > 5: WBP_StreakSummary
- GetCurrentTier(): FDFStreakTier*

─── WBP_KillStreakDisplay (add to ADFRunHUD) ──────────────────
Located top-right corner, above kill counter:

- UTextBlock* StreakCount       ← "x{N}" large number
- UTextBlock* StreakName        ← "EM CHAMAS!" colored by tier
- UProgressBar* TimeBar         ← depletes over StreakWindowDuration (5s)
- UWidgetAnimation* TierUpAnim  ← dramatic scale + glow on tier upgrade
- Animate: number pulses on each new kill
- Hide when streak = 0 (fade out 0.5s)

WBP_StreakAnnounce (center screen, brief):
- Large tier name slides in from right, holds 1.5s, fades out
- Color matches tier FLinearColor
- Example: "⚡ DEVASTADOR!" in orange

WBP_StreakSummary (on streak break if > 5):
- "Sequência Encerrada!"
- "Melhor: {PeakStreak} abates"
- "Ouro bônus: +{BonusGold}"
- Shows for 2s then fades

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 📊 PROMPT 66 — LEADERBOARD & RUN STATISTICS
## ════════════════════════════════════════

```
@ue-serialization-savegames
@ue-data-assets-tables
@ue-ui-umg-slate

Create the Leaderboard and Run Statistics system for DungeonForged UE 5.4.
Local leaderboard + detailed run history via Cronista NPC.
Path convention:
  .h  → Source/DungeonForged/Public/Statistics/<FileName>.h
  .cpp → Source/DungeonForged/Private/Statistics/<FileName>.cpp
Always write full path as comment at top of each file.

─── FDFRunRecord (struct) ─────────────────────────────────────
File: Public/Statistics/DFStatisticsData.h
Complete run snapshot saved after each run:
- FDateTime RunDate
- FName ClassPlayed
- int32 FloorReached
- float RunDuration             ← seconds
- int32 TotalKills
- int32 BossKills
- int32 GoldCollected
- int32 DamageDone
- int32 DamageTaken
- int32 HealingDone
- int32 PeakKillStreak
- TArray<FName> AbilitiesUsed   ← ability tags activated this run
- ETravelReason Outcome         ← Victory, Defeat, Abandon
- int32 MetaXPEarned
- FName ChallengeUsed           ← if challenge run

─── UDFStatisticsSubsystem extends UGameInstanceSubsystem ─────

Properties:
- TArray<FDFRunRecord> RunHistory   ← all runs, max 100 (oldest removed)
- FDFLifetimeStats LifetimeStats    ← aggregate stats

FDFLifetimeStats struct:
- int32 TotalRuns, TotalWins, TotalDeaths
- int32 TotalKills, TotalBossKills
- float TotalPlayTime            ← seconds
- int32 TotalGoldEarned
- int32 TotalDamageDone
- int32 BestFloor
- float BestRunTime              ← fastest victory
- int32 BestKillStreak
- TMap<FName, int32> ClassPlayCount ← runs per class

Functions:
- RecordRun(FDFRunRecord Record):
  → RunHistory.Insert(0, Record) (most recent first)
  → If RunHistory.Num() > 100: RemoveLast
  → UpdateLifetimeStats(Record)
  → SaveGame
- UpdateLifetimeStats(FDFRunRecord Record):
  → Aggregate all fields into LifetimeStats
  → Update bests (floor, time, streak)
- GetLeaderboard(ELeaderboardType Type): TArray<FDFRunRecord>
  → BestFloor: sort by FloorReached desc, then RunDuration asc
  → FastestWin: filter Outcome==Victory, sort by RunDuration asc
  → MostKills: sort by TotalKills desc
  → ByClass(FName): filter by ClassPlayed
- GetLifetimeStats(): FDFLifetimeStats
- GetPersonalBest(ELeaderboardType): FDFRunRecord*

─── WBP_RunHistory extends UDFUserWidgetBase ──────────────────
Via Cronista NPC in Nexus:

Layout:
- Tabs: [Histórico] [Records Pessoais] [Estatísticas de Vida]
- UTextBlock* HeaderSummary ← "23 runs | 8 vitórias | 47:23 jogados"

Histórico tab:
- UScrollBox of WBP_RunRecordCard (one per run, newest first)
  Each card: date, class icon, floor reached, outcome icon, duration, kills
  Expand on click: full stats breakdown

Records Pessoais tab:
- Grid of personal bests:
  Maior Andar, Vitória Mais Rápida, Mais Abates, Maior Streak
  Each: value + class played + date

Estatísticas de Vida tab:
- UTextBlock* blocks for all LifetimeStats fields
- UProgressBar* ClassPlayBar per class (% of total runs)
- Favorite ability: most activated tag in AbilitiesUsed across all runs

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🏘️ PROMPT 57 — ROOM TYPES EXPANSION
## ════════════════════════════════════════

```
@ue-actor-component-archit...
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-physics-collision
@ue-ui-umg-slate
@ue-niagara-effects

Create expanded Room Types for DungeonForged UE 5.4 dungeons.
All rooms integrate with ADFDungeonManager and PCG system (Prompt 9).
Path convention:
  .h  → Source/DungeonForged/Public/Dungeon/Rooms/<FileName>.h
  .cpp → Source/DungeonForged/Private/Dungeon/Rooms/<FileName>.cpp
Always write full path as comment at top of each file.

─── ADFRoomBase extends AActor ────────────────────────────────
Base for all special rooms:

Properties:
- ERoomType RoomType
- bool bIsCleared = false
- ADFMinimapRoom* MinimapRef    ← linked minimap room icon (Prompt 39)
- TArray<ADFDoor*> ConnectedDoors ← locked until cleared

Functions:
- OnRoomEntered(ACharacter* Player): abstract
- OnRoomCleared(): unlock ConnectedDoors, update MinimapRef->VisitRoom()
- LockDoors(): foreach door → door->LockDoor()
- UnlockDoors(): foreach door → door->UnlockDoor()

─── ADFArenaRoom extends ADFRoomBase ──────────────────────────
Combat challenge room — waves of enemies, door locked until clear:

Properties:
- TArray<FDFWaveData> Waves     ← struct: TArray<FName EnemyRows>, SpawnDelay
- int32 CurrentWave = 0
- TArray<ADFEnemyBase*> ActiveEnemies
- UNiagaraComponent* ArenaVFX   ← red barrier particles at doorways
- ULevelSequence* IntroSequence ← brief dramatic camera showing the arena

FDFWaveData struct:
- TArray<FName> EnemyRows
- float WaveStartDelay          ← delay after previous wave cleared
- bool bEliteWave               ← all enemies get +50% HP, +30% dmg

Functions:
- OnRoomEntered: LockDoors(), play intro sequence, StartWave(0)
- StartWave(int32 WaveIndex): spawn enemies from Waves[WaveIndex].EnemyRows
- OnEnemyKilled: ActiveEnemies.Remove, if empty → AdvanceWave()
- AdvanceWave():
  → CurrentWave++
  → if CurrentWave >= Waves.Num(): OnRoomCleared()
  → else: WaitDelay(WaveStartDelay) → StartWave(CurrentWave)
  → Show wave counter: "ONDA 2 / 3" via WBP_WaveCounter (HUD overlay)
- OnRoomCleared: UnlockDoors(), remove arena VFX, spawn bonus loot

─── ADFPuzzleRoom extends ADFRoomBase ─────────────────────────
Puzzle room with levers, pressure plates, moving platforms:

Properties:
- TArray<ADFPuzzleElement*> PuzzleElements
- TArray<bool> ElementStates    ← current state per element
- TArray<bool> SolutionPattern  ← required pattern to solve
- float ResetDelay = 5.f        ← auto-reset if partial solution left

Puzzle elements (ADFPuzzleElement base):
1. ADFLever extends ADFInteractableBase:
   - UStaticMeshComponent (lever mesh)
   - bool bState = false
   - Interact: toggle bState, play lever animation, notify PuzzleRoom
   
2. ADFPressurePlate:
   - UBoxComponent trigger
   - Activates on player weight (OnBeginOverlap), deactivates when player leaves
   - Some plates: require heavy object (push-able boulder)

3. ADFPushableRock extends AActor:
   - UStaticMeshComponent + UBoxComponent
   - Player can push: on overlap + interact: AddImpulse in facing direction
   - Physics-based, can land on pressure plates

Functions:
- OnElementChanged(int32 ElementIndex, bool NewState): check solution
- CheckSolution(): if ElementStates == SolutionPattern → Solved()
- Solved(): OnRoomCleared(), spawn loot (guaranteed Rare+), play solve fanfare
- AutoReset(): if ResetDelay passes without solve → reset all elements

─── ADFSecretRoom extends ADFRoomBase ─────────────────────────
Hidden room behind breakable wall:

Properties:
- UStaticMeshComponent* FakeWall    ← looks like normal wall
- bool bIsRevealed = false
- UNiagaraComponent* HintVFX        ← subtle shimmer on wall (visible close range)
- int32 GuaranteedRarityMinimum = 3  ← Epic minimum for loot

Functions:
- BeginPlay: FakeWall visible, room hidden
- OnPlayerNear (500 units): show subtle HintVFX on wall face
- OnWallDestroyed (breakable or interact): reveal room, bIsRevealed=true
  → FadeOut FakeWall mesh → expose room beyond
  → Spawn discovery VFX + play secret jingle SFX
  → UDFAchievementSubsystem->TrackProgress("Stat.SecretRooms", 1)
- OnRoomEntered: spawn guaranteed Epic+ loot (2-3 items)

─── ADFRestRoom extends ADFRoomBase ───────────────────────────
Safe room with campfire — restore resources, checkpoint:

Properties:
- ADFShrine* RestShrine          ← healing shrine (Prompt 24)
- AActor* CampfireActor          ← cosmetic fire with warmth light
- bool bCheckpointSaved = false

Functions:
- OnRoomEntered:
  → Apply GE_RestRoom_Restore (Instant):
    Health += MaxHealth * 0.3
    Mana = MaxMana (full)
    Stamina = MaxStamina
  → UDFWorldTransitionSubsystem->SaveCheckpoint(FloorCheckpoint)
  → bCheckpointSaved = true
  → Show WBP_RestNotification ("Checkpoint salvo — HP restaurado")
  → Play campfire crackle ambient sound

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🧱 PROMPT 63 — DESTRUCTIBLE ENVIRONMENT
## ════════════════════════════════════════

```
@ue-physics-collision
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-niagara-effects
@ue-actor-component-archit...

Create the Destructible Environment system for DungeonForged UE 5.4.
Uses UE5 Chaos Physics for fragmentation.
Path convention:
  .h  → Source/DungeonForged/Public/Dungeon/Destructibles/<FileName>.h
  .cpp → Source/DungeonForged/Private/Dungeon/Destructibles/<FileName>.cpp
Always write full path as comment at top of each file.

─── ADFDestructibleProp extends AActor ────────────────────────
Base for all destructible environment props:

Properties:
- UGeometryCollectionComponent* GeomCollectionComp ← Chaos fracture mesh
- float MaxHealth = 50.f
- float CurrentHealth
- bool bIsDestroyed = false
- TSubclassOf<ADFLootDrop> HiddenLoot ← optional: item inside (barrel)
- float LootChance = 0.2f             ← 20% chance to contain item
- UNiagaraSystem* DestroyVFX
- USoundBase* DestroySound
- bool bCanBeDestroyedByPlayer = true
- bool bCanBeDestroyedByExplosion = true

Functions:
- TakeDamage override: CurrentHealth -= Damage, if <= 0 → Destroy()
- Destroy():
  → bIsDestroyed = true
  → GeomCollectionComp: apply impulse for dramatic shatter
  → Spawn DestroyVFX at location
  → Play DestroySound (spatialized)
  → If HiddenLoot && FMath::FRand() < LootChance: SpawnActor(HiddenLoot)
  → SetLifeSpan(5.f) for cleanup after physics settle

─── ADFDestructible_Barrel extends ADFDestructibleProp ────────
Wooden barrel — common prop:
- MaxHealth = 30.f, LootChance = 0.3f
- Mesh: barrel (cylindrical, stave fracture pattern)
- DestroyVFX: wood splinter burst
- Can contain: small gold, health potion consumable

ADFDestructible_ExplosiveBarrel extends ADFDestructibleProp:
- MaxHealth = 20.f (more fragile — glass cannon)
- bCanBeDestroyedByFire = true (GE_DoT_Fire contact destroys immediately)
- OnDestroy: also triggers explosion:
  → SphereOverlap radius=400
  → Apply GE_Damage_True (80 flat) to all in radius
  → Apply GE_DoT_Fire (3s) to all
  → Chain explosions if other ExplosiveBarrels in range
  → Camera shake (heavy), screen flash orange
  → Visual: red barrel mesh → glow pre-explosion → massive fireball Niagara
- TelegraphDestruction: if DoT_Fire applied → 1.5s warning (barrel glows red, beeping SFX)

─── ADFDestructible_Bookshelf extends ADFDestructibleProp ─────
- MaxHealth = 40.f, LootChance = 0.5f (books = lore objects often)
- OnDestroy: may spawn ADFLoreObject (40% chance, random lore entry)
- Fracture pattern: shelves collapse, books scatter (ragdoll physics)
- DestroyVFX: dust cloud + paper pages flying

─── ADFDestructible_Pillar extends ADFDestructibleProp ────────
Structural pillar — tactical destruction:
- MaxHealth = 150.f (requires sustained damage)
- OnDestroy: debris falls in a line (directional collapse toward impact)
  → LineTrace beneath pillar → apply GE_Damage_Physical (60) to anything under debris
  → ADFTrap_CollapsingFloor pattern for the fall
  → Creates permanent cover (debris mesh stays as obstacle post-collapse)
- Strategic use: knock pillar onto enemies

─── UDFDestructibleSpawnerComponent extends UActorComponent ───
On ADFDungeonManager — manages procedural destructible placement:

Functions:
- SpawnDestructiblesForRoom(ADFRoomBase* Room, const FDFDungeonFloorRow& Floor):
  → Read Floor.DestructibleDensity (0-1)
  → For each predefined spawn point in room: roll against density
  → Select destructible type weighted:
    Barrel: 50%, ExplosiveBarrel: 15%, Bookshelf: 20%, Pillar: 15%
  → SpawnActor at point with random rotation
- ExplosiveBarrelRules: max 2 per room, min 300 units apart

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🌿 PROMPT 67 — PCG ROOM CONTENT EXPANSION
## ════════════════════════════════════════

```
@ue-procedural-generation
@ue-world-level-streaming
@ue-data-assets-tables
@ue-niagara-effects

Create expanded PCG Room Content for DungeonForged UE 5.4.
Extends the basic dungeon generation (Prompt 9) with biomes, decoration, and variety.
Path convention:
  .h  → Source/DungeonForged/Public/Dungeon/PCG/<FileName>.h
  .cpp → Source/DungeonForged/Private/Dungeon/PCG/<FileName>.cpp
Always write full path as comment at top of each file.

─── EBiomeType (enum) ─────────────────────────────────────────
Values: Stone, Fire, Ice, Crypt, Forest, Arcane

─── UDFBiomeDataAsset extends UPrimaryDataAsset ───────────────
File: Public/Dungeon/PCG/DFBiomeData.h
Per-biome configuration:

Properties:
- EBiomeType BiomeType
- FLinearColor AmbientLightColor ← dungeon mood color
- float AmbientLightIntensity
- UMaterialInterface* WallMaterial
- UMaterialInterface* FloorMaterial
- UMaterialInterface* CeilingMaterial
- TArray<UStaticMesh*> DecorProps  ← biome-specific props (torches, vines, bones)
- UNiagaraSystem* AmbientVFX       ← ambient particles (embers for Fire, snow for Ice)
- USoundBase* AmbientSound
- TArray<FName> BiomeEnemyRows     ← preferred enemies (DT_Enemies filter)
- TSubclassOf<ADFTrapBase> BiomeTrap ← dominant trap type

Biome definitions (describe setups):
Stone  → grey walls, torch sconces, crack decals, standard enemies
Fire   → red/orange walls, lava pools (damage zone), ember VFX, fire enemies
Ice    → blue-white walls, icicle props, snowflake VFX, frost enemies
Crypt  → dark stone, coffins, skeleton decorations, undead enemies
Forest → vine-covered walls, mushroom props, bioluminescent VFX, beast enemies
Arcane → purple energy walls, floating crystal props, arcane mist VFX, mage enemies

─── UDFPCGBiomeComponent extends UActorComponent ──────────────
On ADFDungeonManager — drives biome selection per floor:

Properties:
- TArray<UDFBiomeDataAsset*> AvailableBiomes
- UDFBiomeDataAsset* CurrentBiome
- TMap<int32, EBiomeType> FloorBiomeMap ← floors 1-10 → assigned biome

Functions:
- GenerateBiomeMap(int32 TotalFloors):
  → Assign biomes ensuring variety (no same biome 3 floors in a row)
  → Boss floor (10): always "Arcane" or darkest biome
- GetBiomeForFloor(int32 Floor): UDFBiomeDataAsset*
- ApplyBiomeToRoom(ADFRoomBase* Room, UDFBiomeDataAsset* Biome):
  → Swap wall/floor/ceiling materials on room static meshes
  → Set PostProcessVolume light color + intensity
  → Spawn AmbientVFX (looping Niagara) in room center
  → Set ambient sound on room's UAudioComponent
  → Scatter biome DecorProps at random positions (non-navmesh areas)

─── PCG Graph Extension ───────────────────────────────────────
Describe additional PCG Graph nodes beyond Prompt 9:

PCGGraph_BiomeDecoration (runs after room placement):
- Input: room floor points
- Nodes:
  SpawnOnSurface (decor props) → Filter by navmesh exclusion zone
    → BiomePropSampler (samples from CurrentBiome.DecorProps)
    → ScatterWithVariation (random rot/scale ±15%)
  
  SpawnTorches → every 400 units along walls (using wall normals)
    → Attach PointLightComponent to each torch (warm orange, radius=300)

  SpawnDestructibles → UDFDestructibleSpawnerComponent integration
    → Density from FDFDungeonFloorRow.DestructibleDensity

PCGGraph_CorridorGeneration:
- Spline-based corridors between rooms (not straight only)
- Slight curve variation via PCG spline nodes
- Width: 300-400 units (varies per biome — ice=narrow, forest=wide)

─── WBP_FloorBiomeTransition ──────────────────────────────────
Brief overlay when entering a new biome floor:
- UImage* BiomeArt (atmospheric art per biome)
- UTextBlock* BiomeName ← "Cripta das Sombras" (floor-specific name)
- UTextBlock* FloorText ← "Andar 5"
- Fade in 0.5s, hold 2s, fade out 0.5s
- Shown by WBP_LoadingScreen NextFloor variant (Prompt 48)

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 💡 PROMPT 68 — DYNAMIC LIGHTING SYSTEM
## ════════════════════════════════════════

```
@ue-actor-component-archit...
@ue-gameplay-tags
@ue-niagara-effects

Create the Dynamic Lighting system for DungeonForged UE 5.4.
Lighting that reacts to gameplay events — abilities, status effects, room state.
Path convention:
  .h  → Source/DungeonForged/Public/FX/Lighting/<FileName>.h
  .cpp → Source/DungeonForged/Private/FX/Lighting/<FileName>.cpp
Always write full path as comment at top of each file.

─── UDFLightEventSubsystem extends UWorldSubsystem ────────────

Multicast delegates:
- FOnLightEvent OnTorchExtinguished(FVector Location)
- FOnLightEvent OnTorchReignited(FVector Location)
- FOnLightEvent OnBossPhaseChanged(int32 Phase)
- FOnLightEvent OnPlayerStealthed()
- FOnLightEvent OnRoomEntered(EBiomeType Biome)
- FOnLightEvent OnPlayerBerserk(bool bActive)
- FOnLightEvent OnExplosion(FVector Location, float Radius)

Functions:
- BroadcastEvent(FGameplayTag EventTag, FVector Location, float Value=0):
  → Maps GameplayTag to correct delegate broadcast
  → EventTag examples: Event.Light.TorchOut, Event.Light.Explosion

─── ADFDynamicTorch extends AActor ────────────────────────────
Interactive torch that reacts to ice/fire abilities:

Properties:
- UStaticMeshComponent* TorchMesh
- UPointLightComponent* FlameLight   ← warm orange, radius=400, intensity=800
- UNiagaraComponent* FlameVFX
- bool bIsLit = true

Functions:
- ExtinguishTorch():
  → bIsLit = false
  → FlameLight->SetVisibility(false)
  → FlameVFX->Deactivate()
  → Spawn smoke puff Niagara
  → UDFLightEventSubsystem->BroadcastEvent(Event.Light.TorchOut, Location)
  → Room darkness increases (ambient drops if many torches out)
- ReigniteTorch():
  → bIsLit = true
  → FlameLight->SetVisibility(true)
  → FlameVFX->Activate()
  → Spawn re-ignite Niagara burst

Integration with abilities:
- FrostBolt/Blizzard projectile OnHit torch → ExtinguishTorch()
- Fireball OnHit extinguished torch → ReigniteTorch() + apply GE_DoT_Fire to nearby enemies
- If room has < 30% torches lit: apply GE_Debuff_Darkness to all enemies in room:
  → Enemy perception range -50% (AI sight reduced)

─── UDFDynamicLightingComponent extends UActorComponent ───────
On PlayerCharacter — drives player-proximate lighting effects:

Properties:
- UPointLightComponent* PlayerAmbientLight   ← subtle follow light
  (color = white at base, changes per status)
- TMap<FGameplayTag, FLinearColor> TagLightColors:
  Effect.DoT.Fire → orange (FLinearColor(1,0.4,0))
  Effect.DoT.Frost → icy blue (FLinearColor(0.4,0.7,1))
  State.Berserk → deep red (FLinearColor(1,0.1,0))
  State.Stealthed → very dim purple
  Effect.Buff.BattleHymn → gold (FLinearColor(1,0.85,0.2))
  State.HolyShielded → bright white-gold

Functions:
- SubscribeToASC(UAbilitySystemComponent* ASC): listen to tag changes
- OnTagChanged(FGameplayTag Tag, bool bAdded):
  → If bAdded && TagLightColors.Contains(Tag):
    LerpLightColor(TagLightColors[Tag], 0.3s)
  → On all tags cleared: LerpLightColor(DefaultWhite, 0.5s)
- LerpLightColor(FLinearColor Target, float Duration): FTimerHandle lerp

─── ADFBossArenaLighting ──────────────────────────────────────
Controls boss room atmosphere dynamically:

Properties:
- TArray<UPointLightComponent*> ArenaLights
- FLinearColor Phase1Color = FLinearColor(0.8,0.6,0.3) ← warm dungeon
- FLinearColor Phase2Color = FLinearColor(1,0.2,0.1)   ← deep red menace
- FLinearColor EnrageColor = FLinearColor(1,0,0)        ← pure red horror

Functions:
- OnBossPhaseChanged(int32 NewPhase):
  → Lerp all ArenaLights color to Phase{N}Color over 2s
  → Decrease brightness each phase (darker = more dangerous)
- OnBossEnraged():
  → LerpToEnrageColor over 0.5s (fast, dramatic)
  → Add UDFLightFlickerComponent to all arena lights (rapid flicker at 8hz)
  → Apply LensFlare override on all lights (red)
- OnBossDefeated():
  → Lerp all lights to pure white over 3s (divine light fills room)
  → Remove flicker components

─── UDFLightFlickerComponent extends UActorComponent ──────────
Attach to any ULightComponent for flickering:

Properties:
- float FlickerRate = 8.f        ← hz
- float MinIntensity = 0.3f
- float MaxIntensity = 1.0f
- float FlickerDuration = -1.f  ← -1 = infinite

Functions:
- BeginPlay/ActivateFlicker: start FTimerHandle at FlickerRate hz
- OnFlickerTick: set light intensity to FMath::RandRange(Min,Max) * BaseIntensity
- StopFlicker: clear timer, restore base intensity

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 🎥 PROMPT 69 — REPLAY SYSTEM
## ════════════════════════════════════════

```
@ue-gameplay-framework
@ue-serialization-savegames
@ue-ui-umg-slate
@ue-async-threading

Create the Replay System for DungeonForged UE 5.4.
Records key moments and allows playback via Cronista NPC.
Path convention:
  .h  → Source/DungeonForged/Public/Replay/<FileName>.h
  .cpp → Source/DungeonForged/Private/Replay/<FileName>.cpp
Always write full path as comment at top of each file.

─── FDFReplayMoment (struct) ──────────────────────────────────
File: Public/Replay/DFReplayData.h
Snapshot of a notable game moment:
- float Timestamp               ← seconds into the run
- FVector PlayerLocation
- FRotator PlayerRotation
- FGameplayTag EventTag         ← what happened (Kill.Boss, SecondWind, LevelUp)
- FText MomentDescription       ← "Derrotou o Guardião com 12% de HP"
- float DamageValue             ← if relevant
- FName EnemyRow                ← if relevant
- bool bIsHighlight             ← auto-flagged: bosses kills, near deaths, killstreaks

─── UDFReplaySubsystem extends UGameInstanceSubsystem ─────────

Properties:
- TArray<FDFReplayMoment> CurrentRunMoments   ← cleared each new run
- TArray<FDFReplayMoment> SavedHighlightReel  ← best moments saved to disk (max 30)
- bool bIsRecording = false
- bool bIsPlayingBack = false
- int32 MaxMomentsPerRun = 200

Functions:
- StartRecording(): bIsRecording = true, clear CurrentRunMoments
- StopRecording(): bIsRecording = false
- RecordMoment(FGameplayTag EventTag, FVector Location, FText Description, float Value=0):
  → If !bIsRecording or Num >= MaxMomentsPerRun: return
  → Create FDFReplayMoment, auto-flag bIsHighlight:
    KillStreak >= 10 = highlight
    SecondWind activation = highlight
    Boss kill = highlight
    Damage > MaxHealth * 0.8 in one hit = highlight (near death)
  → Add to CurrentRunMoments
- GenerateHighlightReel():
  → Filter CurrentRunMoments where bIsHighlight
  → Sort by Timestamp
  → Take top 10 most impactful moments
  → Append to SavedHighlightReel (max 30 total across runs), SaveGame
- GetHighlightReel(): TArray<FDFReplayMoment>

Recording trigger hooks (add calls to existing systems):
- ADFEnemyBase::OnDeath (boss): RecordMoment(Kill.Boss, ...)
- UDFKillStreakComponent: on LENDÁRIO tier: RecordMoment(KillStreak.Legendary, ...)
- GA_SecondWind::ActivateAbility: RecordMoment(Survival.SecondWind, ...)
- UDFAttributeSet: massive single hit received: RecordMoment(Hit.NearDeath, ...)
- UDFLevelingComponent::LevelUp: RecordMoment(Progression.LevelUp, ...)

─── WBP_ReplayViewer extends UDFUserWidgetBase ────────────────
Accessed via Cronista NPC in Nexus — "Momentos Épicos":

Layout:
- UScrollBox* MomentList:
  WBP_MomentCard per FDFReplayMoment:
  - UImage* EventIcon (icon per EventTag)
  - UTextBlock* MomentDescription
  - UTextBlock* Timestamp ("3:42 da run")
  - UTextBlock* DamageValue (if relevant)
  - UImage* HighlightStar (if bIsHighlight)
  - On click: jump to playback position

- Note: Full camera replay requires UE replay system DemoNetDriver.
  For this project, implement SIMPLIFIED replay:
  → Show FDFReplayMoment data as a "summary card" with stats
  → No actual 3D camera playback (out of scope without DemoNetDriver setup)
  → Instead: auto-generate a screenshot at moment of boss kill
    via FHighResScreenshotConfig (triggered silently during gameplay)
  → Display screenshot in WBP_MomentCard

─── Auto-Screenshot System ────────────────────────────────────
UDFMomentScreenshotComponent on PlayerCharacter:
- On boss kill: WaitDelay(0.5s) → TakeScreenshot("Moment_Boss_{FloorN}_{Date}")
- On SecondWind: TakeScreenshot("Moment_SecondWind_{Date}")
- Max 20 auto-screenshots stored in game's Screenshots folder
- WBP_ReplayViewer loads these via FPaths::ScreenShotDir()

Output all .h and .cpp with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## 👥 PROMPT 70 — MULTIPLAYER CO-OP EXTENSION
## ════════════════════════════════════════

```
@ue-networking-replication
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-ui-umg-slate
@ue-gameplay-framework

Create the Multiplayer Co-op extension for DungeonForged UE 5.4.
2-player co-op — shared dungeon, independent classes, collaborative mechanics.
Extends Prompt 43 (Network Audit). Build on top of existing GAS replication.
Path convention:
  .h  → Source/DungeonForged/Public/Coop/<FileName>.h
  .cpp → Source/DungeonForged/Private/Coop/<FileName>.cpp
Always write full path as comment at top of each file.

─── UDFCoopSubsystem extends UGameInstanceSubsystem ───────────

Properties:
- bool bIsCoopSession = false
- int32 MaxPlayers = 2
- TArray<APlayerState*> ConnectedPlayers
- ELootDistributionMode LootMode   ← NeedGreed, AutoSplit, FreeForAll

Functions:
- HostCoopSession():
  → IOnlineSubsystem::Get() → CreateSession (LAN, MaxPlayers=2)
  → On success: OpenLevel(NexusMap, bIsServer=true)
- JoinCoopSession(FString ServerAddress):
  → ClientTravel(ServerAddress)
- OnPlayerJoined(APlayerController* PC):
  → ConnectedPlayers.Add(PC->PlayerState)
  → Broadcast OnCoopPlayerJoined
- OnPlayerLeft(APlayerController* PC):
  → ConnectedPlayers.Remove(...)
  → If host leaves: migrate host or end session
- IsCoopActive(): bIsCoopSession && ConnectedPlayers.Num() > 1

─── Co-op specific gameplay mechanics ─────────────────────────

Revive mechanic (co-op only):
- GA_Revive extends UDFGameplayAbility:
  File: Public/Coop/DFAbility_Revive.h
  Tag: Ability.Coop.Revive
  - CanActivateAbility: other player has State.Dead AND distance < 200
  - CommitAbility (no cost)
  - PlayMontageAndWait (ReviveMontage — kneel over ally, 4s channel)
  - WaitGameplayEvent: interrupted if player takes damage during channel
  - On complete: other player respawns at current location with 25% HP
    → Apply GE_Revived (HasDuration 5s): State.Invulnerable (brief protection)
  - If interrupted (took damage): Revive cancelled, player must retry

Boss HP scaling:
- In ADFBossBase::BeginPlay:
  → Check UDFCoopSubsystem::IsCoopActive()
  → If co-op: apply GE_CoopBossScaling (Instant):
    MaxHealth *= 1.6, Strength *= 1.3
  → Scale per connected player count dynamically

XP distribution:
- In ADFEnemyBase::OnDeath:
  → Get all players within 2000 units
  → Each gets full XP (no split — both benefit equally)

─── Loot distribution ─────────────────────────────────────────
UDFLootDistributionComponent (on ADFRunGameState for co-op):

NeedGreed mode (WBP_LootRoll):
- On item drop: broadcast to all players
- Each player has 10s to: [Need] (wants for their class) or [Greed] (any)
- Need > Greed in priority, ties broken by random roll
- WBP_LootRoll: item preview + Need/Greed/Pass buttons + 10s timer

AutoSplit mode:
- Round-robin distribution: alternating item assignments by rarity
- High-rarity items: Need check first

FreeForAll mode:
- First player to interact with ADFLootDrop gets it (default competitive)

─── Co-op HUD additions ───────────────────────────────────────
WBP_CoopPartnerFrame (add to ADFRunHUD for co-op):
- Partner's class icon + Name
- UProgressBar* PartnerHealth (small, top-left corner)
- UProgressBar* PartnerMana
- TArray<UImage*> PartnerAbilityIcons (cooldown state of partner's 4 slots)
- State indicators: Dead (red cross), Reviving (pulsing), Disconnected (grey)

─── ADFNexusGameMode co-op adjustments ────────────────────────
In co-op:
- Both players must confirm class selection in WBP_ClassSelection before run starts
- WBP_ClassSelection: show second player's selected class in real-time
  (Replicated via GameState SelectedClasses array)
- "Iniciar Run" button: only enabled when BOTH players confirmed
- Run cannot start if only 1 player selected

─── UDFCoopSessionWidget extends UUserWidget ──────────────────
Pre-game lobby (shown in Nexus before run):
- UTextBlock* HostInfo ← "Anfitrião: Player1"
- TArray<WBP_PlayerSlot*> (2 slots):
  Each: player name, selected class preview, ready status (green checkmark)
- UButton* StartRun (host only, enabled when all ready)
- UButton* InviteFriend → copy LAN address to clipboard
- UButton* LeaveSession → UDFCoopSubsystem::LeaveSession

Output all .h and .cpp with correct Public/Private paths.
```

---

## ══════════════════════════════════════════════
## 🎮 MENU PRINCIPAL & SELEÇÃO DE CLASSES
## ══════════════════════════════════════════════

## ════════════════════════════════════════
## 🏠 PROMPT 71 — MAIN MENU & SPLASH SCREEN
## ════════════════════════════════════════

```
@ue-gameplay-framework
@ue-ui-umg-slate
@ue-serialization-savegames
@ue-animation-system
@ue-audio-system

Create the Main Menu and Splash Screen system for DungeonForged UE 5.4.
This is the very first screen the player sees on launch.
Path convention:
  .h  → Source/DungeonForged/Public/GameModes/MainMenu/<FileName>.h
  .cpp → Source/DungeonForged/Private/GameModes/MainMenu/<FileName>.cpp
Always write full path as comment at top of each file.

─── Map Setup ─────────────────────────────────────────────────
Map: "MainMenu"
- GameMode: ADFMainMenuGameMode
- No Pawn (spectator only)
- Background: cinematic dungeon environment (pre-built static level)
  → Slow camera dolly through atmospheric dungeon corridor
  → Lumen lighting: torches, fog, god rays through cracks
  → Use UDFCinematicSubsystem (Prompt 54) with looping level sequence

─── ADFMainMenuGameMode extends AGameModeBase ─────────────────

Properties:
- TSubclassOf<ADFMainMenuHUD> HUDClass
- ULevelSequence* BackgroundLoopSequence ← slow atmospheric camera move

Functions:
- InitGame:
  → Play BackgroundLoopSequence (looping, no skip)
  → Load UDFSaveGame → detect bHasSaveData
  → Play UDFMusicManagerSubsystem::SetMusicState(MainMenu)
- PostLogin:
  → Show WBP_SplashScreen sequence first
  → After splashes: show WBP_MainMenu
  → If FirstLaunch (no SaveGame): skip "Continuar", highlight "Nova Aventura"

─── ADFMainMenuHUD extends AHUD ───────────────────────────────
- WBP_SplashScreen (shown first on launch)
- WBP_MainMenu (primary navigation)
- WBP_Credits (shown via Credits button)
- WBP_ConfirmDialog (reusable: "Tem certeza?" for destructive actions)

─── WBP_SplashScreen extends UUserWidget ──────────────────────
Sequence of splash images before main menu:
Full-screen black background, each logo fades in → holds → fades out.

Splash sequence (play in order, total ~4s):
1. Unreal Engine logo (1.2s fade in, 1s hold, 0.8s fade out)
2. Studio logo (same timing)
3. DungeonForged title card (logo + subtitle "Um Roguelike ARPG")
   → More dramatic: scale up from 90%→100% while fading in
   → Hold 1.5s, then transition to WBP_MainMenu

Properties:
- TArray<UTexture2D*> SplashImages
- TArray<float> HoldDurations
- int32 CurrentSplash = 0

Functions:
- PlayNextSplash():
  → Set current image, play FadeIn animation
  → WaitDelay(HoldDuration) → play FadeOut → PlayNextSplash (or ShowMainMenu)
- SkipSplashes(): any key press after 0.5s → jump to main menu
- ShowMainMenu(): RemoveFromParent → ADFMainMenuHUD::ShowMainMenu()

─── WBP_MainMenu extends UUserWidget ──────────────────────────
Primary navigation screen — cinematic dungeon background visible behind it.

Layout:
- Left panel (400px wide, left-aligned, dark gradient overlay):
  - UImage* LogoImage (DungeonForged logo, top of panel)
  - UTextBlock* SubtitleText ← "Um Roguelike ARPG"
  - UVerticalBox* ButtonStack:
    All buttons: custom style (parchment texture, gold text, hover glow)

    WBP_MenuButton "Continuar Aventura":
    - Visible ONLY if SaveGame exists with a run in progress
    - Sub-text: "Andar {CurrentFloor} — {ClassName}" (last run info)
    - On click: UDFWorldTransitionSubsystem::TravelToNexus(ETravelReason::FirstLaunch)
      (loads nexus with existing SaveGame)

    WBP_MenuButton "Nova Aventura":
    - Always visible
    - If SaveGame exists: show WBP_ConfirmDialog first
      ("Iniciar uma nova aventura apagará seu progresso. Confirmar?")
    - On confirm: UDFSaveGame::Reset() → TravelToNexus(FirstLaunch)

    WBP_MenuButton "Opções":
    - On click: show WBP_OptionsScreen (Prompt 45) as overlay

    WBP_MenuButton "Conquistas":
    - On click: show WBP_AchievementList (Prompt 61) as overlay
    - Only visible if SaveGame exists

    WBP_MenuButton "Créditos":
    - On click: show WBP_Credits

    WBP_MenuButton "Sair":
    - On click: WBP_ConfirmDialog → UKismetSystemLibrary::QuitGame

- Bottom-left: UTextBlock* VersionText ← "v0.1.0 | UE 5.4"
- Bottom-right: UTextBlock* CopyrightText

WBP_MenuButton (custom reusable button):
- UButton* Button (transparent, hitbox only)
- UTextBlock* ButtonLabel
- UTextBlock* SubLabel (optional small text below)
- UWidgetAnimation* HoverAnim (shift right 8px + gold glow on hover)
- UWidgetAnimation* PressAnim (scale 0.97 on press)
- Sound: hover SFX (soft parchment rustle), click SFX (seal stamp)

─── WBP_Credits extends UUserWidget ───────────────────────────
Scrolling credits screen:

Layout:
- Full-screen dark background (semi-transparent, dungeon visible behind)
- UScrollBox* CreditsScroll:
  Auto-scrolls upward at 60px/s (can be sped up with input)
  Sections (each with header + content):

  "DungeonForged"
  → Studio Name
  → "Desenvolvido com Unreal Engine 5.4"

  "Design & Programação" → names
  "Arte & VFX" → names
  "Áudio & Música" → names
  "Agradecimentos Especiais" → names + "Epic Games pela Unreal Engine"

  "Tecnologias"
  → "Gameplay Ability System (GAS)"
  → "Enhanced Input System"
  → "Procedural Content Generation (PCG)"
  → "MetaSounds"
  → "Lumen & Nanite"

  End card: DungeonForged logo + "Obrigado por jogar"

- UButton* BackButton (top-left): return to WBP_MainMenu
- UButton* SkipButton (bottom-right): jump to end of credits

─── WBP_ConfirmDialog extends UUserWidget ─────────────────────
Reusable modal confirmation dialog:
- UImage* DarkOverlay (full screen, blocks input beneath)
- UTextBlock* TitleText
- UTextBlock* BodyText
- UButton* ConfirmButton ← "Confirmar" (calls OnConfirm delegate)
- UButton* CancelButton  ← "Cancelar" (closes dialog)
- Animate: scale from 0.8→1.0 with ease-out on open

Functions:
- ShowDialog(FText Title, FText Body, TFunction<void()> OnConfirm):
  → Set texts, play open animation
  → Store OnConfirm lambda
- OnConfirmClicked: call OnConfirm lambda, close dialog
- OnCancelClicked: play close animation, RemoveFromParent

─── UDFSaveGame extensions ────────────────────────────────────
Add to UDFSaveGame (Prompt 13):

- bool bIsFirstLaunch = true          ← cleared after first save
- bool bHasActiveRun = false          ← true if run started but not finished
- FName LastRunClass                  ← for slot preview sub-text
- int32 LastRunFloor                  ← for slot preview sub-text
- FDateTime LastPlayedDate
- FString GameVersion = "0.1.0"       ← for save compatibility checks
- int32 SlotIndex                     ← 0, 1 or 2 — which slot this belongs to

Functions:
- Reset(): clear all run data, keep achievements + unlocks + meta progress
  (roguelike: you never lose permanent progress, only the current run)
- IsCompatible(): check GameVersion vs current (warn if mismatch)

─── UDFSaveSlotManagerSubsystem extends UGameInstanceSubsystem ─

Manages 3 independent save slots. Each slot is a separate UDFSaveGame file.

Properties:
- static const int32 MaxSlots = 3
- int32 ActiveSlotIndex = -1          ← -1 = no slot selected yet
- TArray<UDFSaveGame*> LoadedSlots    ← cached slot data (loaded on menu open)
- TArray<FString> SlotNames           ← "DungeonForged_Slot0/1/2"

Functions:
- Initialize:
  → LoadAllSlotHeaders() — load minimal data from all 3 slots for display
- LoadAllSlotHeaders():
  → For each slot index 0-2:
    UGameplayStatics::DoesSaveGameExist(SlotNames[i])
    If exists: UGameplayStatics::LoadGameFromSlot → cache in LoadedSlots[i]
    Else: LoadedSlots[i] = nullptr (empty slot)
- GetSlotData(int32 SlotIndex): UDFSaveGame* (nullptr if empty)
- SelectSlot(int32 SlotIndex):
  → ActiveSlotIndex = SlotIndex
  → If slot empty: create new UDFSaveGame for this slot
  → Store as active save in UDFRunManager
- SaveActiveSlot():
  → UGameplayStatics::SaveGameToSlot(ActiveSave, SlotNames[ActiveSlotIndex])
- DeleteSlot(int32 SlotIndex):
  → UGameplayStatics::DeleteGameInSlot(SlotNames[SlotIndex])
  → LoadedSlots[SlotIndex] = nullptr
  → Broadcast OnSlotDeleted(SlotIndex)
- IsSlotEmpty(int32 SlotIndex): LoadedSlots[SlotIndex] == nullptr
- GetActiveSlot(): UDFSaveGame*

─── WBP_SaveSlotSelection extends UUserWidget ─────────────────
Shown BEFORE the main menu buttons — player must select a slot first.
OR shown when clicking "Nova Aventura" / "Continuar" from main menu.

Two display modes (ESlotScreenMode):
- SelectToPlay  ← pick which slot to play (from main menu)
- SelectToDelete ← management mode (long press or dedicated button)

Layout:
- UImage* BackgroundPanel (dark overlay, dungeon art behind)
- UTextBlock* Title ← "Selecionar Perfil" or "Gerenciar Perfis"
- UHorizontalBox* SlotRow: 3x WBP_SaveSlotCard side by side
- UButton* BackButton ← return to main menu without selecting

─── WBP_SaveSlotCard (one per slot index 0-2) ─────────────────

Two visual states controlled by RefreshSlotData():

━━━ OCCUPIED SLOT ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Widgets:
- UImage* ClassPortraitArt         ← class-specific portrait art
- UTextBlock* SlotLabel            ← "Perfil 1" / "Perfil 2" / "Perfil 3"
- UTextBlock* ClassNameText        ← "Guerreiro" (localized class name)
- UTextBlock* MetaLevelText        ← "Nexus Nv. 7"
- UProgressBar* MetaXPBar          ← MetaXP / NextLevelThreshold (0..1)
- UTextBlock* LastFloorText        ← "Último andar: 6" OR "☆ Vitória!" (gold)
- UTextBlock* TotalRunsText        ← "23 runs · 8 vitórias"
- UTextBlock* PlayTimeText         ← "47h 23min jogadas"
- UTextBlock* LastPlayedText       ← "Jogado há 2 dias" (relative time)
- UWrapBox* UnlockedClassIcons     ← small 24x24 icons per unlocked class
- UButton* PlayButton              ← "▶ Jogar"
- UButton* NewRunButton            ← "＋ Nova Run" (smaller, secondary)
- UButton* DeleteButton            ← 🗑️ icon (bottom-right corner, small)
- UImage* ActiveRunBadge           ← "Run em andamento" badge (if bHasActiveRun)

━━━ EMPTY SLOT ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Widgets:
- UImage* EmptySlotArt             ← dim dungeon silhouette art
- UTextBlock* SlotLabel            ← "Perfil 1"
- UTextBlock* EmptyText            ← "Slot Vazio"
- UTextBlock* HintText             ← "Clique para criar um novo perfil"
- UButton* CreateButton            ← "＋ Criar Perfil"
- (all occupied widgets hidden via SetVisibility(Collapsed))

─── How card populates data ───────────────────────────────────

Function RefreshSlotData(int32 InSlotIndex):
  Called on: NativeConstruct + OnSlotChanged delegate + after delete

  1. SlotIndex = InSlotIndex
  2. SaveData = UDFSaveSlotManagerSubsystem::GetSlotData(SlotIndex)

  3. If SaveData == nullptr (empty slot):
     → SetVisibility(Collapsed) all occupied widgets
     → SetVisibility(Visible) all empty widgets
     → return

  4. If SaveData valid (occupied slot):
     → SetVisibility(Visible) all occupied widgets
     → SetVisibility(Collapsed) all empty widgets

     Populate each field:
     ClassPortraitArt:
       → UDFClassSelectionSubsystem::GetClassData(SaveData->LastRunClass)
       → Set portrait texture from FDFClassTableRow.ClassPortrait

     SlotLabel:        "Perfil " + FString::FromInt(SlotIndex + 1)
     ClassNameText:    GetClassData(LastRunClass).ClassName (localized FText)
     MetaLevelText:    "Nexus Nv. " + FString::FromInt(SaveData->MetaLevel)
     MetaXPBar:        SaveData->MetaXP / GetNextLevelXP(SaveData->MetaLevel)

     LastFloorText:
       → If SaveData->TotalRunsWon > 0 AND SaveData->bLastRunWasVictory:
         "☆ Vitória no Andar " + SaveData->LastRunFloor (gold color)
       → Else if SaveData->bHasActiveRun:
         "Run ativa: Andar " + SaveData->LastRunFloor (white)
       → Else:
         "Melhor andar: " + SaveData->BestFloor (grey)

     TotalRunsText:
       → FString::Printf("%d runs · %d vitórias",
           SaveData->TotalRunsCompleted, SaveData->TotalRunsWon)

     PlayTimeText:
       → Convert SaveData->TotalPlayTimeSeconds to hours/minutes:
         int32 Hours = TotalSeconds / 3600
         int32 Minutes = (TotalSeconds % 3600) / 60
         → FString::Printf("%dh %dmin jogadas", Hours, Minutes)

     LastPlayedText:
       → FTimespan Delta = FDateTime::Now() - SaveData->LastPlayedDate
       → If Delta.GetDays() == 0 && Delta.GetHours() < 1:
           "Jogado há " + Delta.GetMinutes() + " minutos"
       → Else if Delta.GetDays() == 0:
           "Jogado há " + Delta.GetHours() + " horas"
       → Else if Delta.GetDays() == 1:
           "Jogado ontem"
       → Else:
           "Jogado há " + Delta.GetDays() + " dias"

     UnlockedClassIcons:
       → Clear WrapBox children
       → For each FName in SaveData->UnlockedClasses:
           Spawn UImage (24x24) with class icon texture
           AddChild to WrapBox

     ActiveRunBadge:
       → SetVisibility(SaveData->bHasActiveRun ? Visible : Collapsed)

─── Button Logic per occupied slot ────────────────────────────

▶ PlayButton ("Jogar") — continues last session:
  OnClicked:
  1. UDFSaveSlotManagerSubsystem::SelectSlot(SlotIndex)
     → Loads full SaveGame for this slot as active save
  2. If SaveData->bHasActiveRun:
     → No confirmation needed — resumes directly
     → UDFWorldTransitionSubsystem::TravelToNexus(ETravelReason::FirstLaunch)
     → Nexus loads with existing run state from SaveGame
  3. If !bHasActiveRun:
     → Same as NewRunButton flow below (no run to resume)
     → Auto-redirect to class selection

＋ NewRunButton ("Nova Run") — starts fresh run on this slot:
  OnClicked:
  1. If SaveData->bHasActiveRun:
     → Show WBP_ConfirmDialog:
       Title: "Abandonar Run Atual?"
       Body: "Você está no Andar {LastRunFloor} com {ClassName}.
              Iniciar uma nova run encerrará esta run.
              Seu progresso permanente (Nexus, conquistas) será mantido."
       [Confirmar] [Cancelar]
     → On cancel: return (do nothing)
  2. UDFSaveSlotManagerSubsystem::SelectSlot(SlotIndex)
  3. ActiveSave->bHasActiveRun = false
     ActiveSave->LastRunClass = NAME_None
     UDFSaveSlotManagerSubsystem::SaveActiveSlot()
  4. Open WBP_ClassSelection (Prompt 72) as overlay (ZOrder=20)
     → WBP_SaveSlotSelection remains behind
  5. WBP_ClassSelection::OnClassConfirmed(FName ClassName):
     → Close WBP_ClassSelection
     → UDFWorldTransitionSubsystem::TravelToRun(ClassName)

🗑️ DeleteButton — deletes this save slot:
  OnClicked:
  1. Show WBP_ConfirmDialog:
     Title: "Apagar Perfil {SlotIndex+1}?"
     Body: "Todo o progresso será perdido permanentemente.
            Nexus Nv.{MetaLevel} · {TotalRuns} runs · {TotalWins} vitórias
            Esta ação não pode ser desfeita."
     [⚠️ Apagar Permanentemente] (red button) [Cancelar]
  2. On confirm:
     → UDFSaveSlotManagerSubsystem::DeleteSlot(SlotIndex)
     → RefreshSlotData(SlotIndex) → card switches to empty state
     → Play slot-deleted SFX (paper crumple sound)
     → Brief screen flash (red vignette 0.2s)

─── Button Logic for empty slot ───────────────────────────────

＋ CreateButton ("Criar Perfil"):
  OnClicked:
  1. UDFSaveSlotManagerSubsystem::SelectSlot(SlotIndex)
     → Creates new empty UDFSaveGame for this slot
     → bIsFirstLaunch = true for this slot
  2. Open WBP_ClassSelection (Prompt 72) as overlay
     → Shows all available classes (Guerreiro + Mago unlocked by default)
     → Other classes locked until earned on THIS slot
  3. WBP_ClassSelection::OnClassConfirmed(FName ClassName):
     → Close WBP_ClassSelection
     → ActiveSave->LastRunClass = ClassName
     → UDFSaveSlotManagerSubsystem::SaveActiveSlot()
     → UDFWorldTransitionSubsystem::TravelToNexus(ETravelReason::FirstLaunch)
     → Nexus plays first-time intro sequence (Prompt 47)

─── WBP_SaveSlotSelection — full interaction flow ─────────────

NativeConstruct:
  1. UDFSaveSlotManagerSubsystem::LoadAllSlotHeaders()
  2. For each slot 0-2: SlotCards[i]->RefreshSlotData(i)
  3. Subscribe to UDFSaveSlotManagerSubsystem::OnSlotChanged delegate
     → On slot change: RefreshSlotData for that slot

OnSlotChanged(int32 SlotIndex):
  → SlotCards[SlotIndex]->RefreshSlotData(SlotIndex)
  → Play card-refresh animation (brief fade)

Screen modes:

ESlotScreenMode::SelectToPlay (default):
  - Title: "Selecionar Perfil"
  - All PlayButton + NewRunButton + CreateButton: Visible
  - DeleteButton: Visible (always accessible)
  - BackButton: "← Voltar" → close screen, return to main menu

ESlotScreenMode::ManageProfiles:
  - Title: "Gerenciar Perfis"
  - PlayButton + NewRunButton + CreateButton: Collapsed
  - DeleteButton: more prominent (larger, labeled "Apagar")
  - BackButton: "← Voltar"
  - Hint text below cards: "Selecione um perfil para apagá-lo"

─── Complete New Save Flow (step by step) ─────────────────────

Step 1 — Player clicks "Nova Aventura" on WBP_MainMenu
  → WBP_SaveSlotSelection opens (mode: SelectToPlay)
  → Shows 3 cards with current state

Step 2 — Player selects slot:
  A) Empty slot → CreateButton → go to Step 3
  B) Occupied slot → NewRunButton:
     → If has active run: ConfirmDialog → on confirm → go to Step 3
     → If no active run: go directly to Step 3

Step 3 — WBP_ClassSelection opens (over WBP_SaveSlotSelection)
  → SlotManager already has active slot set
  → Player sees classes available for THIS slot's unlock state
  → Player picks class, reviews stats/abilities
  → Clicks "⚔️ Iniciar Aventura"

Step 4 — Optional: Challenge selection
  → If WBP_ChallengeBoard was used before: active challenge shown in ClassSelection
  → ConfirmDialog if challenge active: "Iniciar com desafio: Iron Man?"

Step 5 — Confirm
  → WBP_ClassSelection::OnClassConfirmed:
     ActiveSave->LastRunClass = SelectedClass
     ActiveSave->bHasActiveRun = true
     ActiveSave->bIsFirstLaunch = false (if was true)
     UDFSaveSlotManagerSubsystem::SaveActiveSlot()
  → TravelToRun(SelectedClass) or TravelToNexus(FirstLaunch) if first time

Step 6 — Loading screen (WBP_LoadingScreen Prompt 48)
  → Shows selected class art + lore text + dungeon tip

Step 7 — Game starts
  → ADFRunGameMode or ADFNexusGameMode loads
  → Player is in game

─── Edge cases to handle ──────────────────────────────────────

Save file corruption:
  → In LoadAllSlotHeaders: wrap each load in try-catch
  → If load fails: treat slot as empty + log error
  → Show warning toast: "Perfil {N}: arquivo corrompido — dados perdidos"

Version mismatch:
  → UDFSaveGame::IsCompatible() check on load
  → If incompatible: show warning in card:
    "Salvo em versão antiga ({OldVersion})"
    PlayButton still works but shows ⚠️ icon
    Option to delete and start fresh

All 3 slots occupied, player wants a 4th:
  → Not possible — 3 is the max
  → "Gerenciar Perfis" button is highlighted to hint deletion

─── WBP_MainMenu — updated flow with slots ────────────────────
Updated button behavior:

"Continuar Aventura":
- Now reads: check if ActiveSlot has bHasActiveRun
- If no slot selected yet → show WBP_SaveSlotSelection (SelectToPlay mode)
  → Player picks slot → if slot has active run → go directly to Nexus
  → If slot is empty → redirect to "Nova Aventura" flow

"Nova Aventura":
- Show WBP_SaveSlotSelection (SelectToPlay mode)
- Player picks any slot (occupied or empty)
- If occupied: WBP_ConfirmDialog
  "Iniciar nova aventura no Perfil 1?
   Seu progresso de runs será mantido, mas a run atual será encerrada."
  (Meta-progress preserved — only active run discarded)
- On confirm: SelectSlot(Index) → open WBP_ClassSelection

"Gerenciar Perfis" (new button, below "Nova Aventura"):
- Opens WBP_SaveSlotSelection in SelectToDelete mode
- Player can view and delete slots
- No navigation to game from this mode

─── First Launch with Save Slots ──────────────────────────────
On very first launch (no slots exist):

1. Splash screens play
2. WBP_SaveSlotSelection shows (all 3 empty)
3. Player clicks any slot → "Criar Perfil" 
4. WBP_ClassSelection opens immediately (no confirm needed — slot is empty)
5. After class selected → TravelToNexus(FirstLaunch)
6. Nexus intro plays, bIsFirstLaunch = false, save to selected slot

─── First Launch experience ───────────────────────────────────
Sequence when bIsFirstLaunch == true:

1. Splash screens play (WBP_SplashScreen)
2. Main menu appears — "Continuar" button hidden
3. Player clicks "Nova Aventura"
4. Skip ConfirmDialog (no existing save to warn about)
5. Travel to Nexus (ETravelReason::FirstLaunch)
6. ADFNexusGameMode::PostLogin detects FirstLaunch:
   → Play short intro cinematic (CineSeq_NexusIntro — narrator explains the dungeon)
   → Ferreiro NPC walks toward player, initiates dialogue (Prompt 55)
   → Tutorial step "NexusArrival" triggers (Prompt 53)
7. bIsFirstLaunch = false, SaveGame

Output all .h and .cpp files with correct Public/Private paths.
```

---

## ════════════════════════════════════════
## ⚔️ PROMPT 72 — CLASS SELECTION SCREEN (COMPLETO)
## ════════════════════════════════════════

```
@ue-ui-umg-slate
@ue-gameplay-abilities
@ue-gameplay-tags
@ue-data-assets-tables
@ue-serialization-savegames
@ue-animation-system

Create the complete standalone Class Selection Screen for DungeonForged UE 5.4.
This replaces the brief WBP_ClassSelection mention in Prompt 47 with a full implementation.
Shown when player interacts with the Nexus Portal (Prompt 47) or starts a new run.
Path convention:
  .h  → Source/DungeonForged/Public/UI/ClassSelection/<FileName>.h
  .cpp → Source/DungeonForged/Private/UI/ClassSelection/<FileName>.cpp
Always write full path as comment at top of each file.

─── UDFClassSelectionSubsystem extends UWorldSubsystem ────────

Properties:
- FName SelectedClass = NAME_None
- UDataTable* ClassTable              ← DT_Classes
- UDFSaveGame* SaveRef
- TSubclassOf<ADFPlayerCharacter> PreviewPawnClass
- ADFPlayerCharacter* SpawnedPreviewPawn ← for 3D preview

Functions:
- OpenClassSelection():
  → Pause input (GameplayTag UI.ClassSelectionOpen)
  → SetGlobalTimeDilation(0.3) ← slight slow-mo while selecting
  → Create WBP_ClassSelection, add to viewport (ZOrder=15)
  → SpawnPreviewPawn() at hidden preview location
- CloseClassSelection(bool bConfirm):
  → Restore TimeDilation
  → Remove UI.ClassSelectionOpen tag
  → If bConfirm: UDFWorldTransitionSubsystem::TravelToRun(SelectedClass)
  → Else: destroy preview pawn, hide widget
- SpawnPreviewPawn():
  → Spawn ADFPlayerCharacter at PreviewSpawnPoint (off-screen dedicated location)
  → Apply idle animation, no input
  → UDFPreviewCaptureComponent renders to RenderTarget
- UpdatePreviewForClass(FName ClassName):
  → Read FDFClassTableRow → swap preview pawn mesh
  → Apply class idle animation variant
  → Update material/tint if class has cosmetic override
- GetClassData(FName ClassName): FDFClassTableRow*
- IsClassUnlocked(FName ClassName): check SaveGame->UnlockedClasses
- GetUnlockConditionText(FName ClassName): FText ← "Complete 3 runs" etc.

─── WBP_ClassSelection extends UDFUserWidgetBase ──────────────
Full-screen class selection — opens over the Nexus world.

Overall layout (3-column):
┌─────────────────────────────────────────────────────────────┐
│  [← Voltar ao Nexus]              [Iniciar Aventura →]      │
├──────────────┬──────────────────────────┬───────────────────┤
│ Class List   │   3D Preview             │  Class Details    │
│ (left panel) │   (center, render target)│  (right panel)    │
└──────────────┴──────────────────────────┴───────────────────┘

─── LEFT PANEL: Class List (280px) ────────────────────────────
- UScrollBox* ClassList (vertical)
  One WBP_ClassListEntry per class in DT_Classes:

WBP_ClassListEntry:
- UImage* ClassPortrait (64x64, class-specific art)
- UTextBlock* ClassName (bold)
- UTextBlock* ClassArchetype ← "Tanque Corpo-a-corpo", "Mago de Distância"
- TArray<UImage*> DifficultyPips ← 1-5 skull icons (class complexity)
- UImage* LockOverlay (semi-transparent if locked)
- UTextBlock* UnlockHintText ← "Complete 5 runs" (visible if locked)
- UButton* SelectButton
- Selected state: gold border + background highlight
- Locked state: grey out, portrait desaturated, lock icon
- Hover: slight scale 1.02 + name brightens

Classes list (one entry per class, in unlock order):
1. Guerreiro    🔓 Always unlocked
2. Mago         🔓 Always unlocked
3. Assassino    🔒 Unlock: complete 2 runs
4. Paladino     🔒 Unlock: win 1 run (any class)
5. Necromante   🔒 Unlock: MetaLevel >= 5

─── CENTER PANEL: 3D Preview (flex, fills remaining space) ────
- UImage* PreviewRenderTarget ← UTextureRenderTarget2D from UDFPreviewCaptureComponent
  (Shows the spawned preview pawn with selected class mesh)

Preview controls:
- Mouse drag left/right on preview → rotate preview pawn (yaw only)
  → UDFClassSelectionSubsystem::RotatePreview(float YawDelta)
  → Clamp rotation to ±180°, smooth lerp
- Scroll wheel: zoom capture camera in/out (200-600 arm length)
- Auto-rotate when idle for 3s (slow 15°/s auto-yaw)

Preview pawn behavior:
- Plays class-specific idle animation (warrior: battle stance, mage: floating orb)
- On class change: brief transition VFX (class-colored particle burst)
- Preview shows equipped cosmetic tint if any (Prompt 62)

Below preview:
- UTextBlock* SelectedClassName (large, centered, class color)
- EItemRarity-style colored border around preview (class color coding):
  Guerreiro=red, Mago=purple, Assassino=dark grey, Paladino=gold, Necromante=dark green

─── RIGHT PANEL: Class Details (340px) ────────────────────────

Section 1 — Description:
- UTextBlock* ClassTitle (large, class color)
- UTextBlock* ClassDescription (3-4 sentences of lore + playstyle)
- UTextBlock* PlaystyleTag ← pill badge: "Agressivo" / "Estratégico" / "Suporte"
- UImage* DifficultyBar ← 5 pip difficulty rating with label

Section 2 — Base Stats (stat bars, relative to max across all classes):
- UTextBlock* "Atributos Base"
- TArray<WBP_StatBar*> (one per attribute):
  Força, Inteligência, Agilidade, Defesa, Vida Máxima
  Each: label + UProgressBar (colored by attribute type) + value text
  Values sourced from FDFClassTableRow.BaseAttributeValues
  Bars normalized relative to highest value across all classes

Section 3 — Starting Abilities (horizontal scroll):
- UTextBlock* "Habilidades Iniciais"
- UScrollBox (horizontal) of WBP_AbilityPreviewIcon:
  WBP_AbilityPreviewIcon:
  - UImage* AbilityIcon (48x48)
  - UTextBlock* AbilityName (small, below)
  - On hover: show WBP_AbilityTooltip (name, description, cost, cooldown)
  Sources ability data from FDFClassTableRow.StartingAbilities → DT_Abilities

Section 4 — Meta Upgrades Preview (if Skill Tree unlocked, Prompt 51):
- UTextBlock* "Melhorias Permanentes Ativas"
- UWrapBox of UImage* icons for unlocked SkillTree nodes for this class
- If no unlocks: "Nenhuma melhoria permanente ainda. Visite o Sábio no Nexus."

Section 5 — Run History for this class (if SaveGame has data):
- UTextBlock* "Histórico com esta classe"
- UTextBlock* RunCount   ← "7 runs"
- UTextBlock* BestFloor  ← "Melhor: Andar 8"
- UTextBlock* WinCount   ← "2 vitórias"
- If no history: "Nunca jogada — seja o primeiro!"

─── BOTTOM BAR ────────────────────────────────────────────────
Left:
- UButton* BackButton ← "← Voltar ao Nexus" → CloseClassSelection(false)

Center (if challenge selected via Prompt 52):
- WBP_ActiveChallengeIndicator:
  UImage* ChallengeIcon
  UTextBlock* ChallengeName ← "Desafio: Iron Man"
  UTextBlock* ModifierSummary ← "HP inimigos +50%"
  UButton* RemoveChallenge (×)

Right:
- UButton* ConfirmButton ← "⚔️ Iniciar Aventura"
  - DISABLED if SelectedClass == NAME_None
  - DISABLED if class is locked
  - On click: show WBP_ConfirmDialog if Challenge active
    ("Iniciando com desafio: {ChallengeName}. Confirmar?")
  - On confirm: CloseClassSelection(true) → TravelToRun

─── WBP_AbilityTooltip (class selection variant) ──────────────
Shown on ability icon hover in Class Details panel:
- UImage* AbilityIcon (large, 80x80)
- UTextBlock* AbilityName (bold)
- UTextBlock* AbilityDescription
- UTextBlock* CostText ← "Custo: 30 Mana"
- UTextBlock* CooldownText ← "Recarga: 8s"
- UTextBlock* TagText ← "Fogo · Projétil"
- Spawns anchored to icon position, auto-repositions if near screen edge

─── Co-op Class Selection extension (Prompt 70) ───────────────
When bIsCoopSession == true:

WBP_ClassSelection shows additional co-op info:
- UTextBlock* CoopStatusText ← "Parceiro: {Name} — Selecionando..."
- WBP_PartnerClassPreview (small, top-right of center panel):
  Shows partner's currently hovered class in small thumbnail
  Updates in real-time via replicated GameState->Player2SelectedClass

Confirm button:
- Disabled until BOTH players have confirmed their class
- Shows "Aguardando parceiro..." if only local player confirmed
- On both confirmed: server calls TravelToRun for both simultaneously

─── UDFClassPreviewRotatorComponent extends UActorComponent ───
On preview pawn — handles rotation input from class selection:

Properties:
- float CurrentYaw = 0.f
- float TargetYaw = 0.f
- float AutoRotateSpeed = 15.f   ← degrees/sec when idle
- float IdleTimer = 0.f
- float IdleThreshold = 3.f
- bool bUserInteracting = false

Functions:
- AddYawInput(float Delta):
  → TargetYaw += Delta * 0.5
  → bUserInteracting = true
  → IdleTimer = 0.f
- TickComponent:
  → If bUserInteracting: lerp CurrentYaw → TargetYaw (10x DeltaTime)
  → IdleTimer += DeltaTime
  → If IdleTimer > IdleThreshold: TargetYaw += AutoRotateSpeed * DeltaTime
  → Apply CurrentYaw to preview pawn actor rotation

Output all .h and .cpp files with correct Public/Private paths.
```

---

## 📊 ÍNDICE COMPLETO — PARTE 3

| Prompt | Sistema | Categoria | Pasta |
|---|---|---|---|
| 49 | Paladino | 🧙 Nova Classe | `GAS/Abilities/Paladin/` |
| 50 | Necromante | 🧙 Nova Classe | `GAS/Abilities/Necromancer/` |
| 51 | Skill Tree Visual | 🌳 Progressão | `UI/SkillTree/` |
| 52 | Challenge & Daily Runs | 🌳 Progressão | `Challenge/` |
| 53 | Tutorial & Onboarding | 🎓 UX | `Tutorial/` |
| 54 | Cinematics & Level Sequences | 🎬 Apresentação | `Cinematics/` |
| 55 | Dialogue System | 📖 Narrativa | `Dialogue/` |
| 56 | Gamepad & Radial Menu | 🎓 UX | `Input/Gamepad/` |
| 57 | Room Types Expansion | 🖥️ UI & QoL | `Dungeon/Rooms/` |
| 58 | Environmental Storytelling | 📖 Narrativa | `Lore/` |
| 59 | Status Visuals on Mesh | 🎨 Visual | `FX/StatusVisuals/` |
| 60 | Inventory Sorting & Filtering | 🖥️ UI & QoL | `UI/Inventory/` |
| 61 | Achievement System | 🏆 Meta | `Achievements/` |
| 62 | Character Cosmetics | 🎨 Visual | `Cosmetics/` |
| 63 | Destructible Environment | 📖 Narrativa | `Dungeon/Destructibles/` |
| 64 | Kill Streak & Combo Chain | 🏆 Meta | `Combat/KillStreak/` |
| 65 | Photo Mode | 🎬 Apresentação | `PhotoMode/` |
| 66 | Leaderboard & Statistics | 🏆 Meta | `Statistics/` |
| 67 | PCG Room Content Expansion | 📖 Narrativa | `Dungeon/PCG/` |
| 68 | Dynamic Lighting System | 🎨 Visual | `FX/Lighting/` |
| 69 | Replay System | 🎬 Apresentação | `Replay/` |
| 70 | Multiplayer Co-op Extension | 🏆 Meta | `Coop/` |
| **71** | **Main Menu & Splash Screen** | **🏠 Menu** | `GameModes/MainMenu/` |
| **72** | **Class Selection (Completo)** | **🏠 Menu** | `UI/ClassSelection/` |

**Ordem recomendada — Parte 3 completa:**
```
71 → 72 → 49 → 50 → 51 → 52 → 53 → 55 → 58 → 67
→ 63 → 57 → 54 → 68 → 59 → 56 → 60 → 61 → 62
→ 64 → 66 → 65 → 69 → 70
```

**Dependências críticas:**
> ⚠️ **Prompt 71** (Main Menu) deve ser o **primeiro** da Parte 3 — é a porta de entrada do jogo.
> ⚠️ **Prompt 72** (Class Selection) depende do **Prompt 47** (Nexus Portal) e **Prompt 51** (Skill Tree).
> ⚠️ Classes **49, 50** dependem do **Prompt 5** (GAS Base Ability) — execute antes.
> ⚠️ Room Types **57** dependem dos **Prompts 9 e 24** (DungeonManager e Interaction).
> ⚠️ Co-op **70** depende dos **Prompts 43 e 46-48** (Network Audit e GameModes).
> ⚠️ Sempre execute o **Prompt 0** (arquivo Parte 1) antes de qualquer prompt deste arquivo.

---

## 🗺️ VISÃO GERAL COMPLETA DO PROJETO

| Arquivo | Prompts | Sistemas |
|---|---|---|
| **CURSOR_PROMPTS.md** (Parte 1) | 0–30 | Core: GAS, Input, Camera, Combat, Movement, AI, Dungeon, Loot, UI, Abilities |
| **CURSOR_PROMPTS_PART2.md** (Parte 2) | 31–48 | GameModes, XP, Shop, Traps, Audio, Equipment, FX, Minimap, Elemental, Events, Debug, Network, Performance, L10n |
| **CURSOR_PROMPTS_PART3.md** (Parte 3) | 49–72 | Classes, SkillTree, Challenges, Tutorial, Cinematics, Dialogue, Lore, Cosmetics, Achievements, Co-op, MainMenu, ClassSelection |

**Total: 73 prompts (0–72) cobrindo o jogo completo.**

---

*DungeonForged — Cursor Prompts PARTE 3 | v2.0 | Prompts 49–72 | 5 Classes · Progressão · Narrativa · Polish · Meta · Co-op · Main Menu · Class Selection*