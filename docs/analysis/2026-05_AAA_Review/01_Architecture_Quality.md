# 01 — Arquitetura & Qualidade Técnica

> Parte da [AAA Technical Review (Maio 2026)](00_Index.md).
> Foco: padrões arquiteturais, qualidade de código, riscos cross-cutting,
> débitos técnicos. Veja docs por sistema para detalhes.

---

## 1. Visão arquitetural geral

DungeonForged segue um padrão **C++-first, Blueprint como camada de assets**,
com forte uso de subsistemas e composição por componentes.

```
DungeonForged (módulo runtime único)
├── GAS/            (123 h)  núcleo: AttributeSet, abilities, effects, elemental
├── Combat/         (49 h)   combo, melee trace, hit reaction, feel/juice
├── UI/             (43 h)   HUD, widgets, minimap, combat text
├── GameModes/      (39 h)   MainMenu / Nexus / Run
├── AI/ + Boss/     (24 h)   Behavior Tree + GAS, boss phases
├── Animation/      (13 h)   AnimInstance 8-way, distance matching, TIP
├── Dungeon/        (10 h)   dungeon manager, PCG, traps
├── Localization/   (10 h)   i18n + acessibilidade
├── Interaction/    (9 h)    interface + event bus
├── Equipment/      (8 h)    paper-doll modular
├── FX/ Performance/ Network/ Audio/ ... (infra)
```

### Padrões usados (e bem usados)

| Padrão | Onde | Avaliação |
|---|---|---|
| **GameInstance Subsystem** | `UDFRunManager`, `ADFDungeonManager` | run state sobrevive a `OpenLevel` |
| **World Subsystem** | `UDFCombatTextSubsystem`, `UDFHitStopSubsystem`, `UDFElementalReactionSubsystem`, `UDFCombatDirectorSubsystem` | escopo correto por mundo |
| **Component composition** | `ADFPlayerCharacter` (~21 componentes) | modular, mas ver §4 |
| **Data-driven design** | DataTables (`FDF*TableRow`) + `UDFCombatTuningData` (PrimaryDataAsset) | tuning sem recompilar |
| **Developer Settings** | `UDFWorldDeveloperSettings`, `UDFRunDeveloperSettings` | resolve CDO-null de subsystem |
| **Native GameplayTags** | `FDFGameplayTags` (~200 tags com comentário) | central, idempotente |
| **GE Components (UE5.4)** | `UDFGEComponent_*` via `ConfigureEffectCDO` | padrão correto pós-5.3 |
| **Central feedback bus** | `UDFCombatFeedbackLibrary::DispatchOnHitConfirmed` | um ponto p/ todo juice |

> **Veredito:** a arquitetura é **idiomática para UE5.4** e bem acima da média
> de projetos solo. A escolha de subsistemas e a separação Menu/Nexus/Run são
> particularmente limpas.

---

## 2. Pontos fortes transversais

1. **GAS coeso** — um `UDFAttributeSet` único, executions de dano claras
   (Physical/Magic/True), cooldown via GE com CDR, e combo-cancel integrado no
   `UDFGameplayAbility` base.
2. **Pipeline de feel centralizado** — hit stop com exclusão de actor (wall-clock
   via `FPlatformTime`), impact framing por-attacker, screen FX, combat text e
   camera shake passam todos por um dispatcher (`UDFCombatFeedbackLibrary`).
3. **Networking pensado desde cedo** — ASC em PlayerState (Mixed) p/ player,
   Minimal p/ trash, Full p/ boss; traces server-only; doc de auditoria
   (`UDFReplicationAudit.h`).
4. **Migração para soft refs** — montages de combo, hit reaction, taunts e DT
   structs migrados para `TSoftObjectPtr` com caches `Transient` e load assíncrono
   (reduz hard refs no CDO → tempo de abrir BP no editor).
5. **Acessibilidade pioneira** — escalas de camera shake / hit-stop / VFX
   respeitam `bReduceMotion`; PT-BR como default; rebinding via Enhanced Input.
6. **Tooling de performance** — object pools (projéteis/loot), Niagara pool,
   room culling, pré-warm de combat text pool.

---

## 3. Riscos arquiteturais cross-cutting

Estes são padrões que aparecem em **múltiplos** sistemas e merecem decisão
arquitetural única (não fix pontual).

### 3.1 🔴 Multicast / RPC roteado via "Player 0"

`UDFNetworkLibrary::MulticastSpawnHitVFX` e helpers similares pegam
`GetPlayerController(World, 0)` para disparar multicasts. Da mesma forma, toda a
IA mira `Player 0` (`UDFBTService_UpdateTarget.cpp:66-68`, `ADFAIController.cpp:154`).

- **Impacto:** num co-op de 2 jogadores, o segundo player não dispara
  multicasts corretamente e nunca é alvo de IA.
- **Direção:** rotear RPCs por instigator (RPC no controller dono) ou via
  `AuthGameMode`; substituir scans de Player 0 por "hostil mais próximo / threat".
- Refs: `UDFNetworkLibrary.cpp:69-125`. Detalhe em [06](06_Infra_Perf_Net.md) §2 e [03](03_AI_Boss.md) §1.

### 3.2 🔴 "Save the snapshot, never restore it"

Padrão recorrente: o sistema **captura** estado mas **não consome**.

- `CaptureRunState()` salva HP/floor mas ignora XP/combo/equipment; `RestoredRunState`
  nunca é lido (`DFRunManager.h:294-296`).
- `RunHistory`, `MerchantRestockRunCounter`, `LifetimeDeaths` têm campo de save
  mas nenhum writer.
- **Direção:** ou completar o ciclo (resume real) ou remover a UX que promete
  ("Continue"). Detalhe em [04](04_Roguelike_Loop.md) §3.

### 3.3 🟡 Dois caminhos para a mesma coisa

Várias features têm **dois pontos de entrada** divergentes — risco de desync e
de erro de designer:

| Duplicação | Caminhos |
|---|---|
| Ability roll | `UDFRunManager::GetRandomAbilityOfferCandidates` (shuffle uniforme) vs `UDFAbilitySelectionSubsystem::RollAbilityChoices` (weighted) |
| Lock-on widget | `ADFRunHUD::WBP_LockOnIndicator` vs `UDFLockOnComponent` cria o próprio |
| HUD root | `ADFHUDBase::MainHUDWidget` vs `ADFRunHUD::WBP_HUD` |
| Combo window | `AN_ComboWindowOpen` + `ANS_DFCancelWindow` + curva de anim |
| Floor advance | `AdvanceToNextFloor` (in-place) vs `TravelToNextFloor` (dead) |
| Save IO | `UDFSaveSlotManagerSubsystem::GetActiveSave()` vs `UDFSaveGame::Load()` legacy |

- **Direção:** eleger um caminho canônico por feature e deprecar o outro.

### 3.4 🟡 Sistemas "dormentes" (dead code)

O maior é o **elemental** (matriz de resistência + reações Melt/Steam/Electrocute
nunca chamados no dano). Também: random events sem caller, between-floor sequence
vazio, Universal abilities só como tag. Mapa completo no [Índice](00_Index.md#mapa-de-loops-abertos-dead-code--stubs-encontrados).

- **Direção:** para cada um, decidir **wirar** (preferível, já há código) ou
  **remover** (evita confundir manutenção e inflar superfície de tags).

### 3.5 🟡 RNG não-determinístico / não-replicado

Dodge e crit usam `FMath::FRand()` na execution; `PickComboVariant` rola no
cliente. Sem seed nem replicação do resultado → desync sob prediction.

- **Direção:** seed de combate determinístico no server, replicar resultado
  (ou ao menos a tag `Effect.Critical`, hoje nunca setada). Detalhe em [02](02_Combat_GAS.md).

---

## 4. Qualidade de código & observações pontuais

| Item | Observação | Severidade |
|---|---|---|
| **Pawn monolítico** | `ADFPlayerCharacter` carrega ~21 componentes; difícil strip p/ dedicated server e testar isoladamente | média |
| **Include duplicado** | `UDFAttributeSet.cpp:4-7` inclui `ADFPlayerCharacter.h` 2× | baixa |
| **Mutação de spec `const`** | `UDFDamageCalculation` escreve `Data.CriticalHit` num spec const | baixa (funciona, não-idiomático) |
| **Mana shield em PostExecute** | reescreve health após avaliação do GE — ordering/race com outros listeners | média |
| **`SummonMinions` tag errada** | tagueia-se como `Ability_Ice_Blizzard` (copy-paste) | baixa, mas confunde queries |
| **Projeto ainda "Third Person Game Template"** | `DefaultGame.ini:3` | branding |
| **GameInstance é BP** | `GameInstanceClass=/Game/.../BP_DFGameInstance` — se o BP não parentar `UDFGameInstance`, a lógica C++ de sessão é ignorada | média |
| **Profiling com `TObjectIterator`** | `UDFPerformanceSubsystem` itera `UNiagaraComponent` a cada 5s — não shipping-safe | média |

---

## 5. Débito técnico — backlog priorizado

| # | Débito | Esforço | Doc detalhe |
|---|---|---|---|
| 1 | Decidir caminho canônico p/ cada duplicação do §3.3 | M | este |
| 2 | Wirar ou remover sistemas dormentes do §3.4 | M–H | [02](02_Combat_GAS.md)/[04](04_Roguelike_Loop.md) |
| 3 | Corrigir multicast/targeting Player-0 | M | [03](03_AI_Boss.md)/[06](06_Infra_Perf_Net.md) |
| 4 | Introduzir meta-attribute `IncomingDamage` + pipeline de absorção | M | [02](02_Combat_GAS.md) |
| 5 | Split do pawn em componentes "cosméticos" gated por significância | H | [06](06_Infra_Perf_Net.md) |
| 6 | Confirmar que `BP_DFGameInstance` parenta `UDFGameInstance` | L | [06](06_Infra_Perf_Net.md) |
| 7 | Suite de testes de compatibilidade de save (`SaveVersion=6`) | M | [04](04_Roguelike_Loop.md) |
| 8 | Renomear projeto / branding | L | este |

---

## 6. Recomendação estratégica

A arquitetura **não precisa de rewrite**. O caminho para AAA é, em ordem:

1. **Fechar loops abertos** (§3.4) — maior ROI, transforma "código presente" em
   "feature jogável".
2. **Endurecer co-op** (§3.1) — sem isso, o pilar multiplayer é frágil.
3. **Adensar sistemas** (combo depth, archetypes, telegraph readability) — onde
   o "feel AAA" realmente mora.
4. **Polir infra** (streaming, significance, mix de áudio) — escala e qualidade
   de produção.

> Cada um desses tem um doc dedicado nesta suíte com tabelas de recomendação
> acionáveis e referências `arquivo:linha`.
