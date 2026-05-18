# DungeonForged — Plano de Melhorias

> **Versão:** 2026-05-18
> **Escopo:** roteiro prático e acionável de melhorias por pilar de design.
> Complementa [`docs/analysis/Game_Analysis.md`](../analysis/Game_Analysis.md) (descritivo)
> com **o que fazer, onde mexer, qual número usar**.

---

## Como usar este plano

Cada doc segue o mesmo formato:

1. **Diagnóstico curto** — onde está o problema (com `arquivo:linha`).
2. **Alterações concretas** — patches, números, tabelas de tuning.
3. **Pseudo-código / snippets C++** prontos para colar/adaptar.
4. **Critério de "feito"** — como testar que ficou bom.

Sempre que possível, mantém:

- **Magic numbers exportados para `UPROPERTY(EditAnywhere)`** em vez de literais — para tunar do editor sem recompilar.
- **Tuning agrupado em DataAssets** (`UDFCombatTuningData`, `UDFFeelTuningData`) — testar mudanças sem mexer no código.
- **Telemetria mínima** — `UE_LOG(LogDFTuning, Verbose, ...)` nos pontos quentes (hit stop disparado, dodge frames, combo timing) para conseguir auditar runs gravadas.

---

## Índice por pilar

| # | Documento | Foco | Prioridade |
|---|---|---|---|
| 01 | [Game Feel](01_GameFeel.md) | Câmera, dodge, movimento, input lag, animação | 🔥 ALTA |
| 02 | [Juice](02_Juice.md) | Hit stop, camera shake, screen FX, VFX, combat text | 🔥 ALTA |
| 03 | [Combat](03_Combat.md) | Combo melee, heavy/light, abilities, fórmula de dano | 🔥 ALTA |
| 04 | [Run Mechanics](04_RunMechanics.md) | Pacing, eventos, draft, scaling por andar, recompensas | 🎯 ALTA |
| 05 | [Enemies & Bosses](05_EnemiesAndBosses.md) | Archetypes, group AI, telegraphs, fases | 🎯 MÉDIA |
| 06 | [Audio Mix](06_AudioMix.md) | Music intensity, elite trigger, layering, SFX banding | 🎨 MÉDIA |
| 07 | [UI / UX](07_UI_UX.md) | HUD adaptativo, boss intro, defeat polish, indicadores | 🎨 MÉDIA |
| 08 | [Accessibility](08_Accessibility.md) | Sliders, colorblind, hold/toggle, motion sickness | ♿ BAIXA |
| 09 | [Implementation Status](09_ImplementationStatus.md) | O que já está no C++ vs. pendente | 📋 |

---

## Top 12 ações de maior impacto / menor esforço

Priorizadas por `(impacto subjetivo / horas de trabalho)`. Cada uma tem um doc detalhado.

| Ordem | Ação | Esforço | Impacto | Doc |
|---|---|---|---|---|
| 1 | Confirmar **scaling de inimigos** `BaseHealth × (1 + 0.15 × Floor) × DifficultyMultiplier` | 1h | ALTO | [04](04_RunMechanics.md#scaling) |
| 2 | Reduzir `ComboWindowDuration` de `0.60` para `0.45` | 5min | ALTO | [03](03_Combat.md#combo-window) |
| 3 | Dispatch central de **hit stop por banda** (`Light/Heavy/Crit/BossSlam`) por `EDFHitFeedbackBand` | 2h | ALTO | [02](02_Juice.md#hit-stop-dispatch) |
| 4 | Aumentar **dodge i-frames** `0.25 → 0.35` + pulse de chromatic aberration | 30min | ALTO | [01](01_GameFeel.md#dodge) |
| 5 | **Heavy attack** (Shift+LMB ou RMB held) — trace 40cm + knockback 1.6× | 4h | ALTO | [03](03_Combat.md#heavy-attack) |
| 6 | **Random event chance crescente** (25/40/60% por terço da run, 0% no boss) | 30min | MÉDIO | [04](04_RunMechanics.md#event-pacing) |
| 7 | **Indicador de dano direcional** (canto vermelho do screen edge) | 2h | MÉDIO | [07](07_UI_UX.md#dano-direcional) |
| 8 | **HUD adaptativo** (fade out fora de combate, fade in dentro) | 2h | MÉDIO | [07](07_UI_UX.md#hud-adaptativo) |
| 9 | **Elite music state** auto-trigger quando `EEnemyTier::Elite` entra em range | 1h | MÉDIO | [06](06_AudioMix.md#elite-trigger) |
| 10 | **3 sliders de acessibilidade** (camera shake %, hit stop %, damage numbers on/off) | 2h | MÉDIO | [08](08_Accessibility.md) |
| 11 | **Boss vulnerability window** em phase transition (`State_BossVulnerable` por 2s, +50% dmg taken) | 2h | MÉDIO | [05](05_EnemiesAndBosses.md#vulnerability-windows) |
| 12 | **Combat text** crit com escala 1.4× + stroke amarelo + horizontal shake | 1h | BAIXO | [02](02_Juice.md#combat-text) |

> **Sequência recomendada:** 1 → 2 → 4 → 5 → 3 → resto. Os 5 primeiros mudam o feel da run inteira em uma tarde.

---

## Conceitos transversais

### A. Data-driven tuning

Crie **um único `UDFCombatTuningData` (UPrimaryDataAsset)** com:

```cpp
// Source/DungeonForged/Public/Data/UDFCombatTuningData.h
UCLASS(BlueprintType)
class UDFCombatTuningData : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    // Melee
    UPROPERTY(EditAnywhere, Category = "Melee")
    float ComboWindowDuration = 0.45f;
    UPROPERTY(EditAnywhere, Category = "Melee")
    float HeavyAttackChargeTime = 0.55f;
    UPROPERTY(EditAnywhere, Category = "Melee")
    float HeavyAttackTraceRadius = 40.f;
    UPROPERTY(EditAnywhere, Category = "Melee")
    float HeavyAttackDamageMultiplier = 2.2f;

    // Dodge
    UPROPERTY(EditAnywhere, Category = "Dodge")
    float DodgeIFrameDuration = 0.35f;
    UPROPERTY(EditAnywhere, Category = "Dodge")
    float DodgeCooldown = 0.7f;
    UPROPERTY(EditAnywhere, Category = "Dodge")
    float DodgeStaminaCost = 20.f;

    // Hit stop banding (overrides do UDFHitStopSubsystem)
    UPROPERTY(EditAnywhere, Category = "HitStop")
    float HitStopDuration_Light = 0.05f;
    UPROPERTY(EditAnywhere, Category = "HitStop")
    float HitStopDilation_Light = 0.08f;

    // ... etc
};
```

Referenciar em `UDFCheatManager::TuningData` (debug live edit) e via `UDFAssetManager::GetCombatTuning()`. Mudar números do editor sem recompilar.

### B. Telemetria mínima

Adicionar `LogDFTuning` ([Source/DungeonForged/Public/DungeonForgedLog.h](../../Source/DungeonForged/Public/DungeonForgedLog.h)) e logar nos pontos críticos:

```cpp
UE_LOG(LogDFTuning, Verbose, TEXT("HitStop %s dur=%.3f dil=%.3f instigator=%s"),
       *BandName, Duration, Dilation, *GetNameSafe(Instigator));
```

Permite gravar uma run com `-log -LogDFTuning Verbose` e analisar timing depois.

### C. Test scenes dedicadas

Criar **`L_TuningRange`**: sala vazia com:
- 3 training dummies (HP infinito, dano on)
- 1 elite dummy
- 1 boss dummy
- Botões: spawnar inimigo X, dar gold Y, set floor Z
- HUD com FPS + frame time + GPU + active montages

Vital para iterar feel sem entrar em uma run completa.

---

## Critérios de "pronto" para o projeto

Quando os 12 itens da tabela estiverem `[x]`:

- [ ] Combate sente "tight" — combo flow sem mash, dodge confiável.
- [ ] Hit stop dispara em **todo** golpe (light/heavy/crit/boss) com banding correto.
- [ ] Andar 1-3 é tutorial, 4-7 é meaty, 8-9 é tense, 10 (boss) é set-piece.
- [ ] Eventos não diluem combate — ~3 por run com curva crescente.
- [ ] Inimigos têm pelo menos 3 archetypes distintos por andar.
- [ ] Boss tem 3 fases com vulnerability windows e telegraphs claros.
- [ ] Música escala intensidade dentro de Combat state (low / high).
- [ ] HUD fade out fora de combate, in dentro.
- [ ] 3 sliders de acessibilidade básicos no menu.
- [ ] Defeat screen exibe causa, best floor highlighted.

Cada doc tem checklist mais granular.

---

## Convenções de notação nos docs

- **`arquivo.h:NN`** — linha exata para encontrar.
- **`[CONFIG]`** — número que deve virar `UPROPERTY` ou DataAsset.
- **`[CODE]`** — alteração de lógica, não só número.
- **`[ASSET]`** — necessita conteúdo novo (montage, niagara, sound).
- **`[BP]`** — alteração esperada em Blueprint, não em C++.

Boa empreitada.
