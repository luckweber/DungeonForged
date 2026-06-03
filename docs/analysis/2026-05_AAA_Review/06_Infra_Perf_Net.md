# 06 — Infraestrutura: Performance, Networking, Movement, Equipment, Acessibilidade

> Parte da [AAA Technical Review (Maio 2026)](00_Index.md).
> Cobre: asset management/streaming, networking/sessões, character/movement,
> equipment/inventory, interaction, localização/acessibilidade, settings, build.
> Encerra com o **roadmap consolidado** da suíte.

---

## 1. Asset management & performance

`UDFAssetManager` registrado em config (`DefaultEngine.ini:64`), registra tags no
boot. `UDFAssetLoaderSubsystem` faz **preload por andar** (floor→enemy rows→soft
paths) via `FStreamableManager::RequestAsyncLoad`. Migração soft-ref de montages
(combo/hit/taunt/item) feita. Tooling: object pools (projétil/loot), Niagara pool,
room culling, pré-warm de combat text.

### Gaps vs AAA
| Gap | Detalhe | Ref |
|---|---|---|
| 🟡 **Sem Primary Asset Types/bundles/chunking** | `GetPrimaryAssetId()` existe, mas sem `PrimaryAssetTypes` em config, sem `LoadPrimaryAssets`, sem asset bundles/chunk rules | `UDFCombatTuningData.cpp:4-7` |
| **Preload sem sinal de conclusão** | callback vazio `[](){}`; sem delegate/erro/release ao trocar andar | `UDFAssetLoaderSubsystem.cpp:87-90` |
| **Preload de ability incompleto** | só class+icon, não montages/FX da ability | `:116-126` |
| **Meshes de gear ainda hard-ref** | `ItemSkeletalMesh`/`Icon`/`ItemMesh` são `TObjectPtr` → puxam memória no load da DT | `DFDataTableStructs.h:313-324` |
| **Sem Significance Manager** | sem tiers de tick de anim p/ inimigos distantes | — |
| **Room culling ≠ streaming** | esconde/desativa por distância; não dá unload de package | `UDFRoomCullingComponent.cpp:59-101` |
| **Profiling com `TObjectIterator`** | itera `UNiagaraComponent` a cada 5s — não shipping-safe | `UDFPerformanceSubsystem.cpp:77-85` |
| **Pools hardcoded** | classes C++ específicas; não data-driven por plataforma | `UDFObjectPoolSubsystem.cpp:9-51` |

---

## 2. 🔴 Networking

`UDFGameInstance` centraliza GAS init + `InitGlobalData()`. Sessão: `HostSession`
→ create/fallback listen; settings LAN co-op (2 players). Mapas via DevSettings.
`UDFNetworkValidator` documenta superfícies de validação; `UDFReplicationAudit.h`
documenta modos GAS.

### Gaps
| Gap | Detalhe | Ref |
|---|---|---|
| 🔴 **Multicast via Player 0** | `UDFNetworkLibrary::Multicast*` pega `GetPlayerController(W,0)` → 2º player co-op não dispara multicasts | `UDFNetworkLibrary.cpp:69-125` |
| **Sem session discovery** | sem `FindSessions`/browser/invite; só `JoinSessionToAddress` direto | `UDFGameInstance.cpp:153-160` |
| **Só listen server** | `bIsDedicated=false` sempre | `:112` |
| **Sem config de OnlineSubsystem** | sem entrada Null/Steam/EOS no `DefaultEngine.ini` | — |
| **GameInstance é BP em config** | `GameInstanceClass=/Game/.../BP_DFGameInstance` — se não parentar `UDFGameInstance`, lógica de sessão C++ é ignorada | `DefaultEngine.ini:5` |
| **Run state não replicado** | `UDFDungeonManager` floor/counts server-local (audit) | `UDFReplicationAudit.h:62-67` |
| **Anti-cheat fraco** | nav-proximity sem speed/teleport validation no CMC; interaction confia no overlap mesmo com distância falha | `UDFInteractionComponent.cpp:221-237` |

---

## 3. Character & movement

`UDFCharacterMovementComponent`: sprint prediction flag, dodge (root motion +
programático), coyote time, jump buffer, double jump, air dash com altitude lock,
landing recovery, strafe mode. `FSavedMove_DF` empacota `bWantsSprint` em
`FLAG_Custom_0` com `CanCombineWith` custom. Tuning de jump via data asset em
runtime. Pawn com ~21 componentes.

### Gaps
| Gap | Detalhe | Ref |
|---|---|---|
| **`FSavedMove` só cobre sprint** | air dash/dodge/jump-phase fora dos flags → risco de mispredict sob perda de pacote | `.h:300-314` |
| **Pawn monolítico** | 20+ componentes num actor → difícil strip p/ dedicated server e testar | `ADFPlayerCharacter.h:77-234` |
| **Interaction tica todo frame** | poderia ser timer/input-driven | `UDFInteractionComponent.cpp:43-48` |

---

## 4. Equipment / inventory

Paper-doll modular (8 partes + leader pose de `Mesh_Base`), equip
server-authoritative (validação→consume→GE→grant GA→replicate→visuals+anim set),
`ReplicatedLoadout` com `OnRep`, inventory owner-only, encumbrance.

### Gaps
| Gap | Detalhe | Ref |
|---|---|---|
| **Ring/Amulet sem mesh** | enum tem Ring1/Ring2/Amulet, mas pawn só registra helmet–offhand | `DFEquipmentTypes.h:18-20` |
| **Visuais hard-loaded** | `ItemSkeletalMesh` hard ptr — sem resolve async no equip | `:551-554` |
| **Inventory cheio = perda de item no unequip** | só loga warning | `:711-722` |
| **PredictCanEquip local-only** | sem visual preditivo antes do `OnRep` | `.h:112-117` |

---

## 5. Interaction

Interface limpa (`CanInteract`/`Interact`/`GetInteractionText`/`Range`), focus por
trace+cone, RPC server `Server_Interact`, overlap registration replicado, event bus
p/ portas, doors com lock/open replicado + timeline multicast.

### Gaps
- Tick por frame p/ focus.
- **Validação server fraca** — bypass de distância via overlap; sem LOS no server.
- Event bus é multicast delegate **local** (não replicado).
- Feedback de porta trancada é `AddOnScreenDebugMessage` (não localizado).

---

## 6. Localização & acessibilidade

4 idiomas com culture codes + persistência; switch PIE-safe; struct de settings
(font scale, reduce motion, color blind, audio sliders, damage numbers); escalas
de combat feel respeitam `bReduceMotion`; color-blind via blendable; rebinding via
Enhanced Input com save/load.

### Gaps
| Gap | Detalhe | Ref |
|---|---|---|
| 🟢 **High contrast stub** | comentário vazio | `UDFAccessibilitySubsystem.cpp:63-67` |
| 🟡 **Music/SFX/Voice ignorados** | só `MasterVolume` aplicado | `:69-84` |
| **`PropagateToPlayerPawns` vazio** | sem aplicação per-pawn (damage numbers, subtitles) | `:125-128` |
| **Sem TTS/screen reader** | ausente | — |
| **Sem contraste WCAG/subtitle sizing** | font só via `ApplicationScale` | `:54-60` |

---

## 7. Settings & build

DevSettings centralizam mapas, data tables de run, loading screen, class preview.
Build moderno UE5.4 (GAS, Enhanced Input, CommonUI, PCG, Niagara, MotionWarping,
AnimationWarping, MotionTrajectory, Chooser, GameplayCameras), IWYU on, módulo
editor separado.

### Gaps
- Sem DevSettings p/ perf/network/accessibility (valores hardcoded em subsystems).
- Class selection settings é `Config=Editor` (não shippa em `DefaultGame.ini`).
- Sem plugin OSS (Steam/EOS) no `.uproject`; sem target de dedicated server.
- Sem `SignificanceManager` nas deps.
- Projeto ainda "Third Person Game Template" (`DefaultGame.ini:3`).

---

## 8. Animação (alto nível)

`UUDFAnimInstance`: locomoção 8-way Start/Loop/Stop com gait, distance matching,
turn-in-place (root yaw offset), jump arc state machine, hot-swap de weapon anim
set, Motion Trajectory + Motion Warping. Plugins de warping/trajectory/chooser
ativos. *Gaps:* assets de locomoção mayoria hard-ref; sem budget de anim ligado a
significância; duplicação player vs `UDFAnimInstance_Enemy`.

---

## 9. Recomendações priorizadas — Infra

| # | Recomendação | Tag | Esforço |
|---|---|---|---|
| 1 | **Corrigir multicast Player-0** — rotear por instigator/AuthGameMode | 🔴 | M |
| 2 | **Replicar run state** do dungeon manager via GameState | 🔴 | M |
| 3 | **Primary Asset bundles por andar** + unload explícito + delegate de conclusão | 🟡 | H |
| 4 | **Completar soft-ref de gear/locomoção** + resolve async no equip/hover | 🟡 | M |
| 5 | **Confirmar `BP_DFGameInstance` parenta `UDFGameInstance`** | 🟡 | L |
| 6 | **Session discovery** (EOS/Steam OSS + find/invite) + target dedicated | 🟡 | H |
| 7 | **Significance Manager** p/ anim/VFX de crowd; remover `TObjectIterator` do tick | 🟡 | M |
| 8 | **Wirar audio sliders + high-contrast + `PropagateToPlayerPawns`** | 🟢 | M |
| 9 | **Validação server de interaction** (LOS + sweep) + feedback localizado | 🟢 | M |
| 10 | **`FSavedMove` p/ dodge/air-dash** ou prediction keys GAS completas | 🟢 | M |
| 11 | **Ring/amulet sockets** + drop-on-ground no unequip cheio | 🟢 | L |
| 12 | **DevSettings p/ perf/net/accessibility** + shippar class selection settings | 🟢 | L |

---

## 10. Roadmap consolidado (toda a suíte)

Fases sugeridas, agregando as recomendações 🔴/🟡 dos 6 docs.

### Fase 1 — Fechar loops & endurecer co-op (fundamental)
- Wirar **elemental** no dano · [02](02_Combat_GAS.md)
- **Multicast/targeting** multiplayer + replicar telegraphs de boss · [03](03_AI_Boss.md)/[06](06_Infra_Perf_Net.md)
- **State machine between-floor** (events/shop/draft/travel) · [04](04_Roguelike_Loop.md)
- **Resume honesto OU cortar "Continue"** + unificar save IO · [04](04_Roguelike_Loop.md)
- `CheckMetaLevelUp` + `RunHistory` no fim da run · [04](04_Roguelike_Loop.md)
- Bug `bCanTelegraph` · [03](03_AI_Boss.md)

### Fase 2 — Adensar sistemas (o "feel AAA")
- Meta-attribute `IncomingDamage` + modelo unificado de poise/stagger/juggle · [02](02_Combat_GAS.md)
- **Archetypes ativos** em C++ + scaling/fase de boss · [03](03_AI_Boss.md)
- **HUD unificado** + mix de áudio + orquestrador de juice · [05](05_Presentation.md)
- Seed/replicar RNG de combate; server resolve combo variant · [02](02_Combat_GAS.md)
- Variância de dungeon (seed/arena/traps) · [04](04_Roguelike_Loop.md)

### Fase 3 — Escala & produção
- **Primary Asset bundles** + Significance Manager · [06](06_Infra_Perf_Net.md)
- Session discovery / dedicated server · [06](06_Infra_Perf_Net.md)
- Combo depth (branching/8-way/juggle aéreo), boss bar com fases, damage radial · [02](02_Combat_GAS.md)/[05](05_Presentation.md)
- Synergy/exclusion tags em boons; async save+backup · [04](04_Roguelike_Loop.md)

### Fase 4 — Polish & compliance
- **Acessibilidade completa** (high-contrast, damage numbers, blur, reduce-motion em HUD) · [05](05_Presentation.md)/[06](06_Infra_Perf_Net.md)
- Camera shakes UE5 modulares + háptico
- Branding (renomear projeto), DevSettings de perf/net
- Suite de testes de compat de save

> Arquivos-chave: `Source/DungeonForged/Public/DFAssetManager.h`,
> `Performance/UDFAssetLoaderSubsystem.h`, `UDFPerformanceSubsystem.h`,
> `Network/UDFGameInstance.h`, `UDFNetworkLibrary.h`,
> `Characters/UDFCharacterMovementComponent.h`, `ADFPlayerCharacter.h`,
> `Equipment/UDFEquipmentComponent.h`, `Interaction/UDFInteractionComponent.h`,
> `Localization/UDFAccessibilitySubsystem.h`, `DungeonForged.Build.cs`,
> `DungeonForged.uproject`.
