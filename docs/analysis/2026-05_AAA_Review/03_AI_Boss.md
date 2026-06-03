# 03 — IA & Bosses

> Parte da [AAA Technical Review (Maio 2026)](00_Index.md).
> Cobre: arquitetura de IA (BT + GAS), `ADFEnemyBase`, archetypes, telegraph,
> `ADFBossBase` (fases/enrage/minions), networking de IA.

---

## 1. Arquitetura de IA

Stack **clássico UE**: `ADFAIController` + Behavior Tree + Blackboard + GAS para
combate/morte. **Sem StateTree, Mass Entity ou crowd sim.** Coordenação por dois
world subsystems: `UDFCombatDirectorSubsystem` (tokens de ataque) e
`UDFAIAwarenessSubsystem` (throttle de telegraph).

### Forças
- Ownership limpo: BT/blackboard/perception no controller; pawn só dados.
- Sight tunado p/ dungeon: 2500cm raio, 70° periférico, perde a 3000cm
  (`ADFAIController.cpp:54-60`); filtra alvos mortos via `State.Dead`.
- Deferral de spawn-birth evita double BT start (`:93-101`).
- Blackboard keys curtas e documentadas (`DFAIKeys.h`).

### Gaps vs AAA
| Gap | Detalhe | Ref |
|---|---|---|
| 🔴 **Player 0 only** | perception e `UpdateTarget` filtram só pawn de Player 0 (TODO no código) | `ADFAIController.cpp:154`, `UDFBTService_UpdateTarget.cpp:66-68` |
| 🟡 **Bug de telegraph gating** | `UpdateTarget` força `bCanTelegraph=true` **toda tick** → pode anular o `TelegraphCoordinator` conforme ordem de service | `UpdateTarget.cpp:105` |
| **Sem last-known-position / investigação** | ao esquecer alvo, limpa e volta a Patrol; sem memória/busca/alerta | `:171-177` |
| **Hearing configurado mas morto** | `HearingConfig` a 2000cm, mas **zero** `MakeNoise`/`ReportNoiseEvent` no projeto | `:64-70` |
| **Dois paths de aquisição** | perception seta `TargetActor`; `UpdateTarget` re-resolve a cada 0.2s sem reconciliação | — |

### 1.1 Coordenação de grupo
`UDFCombatDirectorSubsystem` limita atacantes melee simultâneos
(`MaxAttackTokens=2`) com preempção por prioridade. **Mas** só distingue
Tank/Sniper/Caster/Berserker; Grunt/Skirmisher/Healer/Spawner/Shielder/Bomber
caem todos em prioridade 50 (`:9-18`). Sem slots de flanco, sem split melee/ranged,
sem budget de "ativos vs idle".

---

## 2. `ADFEnemyBase`

### Forças
- `InitializeFromDataTable` canônico (`:488-595`). Floor scaling
  `(1+0.15×Floor)×DifficultyMultiplier` (`:686-700`); Elite **2.5× HP, 1.5× dano**
  (`:705-709`); armor escala com `sqrt` (mais suave).
- Row dirige BT, ranges, patrol, taunts, abilities, elemental, spawn birth.
- Walk speed replicado; death flow robusto (GA death → montage → finalize →
  dissolve → destroy com timers de backup); multicast cosmético p/ proxies.

### Gaps vs AAA
| Gap | Detalhe | Ref |
|---|---|---|
| **`EEnemyTier::Boss` sem multiplicador** | só Elite ganha bônus; tuning de boss é 100% manual na DT | `:705-709` |
| 🟡 **Archetypes quase só metadata** | 11 enums, mas C++ usa só 4 (prioridade de token). Não dirigem target/flee/ability/spacing | `DFDataTableStructs.h:65-77` |
| **Sem threat table** | nearest Player 0; last-attacker só p/ XP/kill credit | `:746-768` |
| **`GrantedAbilitiesByTag` por BP** | se o BP não configurar o map, tags da row são ignoradas silenciosamente | `.h:245-246` |
| **`UDFBTTask_Die` redundante** | morte real é GAS-driven; `bIsDead` setado em vários lugares (sem fonte única) | — |

---

## 3. `ADFBossBase`

### Forças
- Fases por HP ratio (default 0.6/0.3); transição com slam ability **ou** stun GE,
  stat effect, grant de `PhaseAbilities[idx]`, multicast VFX, janela
  `State.BossVulnerable` (`:84-167`). `CurrentPhase` replicado com OnRep.
- Vulnerable = **+50% dano** na calc (`DFDamageCalculation.cpp:121-125`).
- Enrage timer (default 120s) aplica `EnrageEffect` + roar/VFX multicast.
- `ADFBossTriggerVolume` orquestra encounter: seta run phase, tranca portas,
  intro cinematic, boss HUD, deferral de BT.
- Minions: `RegisterSpawnedMinion` com weak refs + cleanup em morte do boss.

### Gaps vs AAA
| Gap | Detalhe | Ref |
|---|---|---|
| 🔴 **Telegraphs de boss não replicam** | `ADFMeteorWarningDecal` sem `bReplicates` → clients em dedicated **não veem o aviso de chão** | `MeteorStrike.cpp:78` |
| **Fase bloqueada por enrage** | `NotifyHealthChanged` retorna cedo se `bIsEnraged` → pode pular transição de fase | `:77-79` |
| **Sem BT por fase em C++** | pacing depende de abilities concedidas + decorators de designer | — |
| **Enrage hard 120s** | não atrelado a DPS/adds/fase; sem UI de countdown em C++ | `.h:72` |
| **`SummonMinions` tag errada** | tagueia-se `Ability_Ice_Blizzard` (copy-paste); cap 6 hardcoded na ability, não data-driven | `SummonMinions.cpp:22,51` |
| **Minions não limpos em phase change** | podem sobrepor pacing de fase | — |
| **Intro por timer fixo** | `IntroEndDelay` (5s) paralelo ao Level Sequence → pode desync | `ADFBossTriggerVolume.cpp:30,93` |
| **Vulnerable fixo 2s, sem callout de UI** | jogador não percebe a janela | `.h:99` |

---

## 4. Telegraph & readability

`UANS_DFEnemyTelegraph` (Anim Notify State): warning Niagara de chão, FX de charge,
SFX de windup, registro no awareness subsystem, tag `State.Combat.Telegraph.Active`.
`UDFBTTask_MeleeAttack` respeita `bCanTelegraph`.

**Gaps:**
- O **bug do §1** (`bCanTelegraph=true` toda tick) potencialmente derrota o
  coordinator — o doc em `DFAIKeys.h` lista UpdateTarget mas **não** o
  TelegraphCoordinator (provável descuido de integração).
- Telegraphs por anim-notify são **locais** a cada máquina → clients só veem se a
  montage replicar/tocar em proxies (depende da net policy da ability).
- Boss decals server-only (§3) + meteor impact actor também não-replicado.

---

## 5. Networking de IA

| Actor | ASC mode | Ref |
|---|---|---|
| Inimigo padrão | **Minimal** | `ADFEnemyBase.cpp:99-100` |
| Boss | **Full** | `ADFBossBase.cpp:29-33` |

Replicado: `bHasDied`, walk speed, display name (+ boss: phase, enraged, name).
**Não replicado:** blackboard, combat state, target, focus, last attacker, patrol.
Correto p/ IA server-authoritative, **mas** telegraphs ligados a montage exigem
contrato cosmético explícito (parcialmente coberto por `Multicast_PlayEnemyCosmeticCue`).

---

## 6. Recomendações priorizadas — IA & Boss

| # | Recomendação | Tag | Esforço |
|---|---|---|---|
| 1 | **Targeting multiplayer** — substituir scans de Player 0 por "hostil mais próximo / threat table" | 🔴 | M |
| 2 | **Replicar telegraphs de boss** — `bReplicates` nos decals OU multicast no início do telegraph (não só no impacto) | 🔴 | M |
| 3 | **Corrigir telegraph gating** — remover `bCanTelegraph=true` incondicional do `UpdateTarget`; deixar o coordinator dono da key | 🟡 | L |
| 4 | **Ativar archetypes em C++** — flee thresholds por archetype, ponteiro de BT na row, ou subtrees `IsArchetype` obrigatórios | 🟡 | M |
| 5 | **Scaling & fase de boss** — multiplicador p/ `EEnemyTier::Boss`; perfis de cooldown/BT por fase; regras enrage×fase | 🟡 | M |
| 6 | **Investigação/alerta** — last-known-position + estado de busca; propagação de alerta entre packs | 🟡 | M |
| 7 | **Wirar hearing ou remover** — emitir noise de player/abilities, ou dropar o sense | 🟢 | L |
| 8 | **Expandir Combat Director** — slots de cast ranged, prioridade de telegraph, budget de adds | 🟢 | M |
| 9 | **Boss adds com comportamento próprio** (proteger boss, explodir) + limpar em phase change | 🟢 | M |
| 10 | **UI de enrage countdown** + callout de janela vulnerable | 🟢 | L |
| 11 | **Considerar StateTree p/ bosses** + EQS p/ posicionamento de ranged/casters | 🟢 | H |
| 12 | Corrigir tag do `SummonMinions`; tornar cap data-driven | 🟢 | L |

> Arquivos-chave: `Source/DungeonForged/Public/AI/ADFAIController.h`,
> `DFAIKeys.h`, `UDFBTService_UpdateTarget.h`, `UDFBTService_TelegraphCoordinator.h`,
> `Public/Boss/ADFBossBase.h`, `ADFBossTriggerVolume.h`, `ADFMeteorWarningDecal.h`,
> `Public/Characters/ADFEnemyBase.h`,
> `Public/Combat/UDFCombatDirectorSubsystem.h`.
