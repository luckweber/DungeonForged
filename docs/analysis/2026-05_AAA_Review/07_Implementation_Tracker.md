# 07 — AAA Review Implementation Tracker

> Atualizado: 2026-05-28. Marca o status de cada recomendação da suíte
> [`2026-05_AAA_Review`](00_Index.md). Legenda: ✅ feito · ⚠️ parcial · ❌ pendente · 🎨 requer editor/BP.

---

## Top 15

| # | Item | Status | Notas |
|---|---|---|---|
| 1 | Elemental no pipeline de dano | ✅ | `UDFElementalLibrary::ApplyOutgoingDamageSpecToTarget` |
| 2 | Multicast via Player 0 | ✅ | `UDFNetworkLibrary::MulticastToAllDFPlayerControllers` |
| 3 | Targeting multiplayer IA | ✅ | `UDFAILibrary` + **threat table** (`UDFAIThreatComponent`) |
| 4 | Telegraphs de boss replicados | ✅ | Multicast + lifespan em decals |
| 5 | Loop between-floor | ✅ | `UDFBetweenFloorSubsystem` |
| 6 | Checkpoint restore / Continue | ✅ | `LoadRunFromCheckpoint`, `ApplyRunStateToPlayer` |
| 7 | Bug `bCanTelegraph` | ✅ | Removido de `UpdateTarget` |
| 8 | Meta level + RunHistory | ✅ | `ProcessMetaLevelUps`, writers fim de run |
| 9 | Archetypes ativos C++ | ✅ | flee, spacing, abilities, blackboard |
| 10 | HUD root único | ✅ | `UDFRunHUDRootWidget` + `ADFRunHUD` |
| 11 | Mix de áudio | ⚠️ | `ApplyAudioVolumes` C++; 🎨 SoundClasses no editor |
| 12 | Primary Asset bundles | ✅ | `UDFAssetLoaderSubsystem` bundle + unload por andar |
| 13 | Poise/stagger/juggle unificado | ✅ | `UDFCombatCrowdControlComponent` |
| 14 | Significance Manager | ✅ | `UDFEnemySignificanceComponent` (distância → anim tick) |
| 15 | Acessibilidade | ⚠️ | HC/blur/propagate C++; damage numbers/TTS incompletos |

---

## 01 — Arquitetura

| Item | Status |
|---|---|
| Multicast Player 0 | ✅ |
| Save snapshot restore | ✅ |
| Duplicações §3.3 (ability roll, lock-on, HUD, floor travel, save IO) | ⚠️ ability roll unificado; save IO via `UDFSaveLibrary` |
| Sistemas dormentes §3.4 | ✅ |
| RNG determinístico combate | ✅ `CombatProcRoll` + combo variant replicate |
| Suite testes save v6 | ❌ |
| Branding rename | 🎨 `DefaultGame.ini` |
| Confirmar BP_DFGameInstance | 🎨 editor |

---

## 02 — Combate & GAS

| Item | Status |
|---|---|
| Meta-attribute IncomingDamage | ✅ |
| Elemental wiring | ✅ |
| Seed dodge/crit + Effect.Critical | ✅ |
| Poise/stagger/juggle | ✅ |
| Server resolve PickComboVariant | ✅ replicate `AuthorityComboVariantIndex` |
| Universal abilities GA | ✅ |
| Cue registry expandido | ✅ `GameplayCue.Element.Reaction.*` + combat cues |
| Combo depth 8-way/juggle | ✅ flags + aerial state |
| Cost via Cost GE | ⚠️ parcial |
| Hit-stop victim | ✅ |
| Lock-on scoring | ✅ |
| Projectile pooling + sweep | ⚠️ pooling ✅; sweep parcial |
| SetBlurAmount | ✅ |

---

## 03 — IA & Boss

| Item | Status |
|---|---|
| Targeting multiplayer | ✅ |
| Boss telegraphs replicate | ✅ |
| Telegraph gating bug | ✅ |
| Archetypes C++ | ✅ |
| Threat table | ✅ `UDFAIThreatComponent` |
| Boss scaling/fase×enrage | ⚠️ minion cleanup ✅; enrage UI via `EnrageCountdownEndWorldTime` |
| Investigação/alerta | ✅ last-known + `Investigate` state |
| Hearing wired | ✅ hits + ability noise |
| SummonMinions tag | ✅ |
| Combat Director expand | ⚠️ parcial |
| StateTree/EQS | ❌ (fora escopo C++ imediato) |

---

## 04 — Roguelike Loop

| Item | Status |
|---|---|
| Between-floor state machine | ✅ |
| Resume honesto | ✅ |
| Save IO unificado | ✅ `UDFSaveLibrary` (legacy só migração) |
| CheckMetaLevelUp + RunHistory | ✅ |
| Floor in-place vs TravelToNextFloor | ✅ in-place canônico; travel deprecated |
| Random events | ✅ |
| Ability roll único | ✅ via `UDFAbilitySelectionSubsystem` |
| PCG seed por andar | ✅ `ComputeFloorGenerationSeed` |
| Synergy/exclusion draft tags | ✅ |
| Async save + backup | ✅ `SaveActiveSlotWithBackup` |
| MerchantRestockRunCounter | ✅ |
| MetaXP data table | ✅ `UDFMetaXPLibrary` + DT default in-memory |
| MaxRunFloor acoplado | ✅ via `ADFRunGameMode::MaxRunFloor` |

---

## 05 — Presentation

| Item | Status |
|---|---|
| HUD root único | ✅ |
| Mix áudio | ⚠️ |
| Orquestrador apresentação | ✅ `UDFPresentationOrchestratorSubsystem` |
| Vitals juice (lag bar) | ⚠️ 🎨 WBP binding |
| Boss bar fases | ⚠️ 🎨 segmentos BP |
| Damage radial 360° | ✅ C++ `Indicator_Radial` |
| Minimap perf | ⚠️ parcial |
| Elite music branch | ✅ |
| Hotbar event-driven | ✅ sem poll quando bound |
| Acessibilidade completa | ⚠️ |

---

## 06 — Infra / Perf / Net

| Item | Status |
|---|---|
| Multicast Player 0 | ✅ |
| Run state replicado | ✅ `EnemiesRemaining`, floor em GameState |
| Primary bundles + unload | ✅ |
| Soft-ref gear async | ⚠️ parcial |
| Significance + no TObjectIterator | ✅ |
| Audio/a11y sliders | ⚠️ |
| Interaction server LOS | ✅ |
| FSavedMove dodge/air-dash | ✅ |
| Ring/amulet sockets | ✅ |
| Drop on unequip full | ✅ |
| DevSettings perf/net/a11y | ✅ `UDFPerformanceDeveloperSettings` |
| Session discovery / dedicated | ❌ requer OSS plugin |
| FSavedMove completo | ⚠️ sprint+dodge+airdash |

---

## Fora de escopo C++ puro (requer editor/assets/plugins)

- SoundClass/submix assets
- WBP bindings (HealthLagBar, boss segments)
- EOS/Steam session discovery
- Dedicated server target
- Branding / rename projeto
- Suite automatizada de compat save
- StateTree/EQS bosses
