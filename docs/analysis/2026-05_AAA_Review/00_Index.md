# DungeonForged — AAA Technical Review (Maio 2026)

> **Data:** 2026-05-28
> **Escopo:** auditoria técnica completa de **todo o jogo**, baseada em leitura
> direta do código C++ em `Source/DungeonForged/` (UE 5.4), feita por deep-dives
> sistema-a-sistema (GAS, Combat, AI/Boss, GameModes/Run, UI/FX/Audio, Infra).
> **Audiência:** dev/designer planejando o caminho para qualidade comercial AAA.
> **Relação com docs anteriores:** complementa (não substitui)
> [`Game_Analysis.md`](../Game_Analysis.md) (abril/2026, foco em game design).
> Esta suíte foca em **qualidade técnica, robustez e gaps de implementação**.

---

## Como ler esta suíte

| Doc | Sistema | Foco |
|---|---|---|
| [`01_Architecture_Quality.md`](01_Architecture_Quality.md) | Arquitetura geral | Padrões, qualidade de código, riscos cross-cutting, débitos |
| [`02_Combat_GAS.md`](02_Combat_GAS.md) | GAS + Combate | Atributos, dano, abilities, combo, trace, feel/juice, lock-on |
| [`03_AI_Boss.md`](03_AI_Boss.md) | IA + Bosses | Behavior Trees, archetypes, telegraph, fases, enrage |
| [`04_Roguelike_Loop.md`](04_Roguelike_Loop.md) | Loop / GameModes | Run lifecycle, save/persistência, progressão, economia, dungeon |
| [`05_Presentation.md`](05_Presentation.md) | UI/FX/Áudio | HUD, combat text, screen FX, camera shake, música adaptativa |
| [`06_Infra_Perf_Net.md`](06_Infra_Perf_Net.md) | Infraestrutura | Asset streaming, networking, movement, equipment, acessibilidade |

Cada doc segue o mesmo formato: **o que existe → pontos fortes → gaps vs AAA →
recomendações priorizadas** com referências `arquivo:linha`.

---

## Sumário executivo

DungeonForged tem uma **fundação de engenharia excepcional para um projeto
solo/pequeno time**: GAS completo (21 atributos, ~41 abilities, ~52
GameplayEffects), CMC custom com network prediction, combate data-driven com
pipeline de feedback centralizado, IA por Behavior Tree + GAS, hub/run/menu
separados, música adaptativa em camadas, paper-doll modular, e até um doc de
auditoria de replicação.

O que separa o projeto de "qualidade AAA" **não é arquitetura** — é
**completude de orquestração e profundidade de sistemas**. Vários fluxos
existem como *scaffolding* ou *dead code*: o sistema elemental nunca é
chamado no combate, eventos aleatórios não têm caller em C++, checkpoints são
salvos mas nunca restaurados, telegraphs de boss não replicam, e archetypes de
inimigo são quase só metadata. Os "ossos" AAA estão lá; falta **fechar os
loops** e **adensar os sistemas**.

### Radar consolidado (técnico)

| Pilar | Nota | Tendência vs abril/2026 |
|---|---|---|
| Arquitetura técnica (C++/GAS) | **9.0** | estável — exemplar |
| Combate (mecânica) | **7.0** | combo agora soft-ref, mais robusto |
| Combate (feel/juice) | **8.0** | pipeline central de feedback é destaque |
| GAS depth | **7.5** | elemental dormente derruba a nota |
| IA de inimigos | **5.5** | archetypes sub-aproveitados, Player 0 only |
| Boss design | **7.5** | sólido; telegraph não-replicado é risco co-op |
| Roguelike loop | **6.5** | muitos loops abertos (events, checkpoint, meta) |
| UI/UX | **7.0** | cobertura ampla; HUD fragmentado, CommonUI ocioso |
| Áudio | **6.5** | layering bom; mix bus incompleto |
| Performance/infra | **7.0** | pooling+preload bons; falta streaming/significance |
| Networking/co-op | **6.0** | bem desenhado, mas multicast via Player 0 quebra co-op |
| Acessibilidade | **6.0** | pioneiro; vários toggles são stubs |

**Avaliação técnica geral: 7.0 / 10** — base comercial sólida, com um conjunto
bem definido de *loops abertos* que, fechados, elevam para 8.5+.

---

## Top 15 prioridades (impacto × esforço)

Ordenadas por ROI. Tags: 🔴 crítico, 🟡 alto valor, 🟢 polish. Esforço L/M/H.

| # | Prioridade | Tag | Esforço | Doc |
|---|---|---|---|---|
| 1 | **Wirar sistema elemental no pipeline de dano** (hoje é dead code) | 🔴 | M | [02](02_Combat_GAS.md) |
| 2 | **Corrigir multicast via Player 0** (quebra co-op de 2 jogadores) | 🔴 | M | [06](06_Infra_Perf_Net.md) |
| 3 | **Targeting multiplayer** (IA só mira Player 0) | 🔴 | M | [03](03_AI_Boss.md) |
| 4 | **Replicar telegraphs de boss** (warning decals server-only) | 🔴 | M | [03](03_AI_Boss.md) |
| 5 | **Fechar o loop between-floor** (events/shop/draft/travel num só state machine) | 🔴 | H | [04](04_Roguelike_Loop.md) |
| 6 | **Checkpoint restore honesto OU remover UX de "Continue"** | 🔴 | M | [04](04_Roguelike_Loop.md) |
| 7 | **Bug telegraph gating** (`bCanTelegraph=true` toda tick anula o coordinator) | 🟡 | L | [03](03_AI_Boss.md) |
| 8 | **`CheckMetaLevelUp` no fim da run** + popular `RunHistory` | 🟡 | L | [04](04_Roguelike_Loop.md) |
| 9 | **Ativar archetypes em C++** (flee/abilities/spacing por tipo) | 🟡 | M | [03](03_AI_Boss.md) |
| 10 | **Unificar HUD sob root único** (eliminar lock-on/HUD duplicados) | 🟡 | M | [05](05_Presentation.md) |
| 11 | **Mix de áudio**: SoundClasses/submixes + wirar sliders de volume | 🟡 | M | [05](05_Presentation.md) |
| 12 | **Primary Asset bundles por andar** + unload (memória) | 🟡 | H | [06](06_Infra_Perf_Net.md) |
| 13 | **Modelo unificado de poise/stagger/juggle** (hoje 2 sistemas paralelos) | 🟡 | M | [02](02_Combat_GAS.md) |
| 14 | **Significance Manager** para anim/VFX de crowd de inimigos | 🟢 | M | [06](06_Infra_Perf_Net.md) |
| 15 | **Completar acessibilidade** (high-contrast, damage numbers, blur) | 🟢 | L | [05](05_Presentation.md), [06](06_Infra_Perf_Net.md) |

---

## Mapa de "loops abertos" (dead code / stubs encontrados)

Estes são os achados mais acionáveis — funcionalidade que **existe mas não
está conectada**. Detalhes e referências nos docs respectivos.

- **Elemental**: `UDFElementalReactionSubsystem::ApplyElementalDamage` / matriz
  de resistência nunca chamados pelo `UDFDamageCalculation` → [02](02_Combat_GAS.md)
- **Random events**: `UDFRandomEventSubsystem::ShouldTriggerEvent`/`RollEvent`
  sem caller em `Source/` → [04](04_Roguelike_Loop.md)
- **Between-floor sequence**: `TriggerBetweenFloorSequence()` sem caller;
  `PresentBetweenFloorFlow_Implementation()` vazio → [04](04_Roguelike_Loop.md)
- **Checkpoint restore**: `RestoredRunState` declarado, nunca referenciado;
  `CaptureRunState` não salva XP/combo/equip → [04](04_Roguelike_Loop.md)
- **`TravelToNextFloor`**: implementado, nunca chamado (andares regeneram
  in-place) → [04](04_Roguelike_Loop.md)
- **`RunHistory` / `MerchantRestockRunCounter` / `LifetimeDeaths`**: campos
  de save sem writer → [04](04_Roguelike_Loop.md)
- **Telegraph decals de boss**: `ADFMeteorWarningDecal` sem `bReplicates` →
  [03](03_AI_Boss.md)
- **`bCanTelegraph = true`** forçado toda tick em `UDFBTService_UpdateTarget`
  anula o `TelegraphCoordinator` → [03](03_AI_Boss.md)
- **Universal abilities** (HealthPotion, SecondWind, BattleHymn, Siphon,
  Berserk, CallLightning): tags existem, sem classe GA → [02](02_Combat_GAS.md)
- **Audio sliders** (Music/SFX/Voice): salvos, não roteados → [05](05_Presentation.md)/[06](06_Infra_Perf_Net.md)
- **`SetBlurAmount`** / `ApplyHighContrast` / `PropagateToPlayerPawns`:
  declarados/stub → [05](05_Presentation.md)/[06](06_Infra_Perf_Net.md)

---

## Metodologia

Análise feita por 6 agentes de exploração read-only, cada um cobrindo um
cluster de sistemas, com instrução de reportar **forças, gaps vs AAA e
referências `arquivo:linha`**. Nenhum arquivo de código foi modificado durante
a auditoria. As notas de `arquivo:linha` refletem o estado do código em
2026-05-28 e podem deslocar com edições futuras — confirme antes de agir.

> **Próximo passo sugerido:** transformar as tabelas "Recomendações" de cada
> doc em issues/tasks. Os itens 🔴 do Top 15 são os de maior ROI e devem ser a
> primeira sprint.
>
> **Tracker de implementação (2026-05-28):** [`07_Implementation_Tracker.md`](07_Implementation_Tracker.md)
