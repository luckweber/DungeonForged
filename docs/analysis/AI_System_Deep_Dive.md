# DungeonForged — Análise Profunda do Sistema de AI de Inimigos

> **Data:** 2026-05-20
> **Escopo:** auditoria arquitetural completa do sistema de AI (regular enemies + boss) e seu wiring com todos os subsistemas de combate existentes.
> **Audiência:** o desenvolvedor solo (você).
> **Premissa:** complementa [`Combat_Advanced_Report.md`](Combat_Advanced_Report.md), [`Critical_Point_Silent_Death.md`](Critical_Point_Silent_Death.md) e [`Game_Analysis.md`](Game_Analysis.md).
> **Densidade:** cada afirmação tem `arquivo:linha`. ~6 horas de leitura forense compactada aqui.

---

## 0. TL;DR

A AI do DungeonForged tem uma **fundação técnica sólida** (controller + perception + BT + GAS + Combat Director) mas vive em **isolamento sistêmico**: cada inimigo é independente, surdo aos eventos de combate ao redor, e o BT é genérico para 10 archetypes diferentes.

> **A descoberta arquitetural mais grave: o `EDFEnemyArchetype` tem 10 variantes (Grunt, Tank, Skirmisher, Caster, Berserker, Healer, Spawner, Shielder, Sniper, Bomber), mas todos compartilham o MESMO Behavior Tree. Tank não tanqueia, Healer não cura, Sniper não kita.**

> **A descoberta de integração mais grave: quando um inimigo é staggerado, ele NÃO libera o attack token do Combat Director. Com 2 tokens disponíveis e 2 inimigos staggerados simultaneamente, ZERO inimigos podem atacar pelos próximos 4.5s — combate trivializa.**

Hoje a AI: ✅ vê, ✅ persegue, ✅ ataca, ✅ tem priority no director, ✅ usa motion warping. Mas: ❌ não reage a parry, ❌ não troca alvo em heavy hit, ❌ não coordena telegraphs, ❌ não recua para curar, ❌ não tem squad role, ❌ boss não tem janela de interrupt.

**Fix de impacto máximo: liberar token na stagger + ramificar BT por archetype + listen-on-parry.** ~12-20h para virar combate de "1v1 × N" para "1vN coreografado".

### Status de implementação (2026-05-18)

| Patch | Descrição | C++ | Editor |
|-------|-----------|-----|--------|
| 1 | Release token on stagger | ✅ `UDFStaggerComponent` | — |
| 2 | Parry → Recover | ✅ `UDFBTService_CombatEventListener` | ⚠️ BB + branch BT |
| 3 | Aggro switch heavy hit | ✅ `UDFHitReactionComponent` | — |
| 4 | Music room clear | ✅ `NotifyRoomCleared` → `OnRoomCombatCleared` | — |
| 5 | Telegraph cap | ✅ Awareness + Coordinator + Melee gate | ⚠️ Service no BT |
| 6 | Subtrees por archetype | ✅ `UDFBTDecorator_IsArchetype` | ⚠️ `BT_Sub_*` assets |
| 7 | Flee return | ✅ `UDFBTService_CheckHealth` | — |
| 8 | Boss interrupt | ✅ `UANS_DFInterruptibleCast` + Shield Bash | ⚠️ Notify no montage boss |

Setup editor: [`docs/improvements/12_AIBlueprintSetup.md`](../improvements/12_AIBlueprintSetup.md)

---

## 1. Visão arquitetural — as 3 camadas

```
┌──────────────────────────────────────────────────────────────────────┐
│  CAMADA 1 — Controller + Perception (ADFAIController)                │
│  ─ Sight 2500cm @ 70° peripheral, Hearing 2000cm                    │
│  ─ Hard-swap target acquisition                                       │
│  ─ Team ID = 2 (player = 1)                                          │
└────────────────────────────┬─────────────────────────────────────────┘
                             ▼
┌──────────────────────────────────────────────────────────────────────┐
│  CAMADA 2 — Behavior Tree + Blackboard                                │
│  ─ Single BT per enemy (from FDFEnemyTableRow::AIBehaviorTree)        │
│  ─ 7 blackboard keys (TargetActor, bIsDead, CombatState, etc.)        │
│  ─ State machine: Idle → Patrol → Chase → Attack → Flee (one-way)     │
└────────────────────────────┬─────────────────────────────────────────┘
                             ▼
┌──────────────────────────────────────────────────────────────────────┐
│  CAMADA 3 — Tasks + Decorators + Services                             │
│  ─ Tasks: Melee, Ranged, FindPatrol, Flee, Taunt, Die                 │
│  ─ Decorators: HasGASTag, IsInRange                                   │
│  ─ Services: CheckHealth (0.5s), UpdateTarget (0.2s)                  │
└──────────────────────────────────────────────────────────────────────┘
```

**Dispatch para subsistemas externos (ver §3 para o wiring completo):**

```
                     ┌──────────────────────────┐
                     │      AI ATIVA            │
                     └──────┬───────────────────┘
                            │
   ┌────────────┬──────────┼──────────────┬──────────────┐
   ▼            ▼          ▼              ▼              ▼
┌────────┐ ┌────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────┐
│Combat  │ │Melee   │ │Stagger   │ │HitReac-  │ │Music         │
│Director│ │Aim     │ │Component │ │tion      │ │(Elite)       │
└────┬───┘ └────┬───┘ └─────┬────┘ └────┬─────┘ └──────┬───────┘
     │          │           │           │              │
     ▼          ▼           ▼           ▼              ▼
  Token       Snap        Sliding    Direction-     Multicast
  request     yaw +       window      al montage    NotifyElite
  by priority warp        poise       + impulse     Engaged
```

---

## 2. Audit por sistema (arquitetura interna)

### 2.1 AI Controller — [`ADFAIController`](../../Source/DungeonForged/Public/AI/ADFAIController.h)

**Possession & spawn:**
- `OnPossess()` ([cpp:76-103](../../Source/DungeonForged/Private/AI/ADFAIController.cpp)) registra callbacks de perception, inicializa blackboard (`bCanSeeTarget=false`, `bIsDead=false`, `CombatState=Patrol`), e spawna BT (se não delayed).
- `OnUnPossess()` ([cpp:105-115](../../Source/DungeonForged/Private/AI/ADFAIController.cpp)) limpa delegates — bom.

**Perception** ([cpp:45-74](../../Source/DungeonForged/Private/AI/ADFAIController.cpp)):

| Stimulus | Config | Avaliação |
|----------|--------|-----------|
| Sight | Range 2500cm, Lose 3000cm, Peripheral 70°, Auto-success 500cm | ✅ Razoável para dungeon indoor |
| Hearing | Range 2000cm | ⚠️ Não há setup de damage stimuli — não escuta ataques |
| Sense | Detecta `Enemies & Neutrals` apenas | ⚠️ Filtro estreito; pode falhar com sub-classes |

**Target acquisition** ([cpp:141-162](../../Source/DungeonForged/Private/AI/ADFAIController.cpp)):
- **Hard swap** — substitui target ao primeiro sense.
- Filtra mortos via `IsDFPerceptionTargetAlive()` (verifica tag `State_Dead` + Health > 0).
- Restringe a `IsPlayerControlled()` apenas — não considera outros inimigos como alvo (PvE pure).

**Team Assignment** ([ADFEnemyBase.h:248-249](../../Source/DungeonForged/Public/Characters/ADFEnemyBase.h)):
- `DefaultEnemyTeamId = 2` fixo, player = 1.
- **Sem subteams** — não há "fation A vs fation B" se você quiser que inimigos lutem entre si em futuras features.

**⚠️ Gaps:**
- Sem listener de `OnDamageTaken → MakeNoise()` — inimigos não escutam pancadas de aliados.
- Sem aging/timeout de target — só esquece via perception forget event (depende de Lose-Sight).
- Sem hierarquia de prioridade (tank tomando dano vs caster baixo HP).

---

### 2.2 Blackboard & State Machine — [`DFAIKeys.h`](../../Source/DungeonForged/Public/AI/DFAIKeys.h)

**7 chaves declaradas** ([lines 14-20](../../Source/DungeonForged/Public/AI/DFAIKeys.h)):

| Key | Type | Default | Writers | Readers | Status |
|-----|------|---------|---------|---------|--------|
| `TargetActor` | Object | nullptr | Perception, UpdateTarget | Decorators, MoveTo | ✅ |
| `TargetLocation` | Vector | (0,0,0) | Perception, FindPatrol | MoveTo | ✅ |
| `bCanSeeTarget` | Bool | false | Perception, UpdateTarget | Attack gating | ✅ |
| `bIsInAttackRange` | Bool | false | UpdateTarget | Decorators | ✅ |
| `bIsDead` | Bool | false | CheckHealth, Die | Root selector | ✅ |
| `PatrolIndex` | Int | 0 | FindPatrol (cycling) | Self | ✅ |
| `CombatState` | Enum | Patrol | SetCombatState, CheckHealth | BT selectors | ✅ |

**Estado de combate** ([DFAIKeys.h:23-31](../../Source/DungeonForged/Public/AI/DFAIKeys.h)):
```cpp
enum EADFAICombatState { Idle, Patrol, Chase, Attack, Flee }
```

**Transições atuais:**

```
                    Patrol ◄────┐
                       │        │
                       │ sense  │ forget target
                       ▼        │
                    Chase ◄─────┴───── (nunca volta para Chase!)
                       │
                       │ in range
                       ▼
                    Attack
                       │
                       │ HP < 20%
                       ▼
                    Flee  ──────► (caminho sem volta)
```

**⚠️ Gap crítico:** **Flee é one-way.** Não há transição de volta para Chase após HP regenerar ou inimigo escapar. O inimigo foge **para sempre** ou até morrer. Se sua intenção é Souls-style retreat-to-heal, falta a perna de retorno.

---

### 2.3 BT Services

**`UDFBTService_CheckHealth`** ([cpp:14](../../Source/DungeonForged/Private/AI/UDFBTService_CheckHealth.cpp)):
- Tick a cada **0.5s**.
- Lê `Health`/`MaxHealth` do AttributeSet via GAS.
- `FleeHealthFraction = 0.2f` (configurável).
- Output: `bIsDead=true` se morto, ou `CombatState=Flee` se ratio < threshold.
- **⚠️ Gap:** sem hooks para auto-heal — inimigos com Healer archetype não têm caminho para "heal up + voltar".

**`UDFBTService_UpdateTarget`** ([cpp:46](../../Source/DungeonForged/Private/AI/UDFBTService_UpdateTarget.cpp)):
- Tick a cada **0.2s**.
- Busca nearest alive player dentro de `SearchRadius=2000cm`.
- **Hard swap** sem stickiness — recalcula a cada tick.
- Opção `bUseLineOfSight` para trace com elevação 50cm.
- **⚠️ Gap:** sem "target stickiness" — alterna entre 2 players em coop com distância similar, criando AI "indecisa".

---

### 2.4 BT Decorators

**`UDFBTDecorator_HasGASTag`** ([h:1-23](../../Source/DungeonForged/Public/AI/UDFBTDecorator_HasGASTag.h)):
- Configurável: `FGameplayTag RequiredTag` (com meta `Categories=Ability`).
- Usado para gatear nodes (bloquear attack se `State.Stunned`, exigir `State.Ready`).
- ✅ Bem desenhado, mas **subutilizado** — não há decoradores para `State.Combat.Telegraph.Active` (evitar telegraph simultâneo) ou `State.Combat.ParryWindow.Open` (reagir a parry).

**`UDFBTDecorator_IsInRange`** ([h:1-22](../../Source/DungeonForged/Public/AI/UDFBTDecorator_IsInRange.h)):
- Configurável: `float Range = 500cm`.
- Lê BB `TargetActor` + self location.
- ✅ Simples e funcional.

---

### 2.5 BT Tasks

| Task | Função | Pontos fortes | Pontos fracos |
|------|--------|--------------|---------------|
| **`Die`** ([h:1-17](../../Source/DungeonForged/Public/AI/UDFBTTask_Die.h)) | Para movement, clear focus, set `bIsDead=true` | Idempotente | Não dispara death GA diretamente (delega para `ADFEnemyBase`) |
| **`FindPatrolPoint`** ([cpp:28](../../Source/DungeonForged/Private/AI/UDFBTTask_FindPatrolPoint.cpp)) | Cycling round-robin via `PatrolIndex` | Simples | Anti-clumping zero; inimigos sincronizam waypoint 0→1→2 |
| **`FleeFromPlayer`** ([cpp:47-78](../../Source/DungeonForged/Private/AI/UDFBTTask_FleeFromPlayer.cpp)) | MoveTo direção oposta, `FleeSampleDistance=800cm`, timeout 12s | Nav-projected back para validar path | Sem return logic; sem busca de cover real |
| **`MeleeAttack`** ([h:1-34](../../Source/DungeonForged/Public/AI/UDFBTTask_MeleeAttack.h)) | `RequiredTag=Ability.Attack.Melee`, ativa via `TryActivateAbilitiesByTag`, espera `OnAbilityEnded` | Timeout 3s previne hang | Sem chamada explícita a `MeleeAim` (delegada à ability) |
| **`RangedAttack`** ([cpp:9](../../Source/DungeonForged/Private/AI/UDFBTTask_RangedAttack.cpp)) | Inherits MeleeAttack + `SetFocus` no target | Reuso de código | Mesmo pipeline — sem leading target, sem cover |
| **`PlayTauntMontage`** ([cpp:24-32](../../Source/DungeonForged/Private/AI/UDFBTTask_PlayTauntMontage.cpp)) | Random pick em `TauntMontages[]`, montage play direto | Sem ability overhead | Sem gating (pode tauntar staggered) |

**⚠️ Gap geral em tasks:**
- **Sem Dodge BT Task** — inimigos não esquivam ataques do player.
- **Sem Block BT Task** — não bloqueiam direcionalmente.
- **Sem Parry BT Task** — não tentam parry do player.
- **Sem KiteFromPlayer task** — caster/sniper não mantêm distância.
- **Sem RecoverPosition task** — knockback joga inimigo, ele continua atacando do chão.

---

### 2.6 Behavior Tree composition — **a lacuna mais grave**

**Estado atual:**
- 1 BT por inimigo, definida no `FDFEnemyTableRow::AIBehaviorTree`.
- BT typical composition (do comentário em `DFAIKeys.h:33-50`):

```
Root → Selector
  ├ [bIsDead==true] → Die
  ├ Sequence (Chase + Attack)
  │   [Decorator: TargetActor IsSet]
  │   ├ Service: UpdateTarget (0.2s)
  │   ├ Service: CheckHealth (0.5s)
  │   └ Selector
  │       ├ [IsInRange: MeleeRange] → MeleeAttack
  │       ├ [IsInRange: RangedRange] → RangedAttack
  │       └ MoveTo(TargetActor)
  └ Sequence (Patrol)
      ├ FindPatrolPoint
      └ MoveTo(TargetLocation)
```

**`EDFEnemyArchetype`** ([DFDataTableStructs.h:64-76](../../Source/DungeonForged/Public/Data/DFDataTableStructs.h)) define:
- Grunt, Tank, Skirmisher, Caster, Berserker, Healer, Spawner, Shielder, Sniper, Bomber.

**🔴 CRÍTICO:** o archetype é armazenado em `ADFEnemyBase::CachedEnemyArchetype` mas **NÃO dispatcha BTs diferentes**. Sniper roda o mesmo BT que Tank. Healer não cura. Shielder não bloqueia.

---

### 2.7 Boss-specific AI — [`ADFBossBase`](../../Source/DungeonForged/Public/Boss/ADFBossBase.h)

**Adições sobre `ADFEnemyBase`:**
- `CurrentPhase` ([h:45](../../Source/DungeonForged/Public/Boss/ADFBossBase.h)), `MaxPhases`, `PhaseThresholds` (e.g. 0.6, 0.3).
- `TryAdvancePhaseFromHealth` ([cpp:84-100](../../Source/DungeonForged/Private/Boss/ADFBossBase.cpp)) → `TriggerPhaseTransition` ([cpp:102-152](../../Source/DungeonForged/Private/Boss/ADFBossBase.cpp)):
  1. PhaseSlamAbility (ou stun GE fallback)
  2. PhaseStatEffect (boost stats)
  3. Grant `PhaseAbilities[NewPhase - 2]`
  4. `Multicast_OnPhaseTransitionVFX`
  5. `BeginBossVulnerabilityWindow` (2s @ +50% dmg)
- `EnrageTimer` 120s ([h:72](../../Source/DungeonForged/Public/Boss/ADFBossBase.h)) → `OnEnrageTimerExpired` aplica `UGE_BossEnrage` (+Str, +Speed, +`CCIgnore`).
- Replication mode forçado para `Full` ([h:32](../../Source/DungeonForged/Public/Boss/ADFBossBase.h)) — clientes veem todos os debuffs.
- BT é o mesmo padrão de enemy (boss usa `RunBehaviorTree` em `ADFBossTriggerVolume::OnIntroEnd_Server` [cpp:199](../../Source/DungeonForged/Private/Boss/ADFBossTriggerVolume.cpp)).

**Boss abilities (3 + boss-only GEs):**
- `UDFBossAbility_ChargeAttack` — 800cm root motion charge, sweep 80cm, 35 dmg. **Sem vulnerability window pós-miss.**
- `UDFBossAbility_GroundSlam` — sphere 500cm, 40 dmg, delay 0.45s, multicast camera shake (inner 0 / outer 2500cm).
- `UDFBossAbility_SummonMinions` — spawna minions de sockets (hand_l, hand_r, Root). **Sem cap enforcement na ability** (depende do AI BT chamar `GetLivingMinionCount`).

**Telegraphed AOEs:**
- `ADFMeteorWarningDecal` ([h:14-49](../../Source/DungeonForged/Public/Boss/ADFMeteorWarningDecal.h)) — red decal + pulse (rate 2) + rumble SFX loop.
- `ADFMeteorImpactActor` ([h:14-69](../../Source/DungeonForged/Public/Boss/ADFMeteorImpactActor.h)) — outer 500 dmg, inner 1000 (true), 2s stun, camera shake.
- `ADFVoidOrbActor` ([h:14-61](../../Source/DungeonForged/Public/Boss/ADFVoidOrbActor.h)) — orb orbita boss (300cm radius, 0.6 rad/s), 80 true dmg + slow 2s, throttle 0.35s.

**⚠️ Boss gaps (top 5):**
1. **Sem interrupt windows** — não há `State.Casting` ou janela onde player pode CC mid-ability.
2. **Sem tells variation por fase** — fase 3 usa mesmo windup que fase 2.
3. **Health bar linear sem segmentos** — sem checkpoint visual (Hollow Knight / Doom usam bar segmentado).
4. **Sem desperate phase** — Souls bosses (Malenia) entram em modo desesperado em <10% HP. Aqui é só "+50% damage on hit".
5. **Sem environmental destruction** — Wukong/Returnal mudam arena durante luta.

---

## 3. Wiring map — integração com sistemas existentes

Esta é a parte que mais surpreende: **a infraestrutura está toda lá, mas os fios estão soltos.**

### 3.1 Combat Director — [`UDFCombatDirectorSubsystem`](../../Source/DungeonForged/Public/Combat/UDFCombatDirectorSubsystem.h)

**O que está wireado ✅:**
- `ADFEnemyBase::BeginPlay` ([cpp:398-407](../../Source/DungeonForged/Private/Characters/ADFEnemyBase.cpp)) → `Director->RegisterEnemy(this)` (authority).
- `ADFEnemyBase::EndPlay` ([cpp:426-434](../../Source/DungeonForged/Private/Characters/ADFEnemyBase.cpp)) → `UnregisterEnemy` + `ReleaseAttackToken`.
- `UDFAbility_Enemy_Melee::ActivateAbility` ([cpp:175-211](../../Source/DungeonForged/Private/GAS/Abilities/UDFAbility_Enemy_Melee.cpp)) → `RequestAttackToken(Enemy)` antes de commit. Falha = bail.
- `UDFAbility_Enemy_Melee::EndAbility` ([cpp:357-383](../../Source/DungeonForged/Private/GAS/Abilities/UDFAbility_Enemy_Melee.cpp)) → `ReleaseAttackToken` se `bHoldsAttackToken`.
- Priority per archetype ([cpp:9-19](../../Source/DungeonForged/Private/Combat/UDFCombatDirectorSubsystem.cpp)): Tank=100, Sniper=90, Caster=80, Berserker=70, Grunt=50.
- `MaxAttackTokens = 2` ([h:36](../../Source/DungeonForged/Public/Combat/UDFCombatDirectorSubsystem.h)).

**🔴 O que NÃO está wireado:**
- **Stagger NÃO libera token.** `UDFStaggerComponent::TriggerStagger` ([cpp:208-267](../../Source/DungeonForged/Private/Combat/UDFStaggerComponent.cpp)) aplica stun GE mas **não fala com o director**. Resultado: 2 inimigos com Tank archetype (priority 100) staggerados simultaneamente bloqueiam os 2 tokens por 4.5s+ de stagger cooldown. Combate trivializa.
- **Cinematic / cutscene** não libera tokens.
- **Stagger token / poise budget grupal** não existe.

### 3.2 MeleeAim — [`UDFMeleeAimComponent`](../../Source/DungeonForged/Public/Combat/UDFMeleeAimComponent.h)

**Wireado ✅:**
- Config enemy: `bConsiderPlayerLockOn=false`, `bConsiderAIBlackboard=true`, `bConsiderSoftCone=true`.
- `UDFAbility_Enemy_Melee::ActivateAbility` ([cpp:208-211](../../Source/DungeonForged/Private/GAS/Abilities/UDFAbility_Enemy_Melee.cpp)) → `Aim->AcquireAndCommitTarget()` antes do montage. Snap yaw com `SnapMaxYawPerActivation=180°`, tolerância 15°.
- `UANS_DFMeleeWarp` ([h:30-89](../../Source/DungeonForged/Public/Combat/AN/ANS_DFMeleeWarp.h)) usa mesmo `ManualTarget` para warp.
- Motion warp `bRotationOnly=true` em enemies (fixou "atacar de costas").

**✅ Esta integração é exemplar.** O fix do "enemy attacks behind" via `AcquireAndCommitTarget` + `SnapYaw` + warp é AAA-tier.

### 3.3 Stagger — [`UDFStaggerComponent`](../../Source/DungeonForged/Public/Combat/UDFStaggerComponent.h)

**Wireado ✅:**
- `HandleHealthChange` ([cpp:138-206](../../Source/DungeonForged/Private/Combat/UDFStaggerComponent.cpp)) acumula damage em `StaggerWindow=3s`.
- `TriggerStagger` ([cpp:208-267](../../Source/DungeonForged/Private/Combat/UDFStaggerComponent.cpp)) aplica `StaggerGameplayEffect` (grants `State.Stunned`) + envia `Event.Combat.Stagger.Triggered`.
- `StaggerCooldown=4.5s` previne re-stagger.

**🔴 O que NÃO está wireado:**
- **AI não tem branch "Stagger"** — BT continua tentando atacar/perseguir durante stun (ability bloqueada por tag, mas BT não sabe que está bloqueado → busy-loop tentando ativar e falhando).
- **Combat Director não libera token** (ver 3.1).
- **Stagger não cancela ability ativa** — se inimigo está no meio de um ataque e leva stagger, a ability não é canceled explicitamente (depende de tag-block, frágil).
- **Sem AI awareness de stagger cooldown** — outro inimigo do grupo não sabe "Joana está vulnerável agora, vou cobrir".

### 3.4 Hit Reaction — [`UDFHitReactionComponent`](../../Source/DungeonForged/Public/Combat/UDFHitReactionComponent.h)

**Wireado ✅:**
- `OnHitReceived` ([cpp:30-150](../../Source/DungeonForged/Private/Combat/UDFHitReactionComponent.cpp)) resolve montage direcional via `UUDFAnimInstance_Enemy` ou fallback.
- Impulse de knockback aplicado se damage > threshold.
- Stagger stun GE aplicado se damage >= `StaggerThreshold`.

**🔴 O que NÃO está wireado:**
- **AI não muda target em heavy hit.** `LastDamageAttacker` é setado ([cpp:751-759](../../Source/DungeonForged/Private/Characters/ADFEnemyBase.cpp)) mas usado APENAS para kill credit e music trigger. BT `TargetActor` não é atualizado. Resultado: tank player toma 80 dmg na cara; inimigo continua batendo no ranged DPS.
- **Knockback não dispara re-pathfind** — inimigo é jogado pelo CMC impulse, BT continua executando moveto antigo.
- **Sem visual de "alerted" pós-hit** — inimigo apenas joga animation de hit, sem reação cognitiva.

### 3.5 Enemy GAS Abilities — [`UDFAbility_Enemy_Melee`](../../Source/DungeonForged/Public/GAS/Abilities/UDFAbility_Enemy_Melee.h)

**Wireado ✅:**
- Granted via `GrantedAbilitiesByTag` ([h:245](../../Source/DungeonForged/Public/Characters/ADFEnemyBase.h)) com tag `Ability.Attack.Melee`.
- Triggered por `UDFBTTask_MeleeAttack` via `TryActivateAbilitiesByTag`.
- Pipeline: commit → request token → acquire target → play montage → hit window → release token.

**🔴 O que NÃO está wireado:**
- **Sem visibility de cooldown no blackboard** — AI não sabe quando `Ability.Cooldown_Charge` está ativa.
- **Sem global enemy cooldown** — cada inimigo decide na hora se ataca; aglutinação caótica.
- **Sem ability priority** dentro do mesmo inimigo (e.g., "se HP < 50%, prefira ranged sobre melee").

### 3.6 Animation Notify States

**Wireado ✅:**
- `UANS_DFEnemyTelegraph` ([h:30-100](../../Source/DungeonForged/Public/Combat/AN/ANS_DFEnemyTelegraph.h)) — adiciona `State.Combat.Telegraph.Active`, fire `Event.Combat.Telegraph.Begin/End`, spawn ground VFX no target.
- `UANS_DFParryWindow` ([h:27-44](../../Source/DungeonForged/Public/Combat/AN/ANS_DFParryWindow.h)) — adiciona `State.Combat.ParryWindow.Open`, fire `Event.Combat.ParryWindow.Open/Close`.
- `UANS_DFMeleeWarp` ([h:30-89](../../Source/DungeonForged/Public/Combat/AN/ANS_DFMeleeWarp.h)) — motion warp com `bRotationOnly=true`.

**🔴 O que NÃO está wireado:**
- **AI não escuta `Event.Combat.Telegraph.Begin` de outros inimigos.** Resultado: 3 inimigos podem telegrafar simultaneamente (3 anéis vermelhos no chão!), criando "noise" visual impossível de dodgear.
- **AI não escuta `Event.Combat.Parry.Triggered` do player.** Parry stuna o inimigo mas o BT continua tentando ativar a ability bloqueada pelo stun tag.
- **Telegraph não tem priority** — boss e grunt usam mesma cor/tamanho.

### 3.7 Music / Elite Trigger — [`UDFMusicManagerSubsystem`](../../Source/DungeonForged/Public/Audio/UDFMusicManagerSubsystem.h)

**Wireado ✅:**
- `ADFEnemyBase::RegisterDamageFromContext` ([cpp:750-768](../../Source/DungeonForged/Private/Characters/ADFEnemyBase.cpp)) detecta primeiro hit em Elite → `Multicast_NotifyEliteEngaged` → music state Elite.

**🔴 O que NÃO está wireado:**
- **Music não baixa em room clear.** Quando o último inimigo morre, ninguém chama `SetMusicState(Exploration)`. A música fica em Combat indefinidamente. Mata o "respiro pós-luta".
- **Sem CombatLow / CombatHigh** baseado em # de inimigos vivos próximos. Combate de 1 inimigo soa igual a combate de 5.
- **Sem Elite music timeout** — se o jogador foge e perde target, música continua em Elite.

### 3.8 Death Flow

**Wireado ✅:**
- Token release no `EndPlay`.
- `SyncDeathToBlackboardAndAI` para `Brain->StopLogic("Death")`.
- XP scaling por floor ([cpp:1286-1291](../../Source/DungeonForged/Private/Characters/ADFEnemyBase.cpp)).

**🔴 O que NÃO está wireado** (já documentado em [`Critical_Point_Silent_Death.md`](Critical_Point_Silent_Death.md)):
- `GameplayCue_EnemyDeath` é stub.
- Sem celebration de "last enemy of room".
- Sem random event subsystem notification de kill count.

### 3.9 Random Events — [`UDFRandomEventSubsystem`](../../Source/DungeonForged/Public/Events/UDFRandomEventSubsystem.h)

**🔴 Wiring inexistente:**
- Sem listener para `OnEnemyDied` para tracking de milestones.
- Sem AI hook para spawnar "special encounter" entre kills.

---

## 4. Top 10 findings críticos (ranked)

### 🔴 Tier S — Bug-Level / Quebra Combate

| # | Finding | Impacto | Fix |
|---|---------|---------|-----|
| **1** | **Stagger não libera attack token.** 2 inimigos staggerados → director bloqueado 4.5s+ | Trivializa combate com staggers consecutivos | 5 linhas em `UDFStaggerComponent::TriggerStagger` |
| **2** | **AI ignora parry trigger.** `Event.Combat.Parry.Triggered` fires mas BT é surdo | Parry sente "passivo" | 1 listener em BT service + branch |
| **3** | **Heavy hit não troca aggro.** `LastDamageAttacker` setado mas BB `TargetActor` não atualiza | Tank no coop é inútil | 3 linhas em `OnHitReceived` |

### 🟠 Tier A — Comportamento Errado

| # | Finding | Impacto | Fix |
|---|---------|---------|-----|
| **4** | **Telegraphs simultâneos sem coordenação.** Múltiplos inimigos windup ao mesmo tempo | Visual noise, impossível dodgear | BT service com tag broadcast/check |
| **5** | **Music não desce em room clear.** Última kill não notifica music manager | Mata o "respiro" pós-luta | 5 linhas em `HandleServerDeath` |
| **6** | **1 BT para 10 archetypes.** Tank, Healer, Sniper, Bomber usam mesmo BT | Combate pobre, archetypes são placebo | BT subtrees por archetype OR BT decorator switches |
| **7** | **Flee é one-way sem retorno.** Inimigo foge, não volta a Chase nem com HP cheio | Fight unwinnable após Flee | Add HealComplete → ReturnToCombat |

### 🟡 Tier B — Polish / Depth

| # | Finding | Impacto | Fix |
|---|---------|---------|-----|
| **8** | **Sem dodge/block/parry BT tasks no inimigo.** Inimigos não esquivam, não bloqueiam, não tentam parry | Combat é "stand and trade" | 3 novos BT tasks |
| **9** | **Boss sem interrupt windows.** Player não pode CC mid-cast | Boss sente robótico | Tag `State.Casting` + interruptible window |
| **10** | **Sem squad coordination.** Cada inimigo é isolado | "1v1 × N", não "1vN" | `UDFAIAwarenessSubsystem` novo |

---

## 5. AAA Target Architecture

### 5.1 Como AAA orchestra AI por archetype

```
                  ┌─────────────────────────────────┐
                  │ Group Awareness Subsystem (novo)│
                  │  - threat broadcast in radius   │
                  │  - shared blackboard "AlertLevel│
                  │  - role election (tank/dps/...)  │
                  └────────────┬────────────────────┘
                               │
        ┌──────────────────────┼──────────────────────┐
        ▼                      ▼                       ▼
   ┌──────────┐         ┌──────────┐           ┌──────────┐
   │BT_Tank   │         │BT_Caster │           │BT_Sniper │
   │ taunt    │         │ stay     │           │ kite     │
   │ block    │         │ back     │           │ headshot │
   │ frontline│         │ DoT/AoE  │           │ stealth  │
   └──────────┘         └──────────┘           └──────────┘
        │                      │                       │
        └──────────────────────┼───────────────────────┘
                               ▼
                  ┌─────────────────────────────────┐
                  │ Shared BT services             │
                  │  - UpdateTarget                 │
                  │  - CheckHealth                  │
                  │  - TelegraphCoordinator (NOVO)  │
                  │  - ParryListener (NOVO)         │
                  └─────────────────────────────────┘
```

### 5.2 Behavior Tree por archetype — exemplos

**Tank BT:**
```
Selector
├ Dead → Die
├ Stagger active → Wait (yield slot)
├ Health < 30% → BlockingRetreat (move back + raise shield)
├ Player nearby → Sequence(Taunt, ChargeIn, MeleeAttack)
└ Idle → Patrol
```

**Caster BT:**
```
Selector
├ Dead → Die
├ Player too close (<300cm) → BlinkAway
├ Casting → continue (HasGASTag State.Casting)
├ Cooldown ready → CastAoE
├ MoveTo optimal range (700cm)
```

**Healer BT:**
```
Selector
├ Dead → Die
├ Ally HP < 50% within range → HealAlly
├ Self HP < 50% → HealSelf
├ Player nearby → Backstep + RangedAttack
```

**Sniper BT:**
```
Selector
├ Dead → Die
├ Player < KiteRange (500cm) → KiteAway
├ LOS clear + InRange → AimedShot (1.5s charge)
├ No LOS → Reposition
```

### 5.3 Group coordination layer

```cpp
// Novo subsystem
class UDFAIAwarenessSubsystem : public UWorldSubsystem
{
public:
    // Broadcast à threat para inimigos no radius
    void BroadcastThreat(AActor* Source, AActor* Threat, float Radius);

    // Inimigo se inscreve para receber alerts
    void RegisterListener(ADFAIController* Controller);

    // Coordenação de telegraphs — quantos inimigos estão telegraphing?
    int32 GetTelegraphingCount(const FVector& Center, float Radius) const;

    // Eleição de roles na sala
    void ElectGroupRoles(const TArray<AActor*>& Group);
};
```

---

## 6. Migration Plan — patch por patch

Ordem otimizada por **impacto × menor esforço × menor risco**.

### 🔥 Patch 1 — Release token on stagger (Tier S #1) — 15 min ✅ C++

**Onde:** [`UDFStaggerComponent::TriggerStagger`](../../Source/DungeonForged/Private/Combat/UDFStaggerComponent.cpp)

**Diff:**
```cpp
void UDFStaggerComponent::TriggerStagger(...)
{
    // ... código existente ...

    // === PATCH: Release director attack token on stagger ===
    if (UWorld* W = GetWorld())
    {
        if (UDFCombatDirectorSubsystem* Dir = W->GetSubsystem<UDFCombatDirectorSubsystem>())
        {
            if (AActor* Owner = GetOwner())
            {
                Dir->ReleaseAttackToken(Owner);
            }
        }
    }
}
```

**Impacto:** maior single-line fix do projeto. Combate não trivializa em multi-stagger.

**Risco:** zero. `ReleaseAttackToken` é idempotente — se inimigo não tinha token, no-op.

---

### 🔥 Patch 2 — AI listens to parry trigger (Tier S #2) — 1h ✅ C++ · ⚠️ BT

**Passo 2a:** Criar novo BT service `UDFBTService_CombatEventListener`:

```cpp
UCLASS()
class UDFBTService_CombatEventListener : public UBTService
{
    GENERATED_BODY()

    virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override
    {
        // Bind: Event.Combat.Parry.Triggered, Event.Hit.Received
        if (UAbilitySystemComponent* ASC = GetOwnerASC(OwnerComp))
        {
            ASC->GenericGameplayEventCallbacks.FindOrAdd(FDFGameplayTags::Event_Combat_Parry_Triggered)
                .AddUObject(this, &UDFBTService_CombatEventListener::OnParryTriggered);
        }
    }

    void OnParryTriggered(const FGameplayEventData* Payload)
    {
        // Set BB key bWasParried = true; BT branch reacts
        // Switch CombatState → Recover (new state)
    }
};
```

**Passo 2b:** Adicionar BB key `bWasParried` (bool, default false).

**Passo 2c:** No BT, adicionar branch:
```
Selector
├ bWasParried → Sequence(WaitRecovery 1s, ClearFlag, ResumeCombat)
├ ... existing ...
```

**Impacto:** parry vira tactical decision. Inimigo recua/abre guarda após parry.

**Risco:** baixo — service só adiciona, não muda fluxo existente.

---

### 🔥 Patch 3 — Aggro switch on heavy hit (Tier S #3) — 30 min ✅ C++

**Onde:** [`UDFHitReactionComponent::OnHitReceived`](../../Source/DungeonForged/Private/Combat/UDFHitReactionComponent.cpp)

**Diff:**
```cpp
void UDFHitReactionComponent::OnHitReceived(
    AActor* Instigator, float Damage, FVector HitDirection2D, ...)
{
    // ... código existente ...

    // === PATCH: Aggro switch on heavy hit ===
    const float HeavyHitThreshold = AggroSwitchDamageThreshold;  // UPROPERTY default 40.f
    if (Instigator && Damage >= HeavyHitThreshold)
    {
        if (AAIController* AIC = Cast<AAIController>(GetOwner()->GetController()))
        {
            if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
            {
                BB->SetValueAsObject(FDFAIKeys::TargetActor, Instigator);
                BB->SetValueAsBool(FDFAIKeys::bCanSeeTarget, true);
            }
        }
    }
}
```

**Impacto:** tank no coop funciona — agressão flui para quem tanqueia.

**Risco:** baixo. Mantém AI rastreando damage attacker — comportamento esperado.

---

### 🟠 Patch 4 — Music downgrade on room clear (Tier A #5) — 30 min ✅ C++

**Onde:** [`ADFEnemyBase::HandleServerDeath`](../../Source/DungeonForged/Private/Characters/ADFEnemyBase.cpp)

**Diff:**
```cpp
void ADFEnemyBase::HandleServerDeath(AActor* Killer)
{
    // ... código existente ...

    // === PATCH: Notify music manager if last enemy in room ===
    if (UWorld* W = GetWorld())
    {
        if (UDFCombatDirectorSubsystem* Dir = W->GetSubsystem<UDFCombatDirectorSubsystem>())
        {
            // After unregister, count living enemies. If 0 → music downgrade.
            const int32 Remaining = Dir->GetRegisteredEnemyCount() - 1;  // -1 because we're not unregistered yet
            if (Remaining <= 0)
            {
                if (UDFMusicManagerSubsystem* Music = W->GetGameInstance()->GetSubsystem<UDFMusicManagerSubsystem>())
                {
                    Music->RequestMusicState(EMusicState::Exploration, 2.0f);  // 2s crossfade
                }
            }
        }
    }
}
```

**Impacto:** "respiro" pós-luta. Hades-tier feel.

**Risco:** baixo.

---

### 🟠 Patch 5 — Telegraph coordination (Tier A #4) — 2h ✅ C++ · ⚠️ BT

**Adicionar BT service `UDFBTService_TelegraphCoordinator`:**

```cpp
// No tick (a cada 0.3s):
// 1. Query AI awareness subsystem: quantos inimigos no radius X têm State.Combat.Telegraph.Active?
// 2. Se count >= MaxConcurrentTelegraphs (default 2), set BB key bCanTelegraph = false
// 3. MeleeAttack BT task checa esse key antes de ativar windup
```

E novo `UDFAIAwarenessSubsystem` (WorldSubsystem) que mantém lista de inimigos com tag de telegraph:

```cpp
TArray<TWeakObjectPtr<AActor>> CurrentlyTelegraphing;

int32 GetTelegraphingCountWithin(const FVector& Center, float Radius) const;
void OnTelegraphBegin(AActor* Enemy);
void OnTelegraphEnd(AActor* Enemy);
```

Wire em `UANS_DFEnemyTelegraph::NotifyBegin` e `NotifyEnd`:
```cpp
if (UDFAIAwarenessSubsystem* AS = GetWorld()->GetSubsystem<UDFAIAwarenessSubsystem>())
{
    AS->OnTelegraphBegin(MeshComp->GetOwner());
}
```

**Impacto:** 3 inimigos não telegrafam simultaneamente. Player consegue ler/dodgear individualmente.

**Risco:** médio — testar com encounters de 4-5 inimigos para confirmar que o cap não trava o combate.

---

### 🟡 Patch 6 — Archetype-specific BT subtrees (Tier A #6) — 8-12h ✅ decorator · ⚠️ subtrees

**Estratégia:** manter o BT principal, mas adicionar **subtrees externos** por archetype, chamados via `BTTask_RunBehaviorTree` (UE 5.4 native).

**Estrutura:**
```
Main BT (compartilhado):
  Root → Selector
    ├ [bIsDead] → Die
    ├ Sequence (combat)
    │   ├ Service: UpdateTarget
    │   ├ Service: CheckHealth
    │   ├ Service: TelegraphCoordinator  (do Patch 5)
    │   ├ Service: CombatEventListener   (do Patch 2)
    │   └ Decorator: HasArchetypeTag → RunSubBT(archetype)
    └ Sequence(Patrol)
```

Subtrees: `BT_Sub_Tank`, `BT_Sub_Caster`, `BT_Sub_Sniper`, `BT_Sub_Healer`, `BT_Sub_Default`.

Dispatch via decorator novo `UDFBTDecorator_IsArchetype`:
```cpp
UPROPERTY(EditAnywhere) EDFEnemyArchetype Archetype;
virtual bool CalculateRawConditionValue(...) const override
{
    if (ADFEnemyBase* E = Cast<ADFEnemyBase>(GetEnemyFromBB(...)))
    {
        return E->GetEnemyArchetype() == Archetype;
    }
    return false;
}
```

**Impacto:** combate de "1v1 × N" vira "1vN com táticas". O archetype enum finalmente é usado.

**Risco:** alto se feito tudo de uma vez. **Sugestão:** começar por 2 subtrees (Tank + Caster), validar feel, depois expandir.

---

### 🟡 Patch 7 — Flee return logic (Tier A #7) — 1h ✅ C++

**Onde:** [`UDFBTService_CheckHealth`](../../Source/DungeonForged/Private/AI/UDFBTService_CheckHealth.cpp)

**Diff:**
```cpp
void UDFBTService_CheckHealth::TickNode(...)
{
    // ... existing ...

    const float Ratio = Health / MaxHealth;

    if (Ratio <= 0.f && !bWasDead)
    {
        BB->SetValueAsBool(FDFAIKeys::bIsDead, true);
        bWasDead = true;
    }
    else if (Ratio < FleeHealthFraction && CurrentState != EADFAICombatState::Flee)
    {
        BB->SetValueAsEnum(FDFAIKeys::CombatState, EADFAICombatState::Flee);
    }
    // === PATCH: Return to Chase if healed back ===
    else if (Ratio > FleeReturnHealthFraction && CurrentState == EADFAICombatState::Flee)
    {
        // FleeReturnHealthFraction default 0.6f (hysteresis prevents flicker)
        BB->SetValueAsEnum(FDFAIKeys::CombatState, EADFAICombatState::Chase);
    }
}
```

**Impacto:** inimigos com Healer / mecânica de regen voltam para combate.

**Risco:** zero.

---

### 🟡 Patch 8 — Boss interrupt windows (Tier B #9) — 4-6h

**Estratégia:**
1. Adicionar tag `State.Combat.Casting.Interruptible` em windups que devem ser CC-able.
2. `UANS_DFInterruptibleCast` notify state (novo) adiciona/remove a tag.
3. CC abilities (stun, freeze) checam: se boss tem a tag, ability interrompe o cast (cancela montage, aplica stun longo, marca vulnerability window).

```cpp
// Em UDFAbility_Warrior_ShieldBash (e outras CC abilities):
void ActivateAbility(...)
{
    if (TargetASC->HasMatchingGameplayTag(FDFGameplayTags::State_Combat_Casting_Interruptible))
    {
        // Cancel boss's montage, apply longer stun, broadcast Event.Combat.BossInterrupted
        InterruptBossCast(Target);
    }
    // ... normal flow
}
```

**Impacto:** boss fight ganha "skill expression" — player pode punir cast com CC.

**Risco:** médio — tunar para que apenas cast TELEGRAFADO seja interruptible (não interromper attacks rápidos).

---

## 7. Test Plan

### 7.1 Smoke tests por patch

| Patch | Como testar | O que esperar |
|-------|-------------|---------------|
| 1 | Spawn 2 Tanks. Stagger ambos rapidamente. Tentar atacar com um 3o inimigo. | 3o consegue token e ataca durante o stagger |
| 2 | Parry um inimigo. Observar comportamento pós-stun. | Inimigo recua/wait antes de re-engajar |
| 3 | Coop: tank tanka, DPS dispara heavy hit (40+ dmg). | Inimigo vira para o DPS |
| 4 | Matar último inimigo de uma sala. | Música cai para Exploration em 2s |
| 5 | Encounter de 4 inimigos. Todos tentam atacar. | Apenas 2 telegrafam simultaneamente |
| 6 | Spawn Tank + Caster + Sniper. Observar 30s. | Tank na frente, Caster atrás castando, Sniper kitando |
| 7 | Lower HP de inimigo Healer abaixo de 20%. Esperar healing ally chegar. | Inimigo foge, é curado, retorna a Chase |
| 8 | CC boss durante cast telegrafado. | Boss é interrompido, fica vulnerable 2s |

### 7.2 Telemetria

`LogDFAI` implementado em `DungeonForgedModule.h` / `.cpp`. Playtest: `-log LogDFAI Verbose`.

### 7.3 Edge cases

1. **Inimigo morre durante stagger** — token deve liberar corretamente
2. **2 players, 1 dies** — todos os inimigos devem virar para o player vivo
3. **Boss interruptado mid-charge** — root motion deve cancelar limpamente
4. **Telegraph cap atingido + inimigo de alta priority quer telegrafar** — preempt o de menor priority
5. **Healer foge mas player segue** — healer deve poder usar habilidade de curar enquanto recua

### 7.4 Network tests

- Listen Server + Client com `Net PktLag=120`:
  - Token release via stagger propaga corretamente
  - BT state change (Flee → Chase) sincroniza
  - Telegraph awareness subsystem consistente em ambos os lados

---

## 8. Success Metrics

| Métrica | Atual | Alvo pós-patches |
|---------|-------|------------------|
| **Combat trivialização por multi-stagger** | Sim (4.5s sem ataque possível) | Não — outros inimigos cobrem |
| **Parry sente tactical** | Não — só dano boost | Sim — inimigo recua, abre janela |
| **Tank no coop é viável** | Não — aggro não troca | Sim — heavy hit puxa aggro |
| **Música acompanha estado de combate** | Não — fica em Combat indefinidamente | Sim — Exploration em room clear |
| **Telegraphs simultâneos visíveis** | 3-4 ao mesmo tempo | Max 2 (cap configurable) |
| **Archetypes têm comportamento distinto** | Não — 1 BT para 10 archetypes | Sim — 5+ subtrees autorais |
| **Healer cura aliados** | Não | Sim — BT subtree dedicado |
| **Sniper mantém distância** | Não | Sim — KiteAway task |
| **Boss tem janelas de interrupt** | Não | Sim — em casts telegrafados |
| **Flee tem volta** | Não — one-way | Sim — hysteresis 0.6f |

---

## 9. Resumo executivo

DungeonForged tem **fundação de AI sólida** (controller, perception, BT, GAS integration, Combat Director, MeleeAim, Stagger, Hit Reaction — todos individualmente bem-desenhados), mas **vive em isolamento sistêmico**:

- Os subsistemas funcionam, mas **não se escutam**.
- O `EDFEnemyArchetype` enum tem 10 variantes mas só **1 BT** controla todos.
- Stagger fires mas **não libera token** → bug-level issue que trivializa combate.
- Parry/HeavyHit/Knockback acontecem mas a AI é **surda** a eles.
- Boss é "enemy + phases + enrage" — sem **interrupt windows**, sem **tells variation**, sem **desperate phase**.

**Roadmap:**
- **Tier S (3 patches, ~2h):** fixes de bug-level. Resolve trivialização e blindness. **Maior delta de qualidade do projeto.**
- **Tier A (4 patches, ~12-15h):** comportamento por archetype, coordenação de telegraphs, music downgrade, flee com retorno. Combate vira "1vN coreografado".
- **Tier B (1 patch, ~4-6h):** boss interrupt windows. Boss vira skill-test, não DPS-race.

**Total: ~20-25 horas focadas para subir AI de 6/10 para 8.5/10.**

> Se você fizer apenas Tier S (~2h), o combate sente **bug-fix-tier melhor** — já vale. Se for até Tier A, o combate vira **AAA-tier**. Tier B é polish de boss.

---

## 10. Arquivos referenciados

### Core AI

| Arquivo | Função |
|---------|--------|
| [`AI/ADFAIController.h`](../../Source/DungeonForged/Public/AI/ADFAIController.h) + cpp | Controller, perception, possession |
| [`AI/DFAIKeys.h`](../../Source/DungeonForged/Public/AI/DFAIKeys.h) | Blackboard keys, state enum |
| [`AI/UDFBTService_CheckHealth.h`](../../Source/DungeonForged/Public/AI/UDFBTService_CheckHealth.h) | Health → flee/dead detection |
| [`AI/UDFBTService_UpdateTarget.h`](../../Source/DungeonForged/Public/AI/UDFBTService_UpdateTarget.h) | Target acquisition |
| [`AI/UDFBTDecorator_HasGASTag.h`](../../Source/DungeonForged/Public/AI/UDFBTDecorator_HasGASTag.h) | Gate por GAS tag |
| [`AI/UDFBTDecorator_IsInRange.h`](../../Source/DungeonForged/Public/AI/UDFBTDecorator_IsInRange.h) | Gate por distância |
| [`AI/UDFBTTask_*.h`](../../Source/DungeonForged/Public/AI/) | Die, FindPatrol, Flee, Melee, Ranged, Taunt |
| [`AI/UDFAIAwarenessSubsystem.h`](../../Source/DungeonForged/Public/AI/UDFAIAwarenessSubsystem.h) | Telegraph counting |
| [`AI/UDFBTService_CombatEventListener.h`](../../Source/DungeonForged/Public/AI/UDFBTService_CombatEventListener.h) | Parry → BB Recover |
| [`AI/UDFBTService_TelegraphCoordinator.h`](../../Source/DungeonForged/Public/AI/UDFBTService_TelegraphCoordinator.h) | Cap concurrent telegraphs |
| [`AI/UDFBTDecorator_IsArchetype.h`](../../Source/DungeonForged/Public/AI/UDFBTDecorator_IsArchetype.h) | Subtree dispatch |
| [`Combat/UDFCombatInterruptLibrary.h`](../../Source/DungeonForged/Public/Combat/UDFCombatInterruptLibrary.h) | Boss cast interrupt |
| [`Combat/AN/ANS_DFInterruptibleCast.h`](../../Source/DungeonForged/Public/Combat/AN/ANS_DFInterruptibleCast.h) | Interruptible window notify |

### Integration

| Arquivo | Função |
|---------|--------|
| [`Combat/UDFCombatDirectorSubsystem.h`](../../Source/DungeonForged/Public/Combat/UDFCombatDirectorSubsystem.h) | Attack token system |
| [`Combat/UDFMeleeAimComponent.h`](../../Source/DungeonForged/Public/Combat/UDFMeleeAimComponent.h) | Target resolution, snap yaw |
| [`Combat/UDFStaggerComponent.h`](../../Source/DungeonForged/Public/Combat/UDFStaggerComponent.h) | Poise window, stun trigger |
| [`Combat/UDFHitReactionComponent.h`](../../Source/DungeonForged/Public/Combat/UDFHitReactionComponent.h) | Directional reactions |
| [`GAS/Abilities/UDFAbility_Enemy_Melee.h`](../../Source/DungeonForged/Public/GAS/Abilities/UDFAbility_Enemy_Melee.h) | Enemy melee pipeline |
| [`Combat/AN/ANS_DFEnemyTelegraph.h`](../../Source/DungeonForged/Public/Combat/AN/ANS_DFEnemyTelegraph.h) | Telegraph windup |
| [`Combat/AN/ANS_DFParryWindow.h`](../../Source/DungeonForged/Public/Combat/AN/ANS_DFParryWindow.h) | Parry detection window |
| [`Combat/AN/ANS_DFMeleeWarp.h`](../../Source/DungeonForged/Public/Combat/AN/ANS_DFMeleeWarp.h) | Motion warp |
| [`Audio/UDFMusicManagerSubsystem.h`](../../Source/DungeonForged/Public/Audio/UDFMusicManagerSubsystem.h) | Music states + elite trigger |

### Boss

| Arquivo | Função |
|---------|--------|
| [`Boss/ADFBossBase.h`](../../Source/DungeonForged/Public/Boss/ADFBossBase.h) + cpp | Phase, enrage, vulnerability |
| [`Boss/ADFBossTriggerVolume.h`](../../Source/DungeonForged/Public/Boss/ADFBossTriggerVolume.h) | Encounter setup |
| [`Boss/UDFBossAbility_*.h`](../../Source/DungeonForged/Public/Boss/) | ChargeAttack, GroundSlam, SummonMinions |
| [`Boss/ADFMeteorWarningDecal.h`](../../Source/DungeonForged/Public/Boss/ADFMeteorWarningDecal.h) | Telegraphed AOE warning |
| [`Boss/ADFVoidOrbActor.h`](../../Source/DungeonForged/Public/Boss/ADFVoidOrbActor.h) | Orbiting barrier orb |

### Data

| Arquivo | Função |
|---------|--------|
| [`Data/DFDataTableStructs.h`](../../Source/DungeonForged/Public/Data/DFDataTableStructs.h) | `FDFEnemyTableRow`, `EDFEnemyArchetype` |
| [`Characters/ADFEnemyBase.h`](../../Source/DungeonForged/Public/Characters/ADFEnemyBase.h) + cpp | Enemy actor, GAS, stats, death |
