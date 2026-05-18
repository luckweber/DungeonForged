# 09 — Status de implementação (C++)

> **Atualizado:** 2026-05-18  
> Complementa o [índice](00_Overview.md). Marca o que já está no código vs. o que ainda depende de BP/assets.

---

## Legenda

| Estado | Significado |
|--------|-------------|
| ✅ | Implementado em C++ (pode precisar tuning no editor) |
| 🟡 | Parcial / infra pronta, falta BP ou conteúdo |
| ⬜ | Não implementado |
| 📦 | Só conteúdo (Niagara, som, montage, UMG) |

---

## Top 12 do Overview

| # | Ação | Estado | Notas |
|---|------|--------|-------|
| 1 | Scaling inimigos por andar | ✅ | `ADFEnemyBase::ApplyBaseStatsFromRow` — `HP/Dmg × (1+0.15×Floor) × DM`; Elite +2.5× HP, +1.5× dmg |
| 2 | Combo window 0.45s | ✅ | `UDFComboComponent::ComboWindowDuration` |
| 3 | Hit stop por banda no melee | ✅ | `UDFMeleeTraceComponent::ApplyDamageToTarget` → `UDFHitStopSubsystem` + shake leve |
| 4 | Dodge i-frames 0.35 + CD 0.7 | ✅ | `UDFCharacterMovementComponent` |
| 5 | Heavy attack (charge) | ✅ | `UDFComboComponent` hold ≥0.55s; `UDFMeleeTraceComponent::ConfigureHeavySwing` |
| 6 | Curva eventos 25/40/60/0 | ✅ | `UDFRandomEventSubsystem::EventChancePerFloorCurve` + `GetEventChanceForFloor` |
| 7 | Indicador dano direcional | ✅ | `UDFDamageDirectionWidget` + `UDFInGameHUDWidget::OnPlayerDamageTaken` |
| 8 | HUD adaptativo | ✅ | `UDFInGameHUDWidget` fade via `State.InCombat` |
| 9 | Elite music auto | ✅ | `Multicast_NotifyEliteEngaged` ao primeiro hit do jogador em Elite |
| 10 | Sliders acessibilidade | ✅ | `UDFOptionsScreenWidget`: `Slider_CameraShakeIntensity`, `Slider_HitStopIntensity`, `Check_ShowDamageNumbers` |
| 11 | Boss vulnerability +50% dmg | ✅ | `ADFBossBase::BeginBossVulnerabilityWindow` + `DFDamageCalculation` ×1.5 |
| 12 | Combat text crit polish | ✅ | Abbrev k/M, escala 1.4×, shake horizontal no crit |

---

## Por documento

### 01 — Game Feel

| Item | Estado |
|------|--------|
| Combo window 0.45 | ✅ |
| Dodge i-frames / cooldown | ✅ |
| Dodge chromatic + shake | ⬜ 📦 |
| Camera lag combate / sprint FOV | ⬜ |
| Landing impact | ⬜ 📦 |
| Input buffer 0.15s | ✅ | `UDFComboComponent::AttackInputBufferDuration` |
| Footstep sound library central | 🟡 (`UDFAnimNotify_FootStep` com superfície) |

### 02 — Juice

| Item | Estado |
|------|--------|
| Hit stop no **atacante** (melee) | ✅ |
| Hit stop na vítima (player) | ✅ (`UDFHitReactionComponent`) |
| `UDFCombatFeedbackLibrary` central | ✅ |
| Camera shakes extra (dodge, sprint, landing…) | ⬜ 📦 |
| Screen FX (boss intro, second wind…) | 🟡 |
| Combat text cluster / drift | ⬜ |
| Niagara hit impact por tipo | ⬜ 📦 |
| `LogDFTuning` | ✅ |

### 03 — Combat

| Item | Estado |
|------|--------|
| Combo window | ✅ |
| Heavy attack (hold LMB, 2.2× dmg, +20 trace, 15 stamina) | ✅ |
| Armor DR `Armor/(Armor+K)` | ✅ |
| Boss vulnerable damage | ✅ |
| CDR / crit DR caps | ✅ | CDR max 0.4, crit chance max 0.75 |
| Stamina regen in/out combat | ✅ | `UDFMMC_StaminaRegen` + `State.InCombat` |
| `State.Exhausted` | ✅ | `UDFStaminaExhaustionComponent` |
| Elemental reactions | 🟡 (`UDFElementalReactionSubsystem`) |

### 04 — Run Mechanics

| Item | Estado |
|------|--------|
| Enemy floor scaling | ✅ |
| Event chance curve | ✅ |
| Defeat cause string | ✅ (`ADFRunGameMode::TriggerDefeat`) |
| Death cause / recap | 🟡 |
| Room types / shrine rotation | ⬜ |
| World gold pickups | ⬜ (gold via `UDFRunManager` no kill) |
| Checkpoints | ✅ |

### 05 — Enemies & Bosses

| Item | Estado |
|------|--------|
| Death pose lock + GAS death | ✅ |
| Dissolve on death (material swap) | ✅ |
| Elite stat + music | ✅ |
| `EDFEnemyArchetype` + BT por arquétipo | 🟡 | Enum + DT row; BTs reusáveis = conteúdo |
| Combat director (max 2 attackers) | ✅ | `UDFCombatDirectorSubsystem` + enemy melee token |
| Boss phases | ✅ (`ADFBossBase`) |
| `State.BossVulnerable` na fase | ✅ |

### 06 — Audio Mix

| Item | Estado |
|------|--------|
| Music states Exploration/Combat/Elite/Boss | ✅ |
| Elite auto-trigger | ✅ |
| CombatLow / CombatHigh split | ✅ | Volume combat layer por contagem de inimigos próximos |
| SFX 4-tier por dano | ⬜ 📦 |
| Submix sliders no menu | 🟡 (volumes em settings, routing = config) |

### 07 — UI / UX

| Item | Estado |
|------|--------|
| HUD fade combate | ✅ |
| Damage direction indicator | ✅ |
| Defeat screen cause | ✅ (`CauseText` em `UDFDefeatScreenWidget`) |
| Boss intro sequence | ⬜ 📦 |

### 08 — Accessibility

| Item | Estado |
|------|--------|
| `bReduceMotion` | ✅ |
| Camera shake / hit stop intensity sliders | ✅ |
| `bShowDamageNumbers` | ✅ |
| Colorblind palette swap | 🟡 |
| Hold/toggle abilities | ⬜ |

---

## Melhorias extras (não estavam nos docs)

| Item | Onde | Estado |
|------|------|--------|
| Morte espúria ao spawn (`bDeathDetectionArmed`) | `ADFEnemyBase` | ✅ |
| Backup destroy se death GA falha | `ScheduleDeathDestroyBackup` sempre | ✅ |
| Loot idempotente | `bDeathLootSpawned` | ✅ |
| Pose lock tick disable | `DFDeathAnimation::LockDeathPoseOnMesh` | ✅ ([PR #1](https://github.com/luckweber/DungeonForged/pull/1)) |
| XP escala com andar no kill | `ADFEnemyBase::HandleServerDeath` | ✅ |
| Finishing blow abaixo % HP | `UDFMeleeTraceComponent` | ✅ |
| Dual melee path (montage vs GAS) | `UDFComboComponent` | 🟡 — documentar em 03 |

---

## Próximos passos recomendados

1. **`DA_CombatTuning`** — criar asset e apontar em `DFAssetManager` → `CombatTuningDataAsset`.
2. **WBP** — adicionar widgets opcionais: `Panel_FadeableHUD`, `DamageDirection` (4 `UImage`), sliders de acessibilidade.
3. **`L_TuningRange`** — mapa de teste de feel.
4. **Combat director / archetypes** — ver [05](05_EnemiesAndBosses.md).
5. **BTs por arquétipo** + **`L_TuningRange`** — ver [05](05_EnemiesAndBosses.md).

---

## Testar rapidamente

```text
df.EnemyDeathDebug 1
-log LogDFTuning Verbose
```

- Matar inimigos: hit stop no atacante, combo mais apertado, HP escala por andar.
- Elite: música muda no primeiro hit.
- Derrota: texto de causa no defeat screen (se o widget usar o parâmetro).
- Eventos: probabilidade depende do andar (chamar `ShouldTriggerEvent(Floor)` no fluxo de transição de andar).
