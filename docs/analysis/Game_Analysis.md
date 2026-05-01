# DungeonForged — Análise de Game Design (Gameplay, Loop, Combate, Feel & Juice)

> **Data:** 2026-04-26
> **Escopo:** análise crítica baseada em leitura direta do código C++ em
> `Source/DungeonForged/` (UE 5.4). Cobre o que está **implementado**,
> não apenas o que está planejado.
> **Audiência:** designer/dev tomando decisões de polish e tuning.

---

## Sumário executivo

DungeonForged é um **ARPG roguelike third-person estilo WoW + Hades**, em UE
5.4, totalmente C++ first com Blueprint apenas como camada de assets/visual.
A engenharia está **muito acima da média** para um projeto solo: GAS completo
com 6 atributos primários, 61 GameplayEffects, 34 habilidades autorais, 5
classes (3 implementadas), 5 elementos com reações cruzadas, sistema de
juice dedicado (hit stop, camera shake, screen effects, combat text pool),
música adaptativa por tag, lock-on, dodge com i-frames, sprint com stamina,
modular paper-doll, crafting, eventos roguelike entre andares, draft 1-de-3
de habilidades, 3 slots de save, meta-progressão no Nexus.

O que **falta** é menos código e mais **tuning de números**, **variedade de
content** (mais inimigos, mais armadilhas, mais arenas) e **alguns refinos
de feel** detalhados nas seções de combate.

---

## 1. Visão geral do gênero e referências

| Eixo | Posição |
|---|---|
| **Câmera/escopo** | Third-person ARPG (sobre o ombro, ~400 cm de spring arm). |
| **Sub-gênero** | Roguelike de masmorra (10 andares por run, build descartável). |
| **Combate** | Action-RPG com hotkeys (Q/E/R/F + LMB melee + Sprint + Dodge). |
| **Meta-loop** | Hub social ("Nexus") com NPCs, upgrades permanentes e MetaXP. |
| **Drop / build** | Tabular (DT_Items + DT_Abilities), com rarity weighting (Common→Legendary). |

**Referências de design implícitas no código:**

- **Hades** — draft 1-de-3 entre andares (`UDFAbilitySelectionSubsystem`),
  hub para ouvir NPCs (`Nexus`), morte = retorna ao hub mantendo meta.
- **WoW / Diablo** — barras de Health/Mana/Stamina no HUD, status icons
  flutuantes, combat floating numbers, lock-on com ciclo Q/E.
- **Returnal / Dead Cells** — biomas/floor types, bosses com fases e
  enrage, traps elementais.
- **Dark Souls** — i-frames no dodge, lock-on com strafe blendspace 8-way,
  stamina drain.

---

## 2. Game Loop — macro e micro

### 2.1. Loop macro (sessão típica)

```
   ┌─ Main Menu (L_MainMenu) ─────────────────────────┐
   │   Splash → Save Slot → Class Selection           │
   └────┬─────────────────────────────────────────────┘
        ▼
   ┌─ Nexus (L_Nexus, ADFNexusGameMode) ──────────────┐
   │   • Plaza: Blacksmith / Merchant / Sage /        │
   │     Alchemist / Chronicler                       │
   │   • MetaXP, MetaLevel, UnlockedNPCs              │
   │   • Run Portal → Class confirm → Travel          │
   └────┬─────────────────────────────────────────────┘
        ▼
   ┌─ Run (L_Run*, ADFRunGameMode) ───────────────────┐
   │   ERunPhase: PreRun → InCombat → BetweenFloors   │
   │              → BossEncounter → Victory / Defeat  │
   │                                                  │
   │   ┌──────────────── andar N ──────────────────┐  │
   │   │   spawn enemies (DT_Dungeon row)          │  │
   │   │   limpa sala (TotalKills++) → portal      │  │
   │   │   ┌──── BetweenFloors UI ────┐            │  │
   │   │   │ • level-up screen (XP)   │            │  │
   │   │   │ • random event 40%       │ →  next    │  │
   │   │   │ • 1-of-3 ability draft   │            │  │
   │   │   └──────────────────────────┘            │  │
   │   └───────────────────────────────────────────┘  │
   │                                                  │
   │   andar Boss → ADFBossBase (3 fases + enrage)    │
   │   Victory  → meta save → ApplyEndOfRunPersistence│
   │   Defeat   → death screen 2s → meta save         │
   └────┬─────────────────────────────────────────────┘
        ▼
        Volta ao Nexus com `ERunNexusTravelReason`
        (Victory / Defeat / Abandon) e PendingUnlocks
```

**Travel canônico**: `UDFWorldTransitionSubsystem` faz o ServerTravel
genérico; `UDFRunManager` carrega o motivo (`ETravelReason`) que o GameMode
de destino consome em `InitGame`.

### 2.2. Loop micro (combate moment-to-moment, ~30s)

```
       Detecta inimigo
   ┌──> Lock-On (RMB) ─────────┐  (camera Combat mode, strafe blend)
   │    Aproxima (Sprint, S/E)
   ▼                              ┌─ Dodge (i-frames 0.25s, 20 stamina) ─┐
   ATAQUE BÁSICO (LMB)             │                                     │
   ├─ Hit 1 → trace 20cm radius   │   se Health < 20% inimigo → finishing│
   │  ├─ Camera shake Light       │   blow GE_Damage_Physical extra       │
   │  ├─ Hit Stop 0.06s @ 0.05x   │                                     │
   │  ├─ Niagara HitImpact        ▼                                     │
   │  ├─ Combat Text floating     enemy reage:                            │
   │  └─ Music: Combat tag        ├─ Light hit montage (<30 dmg)         │
   │                               ├─ Heavy hit (30-60)                   │
   ├─ Hit 2 (combo window 0.6s)   ├─ Stagger (Stunned 1.5s) acima de 30  │
   │                               └─ Knockback (>60) + impulse           │
   └─ Hit 3 (combo window 0.6s)
       (3-step max, então reset)
       
   Habilidade Q/E/R/F (slots 1-4):
   ├─ Custo Mana / Stamina (CommitAbility)
   ├─ Cooldown (CooldownReduction attr aplica)
   ├─ Montage com AnimNotify de trace
   └─ DoT / Buff / Debuff via GE
```

> **Avaliação — loop micro:** sólido, com todos os ingredientes de juice
> presentes (hit stop por banda, camera shake, screen effects, floating
> numbers). Falta polir tempos (ver seção 4).

---

## 3. Combate — análise sistêmica

### 3.1. Atributos e fórmula de dano

`UDFAttributeSet` define **6 primárias + 6 derivadas + 3 vitals**:

| Categoria | Atributos |
|---|---|
| **Vitais** | Health/MaxHealth, Mana/MaxMana, Stamina/MaxStamina |
| **Ofensivos** | Strength, Intelligence, Agility, **CritChance**, **CritMultiplier** (default 2.0), **SpellDamageAmp** |
| **Mitigação** | Armor, MagicResist |
| **Movimento** | MovementSpeedMultiplier, SprintStaminaDrain |
| **Utilidade** | CooldownReduction (0..1), CharacterLevel (replicado) |

A fórmula canônica está em `UDFDamageCalculation` (Prompt 12):

```
final = (BaseDamage + Attribute × scale) × (1 - Mitigation/100)
        × (CritChance roll → × CritMultiplier)
```

**Crit é replicado como tag** (`Effect.Critical` + `SetByCaller Data.CriticalHit = 1.0`),
permitindo o `UDFCombatTextSubsystem` exibir "stars" no número flutuante.
Hit detection do crit é **server-side authoritative**.

#### Pontos fortes
- Separação Strength/Intelligence/Agility cria **3 builds claros** (físico
  pesado / mágico burst / agilidade-crítico).
- `Effect_Damage_Physical / Magic / True` permite finos ajustes de
  resistência por elemento.
- Finishing blow abaixo de 20% HP (`UDFMeleeTraceComponent::FinishingBlowGameplayEffect`)
  cria **execution moments** classicamente satisfatórios.

#### Riscos
- Não vi um **softcap claro** em `Armor` (pode causar tank que ignora
  100% do dano). Considere DR fórmula style WoW: `Armor / (Armor + K×Level)`.
- `CooldownReduction` clamp a 1.0 — sem softcap: **stacking ilimitado de CDR**
  vira "spam mode". Sugerido cap em 0.4 (40%) e diminishing returns.
- Não há **resistência elementar separada** (só MagicResist genérica).
  `UDFElementalLibrary` existe mas o dano pelo `UDFDamageCalculation` parece
  agnóstico — confirmar e considerar pesos por elemento.

### 3.2. Combate melee — combo & trace

`UDFComboComponent` + `UDFMeleeTraceComponent` formam o **núcleo do melee**.

```
3 montages encadeados (ComboMontages[0..2])
    ├─ AN_TraceStart  → MeleeTrace.StartTrace()
    │                  + cleared HitActorsThisSwing
    ├─ AN_ComboWindow → bComboWindowActive (0.6s)
    └─ AN_TraceEnd    → MeleeTrace.EndTrace()

Sweep multi por sphere TraceRadius=20cm
    weapon_start → weapon_end sockets (no Mesh_Weapon)
    canal: ECC_Pawn (configurável)
    server-only authoritative (bServerOnlyTraces = true)

Aplica GE_MeleeDamage:
    SetByCaller Data.Damage    = BaseDamage (override per-swing OK)
    SetByCaller Data.Knockback = BaseKnockback
    Execution: UDFDamageCalculation (rola crit, mitigação)

Próxima parte: HitReactionComponent.OnHitReceived no alvo
    decide entre Light/Heavy/Knockback montage
    aplica GE_Stagger se >= StaggerThreshold (30 dmg)
    Niagara HitImpact + Decal de sangue (8s lifespan)
```

**Avaliação:**

✅ **Fortes**
- Multi-hit prevention via `HitActorsThisSwing` (TWeakObjectPtr) — robusto
  contra duplicatas em sweeps longos.
- Override per-swing (`SetBaseDamageForNextSwing`, `SetBaseKnockbackForNextSwing`) — permite
  combos com finisher mais forte (ex.: hit 3 = 1.5× damage).
- Server-only traces evita exploits cliente.

⚠️ **Atenção**
- **Combo window 0.6s** é generoso. Action games modernos:
  - Hades = ~0.3-0.4s (responsive)
  - Dark Souls = ~0.5s
  - DMC = ~0.4s
  Recomendo **0.45s** para o feel ARPG sem virar mash-fest.
- Apenas **3 hits no combo**, sem branching light/heavy. Considere
  adicionar:
  - **Heavy attack** (RMB held ou Shift+LMB) com trace mais largo +
    knockback elevado.
  - **Combo finisher** no hit 3 que dispara `UDFCameraShake_HeavyHit` em vez
    de Light.
- `TraceRadius=20cm` é estreito demais para armas grandes (machado, claymore).
  Considere expor por arma na DT_Items (`MeleeTraceRadiusOverride`).

### 3.3. Habilidades (GAS) — depth

**34 abilities autorais**, organizadas por classe:

| Classe | Abilities (ativas) | Passivas |
|---|---|---|
| **Warrior** | ShieldBash, WarCry, Whirlwind, IronSkin, Charge, Execute | Fortitude, Retaliation |
| **Mage** | FrostBolt, BlizzardStorm, ArcaneBarrage, **TimeWarp**, ManaShield, Teleport | ArcaneMastery, ManaVortex |
| **Rogue** | Backstab, FanOfKnives, ShadowStep, Eviscerate, Vanish, SmokeScreen | Predator, BleedMastery |
| **Universal (drops)** | HealthPotion, **SecondWind**, BattleHymn, Siphon, **Berserk**, CallLightning | — |
| **Boss-only** | TerrorShout, MeteorStrike, VoidBarrier, GroundSlam, ChargeAttack, SummonMinions, EnragePulse, PhaseTransitionSlam | — |

**Mecânicas excepcionais já implementadas:**

1. **Combo Points (`UDFComboPointsComponent`)** — Rogue-style builder/finisher
   independente do combo melee. Eviscerate consome combo points → dano
   escalado. **Excelente design** para depth no Rogue sem complicar
   Warrior/Mage.

2. **TimeWarp (Mage)** — `Buff_Mage_TimeWarpHaste` + purge de cooldowns
   genérico via `Ability_Cooldown` parent tag. Implementação mostra que o
   sistema entende **GameplayTag hierarchies** corretamente.

3. **Second Wind (universal)** — interrupção de letal damage no
   `AttributeSet::PreAttributeChange` quando `State.Universal.SecondWindAvailable`
   está ativa. **Hugely satisfying mechanic** se bem audio-visual:
   sugiro slow-mo 0.3× por 0.4s + flash branco + sting de áudio.

4. **Stealth chain (Rogue)** — `Vanish` aplica `State.Stealthed`,
   `State.Concealed`, `State.Invisible`; AI ignora via `Event.Stealth.Entered`.
   Backstab tem dano amplificado contra alvos `State.Stealthed`. Bem desenhado.

5. **Reações elementais** — `Effect.Reaction.Melt / Steam / Electrocute`.
   Genshin/Honkai-style; valida que `UDFElementalComponent` cruza tags.

### 3.4. Lock-on, dodge e movimento

| Sistema | Settings atuais | Comentário |
|---|---|---|
| **LockOnRange** | 1500 cm | OK para arenas pequenas/médias. Em bossfights amplos pode ser pouco. |
| **LockOnAngle** | 60° cone | Razoável. Cycle Q/E reordena candidatos. |
| **CombatArmLength** | 300 cm (vs 400 default) | Bom — fecha câmera no combate. |
| **Dodge IFrame** | 0.25s | Curto. Souls = 0.4-0.5s. |
| **Dodge cooldown** | 0.8s | Bom — não vira spam. |
| **Sprint stamina drain** | 15/s | Razoável. |
| **Strafe blendspace** | 8-way ativo só com `bShouldStrafe` (lock-on) | Padrão moderno ARPG. |

⚠️ **Dodge** — recomendo:
- **Tornar i-frame visível** com chromatic aberration pulse (já tem
  `ChromaticAberrationPulse`!) por 0.25s no `GA_Dodge::Activate`.
- **Camera shake suave** ao iniciar dodge para feedback tátil.
- **TeleportOrBlink** já existe no `UDFScreenEffectsComponent` — usar em
  ShadowStep e Teleport por consistência.

---

## 4. "Feel" e Juice — análise crítica

Este é o **maior diferencial técnico do projeto**: um pipeline de juice
completo em C++.

### 4.1. Hit Stop (`UDFHitStopSubsystem`)

```cpp
LightHit:    0.06s @ 0.05× global dilation
HeavyHit:    0.10s @ 0.02×
CriticalHit: 0.14s @ 0.01×
BossSlam:    0.20s @ 0.0×  (clamped to 0.0001)
```

Com **exclusão de actor** (`CustomTimeDilation = 1/Effective`) — o instigator
continua em tempo normal enquanto o mundo congela. **Padrão ouro** (Hades faz
exatamente isso).

✅ **Fortíssimo**. Um dos sistemas mais bem implementados do projeto.

⚠️ **Atenção — disparar quando?**
- Confirme que `LightHit` é chamado para **todo melee normal** (em
  `UDFMeleeTraceComponent::ApplyDamageToTarget`, ramo Light).
- `HeavyHit` deve ser chamado em finishers / habilidades (Whirlwind hit 3,
  Eviscerate finisher).
- `CriticalHit` deve ser chamado **somente em crits** (ler tag
  `Effect.Critical` no spec aplicado).
- `BossSlam` em PhaseTransitionSlam, MeteorImpact e EnragePulse.

> **Sugestão:** centralizar em `UDFCombatFeedbackTypes` um helper estático
> `DispatchHitStopFromBand(EDFHitFeedbackBand)` chamado pelo
> `Client_HitFeedback` do `ADFPlayerCharacter`.

### 4.2. Camera Shakes (`UDFCameraShakes`)

4 shakes legacy implementados: `LightHit`, `HeavyHit`, `BossSlam`, `Explosion`.
Bossbase tem `Multicast_BossLocalAttackFX` com inner/outer radius — atenuação
**baseada em distância** correta.

⚠️ **Faltando:**
- **Death shake** quando o player morre (transição para `OnDeath`).
- **Sprint shake** sutil (subliminar) para reforçar velocidade.
- **Critical hit shake** — atualmente cobertos por HeavyHit, mas crit
  deveria ter **mais perfil de roll** (yaw kick lateral) para destacar.

### 4.3. Screen Effects (`UDFScreenEffectsComponent`)

Lista completa do que já está cabeado:

| Função | Trigger | Visual |
|---|---|---|
| `DamageReceived(percent)` | dano recebido | flash vermelho proporcional |
| `HealReceived(percent)` | cura | flash verde + saturação |
| `BerserkSetActive(bool)` | tag State_Berserk | vignette vermelho + grain + FOV bump |
| `OnDeath()` | OnOutOfHealth | dessaturação + slow vignette |
| `TeleportOrBlink()` | ShadowStep, Teleport | chromatic + flash branco |
| `LowHealthSetEnabledFromRatio()` | Health < 25% | pulse vermelho 0.3→0.7 (1.5s) |
| `ChromaticAberrationPulse()` | parametrizável | spike chromatic |
| `FlashScreen(color, dur, int)` | parametrizável | overlay color |

**Avaliação:** **excelente cobertura**. Comparável a games AAA.

⚠️ **Sugestões:**
- Material parent (`ScreenEffectParentMaterial`) **precisa existir** —
  confirmar que existe em `Content/DungeonForged/FX/` com os parâmetros
  esperados (`VignetteIntensity`, `VignetteColor`, `ChromaticAberration`,
  `BlurAmount`, `SaturationMult`, `FlashIntensity`, `FlashColor`,
  `GrainAmount`).
- Adicionar **"second wind"** como state visual: flash branco 0.3s +
  saturação 1.5× + slow-mo 0.4s.
- **Boss intro** poderia ter `bDeathInProgress`-style takeover (vignette
  forte + saturação −0.5).

### 4.4. Combat Text (`UDFCombatTextSubsystem`)

Pool de **30 widgets reciclados** — zero alocações em runtime após boot.
Suporta tipos: Damage / Crit / Heal / Miss / XP / Status. Spawn por
worldspace location, fade animado pelo widget.

✅ **Sólido** — exatamente o padrão de Diablo/PoE.

⚠️ **Sugestões:**
- Crit deve ter **escala 1.4×** + **shake horizontal** + **stroke amarelo**
  (atualmente Type=Crit decide texto mas presumimos formatação visual no WBP).
- Numbers acima de 100 deveriam ter **kerning extra**, números > 1000
  abreviados ("1.2k") — nice-to-have.

### 4.5. Áudio adaptativo (`UDFMusicManagerSubsystem`)

7 estados, 3 layers (Base/Combat/Boss), 2s crossfade real (não cut), stings
discretos. **Muito bom design.**

```
Idle/Exploration  →  base layer 1.0,  combat 0.0, boss 0.0
Combat            →  base 0.4, combat 1.0
Boss              →  base 0.0, combat 0.0, boss 1.0
Victory           →  sting 8s → return to Exploration
Death             →  sting one-shot
```

⚠️ **Sugestões:**
- **Elite encounter** state existe no enum mas não vi disparo automático.
  Adicionar trigger quando `EEnemyTier::Elite` entra em range.
- **Adaptive intensity dentro de Combat** — stems de "low intensity"
  vs "high intensity" baseados em # de inimigos vivos (>3 = high).
- **Boss enrage** deve trocar para variant "intense" do BossLayer (re-trigger
  crossfade em `OnBossEnraged`).

---

## 5. Roguelike Loop — Pacing e Recompensa

### 5.1. Estrutura por andar

`FDFDungeonFloorRow` (DT_Dungeon):

```
FloorNumber, PossibleEnemyRows[],
MinEnemies / MaxEnemies,
bHasBoss (+ BossEnemyRow),
DifficultyMultiplier
```

`ADFRunGameMode::MaxRunFloor = 10` (default). Boss em andar com `bHasBoss = true`.

### 5.2. Drops e abilities

Entre cada andar (`TriggerBetweenFloorSequence`):

1. **Random Event** (40% chance) — `UDFRandomEventSubsystem`. 14 tipos de
   outcome (Heal, Damage, Gold, AddAbility, RemoveAbility, AddEffect,
   RandomGood, RandomBad, AddItem, ScaleEnemyDamage, SpawnSpecialEncounter,
   RandomHealOrDamage, SwapRandomAbility, Nothing). Eventos podem ter
   choices com **requisito de Agility** ou tags GAS.
2. **Level-up screen** se XP ≥ próximo limiar (`UDFLevelingComponent`,
   max 30). Spend 1 ponto em STR/INT/AGI.
3. **1-of-3 ability draft** (`UDFAbilitySelectionSubsystem`) com weights por
   `EItemRarity`:
   - Common 60% / Uncommon 25% / Rare 12% / Epic 3% (do Prompt 23 design)
   - Skip = +50 gold (configurável)

### 5.3. Análise de pacing

⚠️ **Random event 40% por andar é alto.**
- Em runs de 10 andares = ~3-4 events por run.
- Pode virar "noise" e diluir tensão de combate.
- Sugestão:
  - Andares 1-3: 25%
  - Andares 4-7: 40%
  - Andares 8-9: 60% (jogador investido, eventos = decision moments)
  - Andar 10 (boss): nunca

⚠️ **Ability draft toda transição** vira "build inflation".
- Hades faz draft em ~70% das salas e o resto em "shrines" passivas.
- Considere **shrines passivas (mod GE)** vs **abilities ativas** alternando
  para variar pacing de decisão.

⚠️ **Difficulty curve** — `DifficultyMultiplier` é float, mas não vi
  evidência de **inflation de HP/dano dos inimigos** baseado em
  `RunState.CurrentFloor`. **Crítico:**
- Confirmar em `ADFEnemyBase::ApplyBaseStatsFromRow` se aplica
  `RunState.CurrentFloor × DifficultyMultiplier × Row.BaseHealth`.
- Se não, andares 8-9 ficam **trivials** depois de buffs do jogador.

### 5.4. Eventos roguelike — qualidade

`FDFEventChoice` suporta:
- `RequiredTags` (FGameplayTagContainer) — gating por build
- `RequiredAgility` (float) — skill check estilo CRPG
- `OutcomeType` × 14 tipos
- `OutcomeText` para feedback narrativo

**Excelente fundação** para uma economia narrativa estilo Slay the Spire.

⚠️ **O que falta:**
- **Conditional outcomes** — "se classe == Mage então X, senão Y". Hoje só
  tem requisito (gate). Sugiro: `OutcomeIfClass` + fallback.
- **Persistent flags** entre andares — quest chains do tipo "se aceitou
  pacto demoníaco no andar 3, então boss do 5 sofre malus".
- **Special encounters** — `SpawnSpecialEncounter` é uma porta aberta
  (broadcast delegate). Usar para mini-bosses, mercadores secretos,
  oráculos. **Ótima oportunidade de variedade.**

---

## 6. Bosses — análise das fases

`ADFBossBase`:

```
3 phases (default 0.6, 0.3 HP thresholds)
EnrageTimer = 120s (apply +50% damage, +30% speed, immunity to CC)
PhaseAbilities[NewPhase - 2]   // unlocks
PhaseTransitionMontage + VFX + Stun 1.5s (cinematic moment)
EnrageRoarMontage + EnrageVFX
Multicast_BossLocalAttackFX(origin, shake, niagara, inner, outer)
SpawnedMinions tracked com cap (default 6 from Prompt 22)
```

✅ **Fortíssimo** — phase transition stunning é um pattern AAA (Souls/Bayonetta).

⚠️ **Tuning:**
- **EnrageTimer 120s = 2 min**. Para bosses com 3 fases = ~40s/fase. Tight.
  Em primeira run pode ser punitivo. Sugestão:
  - Primeiro encontro do boss: 180s (forgiving)
  - Após 3 wins do boss: 120s (challenge)
  - "Challenge mode" / Daily: 90s
- **Aviso visual de enrage** — usar `OnBossEnraged` para:
  - Pulsar `WBP_BossHealthBar` em vermelho
  - Camera shake forte no momento
  - Tag UI cinematicLock 0.5s para "freeze frame"
  - Music crossfade para variant "intense"

### 6.1. Boss abilities atuais

| Ability | Mecânica | Camera Shake | Niagara |
|---|---|---|---|
| GroundSlam | sphere overlap 500cm @ delay 0.45s | SlamCameraShake (outer 2500cm) | SlamNiagara |
| MeteorStrike | warning decal → impact actor | (presumido HeavyHit) | impact |
| VoidBarrier | shield phase 2/3 | — | barrier ring |
| TerrorShout | AOE fear (Effect.Debuff.Terrified) | — | shout cone |
| ChargeAttack | root motion charge + trace | (charge) | trail |
| SummonMinions | spawn 3 enemies (cap 6) | — | spawn ring |
| PhaseTransitionSlam | room-wide hit (`Event.Boss.PhaseErupt`) | BossSlam | erupt |
| EnragePulse | persistent buff + pulse de dano | — | aura |

✅ **Variedade de tipos:** AOE telegrafada (slam), warning decal (meteor),
defensive (barrier), CC (terror shout), deslocamento (charge), summons,
cinematic (transition slam), passive aura (enrage pulse). **Boa cobertura.**

⚠️ **Faltando:**
- **Add-phase mecânica** (boss + minions ao mesmo tempo) — `SummonMinions`
  existe mas não vi a fase específica que ativa.
- **Vulnerability windows** — `State_BossVulnerable` existe; usar em phase
  transition stun (1.5s = janela de DPS) e quando charge falha (1s
  vulnerable do Prompt 22).
- **Telegraph readability** — `ADFMeteorWarningDecal` é um bom começo;
  considerar **tom audio** distinto por ability (ex.: low rumble para slam,
  high whine para meteor).

---

## 7. Inimigos comuns e arenas

### 7.1. `ADFEnemyBase`

Estrutura sólida, com:
- AI via Behavior Tree de DT (`AIBehaviorTree` por row)
- Patrol points cíclicos
- Taunt montages
- 3 ranges: Melee/Ranged/Attack
- Replicated walk speed
- Hit Reaction component
- Elemental Component (per-row affinity table)
- Loot drop (gold range + items)
- XP reward

⚠️ **Oportunidades:**

- **Variantes elite/champion** — `EEnemyTier::Elite` está no enum mas não vi
  bonificação automática (HP × 2, dano × 1.5, abilities extra). Implementar.
- **Group AI behaviors** — atualmente cada inimigo é independente. Adicionar
  `UDFAIController::SetCombatRole(Tank/Skirmisher/Caster)` e que apenas 1
  Tank por sala faça aggro.
- **Arquetypes**:
  - **Glass cannon archer** com kiting
  - **Charger** que faz dash no jogador (telegraphed)
  - **Healer-priest** que cura outros — força priorização
  - **Mage** com canalização interrompível (`Event.Hit.Received` cancel)

### 7.2. Traps (5 tipos atuais)

`ADFTrap_SpikePlate`, `ADFTrap_FireJet`, `ADFTrap_PoisonVent`,
`ADFTrap_DartWall`, `ADFTrap_CollapsingFloor` + base com telegraph,
trigger delay, rearm timer, GAS effect, 3 níveis de Niagara
(Active/Trigger/Disabled), `bIsHidden` + custom depth stencil para
detection (via `UDFTrapDetectionComponent`).

✅ **Boa fundação técnica.**

⚠️ **Variedade insuficiente para 10 andares:**
- Sugiro **mais 3-5 tipos**:
  - Pendulum blade (timed swing)
  - Pressure-plate arrow walls (multi-direção)
  - Magic rune (delayed AOE com pattern visual)
  - Acid pool (ground hazard persistente)
  - Pit trap (instant fall + damage on respawn)
- **Combo de traps** — sala onde múltiplas traps ativam em sequência
  (Mario/Tomb Raider style).

---

## 8. UI / UX — análise

### 8.1. HUD em jogo (`UDFInGameHUDWidget`)

Bindings core:
- `GoldText` + `CoinIcon` + `GoldChangePulseAnim`
- `DFStatusEffectBar` (buffs/debuffs do jogador)

Junto com prompts originais:
- `UDFAttributeBarWidget` × 3 (Health/Mana/Stamina)
- `UDFAbilitySlotWidget` × 4 (cooldown overlay material)
- `UDFLockOnWidget` (rotating ring)
- `UDFMinimapWidget` + `UDFMinimapFogComponent`
- `UDFBossHealthBarWidget` (cinematic)
- `UDFCombatTextWidget` (pool 30)
- `UDFLevelXPStatusWidget`
- `UDFLevelUpWidget`

✅ **Cobertura completa de um ARPG**.

⚠️ **Sugestões:**
- **HUD adaptative** — em combate, fade in todas as barras; fora, fade out
  health/mana (Souls-style minimal HUD).
- **Damage taken indicator** — direcional (de qual direção veio o golpe).
  Já existe `HitDirection2D` no `OnHitReceived`; usar para canto vermelho
  do screen edge.
- **Boss intro overlay** — letterbox 16:9 → 21:9 com nome animado (já tem
  `BossDisplayName` em `ADFBossBase`).

### 8.2. Class Selection

Já documentado em [`docs/blueprints/ClassSelection_Setup.md`](../blueprints/ClassSelection_Setup.md).
Notável:
- 3D preview com `SceneCapture2D` em RenderTarget
- Slow-mo 0.3× quando aberto
- Drag-to-rotate, scroll-to-zoom
- Co-op partner hover preview
- Active challenge indicator

✅ **Excelente design de player intent**.

### 8.3. Defeat / Victory

`UDFDefeatScreenWidget` + `UDFVictoryScreenWidget` com `FDFRunSummary`
(floor, kills, gold, time, class, abilities collected). Server RPC
`Client_OpenDefeatScreen(Summary, DefeatCause)`.

⚠️ **Polishing:**
- **Death cause text** — exibir literal do que matou (boss name ou trap
  type). Já tem `DefeatCause: FString`, popular consistentemente em
  `HandlePlayerOutOfHealth`.
- **"Best floor" / "Best kills" highlighted** quando o jogador supera —
  os campos `BestFloorReached`, `BestKillsInRun` existem no save.
- **Replay/Highlights** — capturar os últimos 10s antes da morte
  (Sequencer + UMovieRenderQueue) é luxo, mas valoriza muito.

---

## 9. Meta-progressão (Nexus)

### 9.1. `UDFSaveGame` — campos persistidos

Categorias presentes:
- **Settings**: idioma, accessibility, key bindings
- **Meta**: HighScore, TotalRuns, TotalWins
- **Unlocks**: UnlockedClasses, UnlockedAbilities, UnlockedNPCs,
  CompletedUpgrades
- **Nexus**: MetaXP, MetaLevel, **PendingUnlocks** (queue para tela toast),
  RunHistory[], LifetimeKills, LifetimeDeaths, TotalPlayTimeSeconds,
  TotalGoldEarnedMeta, MerchantRestockRunCounter (every 3 runs),
  BestFloorReached, BestKillsInRun
- **Profile**: bIsFirstLaunch, **bHasActiveRun**, LastRunClass,
  LastRunFloor, LastPlayedDate, GameVersion, SlotIndex
- **Run**: LastCheckpoint, LastCheckpointType (resume after crash)

✅ **Save versioning correto** (`SaveVersion = 6`, `IsCompatible`).

### 9.2. NPCs do Nexus

`ADFNexusNPCBase` + 5 specializations:
- **Blacksmith** — upgrades de equipamento
- **Merchant** — compra de itens (refresh every 3 runs)
- **Sage** — abilities? (lore)
- **Alchemist** — potions / consumíveis
- **Chronicler** — stats lifetime (BestFloor, BestKills, TotalPlayTime)

✅ **Boa diversificação** — cada NPC tem propósito mecânico distinto.

⚠️ **Sugestões:**
- **Blacksmith** — adicionar reforjos com permanent stat boosts (paid in
  gold meta).
- **Merchant** — alguns slots de "Daily Deal" com desconto.
- **Sage** — desbloqueio de sub-classes / variants (Hellblade Warrior,
  Elementalist Mage, etc.).
- **Chronicler** — exibir **leaderboards locais** (top 10 runs) com
  filtros (por classe, por andar máximo).

### 9.3. Meta XP curve

`FDFNexusLevelRow` com `MetaXPRequired` cumulativo, `UnlockNPCRow`,
`UnlockClassRow`, `UnlockUpgradeRows[]`.

⚠️ **Crítico:** o **ritmo de meta progression** define retenção. Hades 1
oferece um upgrade quase toda run nas primeiras 30; Returnal é mais
spaced out. Recomendo:
- **Run 1-5**: 1 unlock significativo por run
- **Run 6-15**: 1 a cada 2 runs
- **Run 16+**: 1 a cada 3-4 runs (mas progressivamente impactantes)

---

## 10. Networking / Co-op (já parcialmente implementado)

Sinais positivos:
- `bServerOnlyTraces = true` no `UDFMeleeTraceComponent`
- ASC em PlayerState (Mixed mode) para player; em Enemy (Minimal) para AI
- `Multicast_BossLocalAttackFX` com inner/outer radius (correto)
- `Client_HitFeedback(Band, Damage%, Instigator)` — apenas o victim recebe
  hit stop local (não trava a sessão inteira)
- `FOnDFEnemyDied` delegate com Killer + ExperienceReward
- `ADFNexusGameState::CoopPartnerHoveredClass` (replicated)
- `ADFNexusPlayerController::Server_BeginRunWithClass`
- `CurrentAbilitySlots` (TArray<FName>, replicated) — para HUD do parceiro

⚠️ **Riscos**:
- **Random events em co-op** — escolha sincronizada? `UDFAbilitySelectionWidget`
  registra-se em `UDFAbilitySelectionSubsystem` e fecha todas as instâncias
  quando uma decide ("first lock-in wins"). Isso pode frustrar — sugerir
  **votação por timer** ou **escolha por jogador independente**.
- **Hit stop em co-op** — como o sistema é global, dois players atacando
  ao mesmo tempo entram em hit stop simultâneo. Confirmar via
  `Client_HitFeedback` que cada cliente tem sua própria timeline.

---

## 11. Acessibilidade & Localização

`FDFAccessibilitySettings` + `UDFAccessibilitySubsystem` + `UDFLocalizationSubsystem`.
SaveGame persiste `PreferredLanguage` (default `pt-BR`), `PreferredCultureCode`,
`SavedKeyBindings` (Enhanced Input remap).

✅ **Pioneiro para projeto solo**. PT-BR como default mostra atenção.

⚠️ **Sugestões:**
- **Color blind modes** — variações de palette para ability icons e
  rarity colors.
- **Hold-to-cast** vs **toggle** — para Sprint, IronSkin, ManaShield (já
  existe pelo enum `EAbilityActivationPolicy`).
- **Damage numbers off** — adicionar toggle (alguns players preferem).
- **Camera shake intensity slider** (0-100%) — multiplica todas as
  invocações em `UDFCameraShakeFunctionLibrary`.
- **Hit stop intensity slider** — alguns players com motion sickness
  podem desativar.

---

## 12. Avaliação consolidada — radar do projeto

| Pilar | Nota | Comentário |
|---|---|---|
| **Arquitetura técnica (C++/GAS)** | 9/10 | GAS exemplar, networking bem pensado, code clean. |
| **Combate (mecânica)** | 7/10 | Sólido mas combo melee precisa de heavy/light branching. |
| **Combate (feel)** | 8/10 | Hit stop + camera shake + screen FX = AAA-tier; só precisa de tuning. |
| **Variedade de habilidades** | 9/10 | 34 abilities autorais + universals; depth excelente. |
| **Roguelike loop** | 8/10 | Draft + events + meta-progression = tudo presente; precisa de tuning de pacing. |
| **Boss design** | 8/10 | Phases + enrage + signature abilities; falta só readability tweaks. |
| **Inimigos comuns** | 6/10 | Base sólida; falta variantes de archetype e group AI. |
| **Traps / hazards** | 6/10 | 5 tipos é pouco para 10 andares; precisa expandir. |
| **UI/HUD** | 8/10 | Cobertura completa; falta só UX adaptativo. |
| **Áudio adaptativo** | 8/10 | Music states + crossfade; falta intensity tiers e elite trigger. |
| **Meta-progressão** | 7/10 | Estrutura completa; o tuning de XP curve dirá tudo. |
| **Acessibilidade** | 7/10 | Pioneiro; faltam alguns sliders importantes. |
| **Networking / co-op** | 7/10 | Bem desenhado, mas decisões em grupo precisam revisão UX. |

**Avaliação geral: 7.7 / 10 — projeto solo de qualidade comercial.**

---

## 13. Top 10 prioridades recomendadas (ordem de impacto vs esforço)

1. **🔥 [HIGH/LOW]** Confirmar e ajustar **scaling de inimigos por andar**
   (`DifficultyMultiplier × CurrentFloor`) em `ADFEnemyBase::ApplyBaseStatsFromRow`.
   Se não estiver aplicando, andar 8-10 fica fácil demais.

2. **🔥 [HIGH/LOW]** Tunar **combo window de 0.6s para 0.45s** no
   `UDFComboComponent::ComboWindowDuration`. Feel mais responsivo.

3. **🎯 [HIGH/MEDIUM]** Implementar **heavy attack** (RMB held) com `UDFCameraShake_HeavyHit`
   + trace radius maior. Cria depth sem refazer combos.

4. **🎯 [HIGH/MEDIUM]** Centralizar **hit stop dispatch** em `EDFHitFeedbackBand`:
   - Light = todo melee normal
   - Heavy = combo finisher / habilidades
   - Critical = quando `Effect.Critical` está no spec
   - BossSlam = abilities específicas de boss

5. **🎨 [MEDIUM/LOW]** Polir **dodge feedback**:
   `ChromaticAberrationPulse(0.25, 0.6)` + camera shake suave + i-frames
   visuais (vinheta azul claro 0.25s).

6. **🎮 [MEDIUM/MEDIUM]** Ajustar **EventChancePerFloor** para curva
   crescente (25% / 40% / 60% por terço da run, 0% no boss).

7. **👹 [MEDIUM/MEDIUM]** Adicionar **3-5 traps novos** (pendulum, rune,
   acid pool, pit, multi-arrow) para atingir ~10 tipos.

8. **🤖 [MEDIUM/HIGH]** Implementar **enemy archetypes** (Tank/Skirmisher/Caster
   roles) com group AI básico (1 tank por sala faz aggro).

9. **🎼 [MEDIUM/LOW]** Conectar **Elite music state** (já no enum) ao
   detector — quando inimigo `EEnemyTier::Elite` entra em range com player.

10. **♿ [LOW/LOW]** Adicionar **3 sliders de acessibilidade**: camera
    shake intensity, hit stop intensity, damage numbers on/off.

---

## 14. Riscos arquiteturais a observar

1. **GameInstance Subsystem `UDFRunManager` carrega state em memória** —
   crash mid-run = perde tudo (exceto LastCheckpoint). **Validar que
   `UDFWorldTransitionSubsystem::SaveCheckpoint` é chamado em momentos
   estratégicos** (após cada andar limpo, após boss kill, após cada 90s).

2. **`SaveVersion = 6` indica 6 migrations já feitas.** Logs do projeto
   precisam ter testes de **reload de save antigo** — adicionar
   `Source/DungeonForged/Tests/SaveCompatibility.spec.cpp`.

3. **`UDFHitStopSubsystem` usa global time dilation com exclusão de
   actor** — em servidor dedicado isso pode descalibrar tickers AI.
   **Em cliente apenas** é o esperado; confirmar que
   `Client_HitFeedback` é o único que dispara em multiplayer.

4. **Memory leaks de Niagara** — `Multicast_BossLocalAttackFX` spawna
   sistemas. Confirmar que `UNiagaraComponent::SetAutoDestroy(true)` está
   no spawn (ou usar `OneShot` API).

5. **Modular mesh leader pose** — Mesh_Helmet/Chest/Legs/Boots/Gloves
   seguem Mesh_Base via leader pose. **Se Mesh_Base for hidden/destroyed
   antes deles, animação congela.** Verificar order de cleanup em `EndPlay`.

---

## 15. Próximos documentos sugeridos

- [ ] `docs/analysis/Combat_Tuning.md` — números concretos para BaseDamage,
  HP por andar, stamina costs, cooldowns.
- [ ] `docs/analysis/Difficulty_Curve.md` — XP curve, MetaLevel curve,
  unlock cadence, boss HP por nível.
- [ ] `docs/analysis/Audio_Mix.md` — todos os sounds com targets de
  loudness (LUFS) e categorias de submix.
- [ ] `docs/analysis/Encounter_Design.md` — templates de salas (3 inimigos
  + 1 trap, 5 inimigos sem trap, 1 elite + 2 minions, etc.).
- [ ] `docs/analysis/Boss_Patterns.md` — frame-by-frame timing dos slams,
  meteor pre-warning, charge windups.

---

## 16. Referências cruzadas (arquivos-chave do projeto)

| Sistema | Arquivo principal |
|---|---|
| Player | `Source/DungeonForged/Public/Characters/ADFPlayerCharacter.h` |
| GAS Attributes | `Source/DungeonForged/Public/GAS/UDFAttributeSet.h` |
| Combo melee | `Source/DungeonForged/Public/Combat/UDFComboComponent.h` |
| Melee trace | `Source/DungeonForged/Public/Combat/UDFMeleeTraceComponent.h` |
| Hit reaction | `Source/DungeonForged/Public/Combat/UDFHitReactionComponent.h` |
| Hit stop | `Source/DungeonForged/Public/FX/UDFHitStopSubsystem.h` |
| Camera shakes | `Source/DungeonForged/Public/FX/UDFCameraShakes.h` |
| Screen effects | `Source/DungeonForged/Public/FX/UDFScreenEffectsComponent.h` |
| Combat text pool | `Source/DungeonForged/Public/UI/Combat/UDFCombatTextSubsystem.h` |
| Music | `Source/DungeonForged/Public/Audio/UDFMusicManagerSubsystem.h` |
| Camera | `Source/DungeonForged/Public/Camera/UDFCameraComponent.h` |
| Lock-on | `Source/DungeonForged/Public/Camera/UDFLockOnComponent.h` |
| Run state | `Source/DungeonForged/Public/Run/DFRunManager.h` |
| Save game | `Source/DungeonForged/Public/Run/DFSaveGame.h` |
| Run gamemode | `Source/DungeonForged/Public/GameModes/Run/ADFRunGameMode.h` |
| Nexus gamemode | `Source/DungeonForged/Public/GameModes/Nexus/ADFNexusGameMode.h` |
| Boss base | `Source/DungeonForged/Public/Boss/ADFBossBase.h` |
| Enemy base | `Source/DungeonForged/Public/Characters/ADFEnemyBase.h` |
| Random events | `Source/DungeonForged/Public/Events/UDFRandomEventSubsystem.h` |
| Ability draft | `Source/DungeonForged/Public/UI/UDFAbilitySelectionSubsystem.h` |
| Leveling | `Source/DungeonForged/Public/Progression/UDFLevelingComponent.h` |
| Native tags | `Source/DungeonForged/Public/GAS/DFGameplayTags.h` |
| Data structs | `Source/DungeonForged/Public/Data/DFDataTableStructs.h` |
| Trap base | `Source/DungeonForged/Public/Dungeon/Traps/ADFTrapBase.h` |

---

**Conclusão final:** o coração técnico do DungeonForged é **muito sólido** —
o trabalho restante para uma "experiência completa" é **8-12 semanas de
tuning + content** (mais inimigos, traps, encounters, números balanceados,
polish de feel). Não há débitos arquiteturais críticos que exigem rewrite;
o que falta é **a camada de design** (números, conteúdo, varidade) que
**só consegue ser feita testando o jogo end-to-end**.

A recomendação imediata é **playtesting agressivo** focado em encontrar:

1. Onde a curva de dificuldade quebra (qual andar fica trivial / qual mata o
   jogador injustamente).
2. Quais abilities dominam e quais nunca são pegas no draft.
3. Onde o feel está "morto" (sem hit stop / sem camera shake disparando).
4. Quanto tempo dura uma run completa (target: 25-40 min).

Boas runs.
