# 04 — Roguelike Loop, GameModes & Progressão

> Parte da [AAA Technical Review (Maio 2026)](00_Index.md).
> Cobre: run lifecycle, world transition, save/persistência, progressão (run +
> meta), economia (draft/events/loot/merchant), dungeon generation, GameModes.

---

## 1. Run lifecycle

`ERunPhase` (PreRun → InCombat → BetweenFloors → BossEncounter → Victory/Defeat).
Estado em memória no `UDFRunManager` (GameInstance subsystem); stats replicados em
`ADFRunGameState`; orquestração de andar no `ADFDungeonManager`.

### Forças
- Separação limpa **run state (GI)** vs **stats replicados (GameState)**.
- "Travel reasons" (`EDFRunTravelReason`) desacoplam intenção de spawn.
- Boss como fase de primeira classe; kill do último grunt suprimido se for boss.
- Idempotência de fim de run (`bEndRunPersistenceApplied`) evita double MetaXP.

### Gaps vs AAA (Hades/Returnal/Dead Cells)
| Gap | Detalhe | Ref |
|---|---|---|
| **`MaxRunFloor` decorativo** | victory só quando `FindFloorRowByNumber(+1)` falha; sem acoplamento ao max | `ADFDungeonManager.cpp:764-768` |
| 🔴 **Between-floor sequence stub** | `TriggerBetweenFloorSequence()` **sem caller**; `PresentBetweenFloorFlow_Implementation()` vazio | `ADFRunGameMode.cpp:372-392`, `ADFRunPlayerController.cpp:605-609` |
| **Floor avança in-place** | `AdvanceToNextFloor`→`StartFloor` regenera PCG no mesmo mapa; `TravelToNextFloor` nunca é chamado | `ADFDungeonManager.cpp:758-770` |
| **Unlock de vitória hardcoded** | floor≥3 enfileira class unlock (placeholder) | `DFRunManager.cpp:400-406` |

---

## 2. World transition

`UDFWorldTransitionSubsystem` orquestra Nexus↔Run com loading screen e `OpenLevel`
deferido (50ms). Pipeline de fim de run: `FinalizeRunData` → `ApplyEndOfRunPersistence`
→ checkpoint `RunEnd` → Nexus.

### Gaps
| Gap | Detalhe | Ref |
|---|---|---|
| **Sem level streaming / World Partition** | loop inteiro = `OpenLevel` full reload de mapas fixos; zero `LoadStreamLevel` | — |
| **`TravelToNextFloor` dead code** | implementado com checkpoint+UI, mas andares pulam isso | `UDFWorldTransitionSubsystem.cpp:218-248` |
| **`bIsTransitioning` não auto-clear** | depende do widget chamar `NotifyLoadingFinished()`; se falhar, bloqueia re-travel | `.h:72` |
| **Checkpoint salva slot errado** | `SaveCheckpoint()` usa `UDFSaveGame::Load()` legacy, não `GetActiveSave()` | `:286-296` |

---

## 3. 🔴 Save & persistência (`DFSaveGame`, v6)

Persiste: settings, meta (HighScore, runs/wins, MetaXP/Level, unlocks, pending
unlocks), lifetime stats, run snapshot (`LastCheckpoint`), profile (slot, versão).
Migrations v2→6, 3 slots + legacy mirror.

### O problema central: "captura mas não restaura"
| Gap | Detalhe | Ref |
|---|---|---|
| **Sem resume real de run** | checkpoints escritos; nada lê `LastCheckpoint` de volta; `RestoredRunState` nunca referenciado | `DFRunManager.h:294-296` |
| **`CaptureRunState` incompleto** | salva HP/Mana/floor/level; **ignora** `RunXP`, `ComboPoints`, `EquippedItems` (campos existem, nunca escritos) | `DFRunManager.cpp:130-164` |
| **"Continue" engana** | botão vai pro **Nexus**, não pro checkpoint da dungeon | `UDFMainMenuUserWidget.cpp:183-214` |
| **`RunHistory` sem writer** | struct + leitura de UI existem; nada faz append no fim da run | `DFSaveGame.h:81` |
| **`MerchantRestockRunCounter` / `LifetimeDeaths` / `TotalGoldEarnedMeta`** | campos documentados, **zero** writers | `DFSaveGame.h:96-98` |
| **`IsCompatible()` frágil** | match exato de string de versão → patch menor quebra load | `:129` |
| **IO síncrono** | sem async, sem backup slot, sem recuperação de corrupção | `:97-106` |
| **Dois paths de save** | mistura `GetActiveSave()` e `UDFSaveGame::Load()` legacy → risco de desync de slot | — |

**Decisão necessária:** implementar resume honesto (ler checkpoint → reidratar
`UDFRunManager` + leveling + equipment) **ou** remover a UX de "Continue" e marcar
o jogo como "run perdida ao sair". Esta é a prioridade #6 do Top 15.

---

## 4. Progressão

### In-run (`UDFLevelingComponent`)
Curva data-driven (`FDFLevelTableRow`, cumulativa), max 30, XP/level/points
replicados owner-only, GE de stat scaling por nível, gasto de pontos via RPC.

### Meta (Nexus)
MetaXP no fim da run: Victory `500+floor×50+kills×2`, Defeat `100+floor×20+kills`,
Abandon `25+floor×5` (hardcoded em `DFRunManager.cpp:349-362`). Thresholds em
`ADFNexusGameState::CheckMetaLevelUp` (data table `FDFNexusLevelRow`).

### Gaps
| Gap | Detalhe | Ref |
|---|---|---|
| 🟡 **Meta level não sobe no fim da run** | `ApplyEndOfRunPersistence` soma MetaXP mas **nunca chama `CheckMetaLevelUp`** → player fica com XP em overflow até interagir no hub | `DFRunManager.cpp` / `ADFNexusGameState.cpp:80-128` |
| **Checkpoint ignora leveling component** | só lê atributo GAS `CharacterLevel`, não `CurrentXP/Level/UnspentPoints` | — |
| **Draft vs level unlocks sobrepõem** | both concedem abilities, sem dedup/prioridade | — |
| **Sem synergy/exclusion tags em boons** | draft é shuffle/weighted simples, sem pools por archetype (Hades god boons) | `DFRunManager.cpp:439-464` |
| **Fórmula de MetaXP hardcoded** | deveria ser data table p/ tuning sem recompilar | `:349-362` |

---

## 5. Economia roguelike

### Draft 1-de-3 (`UDFAbilitySelectionSubsystem`)
Weights 60/25/12/3 (Common→Epic); skip = 50 gold; server-authoritative com lock
co-op (`ActiveFloorOfferId`). **Bom**, mas Legendary é mapeada p/ bucket Epic
(`:89-95`) — sem "momento legendary".

### Random events (`UDFRandomEventSubsystem`)
Enum rico (15 outcomes), curva de chance 25%→60%→0% (floor 10).
🔴 **`ShouldTriggerEvent`/`RollEvent` sem caller em `Source/`** — hook BP
between-floor vazio. O sistema inteiro está dormente.

### Loot & merchant
Drops por kill (base 45% × multiplicador de rarity); chests garantem 2–4; merchant
6 slots floor-gated, reroll escalando (`Base×2^count`). **Gaps:** sem pity/dup
protection; sem shop/rest garantido entre andares; `MerchantRestockRunCounter`
do Nexus morto.

---

## 6. Dungeon generation

Híbrido **PCG + DataTable**: designer controla pool/counts/boss; PCG fornece
transforms de spawn. Fallback timer (10s), snap a navmesh, seleção ponderada por
stat×floor, registro de minimap, trap base extensível (6 tipos).

### Gaps
| Gap | Detalhe | Ref |
|---|---|---|
| **Mesmo mapa reciclado** | `StartFloor` limpa actors + regenera PCG no lugar; sem rotação de bioma/tileset | — |
| **Sem variância de seed por andar** | floor só muda a row de inimigo, não o seed/params do grafo | `:242-272` |
| **Boss spawna como grunt** | mesmo pipeline PCG; sem arena lock/intro no manager | `:516-545` |
| **Fallback de origem perigoso** | PCG vazio → spawn em `(0,0,0)` | `:465` |
| **`PlaceRoomTemplates_Implementation` vazio** | hook BP no-op | `:274-276` |
| **6 traps só** | pouca variedade p/ 10 andares; sem placement procedural | — |

---

## 7. GameModes

**MainMenu** (splash→slot→class→Nexus/run), **Nexus** (5 NPCs:
Blacksmith/Merchant/Alchemist/Sage/Chronicler, unlock via save+condition tables,
portal→`TravelToRun`), **Run** (dungeon manager, pawn por classe, defeat/victory
screens, time limit opcional).

### Gaps
- "Continue" ≠ resume (vai ao hub).
- `PlayNexusArrivalPresentation_Implementation` stub — sem beat narrativo pós-run.
- Victory/defeat com timers fixos 5s (sem input p/ avançar).
- `bIsFirstLaunch` salvo mas pouco usado (onboarding vs veterano).

---

## 8. Recomendações priorizadas — Loop & Progressão

| # | Recomendação | Tag | Esforço |
|---|---|---|---|
| 1 | **State machine única between-floor** (events → shop/rest → draft → travel) substituindo stubs | 🔴 | H |
| 2 | **Resume honesto OU remover "Continue"** — completar `CaptureRunState`+restore, ou cortar a UX | 🔴 | M |
| 3 | **Unificar save IO** sob `UDFSaveSlotManagerSubsystem` (matar path legacy) | 🔴 | M |
| 4 | **`CheckMetaLevelUp` no fim da run** + popular `RunHistory` (fanfarra imediata ao voltar) | 🟡 | L |
| 5 | **Decidir floor in-place vs `TravelToNextFloor`** — implementar um, remover o outro | 🟡 | M |
| 6 | **Wirar random events** no between-floor (ou remover o subsystem) | 🟡 | M |
| 7 | **Single ability-roll service** (matar duplicação shuffle vs weighted) + bucket Legendary distinto | 🟡 | M |
| 8 | **MetaXP/thresholds via data table** (tuning sem recompilar) | 🟢 | L |
| 9 | **Variância de dungeon** — seed/params PCG por andar; arena dedicada de boss; falha "loud" no fallback de origem | 🟡 | M |
| 10 | **Mais traps + placement procedural**; shop/rest garantido por terço da run | 🟢 | M |
| 11 | **Synergy/exclusion tags em boons** (pools por build, duo boons) | 🟢 | H |
| 12 | **Async save + backup slot + checksum** | 🟢 | M |
| 13 | **Wirar `MerchantRestockRunCounter`** (rotação de estoque do hub) | 🟢 | L |
| 14 | **Beat narrativo de arrival no Nexus** + victory/defeat avançável por input | 🟢 | L |

> Arquivos-chave: `Source/DungeonForged/Public/Run/DFRunManager.h`, `DFSaveGame.h`,
> `Public/GameModes/Run/ADFRunGameMode.h`, `ADFRunGameState.h`,
> `Public/World/UDFWorldTransitionSubsystem.h`, `Public/ADFDungeonManager.h`,
> `Public/UI/UDFAbilitySelectionSubsystem.h`, `Public/Events/UDFRandomEventSubsystem.h`,
> `Public/Progression/UDFLevelingComponent.h`, `Public/Merchant/ADFMerchantActor.h`,
> `Public/GameModes/Nexus/ADFNexusGameMode.h`.
