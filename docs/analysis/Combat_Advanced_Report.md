# DungeonForged — Relatório Avançado do Sistema de Combate

> **Data:** 2026-05-20 · **Status atualizado:** 2026-05-18
> **Escopo:** auditoria técnica profunda de combate, combos, habilidades, juice, animação e replicação. Comparativo direto com referências AAA (Sekiro, God of War Ragnarok, DMC5, Nioh 2, Hi-Fi Rush, Returnal, Black Myth Wukong).
> **Objetivo:** identificar as lacunas concretas entre o estado atual e a sensação "fluida, responsiva, juicy, feeling AAA" que você quer.
> **Premissa de leitura:** este doc presume familiaridade com [`Game_Analysis.md`](Game_Analysis.md) e a série [`docs/improvements/`](../improvements/00_Overview.md).
> **Setup Blueprint/Editor:** ver [`10_CombatBlueprintSetup.md`](../improvements/10_CombatBlueprintSetup.md).

---

## Status de implementação (2026-05-18)

Legenda: **✅ C++ concluído** · **⚠️ C++ feito, falta config BP/assets/playtest** · **❌ pendente**

| Área | Status |
|------|--------|
| Tier S (S1–S6) | ✅ 6/6 |
| Tier A (A1–A8) | ✅ 8/8 C++ · ⚠️ montages/tuning data no editor |
| Tier B (B1–B14) | ✅ 13/14 C++ · ⚠️ B9 predição básica · ⚠️ assets BP |
| Validação §4 / §5.2 / §5.3 | ❌ playtest e `L_CombatRange` |
| Co-op random events (N5) | ❌ design, fora de escopo C++ |

**Posição atual revisada: ~9/10 em engenharia** — feel AAA percebido depende de preencher `DA_CombatTuning`, notifies nas montages e playtest de rede.

---

## TL;DR — onde você está vs. AAA

**Posição atual: ~9/10 em C++** — as 6 frentes críticas do TL;DR original foram implementadas. O pipeline de combate está centralizado; o que resta é **configuração no editor** e **validação em playtest**.

| # | Frente original | Status |
|---|-----------------|--------|
| 1 | Centralizar dispatch de feedback (`DispatchOnHitConfirmed`) | ✅ inclui HitStop, shake, screen FX, combat text, impact VFX/SFX |
| 2 | Projectile Parity Gap | ✅ Knife/Fireball/Frostbolt/Arcane Missile → mesmo pipeline |
| 3 | Input-feel (buffer, hitstop pause, refresh on-hit, stick, commit-grade) | ✅ |
| 4 | Bug replicação combo (C7) | ✅ + `bComboHeavyFinisherPending` (N4) |
| 5 | CooldownReduction aplicado | ✅ cap 0.4 + DR soft |
| 6 | Dodge juice | ✅ FOV + chromatic + shake |

**Próximo passo:** [`10_CombatBlueprintSetup.md`](../improvements/10_CombatBlueprintSetup.md) + checklist §4 + level `L_CombatRange` (§5.2).

---

## 1. Mapa da arquitetura (referência rápida)

```
                        ┌───────────────────────────────────────┐
                        │  Input → DFInputConfig + Enhanced     │
                        └────────────────┬──────────────────────┘
                                         ▼
        ┌────────────────────────────────────────────────────────┐
        │           UDFComboComponent (núcleo do melee)          │
        │  ─ AttackInputBufferDuration = 0.15s                   │
        │  ─ ComboWindowDuration = 0.45s                          │
        │  ─ HeavyChargeThreshold 0.55s / MaxHeavy 1.4s          │
        │  ─ Directional resolve (vel.X / vel.Y / default)        │
        │  ─ Server_ChainMeleeComboStep RPC                      │
        └──────┬───────────────────────────┬─────────────────────┘
               ▼                           ▼
   ┌───────────────────────┐   ┌──────────────────────────────┐
   │ UDFMeleeAimComponent  │   │ GAS: UDFAbility_*MeleeSwing  │
   │ ─ ManualTarget > Lock │   │ ─ activate → PlayMontage     │
   │ ─ Lock > AI BB        │   │ ─ NetExec: LocalPredicted    │
   │ ─ Soft cone sweep     │   └──────────────┬───────────────┘
   │ ─ SnapYaw 15°         │                  ▼
   └──────────┬────────────┘     ┌─────────────────────────────┐
              │                  │ AnimMontage timeline:       │
              │                  │  [DF Melee Warp Target]     │
              │                  │  [AN_TraceStart]            │
              ▼                  │  [AN_TraceEnd]              │
   ┌──────────────────────┐      │  [DF Cancel Window]         │
   │ UMotionWarpingComp   │◀─────│  [AN_ComboWindowOpen]       │
   └──────────────────────┘      └────────────┬────────────────┘
                                              ▼
              ┌──────────────────────────────────────────────────┐
              │  UDFMeleeTraceComponent — server-only swept-sphere│
              │   ─ HitActorsThisSwing (dedup por swing)         │
              │   ─ BuildDamageSpec → SetByCaller Data.Damage    │
              │   ─ Damage GE → UDFDamageCalculation             │
              └───────────────────┬──────────────────────────────┘
                                  ▼
              ┌──────────────────────────────────────────────────┐
              │  UDFDamageCalculation (Exec)                     │
              │   ─ Physical: Str × 0.5 → Armor/(Armor+K)        │
              │   ─ Magic:    Int × 0.5 × (1+SpellAmp) → MR%     │
              │   ─ Crit roll (DR > 0.5)                          │
              │   ─ State.BossVulnerable × 1.5                   │
              └──────┬──────────────────────┬────────────────────┘
                     ▼                      ▼
   ┌──────────────────────────┐   ┌────────────────────────────┐
   │ UDFAttributeSet          │   │ UDFHitReactionComponent    │
   │  ─ PostGEExecute:        │   │  ─ Direction-aware montage │
   │    spawn combat text,    │   │  ─ Light/Heavy/Knockback   │
   │    DispatchHitReceived   │   │  ─ Damage.Source variants  │
   └──────────────────────────┘   └──────────────┬─────────────┘
                                                 ▼
                       ┌─────────────────────────────────────┐
                       │ UDFStaggerComponent                 │
                       │  ─ Sliding window (3s)              │
                       │  ─ Poise threshold → stun GE        │
                       └──────────────┬──────────────────────┘
                                      ▼
              ┌───────────────────────────────────────────────┐
              │  FEEL DISPATCH (centralizado — A1 ✅)         │
              │   ─ UDFCombatFeedbackLibrary::              │
              │     DispatchOnHitConfirmed                  │
              │   ─ HitStop + Shake + ScreenFX + CombatText │
              │   ─ Impact VFX/SFX via DA_CombatTuning      │
              └───────────────────────────────────────────────┘
```

**Subsistemas adjacentes em uso:**
- [`UDFCombatDirectorSubsystem`](../../Source/DungeonForged/Public/Combat/UDFCombatDirectorSubsystem.h) — tokens de ataque (max 2)
- [`UDFCombatStateLibrary`](../../Source/DungeonForged/Public/Combat/UDFCombatStateLibrary.h) — tag `State.InCombat`
- [`UDFElementalReactionSubsystem`](../../Source/DungeonForged/Public/GAS/Elemental/UDFElementalReactionSubsystem.h) — Melt/Electrocute/Steam
- [`UDFStaminaExhaustionComponent`](../../Source/DungeonForged/Public/Combat/UDFStaminaExhaustionComponent.h) — `State.Exhausted` por 0.5s
- [`UDFLockOnComponent`](../../Source/DungeonForged/Public/Camera/UDFLockOnComponent.h) — soft lock + Q/E cycle
- [`UDFCameraComponent`](../../Source/DungeonForged/Public/Camera/UDFCameraComponent.h) — Default 400 / Combat 300 / LockOn 350 arm length

---

## 2. Diagnóstico por domínio

### 2.1 Combo, input buffer, fluidez

**Pontos fortes (já tem):**
- Buffer em dois níveis: `bSwingInputBuffered` (durante swing) + `bComboInputBuffered` (durante combo window). Ambos sobrevivem a transições entre seções de montage.
- Combo data-driven via [`FDFComboStep`](../../Source/DungeonForged/Public/Data/DFDataTableStructs.h) (light + heavy finisher branch).
- Heavy attack em 3 tiers (tap = 2.2×, max charge 1.4s = 3.5× dmg).
- Variantes direcionais resolvidas em [`UDFComboComponent::ResolveDirectionalComboMontage`](../../Source/DungeonForged/Private/Combat/UDFComboComponent.cpp).
- Cancel window via `UANS_DFCancelWindow` adiciona tag `State.Combat.CancelWindow.Open` — heavy gateia, dodge sempre passa.

**Gaps AAA concretos:**

| # | Gap | Referência AAA | Impacto | Status |
|---|-----|----------------|---------|--------|
| C1 | **Input buffer = 150ms.** Sekiro/GoW usam 200–250ms. | Sekiro = 220ms; GoW Ragnarok = 200ms | ALTO | ✅ 0.20s via `UDFCombatTuningData` / component |
| C2 | **Buffer não pausa durante hitstop.** | DMC5/Sekiro pausam buffer durante hit-lag | MÉDIO | ✅ `IsHitStopActive()` em `IsInputBufferExpired` |
| C3 | **Combo não refresca on-hit.** | GoW/DMC5 estendem combo se acerta | ALTO | ✅ `NotifyOwnerHitConfirmed(+0.30s)` |
| C4 | **Directional combos por velocity, não stick.** | Sekiro lê stick direto | MÉDIO | ✅ `MovementInputVector` do CMC |
| C5 | **Sem "commit grade".** | DS/GoW no-return frames | ALTO | ✅ `UANS_DFNoCancelWindow` · ⚠️ notifies nas montages |
| C6 | **Blend combo hardcoded 0.08s/0.0s.** | AAA 120–200ms tunável | MÉDIO | ✅ `FDFComboStep::ChainBlendInTime` + component default |
| C7 | **Bug replicação `bComboChainAdvancePending`.** | — | ALTO MP | ✅ `UPROPERTY(Replicated)` + `GetLifetimeReplicatedProps` |

**Validação do bug C7:** ✅ **Resolvido** — ver `UDFComboComponent::GetLifetimeReplicatedProps`.

---

### 2.2 Detecção de hit, hitbox, dano

**Pontos fortes:**
- Server-authoritative (`bServerOnlyTraces = true`).
- Multi-sphere sweep por tick com fallback hand+forward se sockets ficarem stale (>350cm do owner).
- Dedup por swing via `HitActorsThisSwing` (TWeakObjectPtr).
- Override per-swing de damage/knockback (`SetBaseDamageForNextSwing`).
- Hit reactions direcionais (front/back/left/right via dot 2D) e por banda (Light/Heavy/Knockback) + variantes por `Damage.Source.*` tag.
- Stagger com sliding window 3s + threshold + cooldown (4.5s) — bem desenhado.

**Gaps AAA concretos:**

| # | Gap | Impacto | Status |
|---|-----|---------|--------|
| H1 | **Sem interpolação de trace entre ticks.** | ALTO em ataques curtos | ✅ `TraceSubStepCount` (B1) |
| H2 | **Sphere única por swing.** | ALTO | ✅ Capsule/Cone + `TraceShapeByWeaponTag` · ⚠️ preencher `DA_CombatTuning` / DT_Items |
| H3 | **Sem multi-hitbox.** | MÉDIO | ✅ `ExtraTraceZones` (B3) |
| H4 | **Sem body-part-specific reactions.** | MÉDIO | ✅ `BoneHitMontages` · ⚠️ mapa no BP inimigo |
| H5 | **Projectile Parity Gap.** | **CRÍTICO** | ✅ `DispatchProjectileHitConfirmed` + hit reaction |
| H6 | **Sem dedup de tempo em projétil.** | BAIXO | ✅ `UDFProjectileHitTrackerComponent` |
| H7 | **Sem damage-source tagging consistente em projétil.** | BAIXO | ✅ SetByCaller + tags dinâmicas |

**Evidência H5:** ✅ Projéteis passam por `UDFCombatFeedbackLibrary::DispatchProjectileHitConfirmed` → `DispatchOnHitConfirmed`.

---

### 2.3 GAS — abilities, attributes, effects

**Pontos fortes:**
- AttributeSet limpo: 3 vitals + 6 primárias + 2 mitigação + 2 secondary offensive + utility — todas replicadas com REPNOTIFY_Always e clamps em `PreAttributeChange`.
- SecondWind rescue mechanic em `PostAttributeChange` (resgata em 25% HP se tag `State.Universal.SecondWindAvailable` ativa).
- Damage pipeline tem **single source of truth**: `UDFDamageCalculation` (Execution).
- Tag taxonomy excelente: `Ability.*`, `State.*`, `Event.*`, `Effect.*`, `Data.*` — hierárquica e consistente.
- 34+ abilities autorais bem diferenciadas por classe (Warrior melee/CC, Mage range/haste, Rogue mobility/DoT). Cada uma tem `CanActivateAbility` próprio + traces customizados.
- Passives auto-grant + auto-activate via `OnGiveAbility`. NetExecutionPolicy = ServerOnly nelas (corretíssimo).
- Elemental reactions (Melt / Electrocute / Steam) com affinity matrix por inimigo + GE optional.

**Gaps AAA concretos:**

| # | Gap | Impacto | Status |
|---|-----|---------|--------|
| G1 | **`CooldownReduction` nunca aplicado.** | ALTO | ✅ `UDFGameplayAbility::ApplyCooldown` |
| G2 | **Sem Global Cooldown (GCD).** | MÉDIO | ✅ `UDFAbilityGlobalCooldownSubsystem` (opt-in) |
| G3 | **Sem Status Resist / Tenacity.** | MÉDIO | ✅ attribute + `UDFGEComponent_StatusResistDuration` |
| G4 | **Sem Lifesteal / SpellVamp.** | MÉDIO | ✅ `UDFAttributeSet` + `DFDamageCalculation` |
| G5 | **Sem Dodge%/Block%.** | BAIXO | ✅ attributes + roll em damage calc |
| G6 | **Damage event scattered.** | MÉDIO | ✅ `DispatchOnHitConfirmed` + `UDFCombatEventsLibrary::BroadcastDamageDealt` |
| G7 | **Sem rollback de prediction.** | BAIXO | ⚠️ `Client_NotifyAbilityActivationRejected` (stop montage; sem refund de resource) |
| G8 | **Ability cancel windows cross-ability.** | MÉDIO | ✅ `UANS_DFAbilityCancelWindow` · ⚠️ notifies nas montages |

**Sugestão G1:** ✅ **Aplicado** — ver `UDFGameplayAbility.cpp::ApplyCooldown`.

---

### 2.4 Game feel — juice, câmera, motion warping, lock-on

**Pontos fortes (impressionante para projeto solo):**
- `UDFHitStopSubsystem` com **real-world time** (FPlatformTime, immune à time dilation) — 4 bandas (Light 0.06s / Heavy 0.10s / Critical 0.14s / BossSlam 0.20s) com exclusão de actor.
- 4 camera shakes (LightHit, HeavyHit, BossSlam, Explosion) com playback scale + accessibility intensity.
- `UDFScreenEffectsComponent` com vignette + chromatic + flash + saturation + grain + blur + death slowmo + low-health pulse.
- Motion warping plenamente integrado: `UDFMeleeAimComponent` resolve target (ManualTarget > LockOn > AI BB > soft cone) e `UANS_DFMeleeWarp` aplica warp via `UMotionWarpingComponent`.
- 4 AnimNotifyStates de combate (`Cancel`, `Parry`, `MeleeWarp`, `EnemyTelegraph`) cobrindo o ciclo windup → impact → recovery.
- Lock-on com Q/E cycle, soft search 1500cm/60°, smooth camera lag (0.12s).
- Trail VFX com prune por tag (`WeaponTrailVFX`).
- Combat text pool de 30, abreviação k/M, crit escala 1.4×.

**Gaps AAA concretos:**

| # | Gap | Impacto | Status |
|---|-----|---------|--------|
| F1 | **Feedback fragmentado.** | **CRÍTICO** | ✅ `DispatchOnHitConfirmed` + `FDFHitConfirmedContext` |
| F2 | **HitStop não escala com magnitude.** | MÉDIO | ✅ `MagFactor` em `PlayBand` (A3) |
| F3 | **Sem dodge juice.** | ALTO | ✅ `ApplyDodgeJuice` + FOV punch |
| F4 | **Sem parry shake catalog.** | MÉDIO | ✅ `UDFCameraShake_ParrySuccess` |
| F5 | **Lock-on sem Z-anchor aéreo.** | BAIXO | ✅ `UDFCameraComponent::ResolveLockOnAimPoint` (B10) |
| F6 | **Camera sem FOV punch.** | MÉDIO | ✅ dodge + spectacle FOV |
| F7 | **Trail VFX não pooled.** | BAIXO | ✅ `UDFWeaponTrailPoolComponent` · ⚠️ assign no BP player |
| F8 | **Sem attack-type tag em hitstop/shake.** | MÉDIO | ✅ tags `Impact.*` + `ImpactFeedbackByTag` · ⚠️ preencher DA |
| F9 | **Lag vignette vs hitstop.** | BAIXO | ✅ sync via `GetHitStopRemainingSeconds` |
| F10 | **Sem finisher cinematic chain.** | MÉDIO | ✅ `UDFAbility_Warrior_Execute` QTE multi-hit · ⚠️ montages/HUD |
| F11 | **Sem on-kill spectacle.** | MÉDIO | ✅ `UDFCombatSpectacleSubsystem` + bloom/FOV (B8) |

**Para F1:** ✅ **Implementado** — ver `UDFCombatFeedbackLibrary::DispatchOnHitConfirmed` e `Effect.Combat.FeedbackCentralized` (evita combat text duplicado no AttributeSet).

---

### 2.5 Animação — notifies, montages, layers

**Pontos fortes:**
- 4 AnimNotifyStates de combate bem desenhadas, com side-effects via GAS events e loose tags. Replicação automática por anim system.
- 4 AnimNotifies não-state (`AN_ComboWindowOpen`, `AN_TraceStart`, `AN_TraceEnd`, `AN_SendGameplayEvent`) com fallback de timer no [`UDFMeleeTraceComponent::ScheduleAuthorityTraceWindowsFromMontage`](../../Source/DungeonForged/Private/Combat/UDFMeleeTraceComponent.cpp) — robusto contra LocalPredicted ability não disparar notifies no server.
- Armed/unarmed layers per-weapon ([`Player_Armed_Unarmed_Layers.md`](../animation/Player_Armed_Unarmed_Layers.md)).
- Footstep notify com surface detection.
- Trail VFX notify com auto-prune.
- Death pose lock pattern (já confirmado em [`feedback_death_pose_locking.md`](../../memory/feedback_death_pose_locking.md)).

**Gaps AAA concretos:**

| # | Gap | Impacto | Status |
|---|-----|---------|--------|
| A1 | **Sem `AN_DodgeCancelWindow`.** | BAIXO (escolha) | ❌ design intencional — dodge não gated |
| A2 | **Sem `AN_AbilityCancelWindow` cross-ability.** | MÉDIO | ✅ `UANS_DFAbilityCancelWindow` · ⚠️ montages |
| A3 | **Sem `AN_HitConfirm`.** | BAIXO | ✅ `AN_HitConfirm` (B14) |
| A4 | **Sem `AN_RootMotionScaleOverride`.** | BAIXO | ✅ `UAN_RootMotionScaleOverride` |
| A5 | **Blend combo hardcoded.** | MÉDIO | ✅ `FDFComboStep::ChainBlendInTime` (C6) |

---

### 2.6 Replicação — server authority, prediction, anti-cheat surface

**Pontos fortes:**
- `bServerOnlyTraces = true` no melee — exploits client impossíveis.
- ASC em PlayerState (Mixed mode); em Enemy (Minimal). Correto para action ARPG.
- Loose tags do ANS (Cancel, Parry, Telegraph) são locais — não tem custo de replication.
- GAS events via `SendGameplayEventToActor` propagam corretamente.
- `Client_HitFeedback` no `ADFPlayerCharacter` para feedback local em co-op.
- Boss `Multicast_BossLocalAttackFX` com inner/outer radius — atenuação correta.
- Projéteis com `HasAuthority()` em todos os `OnHit`.
- Stagger `bServerAuthoritative = true`.
- HitReaction `if (!GetOwner()->HasAuthority()) return` no `OnHitReceived`.

**Gaps AAA concretos:**

| # | Gap | Impacto | Status |
|---|-----|---------|--------|
| N1 | **Bug C7** (`bComboChainAdvancePending`). | ALTO MP | ✅ (= C7) |
| N2 | **Sem client prediction de hit feedback.** | MÉDIO | ⚠️ `bClientPredictHitFeel` overlap local (B9 básico) |
| N3 | **Sem rollback de Resource cost.** | BAIXO | ⚠️ (= G7 parcial) |
| N4 | **`bComboHeavyFinisherPending` não replicado.** | BAIXO | ✅ `UPROPERTY(Replicated)` |
| N5 | **Random event co-op "first lock-in wins".** | MÉDIO co-op | ❌ fora de escopo combate |

---

## 3. Prioridade — matriz impacto × esforço

> Ordenado por **impacto/horas**. Cada linha referencia o gap ID das seções 2.x.

### Tier S — ganho percebido enorme, esforço baixo

| # | Ação | Gaps | Status |
|---|------|------|--------|
| S1 | **Fix C7** — `bComboChainAdvancePending` replicado | C7 | ✅ |
| S2 | **Aplicar CooldownReduction** com cap 0.4 + DR | G1 | ✅ |
| S3 | **Input buffer 0.15 → 0.20s** | C1 | ✅ |
| S4 | **Combo refresh on-hit** (+0.30s) | C3 | ✅ |
| S5 | **Dodge juice** — chromatic + flash + shake + FOV | F3, F6 | ✅ |
| S6 | **Buffer pause em hitstop** | C2 | ✅ |

**Tier S: ✅ 6/6 concluído em C++.**

### Tier A — alto impacto, esforço médio

| # | Ação | Gaps | Status |
|---|------|------|--------|
| A1 | **Centralizar `OnHitConfirmed`** | F1, G6 | ✅ · ⚠️ preencher `ImpactFeedbackByTag` no DA |
| A2 | **Projectile Parity** | H5, H6, H7 | ✅ |
| A3 | **Damage-magnitude HitStop scaling** | F2 | ✅ |
| A4 | **Attack-type tags em HitStop/Shake** | F8, A3 | ✅ · ⚠️ assets VFX/SFX por tag |
| A5 | **`AN_AbilityCancelWindow` genérico** | G8, A2 | ✅ · ⚠️ notifies nas montages |
| A6 | **Directional input por stick** | C4 | ✅ |
| A7 | **Commit-grade nas montages** | C5 | ✅ · ⚠️ `UANS_DFNoCancelWindow` nas montages |
| A8 | **Status Resist attribute + DR para CC** | G3 | ✅ |

**Tier A: ✅ 8/8 concluído em C++** — polish percebido depende de config no editor.

### Tier B — esforço maior, impacto especializado

| # | Ação | Gaps | Status |
|---|------|------|--------|
| B1 | **Trace interpolation** (sub-stepping) | H1 | ✅ |
| B2 | **Per-weapon trace shape** (capsule/cone) | H2 | ✅ · ⚠️ `TraceShapeByWeaponTag` + DT_Items |
| B3 | **Multi-hitbox por swing** | H3 | ✅ |
| B4 | **Body-part-specific reactions** | H4 | ✅ · ⚠️ `BoneHitMontages` no BP inimigo |
| B5 | **Stagger DR + per-attack tag multipliers** | — | ✅ |
| B6 | **Passive poise regen** | — | ✅ |
| B7 | **Finisher cinematic chain** (Execute QTE) | F10 | ✅ · ⚠️ montages + HUD prompt |
| B8 | **On-kill spectacle** (room clear) | F11 | ✅ |
| B9 | **Client prediction de hit feedback** | N2 | ⚠️ básico (`bClientPredictHitFeel`) |
| B10 | **Lock-on Z-anchor** aéreo | F5 | ✅ |
| B11 | **GCD layer** opcional | G2 | ✅ |
| B12 | **Lifesteal / Dodge% / Block%** | G4, G5 | ✅ |
| B13 | **Trail VFX pool** | F7 | ✅ · ⚠️ assign no BP player |
| B14 | **Per-attack `AN_HitConfirm`** | A3 | ✅ · ⚠️ notifies nas montages |

**Tier B: ✅ 13/14 em C++** — B9 é versão básica; polish completo de predição fica como melhoria futura.

---

## 4. Plano de execução — status e validação

**Implementação C++:** ✅ Tiers S, A e B (exceto B9 parcial) concluídos.

**Próximos passos (editor + playtest):** ver [`10_CombatBlueprintSetup.md`](../improvements/10_CombatBlueprintSetup.md).

**Critério de "AAA feel" alcançado:**

| Critério | C++ | Validação |
|----------|-----|-----------|
| Hit-confirmation latency < 50ms (LogDFFeel) | ✅ pipeline centralizado | ❌ medir em playtest |
| 100% hits (melee + projétil + AoE) pelo mesmo caminho | ✅ `DispatchOnHitConfirmed` | ❌ confirmar com `-log LogDFFeel Verbose` |
| Dodge evasivo (FOV + chromatic + shake) | ✅ `ApplyDodgeJuice` | ❌ playtest subjetivo |
| Combo refresh on-hit (+0.3s) | ✅ | ❌ playtest |
| Buffer 0.20s + pause em hitstop | ✅ | ❌ playtest |
| Cancel-into-ability (≥5 abilities) | ✅ notify existe | ⚠️ falta colocar notifies nas montages |
| Projétil = melee em hit reaction + juice | ✅ | ❌ playtest |
| Crit visualmente distinto | ✅ | ❌ playtest |
| Net test (PktLag=120): combo sem double-activate | ✅ C7/N4 fix | ❌ §5.3 |

---

## 5. Validação contínua

### 5.1 Telemetria — adicionar `LogDFFeel`

Já existe `LogDFTuning`; adicionar `LogDFFeel` para auditar dispatch de juice:

```cpp
// DungeonForgedLog.h
DECLARE_LOG_CATEGORY_EXTERN(LogDFFeel, Log, All);

// Em DispatchOnHitConfirmed (depois do A1):
UE_LOG(LogDFFeel, Verbose,
       TEXT("[Hit] Band=%s Mag=%.1f Crit=%d Inst=%s Vic=%s Loc=%s Tags=[%s]"),
       *UEnum::GetValueAsString(Ctx.Band), Ctx.Magnitude, Ctx.bIsCrit,
       *GetNameSafe(Ctx.Instigator), *GetNameSafe(Ctx.Victim),
       *Ctx.Location.ToString(), *Ctx.Tags.ToStringSimple());
```

Playtest gravado com `-log LogDFFeel Verbose -LogDFTuning Verbose -LogGameplayCues Verbose` produz timeline auditável: pegar 60s de gameplay, calcular:
- Hits totais
- % com HitStop disparado (alvo: 100%)
- % com Camera Shake (alvo: ≥ 90%)
- Latency média input → primeiro log de feedback (alvo: < 50ms)

### 5.2 Cenário de teste — `L_CombatRange`

**Status: ❌ pendente** — level ainda não criado.

Documentado em [`00_Overview.md §C`](../improvements/00_Overview.md) e [`10_CombatBlueprintSetup.md`](../improvements/10_CombatBlueprintSetup.md):
- 3 training dummies HP infinito
- 1 elite dummy
- 1 boss dummy
- Console buttons: spawn N grunts, gold N, floor N, give all abilities, heal full, toggle inf stamina
- HUD com FPS + frame time + active montages + buffer state

Acessível via `open L_CombatRange` — iteração de feel em 1 minuto, não em uma run completa.

### 5.3 Network testing

**Status: ❌ pendente** — executar após config BP e `L_CombatRange`.

- Listen Server + Client com `Net PktLag=120` `Net PktLagVariance=20`:
  - Combo chain LP → SLP (server confirma) sem double-activate.
  - Aim snap acontece no client primeiro (LocalPredicted) e server confirma.
  - HitStop é local-only (cliente A em hitstop não congela cliente B).
- Dedicated Server smoke: 4 players atacando em sequência, sem desync de cooldown.

---

## 6. Apêndice — Referência de implementação (aplicado)

> Os diffs abaixo descrevem o que foi **implementado** no codebase. Servem como referência histórica, não como patches pendentes.

### 6.1 Fix do bug C7 (replicação de combo)

```cpp
// UDFComboComponent.h — linha ~39
UPROPERTY(Replicated)
bool bComboChainAdvancePending = false;

// UDFComboComponent.cpp — adicionar em GetLifetimeReplicatedProps (criar se não existe)
void UDFComboComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UDFComboComponent, bComboChainAdvancePending, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UDFComboComponent, LockedComboActivationStep, COND_OwnerOnly);
}
```

### 6.2 Cooldown Reduction aplicado (G1)

```cpp
// UDFGameplayAbility.cpp — em ApplyCooldown(), antes do GE spec:
float EffectiveCooldown = BaseCooldown;
if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
{
    if (const UDFAttributeSet* AS = ASC->GetSet<UDFAttributeSet>())
    {
        const float CDR = FMath::Clamp(AS->GetCooldownReduction(), 0.f, 1.f);
        const float Cap = 0.4f;
        const float Hard = FMath::Min(CDR, Cap);
        const float Soft = (CDR > Cap)
            ? ((CDR - Cap) / ((CDR - Cap) + 0.6f) * 0.1f)
            : 0.f;
        EffectiveCooldown = BaseCooldown * (1.f - (Hard + Soft));
    }
}
// Aplica EffectiveCooldown como SetByCaller Data.Cooldown
```

### 6.3 Combo refresh on-hit (S4)

```cpp
// UDFComboComponent.cpp — adicionar método público
void UDFComboComponent::NotifyOwnerHitConfirmed(float ExtensionSeconds /*= 0.30f*/)
{
    if (!bComboWindowActive) return;
    if (UWorld* W = GetWorld())
    {
        const float NewExpire = W->GetTimeSeconds() + ExtensionSeconds;
        if (NewExpire > ComboWindowExpireTime)
        {
            ComboWindowExpireTime = NewExpire;
            UE_LOG(LogDFFeel, Verbose, TEXT("[Combo] Refresh on-hit +%.2fs"), ExtensionSeconds);
        }
    }
}

// UDFMeleeTraceComponent::ApplyDamageToTarget — após bAppliedDamage = true:
if (Owner && bAppliedDamage)
{
    if (UDFComboComponent* Combo = Owner->FindComponentByClass<UDFComboComponent>())
    {
        Combo->NotifyOwnerHitConfirmed(ComboRefreshOnHitSeconds /*UPROPERTY default 0.30f*/);
    }
}
```

### 6.4 OnHitConfirmed central (A1) — esqueleto

```cpp
// UDFCombatFeedbackLibrary.h
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFHitConfirmedContext
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<AActor> Instigator;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<AActor> Victim;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector Location = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector Normal = FVector::UpVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector HitDirection2D = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Magnitude = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float DamagePercent = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIsCrit = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTagContainer Tags;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EDFHitFeedbackBand Band = EDFHitFeedbackBand::Light;
};

UFUNCTION(BlueprintCallable, Category="DF|Feel", meta=(WorldContext="WorldContext"))
static void DispatchOnHitConfirmed(UObject* WorldContext, const FDFHitConfirmedContext& Ctx);
```

```cpp
// UDFCombatFeedbackLibrary.cpp — esqueleto da implementação
void UDFCombatFeedbackLibrary::DispatchOnHitConfirmed(
    UObject* WorldContext, const FDFHitConfirmedContext& Ctx)
{
    if (!WorldContext) return;
    UWorld* W = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
    if (!W) return;

    // 1. HitStop com magnitude scaling
    if (UDFHitStopSubsystem* HS = W->GetGameInstance()->GetSubsystem<UDFHitStopSubsystem>())
    {
        const float MagFactor = FMath::Clamp(Ctx.DamagePercent / 0.15f, 0.5f, 1.5f);
        HS->RequestHitStop(Ctx.Band, Ctx.Instigator, MagFactor);
    }

    // 2. Camera shake por banda + atenuação radial
    UDFCameraShakeFunctionLibrary::PlayBandShake(WorldContext, Ctx.Band, Ctx.Instigator);

    // 3. Screen effects no victim
    if (Ctx.Victim)
    {
        if (UDFScreenEffectsComponent* FX = Ctx.Victim->FindComponentByClass<UDFScreenEffectsComponent>())
        {
            FX->ApplyHitFromCombat(Ctx.Band, Ctx.DamagePercent, Ctx.Instigator);
            if (Ctx.bIsCrit) FX->ChromaticAberrationPulse(0.10f, 1.2f);
        }
    }

    // 4. Niagara impact por tag
    SpawnHitImpactByTags(W, Ctx);

    // 5. SFX layer (impact + tail + crit sting opcional)
    PlayHitSoundByTags(W, Ctx);

    // 6. Combat text
    if (UDFCombatTextSubsystem* CT = W->GetSubsystem<UDFCombatTextSubsystem>())
    {
        const ECombatTextType T = Ctx.bIsCrit ? ECombatTextType::Crit : ECombatTextType::Damage;
        CT->SpawnFloatingText(Ctx.Location + FVector(0,0,120), Ctx.Magnitude, T, Ctx.HitDirection2D);
    }

    UE_LOG(LogDFFeel, Verbose,
        TEXT("[OnHit] Band=%s Mag=%.1f Crit=%d Tags=[%s]"),
        *UEnum::GetValueAsString(Ctx.Band), Ctx.Magnitude, Ctx.bIsCrit,
        *Ctx.Tags.ToStringSimple());
}
```

E refatorar todas as chamadas atuais para usar essa função única — **✅ mapping concluído:**

| Local | Status |
|-------|--------|
| `UDFMeleeTraceComponent::ApplyDamageToTarget` | ✅ |
| `ADFKnifeProjectile::OnHit` | ✅ |
| `DFFireballProjectile::OnHit` | ✅ |
| `DFFrostBoltProjectile::OnHit` | ✅ |
| `DFArcaneMissileProjectile::OnHit` | ✅ |
| `DFBlizzardZone::TickDamage` | ✅ |

---

## 7. Conclusão

DungeonForged **implementou em C++** a fundação de combate AAA descrita neste relatório:

| Tier | Itens | Status C++ |
|------|-------|------------|
| S | 6 | ✅ 6/6 |
| A | 8 | ✅ 8/8 |
| B | 14 | ✅ 13/14 (B9 parcial) |

**Posição atual: ~9/10 em engenharia.** O pipeline está centralizado (`DispatchOnHitConfirmed`), projéteis têm paridade com melee, input-feel refinado (buffer, hitstop pause, refresh on-hit, stick), bugs de replicação corrigidos (C7, N4), e polish de dodge/cooldown/status/finisher/spectacle implementados.

**O que separa o jogo da meta percebida pelo jogador:**

1. **Configuração no editor** — `DA_CombatTuning`, notifies nas montages, `FinisherHitMontages`, HUD de finisher ([`10_CombatBlueprintSetup.md`](../improvements/10_CombatBlueprintSetup.md))
2. **Validação** — playtest com checklist §4, `L_CombatRange` (§5.2), net sim §5.3
3. **Melhorias futuras opcionais** — B9 predição completa, rollback de resource (G7/N3), co-op random events (N5)

> O maior risco agora é **não validar** o feel após a implementação C++. Gravar 2 min de gameplay antes/depois e rodar `Net PktLag=120` fecha o ciclo de confiança.

---

## 8. Próximos documentos sugeridos

- ~~`docs/analysis/OnHitConfirmed_Migration.md`~~ — **obsoleto** (migração concluída)
- [`docs/improvements/10_CombatBlueprintSetup.md`](../improvements/10_CombatBlueprintSetup.md) — **ativo** — checklist de config BP/editor
- `docs/analysis/Combat_NetSim_Plan.md` — plano de testes com `Net PktLag` (§5.3)
- `docs/improvements/11_FinisherSystem.md` — design completo da cinematic finisher (B7 assets)
- `docs/analysis/Game_Balance_Tuning.md` — números pós-CooldownReduction para todas as abilities
