# 8-Way Locomotion — Start / Loop / Stop Setup

Suporte C++ para alimentar animações direcionais 8-way com **Start → Loop → Stop** por gait (Walk / Run / Sprint), incluindo os 5 refinamentos AAA: Distance Matching, Stride Warping, Foot Locker, Turn-In-Place e Aim Offset.

> **Status do código:** compila limpo (`Rebuild All: 1 succeeded`). Todo o trabalho restante é wiring no AnimBP + curvas nos assets.
>
> **Atualização 2026-05-26 — Stop AAA:** ver **§5.6** para a cadeia de fixes do Stop (deadlock → brusco → câmera lenta → transição para Idle). Mudou a semântica de `bTransition_StopToIdle` e adicionou `WalkStopBrakingDeceleration` (CMC) + novos defaults de catch-up. Vários trechos abaixo (§1, §2.4, §5.1 Passo 4, §8, §9) foram atualizados; onde houver conflito com texto antigo, **§5.6 manda**.
>
> **Engine alvo:** Unreal Engine **5.4** (`DungeonForged.uproject` → `EngineAssociation: 5.4`). Os nomes de nós/funções e o fluxo de Distance Matching abaixo foram validados contra a documentação oficial da Epic (ver §11 — Referências externas).
>
> **Mapa de assets do seu pacote Fab:** se você está usando o pacote *Fighter / Action-RPG* (Idle/Attack/Walk/Run/Jump/Dodge/Roll/Hit/Turn — armado e desarmado), vá direto à **§10 — Mapeamento do pacote Fab → sistemas** para ver onde cada categoria de animação entra (locomoção 8-way, weapon layers combat, dodge/roll, dash aéreo, jump 4-way, turn-in-place).

---

## 0. Por que migrar do Blend Space

Blend Space é ótimo para **Loop** (animações cíclicas com velocidade constante), mas **não funciona para Start/Stop** porque essas são animações com perfil de velocidade variável (0 → max no Start, max → 0 no Stop).

### Comparação de propriedades

| Propriedade | Blend Space | Start / Stop animation |
|---|---|---|
| Estrutura | Cíclica (loopa) | **Não-cíclica** (one-shot) |
| Velocidade | Constante ao longo da anim | **Variável** (acelera ou desacelera) |
| Como amostra | Por **fase** (0–1) ou sync marker | Por **distância percorrida** (cm) |
| Sincronização | Tempo + sync groups | **Distance Matching** (curva por frame) |

Se você jogar um Start dentro de um Blend Space, ele toca em playrate fixo independente da aceleração real do CMC — pés escorregam por 0.3–0.5 s, exatamente o problema que queremos resolver.

### Estrutura antiga vs nova

**Antigo (1 nó):**
```
Speed (Y) ─┐
            ├─→ [Blend Space 2D] ─→ Output
Direction(X)┘
```

**Novo (3 estados sequenciais dentro do `Walk_Run_SM`):**
```
[Idle Pose]
      ↓ bTransition_IdleToStart  (snapshot LocomotionStartDirection)
[Start  : Sequence Player ← Get Locomotion Start Anim]   ← não-loop, controlado por Distance
      ↓ bTransition_StartToLoop
[Loop   : Sequence Player ← Get Locomotion Loop Anim ]   ← loop, direção atualiza todo frame
      ↓ bTransition_LoopToStop  (snapshot LocomotionStopDirection)
[Stop   : Sequence Player ← Get Locomotion Stop Anim ]   ← não-loop, desacelera para 0
      ↓ bTransition_StopToIdle
[Idle Pose]
```

Cada estado é um **Sequence Player** com o pino `Sequence` exposto e ligado a um getter `BlueprintPure` — o resolver C++ pega o asset certo pela direção e gait atuais.

### Como cada momento de velocidade é tratado

| Momento | Velocidade esperada | Quem controla |
|---|---|---|
| Início do Start | ~0 cm/s | Root motion da anim Start (0 → autoral) |
| Fim do Start = Início do Loop | velocidade autoral (ex.: 400) | Root motion da Start chega nesse valor |
| Loop em ciclo | velocidade real do CMC | **Stride Warping** estica/encolhe passos (§5.2) |
| Início do Stop | velocidade do Loop | Root motion do Stop começa nesse valor |
| Fim do Stop | ~0 cm/s | Root motion do Stop desacelera para 0 |

**Segredo:** o CMC continua dirigindo a cápsula via `MaxWalkSpeed` (não a animação). A animação **acompanha** via Distance Matching (§5.1) no Start e Stride Warping (§5.2) no Loop. O **Stop também usa Distance Matching** (§5.1 Passo 4): a anim Run_Stop tem root motion de ~202 cm, então o braking do CMC precisa **deslizar** essa distância (`WalkStopBrakingDeceleration`, §5.6) para o distance-match casar os pés, e o catch-up sincroniza o fim do clip com a parada. Sem isso o Stop fica brusco, em câmera lenta, ou travado — ver a cadeia de fixes em **§5.6**.

### Que partes mudam de assets

| Estado | Antes | Depois |
|---|---|---|
| `Idle` | 1 asset (`IdleAnimation`) | **igual** — sem mudança |
| `Walk/Run Loop` | 1 Blend Space (`MovementBlendSpace`) | **8 animações** por gait (`Loop_F`, `Loop_FR_45`, …) |
| `Walk/Run Start` | não existia (Blend Space cobria) | **8 animações** por gait (`Start_F`, `Start_FR_45`, …) |
| `Walk/Run Stop` | não existia (Blend Space cobria) | **8 animações** por gait (`Stop_F`, `Stop_FR_45`, …) |

Total por gait: **24 sequences** (8 Start + 8 Loop + 8 Stop). Você não precisa preencher todas — o resolver faz fallback automático: diagonal vazia → cardinal correspondente; cardinal vazio → Forward.

### Migração gradual (recomendado)

O sistema **mantém compatibilidade** — `FUDAnimSet` ainda tem `MovementBlendSpace` e `StrafeBlendSpace` antigos. Se `WalkSet` e `RunSet` ficarem vazios, o resolver retorna `nullptr` e o AnimBP pode cair de volta no Blend Space via `ResolveLocomotionBS(bStrafe)`.

Sugestão de ordem de migração:

1. **Run com 8-way Start/Loop/Stop** — é o gait mais usado em gameplay; maior impacto visual.
2. **Walk com 8-way** — quando você gravar/comprar as walk_start_*/walk_stop_*.
3. **Sprint** — opcional; pode reusar Run assets com Stride Warping mais agressivo.

Até lá, Walk pode continuar no Blend Space, e os estados `Start/Stop` da sub-state machine ficam ociosos (nunca entram porque os getters retornam `nullptr` → transição falha graciosamente).

---

## 1. Resumo do que foi adicionado em C++

### `FUDLocomotionAnimSet` (`Public/Animation/DFAnimSetTypes.h`)

Bundle com **24 slots** por gait — 8 directions × { Start, Loop, Stop }. Nomes exatos das propriedades:

```
Start_F, Start_FR_45, Start_R_90, Start_BR_135, Start_B_180, Start_BL_135, Start_L_90, Start_FL_45
Loop_F,  Loop_FR_45,  Loop_R_90,  Loop_BR_135,  Loop_B_180,  Loop_BL_135,  Loop_L_90,  Loop_FL_45
Stop_F,  Stop_FR_45,  Stop_R_90,  Stop_BR_135,  Stop_B_180,  Stop_BL_135,  Stop_L_90,  Stop_FL_45
```

Resolver com **fallback automático**: slot vazio → diagonal cai no cardinal (ex.: `Start_FR_45` ausente → tenta `Start_F`); cardinal cai em `Start_F`. Slots vazios são tratados; basta preencher os que tem.

### `FUDAnimSet` estendido com `WalkSet` e `RunSet`

```cpp
FUDLocomotionAnimSet WalkSet;
FUDLocomotionAnimSet RunSet;

UAnimSequenceBase* ResolveLocomotionStart(EDFGait Gait, EDFMovementDirection Dir) const;
UAnimSequenceBase* ResolveLocomotionLoop (EDFGait Gait, EDFMovementDirection Dir) const;
UAnimSequenceBase* ResolveLocomotionStop (EDFGait Gait, EDFMovementDirection Dir) const;
```

**Fallback de gait (exato, conforme `DFAnimSetTypes.cpp`):** existem **apenas dois** sets na struct — `WalkSet` e `RunSet`. O resolver escolhe `WalkSet` quando `Gait == Walk`, e `RunSet` em **qualquer outro caso** (ou seja, `Run` **e** `Sprint` usam `RunSet`). Se o set escolhido devolver `nullptr` (direção e fallbacks de cardinal vazios) e o gait **não** for `Walk`, ele cai para `WalkSet`. Se ainda assim for `nullptr`, o getter retorna nulo e o AnimBP pode usar o Blend Space legado.

> **Sprint não tem set próprio.** Para diferenciar Sprint de Run visualmente, reuse os assets de Run e empurre o Stride Warping mais forte (alpha maior / `AuthoredLoopSpeed` calibrado para a velocidade de Run), conforme §5.2 e §8.

### `EDFGait` (`Public/Animation/UDFLocomotionTypes.h`)

`Idle / Walk / Run / Sprint` — calculado por threshold no AnimInstance.

### `UUDFAnimInstance` — variáveis novas (BP-facing)

| Variável | Tipo | Propósito |
|----------|------|-----------|
| `Gait` | `EDFGait` | Gait atual (auto) |
| `MovementDirection` | `EDFMovementDirection` | Direção atual (atualiza todo frame) |
| `LocomotionStartDirection` | `EDFMovementDirection` | **Snapshot** capturada na borda Idle→Start |
| `LocomotionStopDirection` | `EDFMovementDirection` | **Snapshot** capturada na borda Loop→Stop (input liberado) |
| `bIsAccelerating` | `bool` | `bHasInput && Speed > IdleSpeedDeadband` |
| `bTransition_IdleToStart` | `bool` | **Edge** (rising) — dispara 1 frame ao iniciar movimento |
| `bTransition_StartToLoop` | `bool` | **Sustained** — alto enquanto `LocomotionStartElapsed >= StartMaxPlayTime` |
| `bTransition_LoopToStop` | `bool` | **Sustained** — alto desde soltar input até a fase de Stop encerrar; vira **false** no mesmo frame que `bTransition_StopToIdle` vira true (ver §5.6) |
| `bTransition_StopToIdle` | `bool` | **Sustained** — alto quando a anim Stop atinge o **motion end** (sem novo input) **ou** `Speed < IdleSpeedDeadband && !input`. Use nas **duas** transições para idle (layer **e** main). Semântica mudou em 2026-05-26 — ver §5.6 |
| `LocomotionStartElapsed` | `float` | Tempo no Start (s); zera ao ficar parado |

> **Semântica edge vs sustained:** AnimBP transitions disparam apenas na borda de subida da condição, então flags sustentadas funcionam bem. Não tente fazer edge-detection manual no AnimBP — deixe a state machine cuidar.

### Tuning (Class Defaults do `ABP_JSHeroCharacter` → `DF | Locomotion | Directional`)

| Variável | Default | Função |
|----------|---------|--------|
| `WalkSpeedThreshold` | **50** cm/s | Acima → `Walk`; abaixo → `Idle` |
| `RunSpeedThreshold` | **350** cm/s | Acima → `Run` (ou `Sprint` se flag) |
| `IdleSpeedDeadband` | **5** cm/s | Velocidade abaixo deste valor = considerado parado |
| `StartMaxPlayTime` | **0.80** s | Tempo máximo no Start antes de forçar Start→Loop (default C++; alinhar ao asset, ex. `Run_Start_F_0` ≈ 0,83 s) |
| `StopTailCatchUpSpeedThreshold` | **220** cm/s | (cat. `DistanceMatching`) Speed abaixo do qual o catch-up do Stop engata. Mantenha ≈ run speed senão o Stop toca em **câmera lenta** (§5.6 Fix 3) |
| `StopTailCatchUpSeconds` | **0.20** s | (cat. `DistanceMatching`) ritmo do catch-up; menor = Stop completa mais rápido (§5.6 Fix 3) |
| `WalkStopBrakingDeceleration` | **600** cm/s² | (CMC, cat. `DF\|Movement`) deslize na frenagem para a anim Stop ter distância (§5.6 Fix 2) |

### Getters Blueprint (`BlueprintPure`)

```
Get Locomotion Start Anim → UAnimSequenceBase*
Get Locomotion Loop Anim  → UAnimSequenceBase*
Get Locomotion Stop Anim  → UAnimSequenceBase*
```

`Start` e `Stop` usam suas direções snapshot; `Loop` usa `MovementDirection` em tempo real (atualiza enquanto o jogador esterça).

---

## 2. Setup AnimBP

### 2.0 Mapa visual do seu AnimGraph (estado atual → alvo)

Estes diagramas batem com os nós reais do seu `ABP_JSHeroCharacter` (capturas do editor). Use como referência de "onde estou" vs "onde quero chegar".

#### A) AnimGraph principal — **como está hoje**

```
[Main States]            [Slot 'DefaultSlot']        [Control Rig]
 (State Machine) ───────▶  Group 'DefaultGroup' ────▶  Alpha = 1.0          ───▶ [Output Pose]
                            (montages: attack,          Should Do IKTrace ◀── (NOT)── (Is Falling)
                             dodge, hit, etc.)

[Locomotion]
 (State Machine) ───────▶ [Locomotion] (pose)     ← SM paralela/auxiliar; ver nota
```

- **`Main States`** é a cadeia que vai ao `Output Pose`: locomoção/estados base → **`Slot 'DefaultSlot'`** (onde as montages de attack/dodge/hit se sobrepõem) → **`Control Rig`** (foot IK).
- O **Control Rig** já faz o foot IK e está **gated** por `Should Do IKTrace = NOT(Is Falling)` — ou seja, IK de pé só no chão. 👍 Isso **substitui** o "Two Bone IK" manual que a §5.3 descrevia (ver nota em §5.3/§6).
- A SM **`Locomotion`** separada (que termina num pose isolado) é auxiliar/legada. O fluxo que importa para este doc é o que passa por `Main States → Slot → Control Rig → Output`. Se o seu `Walk/Run` real mora dentro de `Main States` (ou num linked layer chamado por ele), é **lá** que entra o `Walk_Run_SM` (§2.2).

#### B) AnimGraph principal — **alvo (com refinamentos AAA)**

Os refinamentos entram **entre** a saída da locomoção e o `Slot 'DefaultSlot'` (warping sobre a pose de locomoção, antes das montages), mantendo o seu `Control Rig` no fim:

```
[Main States / Locomotion]
        │
        ▼
[Stride Warping]              ← §5.2  (alpha = StrideWarpingAlpha)
        │
        ▼
[Layered Blend per Bone]      ← §5.5  Aim Offset (Branch: spine_01, alpha = AimOffsetAlpha)
        │
        ▼
[Slot 'DefaultSlot']          ← montages (attack / dodge / hit) por cima
        │
        ▼
[Control Rig]                 ← §5.3  foot IK que você JÁ tem (Should Do IKTrace = NOT Is Falling)
        │
        ▼
[Output Pose]
```

> **Distance Matching (§5.1)** não aparece aqui porque vive **dentro** dos estados `Start`/`Stop` do `Walk_Run_SM` (controla o tempo do clip), não no graph principal.

#### C) Locomotion SM — **como está hoje** (sua 2ª captura)

```
[Entry] ──▶ ( Idle )  ⇄  ( Walk / Run )
```

#### D) Locomotion SM — **alvo**: expandir o estado `Walk / Run`

Você **não** muda a SM de cima; você abre o estado `Walk / Run` e troca o conteúdo dele por uma **sub-state machine** `Walk_Run_SM` (§2.2):

```
( Walk / Run )  ← abrir este estado e colocar dentro:

   [Entry] → [Idle Pose] ──▶ [Start] ──▶ [Loop] ──▶ [Stop] ──▶ [Idle Pose]
                  ▲                                                 │
                  └─────────────────────────────────────────────────┘
```

> Resumo do caminho: **(C)** continua igual por fora; **(D)** é o miolo novo do estado `Walk/Run`; **(B)** é a ordem final do graph principal mantendo o seu `Control Rig`.

---

### 2.1 Preencher `DefaultAnimSet` no `ABP_JSHeroCharacter`

Asset: `Content/DungeonForged/Character/JSHero/Animation/BPAnim/ABP_JSHeroCharacter.uasset`

**Class Defaults → DefaultAnimSet → DF | Anim Set - Locomotion | Directional:**

1. Expanda **WalkSet** → arraste as 8 Start, 8 Loop e 8 Stop de Walk.
2. Expanda **RunSet** → arraste as 8 Start, 8 Loop e 8 Stop de Run.

> Use a pasta real do seu projeto (provavelmente `Content/DungeonForged/Character/JSHero/Animation/...`). Ajuste o caminho conforme seu layout.

Slots vazios são OK — o resolver cai no Forward correspondente.

### 2.2 Substituir o estado `Walk/Run` no Locomotion SM

#### Como está hoje (confirmado pelos seus prints)

Sua locomoção usa **linked anim layers** em 3 assets:

```
[ABP_JSHeroCharacter]  (AnimBP principal, parent: UDFAnimInstance)
   └─ AnimGraph → SM "Locomotion" → estado "Walk/Run"
        └─ [Linked Anim Layer]
              Layer          = FullBody_LocomotionState
              Instance Class = ABP_JCHero_UnArmed_Layer
              → Output Animation Pose

[ABP_JCHero_UnArmed_Layer]  (parent: ABP_LayerBase)
   └─ Default Anim Set: IdleAnimation=Idle_Seq, MovementBlendSpace=BS_JCHero_IdleWalk_UnArmed,
                        StrafeBlendSpace=None, WalkSet/RunSet=(a preencher)
   └─ Anim Graph Overrides: FullBody_IdleState, FullBody_LocomotionState

[ABP_LayerBase]  (parent: UDFAnimInstance)
   └─ função FullBody_LocomotionState =
        [Default Anim Set] → [Break UD Anim Set] → [Blendspace Player (movement)]
                                                  → [Blendspace Player (strafe)]
                                                  → [Blend Poses by bool (bShouldStrafe)] → [Output Pose]
```

> Ou seja: o estado `Walk/Run` do main não tem nós de locomoção — ele só **chama** a função `FullBody_LocomotionState`, cujo conteúdo real mora em `ABP_LayerBase` (e o `Default Anim Set` é preenchido no layer filho `ABP_JCHero_UnArmed_Layer`).

#### Onde colocar o 8-way (decisão)

Implemente o `Walk_Run_SM` **dentro da função `FullBody_LocomotionState` do `ABP_LayerBase`**. Motivo: os getters `Get Locomotion Start/Loop/Stop Anim` leem o `ActiveAnimSet`/`DefaultAnimSet` da **instância do layer** (cada layer fornece o seu set). Colocando a SM no `ABP_LayerBase`, **tanto o desarmado quanto o armado herdam** a mesma lógica — você só troca os assets do Anim Set por layer. Não precisa duplicar grafo.

> ✅ **As variáveis funcionam dentro do layer** — mas a instância do linked layer **não** recalcula sozinha `Gait`, `bIsAccelerating` nem as flags `bTransition_*` (isso roda só no AnimInstance **principal** do mesh). O C++ **copia** esse estado do main para o layer a cada frame (`CopyDirectionalLocomotionStateFrom` / `PropagateDirectionalLocomotionToLinkedAnimLayers`). Sem isso, `bIsAccelerating` fica sempre `false` no layer e a `Walk_Run_S` nunca sai do `Idle Pose` — mesmo com `[Loco|Main] Accel=1` no log. Com `df.LocomotionDebug 1`, confirme que **`[Loco|Layer] Accel=1`** aparece junto do Main.

> ⚠️ **Cuidado com o override:** no print do `ABP_JCHero_UnArmed_Layer`, o painel **Anim Graph Overrides** lista `FullBody_LocomotionState`. Se esse layer tiver um **override real** (conteúdo próprio) dessa função, editar só o `ABP_LayerBase` **não** terá efeito. Duas saídas:
> - **(Recomendado)** No `ABP_JCHero_UnArmed_Layer`, clique direito em `FullBody_LocomotionState` → **Remove Override** (ou "Reset to Parent") para ele herdar a SM do base.
> - **(Alternativa)** Ou edite a SM **dentro** do override do próprio layer (e repita no armado depois).

#### Passo-a-passo da migração

1. Abra **`ABP_LayerBase`** → função **`FullBody_LocomotionState`**.
2. **Não apague tudo ainda.** Selecione os nós atuais (`Break UD Anim Set → Blendspace Players → Blend Poses by bool`) e **recorte/afaste** para o lado — eles viram o seu **fallback** (§ "Fallback Blend Space" abaixo).
3. Crie uma **State Machine** nova chamada `Walk_Run_SM` e ligue a saída dela ao `Output Pose`.
4. Monte os 4 estados e transições conforme as tabelas abaixo (Idle Pose / Start / Loop / Stop).
5. Ligue os getters `Get Locomotion Start/Loop/Stop Anim` nos `Sequence Evaluator`/`Sequence Player` (pin `Sequence` exposto).
6. Compile. Com `WalkSet`/`RunSet` ainda vazios, os getters retornam `nullptr` e a SM vai direto Idle→Loop sem quebrar (ver Fallback).
7. Preencha `WalkSet`/`RunSet` no `Default Anim Set` do `ABP_JCHero_UnArmed_Layer` (§2.1) conforme for tendo os assets.

#### Fallback Blend Space (migração gradual segura)

Para não perder a locomoção enquanto os 8-way não estão completos, mantenha o caminho antigo como fallback. A forma mais simples:

- No estado **`Loop`**, em vez de um Sequence Player puro, use um **`Blend Poses by bool`**:
  - **bool** = "tenho asset 8-way?" → ligue em algo verdadeiro quando o getter não for nulo. Como não há um bool C++ pronto, o truque prático: ligue o getter `Get Locomotion Loop Anim` num **Sequence Player (pin Sequence exposto)**; se o asset vier nulo, o player não toca nada — então deixe o **Blend Space antigo** (`Break Anim Set → Movement/Strafe Blendspace`) como a outra pose e alterne por `bShouldStrafe` igual hoje.
- Enquanto `WalkSet`/`RunSet` estiverem vazios, a SM pode nem entrar em Start/Loop (transições falham com getter nulo) e você cai no Blend Space. Conforme preenche, o 8-way assume.

> Quem prefere algo mais limpo: exponha um `UFUNCTION BlueprintPure bool HasDirectionalLocomotion() const` no `UDFAnimInstance` (1 linha: `return ActiveAnimSet.RunSet.IsValid() || ActiveAnimSet.WalkSet.IsValid();`) e use como condição do `Blend Poses by bool` topo (8-way vs Blend Space). Opcional.

#### Estrutura — Substate Machine `Walk_Run_SM`

```
[Entry] → [Idle Pose] ⇄ [Start] → [Loop] → [Stop] → [Idle Pose]
                              ↑       │
                              └───────┘  (Loop → Start em redirect brusco)
```

#### Estados

| Estado | Conteúdo | Loop? |
|--------|----------|-------|
| `Idle Pose` | Sequence Player → `IdleAnimation` (ou cached pose vinda do estado pai) | **true** |
| `Start` | **Sequence Evaluator** + `Advance Time by Distance Matching` (§5.1) — pin `Sequence` → **Get Locomotion Start Anim** | **false** |
| `Loop` | Sequence Player com pin `Sequence` exposto, ligado a **Get Locomotion Loop Anim** | **true** |
| `Stop` | **Sequence Evaluator** + `Distance Match to Target` (§5.1) — pin `Sequence` → **Get Locomotion Stop Anim** | **false** |

> **Importante sobre o pin Sequence:** clique com botão direito no Sequence Player → **Expose As Pin → Sequence**. Isso converte a propriedade `Sequence` (que é "bind" estático) em **pin de entrada** ligável. Ligue o getter `Get Locomotion Loop Anim` nesse pin. Cada frame o Sequence Player re-resolve qual asset tocar — assim trocar de direção durante o Loop atualiza a animação instantaneamente sem precisar transicionar de estado.

> ⚠️ **Por que `bIsAccelerating` e não `bTransition_IdleToStart` na saída do `Idle Pose`:** `bTransition_IdleToStart` é uma **borda de 1 frame**. Como a `Walk_Run_SM` é **aninhada** dentro do estado `Walk/Run` (que só ativa em `Speed > 10`), essa borda costuma ser disparada **antes** da SM interna ficar ativa → o `Idle Pose` nunca a vê e fica preso (personagem "anda parado no idle"). `bIsAccelerating` é **nível** (fica `true` enquanto move com input), então funciona de forma confiável. A direção do Start (`LocomotionStartDirection`) já é capturada em C++ na borda, então continua correta. Veja o diagnóstico completo em §3.2.

#### Transições

| Transição | Condição | Blend (s) | Mode |
|-----------|----------|-----------|------|
| `Entry → Idle Pose` | (default) | — | — |
| `Idle Pose → Start` | **`bIsAccelerating`** (nível — ver nota) | 0.10 | Hermite |
| `Idle Pose → Loop` *(opcional)* | `bIsAccelerating` **AND** `LocomotionStartElapsed >= StartMaxPlayTime` | 0.10 | Hermite |

> **C++ (v2026-05):** `LocomotionStartGait` e `LocomotionStopGait` congelam o gait usado em Start/Stop. `Get Locomotion Stop Anim` usa `LocomotionStopGait`. `StartElapsed` deixa de subir indefinidamente após `StartMaxPlayTime` quando você já está em Loop.
| `Start → Loop` | `bTransition_StartToLoop` **OR** `Time Remaining (ratio) < 0.08` | 0.12 | Hermite |
| `Loop → Stop` | `bTransition_LoopToStop` | 0.15 | Hermite |
| `Stop → Idle Pose` | **`bTransition_StopToIdle`** (dispara no motion end desde 2026-05-26 — ver §5.6 Fix 4 / §2.4) | **0.10–0.15** | Hermite |
| `Stop → Start` (ou `Stop → Loop`) | **`bTransition_StopToMove`** — obrigatório se usar Stop com Evaluator | 0.08–0.15 | Hermite |
| `Stop → Start` (alternativa) | `bTransition_IdleToStart` — só 1 frame; pode falhar | 0.08 | Hermite |
| `Loop → Start` *(opcional)* | mudança brusca de direção — ver §2.3 | 0.10 | Hermite |

#### Re-direct durante o Loop (opcional)

O Loop já segue `MovementDirection` em tempo real, então uma rotação suave (ex.: virar enquanto corre) é absorvida pelo próprio Loop player. A transição `Loop → Start` só é útil se você quiser tocar a **animação de Start** novamente em um redirect ≥ 90°:

```
Condição: AbsDelta(Yaw com MovementDirection trocando de half-plane) > 90  AND  bIsAccelerating
```

Implementar como uma flag custom no AnimInstance se quiser esse efeito; para 99% dos casos pode pular.

### 2.3 Idle (turn-in-place opcional)

Se quiser TIP, veja §6.4. Para começar, basta o `Idle Pose` simples.

### 2.4 SM externa `Locomotion` no `ABP_JSHeroCharacter` (não cortar Start/Stop)

A `Walk_Run_S` vive no **linked layer**. A SM **externa** no main ABP liga/desliga o layer inteiro. Se ela sair de `Walk/Run` cedo demais, o Stop (ex. `Run_Stop_F_0_Seq`, **1,5 s**) some mesmo com a SM interna correta.

#### Transições recomendadas (main → `AnimGraph` → `Locomotion`)

> **Atualizado 2026-05-26 (§5.6 Fix 4).** O `bTransition_StopToIdle` passou a disparar no **motion end** da anim Stop (não mais no primeiro frame parado). Por isso **as duas** transições para idle — a do layer (`Stop → Idle Pose`) e a do main (`Walk/Run → Idle`) — agora usam o **mesmo flag**, disparando juntas.

| Transição | Condição | Blend (s) | Por quê |
|-----------|----------|-----------|---------|
| `Idle → Walk/Run` | `Speed > 10` (ou `bIsAccelerating`) | 0.20 | Liga o layer quando começa a andar |
| `Walk/Run → Idle` | **`bTransition_StopToIdle`** | 0.20 | Sai junto com o layer, no fim do Stop — sem descompasso |

> **Não use só `Speed <= 10` no `Walk/Run → Idle`.** Com isso, ao soltar W a cápsula ainda está em 300+ cm/s e cai abaixo de 10 em poucos frames — o main vai para `Idle` e mata o linked layer antes do pé plantar no Stop. O `bTransition_StopToIdle` já espera o motion end.

#### `Stop → Idle Pose` (layer) — usa `bTransition_StopToIdle`

> ⚠️ **Mudança de semântica.** A versão anterior deste guia mandava o layer usar `GetRelevantAnimTimeRemaining() <= 0.05` e **evitar** `bTransition_StopToIdle`, porque na época esse flag disparava no primeiro frame parado (`Speed < deadband`) → blend de 0,1 s para Idle Pose → `Run_Stop_F_0` quase invisível.
>
> Com o **§5.6 Fix 4a**, o `bTransition_StopToIdle` passou a disparar no **motion end** do clip (sem novo input). Então agora a regra correta do layer é simplesmente:
>
> ```
> Stop → Idle Pose : bTransition_StopToIdle
> ```
>
> Isso casa com o main (mesma regra) e elimina a "travadinha" do último frame. Se sobrar micro-pop, suba a Duração do blend de `0.10` para `0.15`.

#### Start cortado antes do asset terminar

Default C++ `StartMaxPlayTime` = **0,80 s** (antes 0,45 s). Alinha com `Run_Start_F_0_Seq` (~0,83 s). Ajuste no Class Defaults se o seu Start for mais curto/longo.

---

## 3. Como debugar (locomoção básica)

### Variables Watch (Anim Preview Editor)

| Variável | O que observar |
|----------|----------------|
| `Speed` | XY velocity do CMC (cm/s) |
| `Gait` | `Idle` → `Walk` → `Run` conforme velocidade |
| `MovementDirection` | Atualiza em tempo real conforme você esterça |
| `LocomotionStartDirection` | **Congela** no frame do takeoff (Idle → Start) |
| `LocomotionStopDirection` | **Congela** no frame em que o input é solto |
| `bTransition_IdleToStart` | Pisca 1 frame na partida |
| `bTransition_StartToLoop` | Sobe quando passa `StartMaxPlayTime` |
| `LocomotionStartElapsed` | Cresce de 0 até `StartMaxPlayTime`, então congela |

### Fluxo esperado

1. **Parado** → `Gait=Idle`, todas as flags em false.
2. **W pressionado** → `bTransition_IdleToStart=true` por 1 tick → SM entra em `Start`; `LocomotionStartDirection=Forward`.
3. **Após `StartMaxPlayTime` (~0,80 s)** → `bTransition_StartToLoop=true` → SM entra em `Loop` (Start visível quase inteiro).
4. **Esterçar enquanto corre** → `MovementDirection` muda → `Get Locomotion Loop Anim` retorna outro asset → Sequence Player troca sem mudança de estado.
5. **W solto** → `bTransition_LoopToStop=true` → SM entra em `Stop`; `LocomotionStopDirection` congela na direção atual; a cápsula **desliza** (braking `WalkStopBrakingDeceleration`, §5.6) e o `Time=` do clip sobe `0 → 0.77 (end)` acompanhando o deslize.
6. **Stop atinge o motion end** → `bTransition_LoopToStop` vira false e `bTransition_StopToIdle=true` (§5.6 Fix 4) → layer **e** main vão a `Idle` **no mesmo instante** (ambas as transições usam `bTransition_StopToIdle`).

### 3.1 Como abrir o debugger no lugar certo (linked layer!)

⚠️ **A SM `Walk_Run_S` mora no LAYER (`ABP_Test_UnArmed_Layer` / `ABP_LayerBase`), não no `ABP_JSHeroCharacter`.** Para debugar:

1. Dê **Play (PIE)**.
2. Abra o asset **`ABP_TestLayerBase`** (onde está a `Walk_Run_S`).
3. Na barra de topo, no dropdown **"Nenhum objeto de depuração selecionado"**, escolha a instância do seu personagem → o sub-item do **linked layer** (`...FullBody_LocomotionState`). Agora o grafo mostra o **estado ativo destacado** e os valores das variáveis ao vivo.
4. Alternativa rápida no jogo: console **`ShowDebug Animation`** — mostra a SM e os estados ativos (inclusive aninhados) com peso.

> Se você selecionar o `ABP_JSHeroCharacter` no debugger, verá só a SM externa (`Idle ⇄ Walk/Run`), **não** a `Walk_Run_S`. Por isso parece "idle".

### 3.2 Diagnóstico: "anda mas não toca Start/Loop, só toca ao parar"

Esse é o sintoma mais comum nessa arquitetura. Siga a árvore:

**① O estado `Walk/Run` (externo) está ativo enquanto você anda?**
Olhe a SM externa no `ABP_JSHeroCharacter`. Se **não** entra em `Walk/Run`, o problema é a transição externa (`Speed > 10`). Provável: o personagem nem está ganhando velocidade, ou `Speed` não chega no AnimInstance. Confirme `Speed` no watch.

**②a Linked layer sem flags de locomoção (causa nº 1 nos logs `S>L=1` mas personagem no idle)**  
Se `[Loco|Main]` mostra `Accel=1` / `S>L=1` mas **`[Loco|Layer] Accel=0`** (ou Layer nem aparece), a `Walk_Run_S` está lendo variáveis **zeradas** na instância do layer. **Fix:** recompile com a propagação C++ (§2.2 nota acima) ou, até lá, teste temporariamente a SM no main AnimBP (sem linked layer).

**②b Entra em `Walk/Run`, mas a SM interna `Walk_Run_S` fica presa em `Idle Pose`?** ← **causa nº 2 (borda / condição)**

Esse é o **bug da borda em SM aninhada**: a transição `Idle Pose → Start` usa `bTransition_IdleToStart`, que é uma **borda de 1 frame** disparada quando você começa a se mover (`Speed > 5`). Mas a SM interna só fica ativa quando a externa entra em `Walk/Run` (`Speed > 10`) — **1–2 frames depois**, quando a borda **já passou**. Resultado: `Idle Pose` nunca recebe o `true` → fica preso no idle enquanto anda.

**✅ Correção (recomendada):** troque a condição de `Idle Pose → Start` de borda para **nível**:

| Transição | Condição ANTIGA (frágil) | Condição NOVA (robusta) |
|-----------|--------------------------|--------------------------|
| `Idle Pose → Start` | `bTransition_IdleToStart` | **`bIsAccelerating`** |

`bIsAccelerating` é `true` o tempo todo enquanto você move com input — então, assim que a SM interna fica ativa, ela sai de `Idle Pose` imediatamente. A **direção** do Start (`LocomotionStartDirection`) continua correta porque é capturada em C++ no momento da borda, independente do frame.

> **Opcional (mais à prova de falha):** adicione também `Idle Pose → Loop` com condição `bIsAccelerating AND LocomotionStartElapsed >= StartMaxPlayTime` — assim, se você entrou em `Walk/Run` já em velocidade plena (sem fase de Start), vai direto pro `Loop` sem tocar o Start do zero.

**③ Sai do `Idle Pose` (entra em Start/Loop) mas aparece pose congelada / T-pose?**
O getter está retornando `nullptr`. Duas causas:

- **(a) Anim Set na instância errada.** Os getters `Get Locomotion Start/Loop/Stop Anim` leem o `ActiveAnimSet` **da instância do layer** (`ActiveAnimSet = DefaultAnimSet` no init). Preencha `WalkSet`/`RunSet` no **`Default Anim Set` do `ABP_Test_UnArmed_Layer`** — **não** adianta preencher só no `ABP_JSHeroCharacter` (instância diferente). *(Nos seus prints, o Start estava preenchido no main e Loop/Stop no layer — unifique tudo no layer.)*
- **(b) Gait mismatch (Walk não cai em Run).** O resolver escolhe `WalkSet` quando `Gait == Walk` e **só** cai para `WalkSet` vindo de Run — **nunca Walk → Run**. Então, se você preencheu **só o `RunSet`** e está andando devagar (`Gait == Walk`, `Speed` entre 50 e 350), o Loop fica `nullptr` → congela. **Fix:** preencha também o `WalkSet`, **ou** baixe `RunSpeedThreshold`, **ou** preencha o set que casa com a velocidade do teste. Confirme `Gait` no watch.

**④ Pino `Sequence` não exposto/ligado?**
No `Sequence Player` do Loop, confirme **Expose As Pin → Sequence** e que o `Get Locomotion Loop Anim` está **ligado** nesse pino. Sem isso ele toca um asset fixo (ou nada).

#### Variáveis-chave para assistir (no debugger do layer)

| Variável | Diz o quê |
|----------|-----------|
| `Speed` / `Gait` | Confirma velocidade e qual set (`Walk`/`Run`) está sendo resolvido |
| `bIsAccelerating` | Deve ficar **`true`** enquanto anda com input (use como condição nova) |
| `bTransition_IdleToStart` | Pisca **1 frame** na partida — se você "perde" esse frame, é o bug ② |
| `LocomotionStartElapsed` | 0 → `StartMaxPlayTime` durante o Start |
| `MovementDirection` | Direção resolvida do Loop |
| **estado ativo da `Walk_Run_S`** | Idle Pose? Start? Loop? — destacado no grafo |

### 3.3 Comandos de console — debug visual + log em tempo real

Existem dois grupos: **comandos nativos da Unreal** (funcionam já, sem compilar) e o **comando do projeto `df.LocomotionDebug`** (precisa recompilar o C++).

#### A) Nativos da Unreal (zero código)

| Comando (no console `~` em PIE) | O que mostra |
|---------------------------------|--------------|
| `ShowDebug Animation` | HUD on-screen com **state machines ativas**, estados aninhados com **peso**, montagens, curvas e relevant anims em tempo real. Melhor visão geral. |
| `ShowDebug Animation` + `PageUp`/`PageDown` | Alterna entre os "Debug Display" (Graph, FullGraph, CurveValues, Montage, Notifies…). |
| `a.animnode.statemachine.debug 1` | Loga transições de state machine. |
| `a.animnode.distancematching.debug 1` | Visualiza o avanço dirigido por distância (se a build expõe). |
| `a.animnode.stridewarping.debug 1` | Desenha o stride warping (passos/pés). |
| `a.animnode.orientationwarping.debug 1` | Desenha o orientation warping. |
| **AnimBP "Debug Object"** (no editor, em PIE) | Selecione a instância → **sub-item do linked layer** (`...FullBody_LocomotionState`) e veja o **estado ativo destacado** + variáveis ao vivo. Veja §3.1. |
| **Rewind Debugger** (`Tools → Debug → Rewind Debugger`) | Grava e faz scrub da timeline de anim/montagem/notifies/variáveis — ideal para inspecionar "o que tocou quando". |

> ⚠️ Lembre da §3.1: aponte o Debug Object para o **layer** (`ABP_LayerBase`), senão você vê só a SM externa e parece "idle".

#### B) Comando do projeto: `df.LocomotionDebug` (novo)

CVar: `df.DebugLocomotion` (gated em `!UE_BUILD_SHIPPING`). Foi feito sob medida para a SM 8-way **Start/Loop/Stop** — mostra **MAIN e LAYER lado a lado** (pra você ver imediatamente se o `ActiveAnimSet` está na instância errada).

| Comando | Nível | Efeito |
|---------|-------|--------|
| `df.LocomotionDebug` | toggle | Alterna 0 → 1 → 2 → 3 → 0 |
| `df.LocomotionDebug 1` / `log` | 1 | Log `[Loco]` no Output Log (filtra por `Loco`) |
| `df.LocomotionDebug 2` / `hud` | 2 | Log + **HUD on-screen**: bloco **verde `[MAIN]`** e bloco **amarelo `[LAYER]`** com Speed, Gait, Dir, flags de transição e os assets Start/Loop/Stop resolvidos |
| `df.LocomotionDebug 3` / `draw` | 3 | HUD + **setas no mundo**: **azul = facing**, **verde = velocidade** |
| **`df.LocomotionDebug 4`** / `deep` / `verbose` | 4 | HUD expandido + o **mesmo texto** no Output Log como `[Loco|HUD]` a cada **0,25 s** (parado ou andando) |
| Níveis **2–3** com log | 2+ | Se `1` ou `2` estiver ativo, `[Loco|HUD]` espelha o HUD base a cada **0,5 s** |
| `df.LocomotionDebug dump` | — | Dump base + bloco **Deep** no Output Log |
| `df.LocomotionDebug 0` / `off` | 0 | Desliga |

**O HUD on-screen (nível 2) mostra, por instância:**

```
== Locomotion [MAIN] ABP_JSHeroCharacter_C ==
Speed=0 Gait=Idle Dir=Forward Accel=0 Strafe=0
StartDir=Forward StopDir=Forward StartElapsed=0.00/0.45s
Trans: Idle>Start=0 Start>Loop=0 Loop>Stop=0 Stop>Idle=0 TIP=0
Anim Start=(null)        ← se aparecer (null) aqui mas o LAYER tem o asset, o set está na instância errada
Anim Loop =(null)
Anim Stop =(null)
StrideAlpha=0.00 DistMatch=0
--- Deep (capsule vs anim) ---
VelXY=540 Dir=0.0 Input=1.00 MaxWS=540 RunCfg=540 SprintCfg=750 Sprint=0
GaitThr Walk>=50 Run>=350 | AuthoredLoop=400 StrideScale=1.35 (vs curve 1.28)
Loop: Run_Loop_F_0_Seq len=0.80s RM=1 Dist 0->432 avgSpd~540
WARN AuthoredLoopSpeed(400) != loop Distance avg(540) — tune Class Defaults
== Locomotion [LAYER] ABP_LayerBase_C ==
Speed=420 Gait=Run Dir=Forward Accel=1 Strafe=0
...
```

> **Diagnóstico direto do seu bug "fica no idle ao andar":** com `df.LocomotionDebug 2`, ande e observe o bloco **`[LAYER]`**. Se `Accel=1` e `Gait=Run` mas `Trans: Idle>Start` nunca pisca `1` (ou pisca e a SM não sai do idle), confirma o **bug da borda** da §3.2 → troque a condição para `bIsAccelerating`. Se `Anim Loop=(null)` no layer, é **Gait/AnimSet mismatch** (§3.2 ③).

**Arquivos** (já implementados): `df.DebugLocomotion` em `DFLocomotionDebug.h/.cpp`, HUD/setas em `UDFAnimInstance::NativeUpdateAnimation`, string em `BuildDirectionalLocomotionDebugString()`, comando em `UDFCheatManager.cpp`. **Recompile** o módulo `DungeonForged` para ativar.

---

## 4. Padrão de naming dos assets

Convenção `EDFMovementDirection` ↔ slot da struct ↔ suffix comum em assets:

| `EDFMovementDirection` | Slot | Suffix típico no asset |
|------------------------|------|------------------------|
| `Forward` | `Start_F` / `Loop_F` / `Stop_F` | `_F_0`, `_F` |
| `ForwardRight` | `Start_FR_45` etc. | `_F_R_45`, `_FR_45` |
| `Right` | `Start_R_90` etc. | `_R_90`, `_R` |
| `BackwardRight` | `Start_BR_135` etc. | `_B_R_135`, `_BR_135` |
| `Backward` | `Start_B_180` etc. | `_B_180`, `_B` |
| `BackwardLeft` | `Start_BL_135` etc. | `_B_L_135`, `_BL_135` |
| `Left` | `Start_L_90` etc. | `_L_90`, `_L` |
| `ForwardLeft` | `Start_FL_45` etc. | `_F_L_45`, `_FL_45` |

Não há restrição de naming dos assets — o mapeamento é feito ao arrastar a sequence no slot correspondente.

---

## 5. Refinamentos AAA — Plugins, C++ e Setup no Editor

### Plugins habilitados (`DungeonForged.uproject`)

| Plugin | Função | Status no `.uproject` |
|--------|--------|------------------------|
| `AnimationWarping` | Nós **Stride Warping**, **Orientation Warping** e **Slope Warping** no AnimGraph | ✅ Enabled |
| `MotionTrajectory` | `UCharacterTrajectoryComponent` (já adicionado em C++ no `ADFPlayerCharacter::CharacterTrajectory`) | ✅ Enabled |
| `AnimationLocomotionLibrary` | Funções de **Distance Matching** (`AnimDistanceMatchingLibrary`) + nó **Foot Placement** | ✅ Enabled |
| `MotionWarping` | `UMotionWarpingComponent` (root-motion steering de ataques / mira) | ✅ Enabled |
| `Chooser` *(opcional)* | Seleção data-driven de animações por contexto | ✅ Enabled |

Módulos C++ ligados em `DungeonForged.Build.cs` (`PublicDependencyModuleNames`): `AnimGraphRuntime`, `AnimationWarpingRuntime`, `MotionTrajectory`, `MotionWarping`.

> **Importante (UE 5.4):** o "Distance Matching" **não é um nó único** chamado `Distance Matching to Time`. No 5.4 ele é um conjunto de **anim node functions** da `AnimDistanceMatchingLibrary` (`Advance Time by Distance Matching` para o Start; `Distance Match to Target` para o Stop) que você liga a um nó **Sequence Evaluator** via uma função "On Update" do nó. A curva de distância é gerada por um **Animation Modifier (`Distance Curve Modifier`)**, não por "Apply Root Motion → Distance". Detalhes corretos na §5.1. O módulo runtime (`AnimationLocomotionLibraryRuntime`) **não precisa** estar no `Build.cs` porque o wiring é todo em Blueprint/AnimBP — basta o plugin habilitado.

### Variáveis C++ feeders (AnimInstance → AnimBP)

| Variável | Tipo | Para que serve |
|----------|------|----------------|
| `DistanceMatchingDistance` | `float` | Distância XY acumulada desde a borda Idle→Start |
| `DistanceMatchingDelta` | `float` | Delta XY deste frame → **Advance Time by Distance Matching** (Start) |
| `DistanceMatchingStopToTarget` | `float` | Distância restante até parar → **Distance Match to Target** (Stop) |
| `AuthoredStopDistance` | `float` | Span da curva Stop (default **202** cm, `Run_Stop_F_0`) |
| `DistanceMatchingStartSpeed` | `float` | Velocidade no frame do takeoff (debug / curvas adaptivas) |
| `StrideWarpingAlpha` | `float` | 0..1 suavizado → alpha do nó **Stride Warping** |
| `AuthoredLoopSpeed` | `float` | Velocidade de autoria das animações Loop (cm/s) — base do warping |
| `StrideWarpingMinSpeed` | `float` | Abaixo disso, alpha = 0 (não warp em desaceleração) |
| `RootYawOffset` | `float` | Offset (deg) Body↔Aim. Equivale a `AimYaw` quando idle |
| `YawDeltaThisFrame` | `float` | Delta do actor yaw deste tick (debug) |
| `bTransition_TurnInPlace` | `bool` | Sustained — alto enquanto `|RootYawOffset| > TurnInPlaceTriggerYaw` em idle |
| `TurnInPlaceDirection` | `float` | `+1` direita, `-1` esquerda (escolha do asset) |
| `TurnInPlaceTriggerYaw` | `float` | Threshold de disparo (default `60°`) |
| `bLeftFootPlanted` / `bRightFootPlanted` | `bool` | Curva `FootPlant_L/R` > 0.5 — alimenta **Foot Locker** |
| `FootPlantCurveLeft / Right` | `FName` | Nome da curva no asset (default `FootPlant_L` / `FootPlant_R`) |
| `AimOffsetAlpha` | `float` | 0..1 suavizado → alpha do **Apply Additive / Layered Blend** do AimOffset |
| `SetAimOffsetEnabled(bool)` | `UFUNCTION` | Gameplay liga (ex.: ao entrar em lockon) |
| `ConsumeRootYawOffset(float)` | `UFUNCTION` | TIP montage chama via AnimNotify se você usa drain manual |

> Ordem dos updates (importante): `CalculateAimOffsets` **roda antes** de `UpdateTurnInPlace`, garantindo que `AimYaw` esteja atualizado quando o TIP for calculado. **Não inverta a ordem.**

---

### 5.1 Distance Matching — Sincronizar Start/Stop com a distância percorrida

**Problema que resolve:** sem isso, o `Start_*` toca em playrate fixo enquanto o personagem acelera de 0→max. Resultado: pés escorregam no chão durante 0.3–0.5s. O mesmo vale para o `Stop_*` na desaceleração.

> ⚠️ **Correção de precisão (UE 5.4):** as versões antigas deste guia falavam em um nó "Distance Matching to Time" e em "Apply Root Motion → Distance". **Isso não existe no 5.4.** O fluxo correto, validado contra a documentação oficial (§11), usa um nó **Sequence Evaluator** dirigido por **anim node functions** da `AnimDistanceMatchingLibrary`, e a curva gerada por um **Animation Modifier**.

#### Passo 1 — Componente de trajetória (já em C++)

`ADFPlayerCharacter::CharacterTrajectory` (`UCharacterTrajectoryComponent`, plugin `MotionTrajectory`) já está adicionado em C++. Nada a fazer no Blueprint — confirme na hierarquia do `ADFPlayerCharacter`/seu BP de personagem que o componente `CharacterTrajectory` aparece. Acessível via getter **Get Character Trajectory**. *(Obs.: para Distance Matching puro de Start/Stop você nem precisa da trajetória — basta `DistanceMatchingDistance` do AnimInstance. A trajetória é necessária se um dia migrar para Motion Matching.)*

#### Passo 2 — Gerar a curva de distância no asset (Animation Modifier)

Documentação oficial: [Distance Matching in Unreal Engine](https://dev.epicgames.com/documentation/unreal-engine/distance-matching-in-unreal-engine).

Para **cada** `Start_*` e `Stop_*` (8 direções × 2 gaits):

1. Abra a Animation Sequence no Persona.
2. Painel **Animation Data Modifiers** → **Add Modifier** → **`Distance Curve Modifier`**.
3. Configure o modifier:
   - **Curve Name:** `Distance`
   - **Axis:** **`XY`** para locomoção horizontal (Start/Stop run/walk). **Não use `Z`** — isso mede altura e gera curva ~0 (o HUD mostra `(no Distance curve)`).
   - **Stop at End:** ligado no Stop.
   - **Sample Rate:** 30 (default Epic).
4. Clique **Aplicar todos os modificadores** (topo do painel) → **Save** o asset.
5. Verifique no **Curve Editor**: Start **0 → +N** cm; Stop **−N → 0** cm (ex.: `Run_Stop_F_0`: **−202 → 0**). Se o eixo Y mostra `0` a `−0.00003`, o Axis está errado — refaça com **XY**.

#### Passo 2a — Se o modifier der erro (`DistanceCurveModifier` gerou erros)

Isso acontece quando o root **não se desloca** no plano XY — comum com estas opções no `Run_Stop_F_0_Seq`:

| Setting | Valor para gerar curva |
|---------|------------------------|
| **EnableRootMotion** | ✅ ligado |
| **Force Root Lock** | ❌ **desligado** (enquanto aplica o modifier) |
| **Root Motion Root Lock** | **Anim** ou **Zero** — **não** `Ref Pose` + Force Lock |
| **Distance Curve Modifier → Axis** | **XY** |
| Curva `Distance` antiga inválida | Delete a curva flat (~0) no Curve Editor antes de reaplicar |

Depois de gerar a curva correta, você pode religar **Force Root Lock** se o gameplay exigir — a curva já estará bakeada no asset.

#### Passo 2a-b — Aviso SmartName: *"Could not find SmartName Container for Curve Name Distance while trying to remove the curve"*

O modifier tenta **apagar** uma curva `Distance` antiga antes de recriar. No `Run_Stop_F_0_Seq` essa curva ficou **órfã** (tentativa anterior com Axis Z ou curva flat) — existe no asset, mas o skeleton não tem o nome registrado no container legado.

**Clique `Não` no diálogo** até limpar o estado abaixo (aplicar com `Sim` só recria o lixo).

**Opção A — Registrar `Distance` no Skeleton (recomendado; espelha o que já funciona no `Walk_Stop_F_0`):**

1. Abra o **Skeleton** usado por `Run_Stop_F_0_Seq` (Details da sequence → **Skeleton**).
2. No Skeleton Editor: painel **Curve Metadata** / **Animation Curves** (ou *Window → Skeleton → Curve Names*).
3. **Add** curva **`Distance`** — tipo **Animation Curve (Float)**, **não** Morph/Material.
4. **Save** o Skeleton.
5. Volte ao `Run_Stop_F_0_Seq` → timeline **Curvas**:
   - Se `Distance` aparecer na lista: selecione → **Delete** (ou remova todas as keys no Curve Editor).
6. **Save** a sequence (sem curva `Distance` ou vazia).
7. **Distance Curve Modifier** → **Aplicar** → agora **`Sim`** → **Save** a sequence.
8. Confirme no Curve Editor: **~−200 → 0** (eixo em **cm**, não `0.00003`).

**Opção B — Copiar do asset que já funciona:**

1. No Content Browser: **Duplicate** `Walk_Stop_F_0_Seq` → renomeie temporariamente.
2. **Replace** o conteúdo de animação pelo de `Run_Stop_F_0` (*Retarget* ou reassign sequence source se for o mesmo skeleton).
3. Ou: duplique `Walk_Stop`, abra, **File → Reimport** / troque só o clip FBX do run stop — mantém a curva `Distance` válida do walk como base e reaplique o modifier com **Axis XY**.

**Opção C — Curva manual + modifier (se A/B falharem):**

1. Curve Editor → **Add** → Float Curve **`Distance`**.
2. Duas keys: tempo `0` = `0`, tempo final (1.5s) = `-200` (valores aproximados).
3. **Save** → depois rode o modifier para substituir pelos valores do root motion.

> Compare com `Walk_Stop_F_0_Seq`: no HUD já aparece `Dist -65->0`. Abra o walk stop e veja no Skeleton se `Distance` está listada — replique no mesmo skeleton para o run stop.

#### Passo 2b — Compressão (obrigatório para ler curva em runtime)

1. Content Browser → Create Advanced Asset → **Anim Curve Compression Settings** (ou use o `UniformIndexableAnimation...` que você já tem).
2. **Codec:** **Uniform Indexable** (como na doc Epic).
3. No `Run_Stop_F_0_Seq` → Details → **Compression** → **Curve Compression Settings** = esse asset.
4. Re-save a sequence. Sem isso, `EvaluateCurveData` no debug pode falhar mesmo com curva visível no editor.

> A curva é a distância percorrida pelo **root bone** na animação. Por isso os clips de Start/Stop **precisam ter root motion** (mesmo que você não use o root motion para mover a cápsula — o CMC continua dirigindo).

#### Passo 3 — AnimBP: trocar o Sequence Player por um Sequence Evaluator + anim node function

Dentro do estado **`Start`** da `Walk_Run_SM`:

1. **Remova** o Sequence Player e adicione um nó **`Sequence Evaluator`**.
2. Exponha o pino **Sequence** (botão direito → *Expose as Pin → Sequence*) e ligue **Get Locomotion Start Anim**.
3. No nó Sequence Evaluator, no Details → **On Update** → crie/abra uma **Anim Node Function** (binding de função do nó). Dentro dela chame:

```
Advance Time by Distance Matching
  ├─ Update Context     ← pino "Update Context" da função do nó
  ├─ Sequence Evaluator ← referência do próprio nó (pino exposto na função)
  ├─ Distance Traveled  ← DistanceMatchingDistance (delta do frame; ver nota)
  ├─ Distance Curve Name ← "Distance"
  └─ Play Rate Clamp    ← (0.75, 1.25)  [ (0,0) = sem clamp ]
```

- **`Advance Time by Distance Matching`** avança o tempo do clip pela distância percorrida desde o último update, em vez do tempo. É a função correta para o **Start** (anim cíclica/one-shot de aceleração).
- `DistanceMatchingDistance` no AnimInstance é a distância **acumulada** desde o takeoff; o pino da função quer o **delta por frame**. Há duas opções: (a) usar a versão que aceita acumulado mantendo o evaluator em modo "explicit time" + `Distance Match to Target`, ou (b) alimentar o delta. O caminho mais simples e robusto no 5.4 é usar `Advance Time by Distance Matching` com o **delta de distância** do frame. Se preferir não calcular delta no BP, exponha um feeder de delta no AnimInstance (1 linha) ou use o nó com o `Play Rate Clamp` para suavizar.

4. **Transição `Start → Loop`:** mantenha a condição existente (`bTransition_StartToLoop` **OR** `Time Remaining (ratio) < 0.10`). O Distance Matching só controla o **timing interno** do Start.

#### Passo 4 — Stop com Sequence Evaluator (obrigatório para ver o pé plantar)

**Por que não usar Sequence Player no Stop:** o Player avança no **tempo** (1,5 s no `Run_Stop_F_0_Seq`), mas a cápsula para em **~0,1 s**. Você só vê o começo do clip. Nos tutoriais (GDC / Epic), o Stop usa **Sequence Evaluator** + **`Distance Match to Target`**: o tempo do clip segue a **distância que falta** para parar, não o relógio.

**Pré-requisito:** curva `Distance` no asset (Animation Modifier → **Distance Curve Modifier** → Apply). No `Run_Stop_F_0_Seq` deve ir de **~−202 → 0** (HUD deep: `Dist -202->0`).

**Estado `Stop` no `ABP_TestLayerBase`:**

1. Apague o **Sequence Player**.
2. Adicione **Sequence Evaluator**.
3. Botão direito no nó → **Expose as Pin → Sequence** → ligue **Get Locomotion Stop Anim**.
4. No Details do Sequence Evaluator → **On Update** → **Create/Override Anim Node Function**.
5. Dentro da função (não no Event Graph):

```
Distance Match to Target
  ├─ Update Context      ← pino da função do nó
  ├─ Sequence Evaluator  ← referência ao próprio nó (expose na função)
  ├─ Distance to Target  ← DistanceMatchingStopToTarget  (C++)
  └─ Distance Curve Name ← "Distance"
```

6. **Ligue a saída do Sequence Evaluator** ao **Output Animation Pose** (sem isso o estado Stop não emite pose).
7. **Explicit Time — crítico:** **não** deixe `Explicit Time = 0` fixo (congela no primeiro frame / perna no ar). Ligue só **`DistanceMatchingStopExplicitTime`** (C++).
8. **Preferência com curva bakeada (`DistCurve=1`):** use **só** `On Update` → **`Distance Match to Target`** com `DistanceMatchingStopToTarget` — **remova** o pin `Explicit Time` (o C++ ainda preenche `DistanceMatchingStopExplicitTime` para debug/fallback, mas o Evaluator não deve usar os dois).
9. **Run→Stop “fora de sincronia”:** se a cápsula para em ~0,1 s mas `StopTime` fica em 0,1/1,5 s com o personagem parado, o problema era mapeamento linear + decay lento com `Speed=0`. O `StopTarget` consome **distância percorrida** (`Speed×Δt`) e, parado, faz **catch-up**. **Requer o braking deslizante do CMC** (`WalkStopBrakingDeceleration`, §5.6 Fix 2) — senão a cápsula para em ~6 cm e não há distância para consumir.
10. **Stop em “câmera lenta”:** o catch-up precisa engatar **cedo** (`StopTailCatchUpSpeedThreshold = 220`, não 25) e em ritmo natural (`StopTailCatchUpSeconds = 0.20`). Defaults atualizados — **§5.6 Fix 3**. No HUD o `Time=` deve subir `0 → 0.77 (end)` acompanhando o deslize, sem rastejar. `StopCurveNearZeroCm` (8) define o fim-de-movimento. **Stride Warping** fica desligado durante o Stop (evita `Stride Scale 0,5` no overlay).
11. **Sair do Stop ao andar de novo:** adicione transição **`Stop → Start`** (ou `Stop → Loop`) com regra **`bTransition_StopToMove`** (sustained enquanto W durante o Stop). Sem isso o SM só tem `Stop → Idle` e você anda com a pose do fim do Stop presa no Evaluator.
12. **Looping** no Evaluator: **desligado**.
13. **Transição Stop → Idle Pose:** use **`bTransition_StopToIdle`** (agora dispara no motion end — **§5.6 Fix 4**). *Não* use mais `GetRelevantAnimTimeRemaining()`; e use o mesmo flag na transição `Walk/Run → Idle` do main (§2.4).

**Variáveis C++ (já expostas após recompilar):**

| Pin no AnimBP | Variável | Comportamento |
|---------------|----------|----------------|
| Distance to Target (Stop) | `DistanceMatchingStopToTarget` | No frame que solta W, estima ~202 cm × (Speed/RunThreshold); desce com a velocidade até 0 |
| Distance Traveled (Start) | `DistanceMatchingDelta` | `Speed × DeltaTime` por frame no takeoff |
| Class Defaults | `AuthoredStopDistance` | **202** (ajuste se o seu `Run_Stop` tiver outra curva) |

**Teste com `df.LocomotionDebug 4`:** ao soltar W, `StopTarget` deve começar alto (~150–200 em run) e ir a **0** quando a cápsula para — enquanto isso o personagem deve percorrer o corpo inteiro do Stop (joelho flexionando, pé plantando).

**Start (mesmo padrão):** troque o Sequence Player por Evaluator + **Advance Time by Distance Matching** com `DistanceMatchingDelta` e curva `"Distance"` (Passo 3 acima).

#### Debug
- Anim Preview → observe `DistanceMatchingDistance` crescendo de 0 no takeoff.
- Console (PIE): `a.animnode.distancematching.debug 1` (se disponível na sua build) para ver o tempo dirigido por distância.
- Esperado: pés colados ao chão durante toda a aceleração (e desaceleração, se fez o Passo 4).

---

### 5.2 Stride Warping — Esticar passos pela velocidade real

**Problema que resolve:** suas Loops são autoradas a uma velocidade fixa (ex.: 400 cm/s). Se o `MaxWalkSpeed` for 500, o Loop original arrasta os pés. Com warping, o passo é esticado/encolhido para casar com a velocidade atual.

> 📘 **Guia dedicado e detalhado:** [`19_StrideWarping_Setup.md`](19_StrideWarping_Setup.md) — passo-a-passo completo do nó (campos obrigatórios, Foot Definitions, pré-requisito de IK bones no skeleton, Mode Manual vs Graph, debug e troubleshooting do `WARNING!`). Use-o como referência principal; o resumo abaixo é só uma visão rápida.

#### Resumo rápido

1. **Fiação (component space):** `... → [Local To Component] → [Stride Warping] → [Component To Local] → [Slot 'DefaultSlot'] → ...`
2. **Pré-requisito:** o skeleton **precisa** ter `ik_foot_root`, `ik_foot_l`, `ik_foot_r` (senão o dropdown fica `None` e o nó dá `WARNING!`). Ver doc 19, §2.
3. **Campos obrigatórios (matam o warning):**

   | Setting | Valor |
   |---------|-------|
   | **Pelvis Bone** | `pelvis` |
   | **IK Foot Root Bone** | `ik_foot_root` |
   | **Foot Definitions [0]** | IK `ik_foot_l` · FK `foot_l` · Thigh `thigh_l` · Num Bones In Limb `2` |
   | **Foot Definitions [1]** | IK `ik_foot_r` · FK `foot_r` · Thigh `thigh_r` · Num Bones In Limb `2` |

4. **Mode (escolha):**
   - **Graph** (recomendado, se Loops têm root motion): o nó calcula o scale — ligue **`Speed`** no pino `Locomotion Speed`. `Alpha ← StrideWarpingAlpha`.
   - **Manual** (Loops in-place): ligue **`Speed ÷ AuthoredLoopSpeed`** no pino `Stride Scale`. `Alpha ← StrideWarpingAlpha`.

   > ⚠️ O nó **não** tem pino "Locomotion Speed at Authored". A equação é `StrideScale = LocomotionSpeed / RootMotionSpeed`; em Graph o nó faz isso, em Manual você fornece o resultado. `Stride Scale = 1.0` fixo = **sem warp**.

5. **Stride Scale Modifier:** Clamp Result ON, Min `0.5` / Max `1.5`.
6. **Calibrar `AuthoredLoopSpeed`** pela velocidade real do root motion da `Loop_F`.

#### Debug
- Painel **Depurar** do nó → `Enable Debug Draw`, `Debug Draw IKFoot Origin`/`Adjustment`.
- Esperado: passos longos acima de `AuthoredLoopSpeed`, encolhidos abaixo. Pelvis com ajuste sutil.

---

### 5.3 Foot Locker — Travar o pé plantado (anti-deslize)

**Problema que resolve:** mesmo com Stride Warping, em terreno desnivelado / pequenas inconsistências o pé "desliza" 1–2 cm. Foot Locker mantém o pé plantado fixo enquanto a curva `FootPlant_*` > 0.5.

#### Parte A — Curvas no asset

1. Em cada Walk/Run **Loop** (8 direções × 2 gaits):
   - Persona → Window → **Curves** → Add → **Float**: `FootPlant_L` e `FootPlant_R`.
   - Edite com valor **1.0** durante o contato do pé com o chão, **0.0** no swing.
   - Faça rampas suaves de 2–3 frames nos extremos (evita pop do IK).
2. No **Skeleton asset** → tab Curves → confira que `FootPlant_L` e `FootPlant_R` aparecem (sem flags Material/Morph).

> Você pode também animar a curva nas Stops, mas é menos crítico (movimento é rápido).

#### Parte B — AnimGraph

> 🟢 **Você já tem um `Control Rig` no fim do graph (`Should Do IKTrace = NOT Is Falling`).** Esse Control Rig provavelmente já resolve o foot IK por trace — então **você NÃO precisa adicionar os Two Bone IK abaixo**. As opções:
> - **Recomendado (mantém seu setup):** continue usando o `Control Rig`. Para anti-deslize do pé plantado, exponha `bLeftFootPlanted`/`bRightFootPlanted` como **inputs do Control Rig** (variáveis no CR Blueprint) e, dentro do CR, trave o efetor do pé na posição memorizada enquanto a flag for `true`. Assim o "Foot Locker" vira lógica do seu próprio Control Rig, sem nós extras no AnimGraph.
> - **Alternativa (sem Control Rig):** se um dia remover o Control Rig, use os **Two Bone IK** descritos abaixo.
>
> Em ambos os casos as **curvas `FootPlant_L/R` da Parte A** continuam necessárias — elas alimentam `bLeftFootPlanted`/`bRightFootPlanted` no AnimInstance.

Adicione **Two Bone IK** para cada pé após o Stride Warping *(apenas se você NÃO usar Control Rig para foot IK)*:

```
[Stride Warping] → [Two Bone IK: foot_l] → [Two Bone IK: foot_r] → (próximo estágio)
                            ↑                          ↑
                            bLeftFootPlanted           bRightFootPlanted
                            (Alpha Bool Value)         (Alpha Bool Value)
```

Configuração de **Two Bone IK** (esquerda, espelhe para a direita):

| Pino | Valor |
|------|-------|
| **IK Bone** | `foot_l` |
| **Effector Location Space** | `World Space` |
| **Effector Target Location** | posição **memorizada** do `foot_l` no frame em que `bLeftFootPlanted` subiu de false→true |
| **Joint Target Location** | offset do joelho (frontal) |
| **Alpha Input Type** | `Bool` |
| **Alpha Bool Value** | `bLeftFootPlanted` |
| **Alpha Bool Blend** | 0.10 s in / out |

**Memorizar a posição plantada — duas opções:**
- **(simples)** Use o nó **Foot Placement** do plugin `AnimationLocomotionLibrary`. Ele faz cache do plant point automaticamente quando a curva passa 0.5.
- **(manual)** Em um AnimGraph Function, capture `GetSocketLocation("foot_l", World)` quando `bLeftFootPlanted` muda de false→true; mantenha o valor enquanto continua plantado.

#### Debug
- Console: `Show Skeleton 1` + `ShowDebug Bones`. IK targets em vermelho.
- Esperado: pés fixos no chão durante o plant, sem deslize lateral mesmo em rampa.

---

### 5.4 Turn-In-Place — Giro quando parado

**Problema que resolve:** sem TIP, se o player gira a câmera com personagem parado, o personagem teleporta (ou roda instantaneamente) para alinhar. Com TIP, ele toca uma animação de giro suave.

#### Como o C++ funciona

O `UDFAnimInstance::UpdateTurnInPlace()` roda **após** `CalculateAimOffsets()`. Em idle, `RootYawOffset = AimYaw` — ou seja, o delta entre orientação do controller e do actor. Quando o player gira a câmera, `AimYaw` cresce → `RootYawOffset` cresce. Quando `|RootYawOffset| > TurnInPlaceTriggerYaw` (default 60°), `bTransition_TurnInPlace` sobe.

**Pré-requisito do CharacterMovement:**

| Variável | Valor | Por quê |
|----------|-------|---------|
| `bOrientRotationToMovement` | **true** | Personagem orienta na direção do movimento quando se move |
| `bUseControllerRotationYaw` | **false** | Personagem **não** rotaciona automaticamente para o controller — TIP precisa que o actor yaw fique livre |

Isso significa: parado, o actor yaw fica fixo até o TIP rotacioná-lo. Movendo, `bOrientRotationToMovement` cuida do alinhamento.

#### Rotacionando o actor a partir da animação — duas opções

**Opção A — Root Motion (recomendado):**
- A animação `Turn_R_90` tem Root Motion habilitado **na rotação** (não na translação).
- UE auto-aplica a rotação no actor durante a animação.
- Conforme o actor rotaciona, `AimYaw` diminui → `RootYawOffset` diminui → `bTransition_TurnInPlace` cai naturalmente.

**Opção B — Custom AnimNotify drain:**
- Animação sem root motion; você dispara `UUDFAnimInstance::ConsumeRootYawOffset(amount)` num notify e separadamente rotaciona o actor via `SetActorRotation` ou notify state.
- Mais complexo. Use só se não puder colocar root motion.

#### Assets necessários

| Asset | Yaw delta | Quando usar |
|-------|-----------|-------------|
| `TIP_L_90` | ~–90° | `RootYawOffset < -45° && RootYawOffset > -135°` |
| `TIP_R_90` | ~+90° | `RootYawOffset > +45° && RootYawOffset < +135°` |
| `TIP_180` | ±180° | `|RootYawOffset| > 135°` |

#### AnimBP — adicionar estado `TurnInPlace` em paralelo ao `Idle Pose`

A maneira mais limpa é, **no `Walk_Run_SM`**, expandir `Idle Pose` para uma sub-state machine de dois estados:

```
[Idle Pose Static] ⇄ [TurnInPlace]
```

Transições:

| Transição | Condição | Blend (s) |
|-----------|----------|-----------|
| `Idle Pose Static → TurnInPlace` | `bTransition_TurnInPlace` | 0.10 |
| `TurnInPlace → Idle Pose Static` | `Abs(RootYawOffset) < 5.0` **OR** `Time Remaining (ratio) < 0.10` | 0.10 |

Dentro do estado `TurnInPlace`, use **Blend Poses by Float** baseado em `RootYawOffset`:

```
RootYawOffset < -135  → TIP_180 (rotação esquerda)
-135 ≤ RootYawOffset < 0  → TIP_L_90
0 < RootYawOffset ≤ 135  → TIP_R_90
RootYawOffset > 135  → TIP_180 (rotação direita)
```

#### Debug
- Anim Preview → `RootYawOffset` deve crescer ao girar a câmera com personagem parado.
- Console: `ShowDebug Animation` (na PIE) — observe `AimYaw` e `RootYawOffset`.

#### Caveats
- **Não habilite `bUseControllerRotationYaw=true`** — isso faz o actor rotacionar todo frame, zerando `AimYaw` e o TIP nunca dispara.
- Se `Speed > IdleSpeedDeadband`, `RootYawOffset` é drenado automaticamente — locomoção absorve a orientação.

---

### 5.5 Aim Offset — Overlay de mira (Pitch / Yaw)

**Problema que resolve:** o tronco/cabeça precisa apontar para a mira da câmera durante strafe / lockon, independente da direção do movimento. `AimPitch` / `AimYaw` já existem em C++; falta o asset e o wiring.

#### Editor — Setup

1. **Criar Aim Offset asset:**
   - Content Browser → Right-click → **Animation → Aim Offset**.
   - Skeleton: o skeleton do seu personagem (ex.: `SK_Manny_Skeleton` ou o skeleton específico do JSHero).
   - Nome sugerido: `AO_JSHero_Aim`.

2. **Configurar eixos:**
   - **X (`AimYaw`):** –90 a +90, 5 pontos: –90 / –45 / 0 / +45 / +90.
   - **Y (`AimPitch`):** –60 a +60, 3 pontos: –60 / 0 / +60.
   - **Snap to Grid:** ON.
   - Em cada ponto: drop de uma pose 1-frame correspondente (ex.: `AimPose_Center`, `AimPose_UpRight`, …).

3. **AnimBP — wiring final, após o Foot Locker:**

   ```
   [Foot Locker IK] → [Layered Blend per Bone] → [Output Pose]
                              ↑
                              Blend Pose 0: AO_JSHero_Aim Player
                                  (Pitch ← AimPitch, Yaw ← AimYaw)
                              Blend Weights[0]: AimOffsetAlpha
   ```

4. **Configurar `Layered Blend per Bone`:**

   | Setting | Valor |
   |---------|-------|
   | **Layer Setup → Branch Filters[0] → Bone Name** | `spine_01` (ou bone equivalente do seu rig) |
   | **Branch Filters[0] → Blend Depth** | 3 (afeta spine_01, spine_02, spine_03+) |
   | **Mesh Space Rotation Blend** | ON |
   | **Curve Blend Option** | `NormalizeByWeight` |

5. **Ativação do alpha:**
   - C++ já interpola `AimOffsetAlpha` para 1 quando `bShouldStrafe == true` (lockon / combat) OR `bAimOffsetRequested == true`, para 0 caso contrário.
   - Para controle manual de gameplay use **`SetAimOffsetEnabled(bool)`**:

     ```cpp
     // No AnimBP (Event Graph) ou via Blueprint do personagem:
     // Get Anim Instance → Cast to UDFAnimInstance → Set Aim Offset Enabled (true/false)
     ```

   > ⚠️ **Precisão:** em `UDFAnimInstance.h`, tanto `SetAimOffsetEnabled` quanto `ConsumeRootYawOffset` são declaradas em `private:` (porém `UFUNCTION(BlueprintCallable)`). Isso significa: **podem** ser chamadas por **Blueprint** (BP ignora o `private` do C++), mas **não** podem ser chamadas a partir de outro `.cpp` por uma referência `UDFAnim->SetAimOffsetEnabled(...)` — isso não compila. Se você precisar acionar por C++ de gameplay, mova a declaração para `public:` no header e recompile, ou faça o toggle via Blueprint.

#### Debug
- Anim Preview → mexa `AimPitch` / `AimYaw` manualmente; tronco deve apontar.
- Esperado: corpo segue câmera no Pitch/Yaw apenas durante strafe / lockon; em exploração livre, `AimOffsetAlpha = 0` e o aim offset não aparece.

---

### 5.6 Stop AAA — deslize, catch-up e saída para Idle (cadeia de fixes 2026-05-26)

> Esta seção documenta os ajustes feitos para o Stop sair de "travado / brusco / câmera lenta" e chegar num stop suave que toca a animação inteira e transiciona limpo para Idle. Os 4 problemas foram resolvidos **em ordem** (cada fix revelava o próximo). Os arquivos tocados: `UDFAnimInstance.cpp`, `UDFCharacterMovementComponent.cpp/.h`.

#### Visão geral — 4 problemas, 4 fixes

| # | Sintoma | Causa-raiz | Fix | Onde |
|---|---|---|---|---|
| 1 | Stop **travava** (deadlock) — anim congelada, SM presa | `bInLocomotionStopPhase` ficava true junto com `bTransition_StopToIdle` → dois edges conflitantes na AnimBP | Force-clear da fase de Stop quando parado (flags mutuamente exclusivos) | `UDFAnimInstance.cpp` |
| 2 | Stop **brusco** — animação de Stop não aparecia | A cápsula freava em ~6 cm, mas a anim Run_Stop tem root motion de **202 cm** → distance-matching sem distância para consumir | Braking deslizante no CMC (`WalkStopBrakingDeceleration`) | `UDFCharacterMovementComponent` |
| 3 | Stop em **câmera lenta** | O tail catch-up só ligava em `Speed < 25`; o slow motion acontece em `Speed 130–370` → explicit time rastejava | Subir `StopTailCatchUpSpeedThreshold` (25 → 220) e baixar `StopTailCatchUpSeconds` (0.35 → 0.20) | `UDFAnimInstance.h` (Class Defaults) |
| 4 | Transição **Stop → Idle estranha** | `bTransition_StopToIdle` só disparava em `Speed < 5`, mas a anim termina em `Speed ≈ 63` → último frame congela ~0.1 s; e o main SM saía em `Speed <= 10` (descompasso) | StopToIdle dispara no **motion end** + alinhar as duas SMs no mesmo flag | `UDFAnimInstance.cpp` + AnimBP |

#### Fix 1 — Deadlock: flags Stop mutuamente exclusivos

`UUDFAnimInstance::UpdateDirectionalLocomotion`, bloco "fully stopped" (`!bMoving && !bHasInput`): a fase de Stop agora **encerra incondicionalmente** quando o personagem está fisicamente parado. Antes, ela só encerrava se `DistanceMatchingStopToTarget <= KINDA_SMALL_NUMBER`, então quando o braking parava a cápsula antes da distância autoral ser consumida, `bInLocomotionStopPhase` ficava preso → `bTransition_LoopToStop` e `bTransition_StopToIdle` ficavam **ambos true** → a AnimBP deadlockava na State Stop (`df.LocomotionDebug`: `L>P=1 & P>I=1`, `Time` congelado).

```cpp
if (!bMoving && !bHasInput)
{
    bTransition_StopToIdle = true;
    bTransition_LoopToStop = false;    // mutuamente exclusivo com StopToIdle
    bTransition_StopToMove = false;
    bInLocomotionStopPhase = false;     // encerra a fase, incondicional
    bStopToMoveLatch = false;
    bWasInLocomotionStopPhasePreviousFrame = false;
    // ...reset do start state...
}
```

#### Fix 2 — Braking deslizante (CMC) para a anim ter distância

`UDFCharacterMovementComponent` (construtor + `BeginPlay`). O default do UE (`GroundFriction 8 × BrakingFrictionFactor 2 = fricção 16`) para a cápsula em ~6 cm. As animações de Stop têm **root motion** (`RM=1`) e foram autoradas para deslizar uma distância fixa (`Run_Stop` ≈ 202 cm). Sem deslize, o distance-matching não tem nada a consumir → a anim nem toca.

```cpp
// Construtor + BeginPlay (re-aplicado depois dos defaults do BP):
bUseSeparateBrakingFriction = true;
BrakingFriction = 0.f;                              // desaceleração pura, sem fricção
BrakingDecelerationWalking = WalkStopBrakingDeceleration;  // default 600
NormalBrakingDecelerationWalking = WalkStopBrakingDeceleration; // landing restaura p/ cá
```

**`WalkStopBrakingDeceleration`** (`UPROPERTY` em `DF|Movement`, default **600**) controla o trade-off responsividade ↔ animação completa. Deslize a partir de Run (429 cm/s) vs os 202 cm da anim:

| Valor | Deslize | % da anim Run_Stop | Sensação |
|---|---|---|---|
| 450 | ~204 cm | 100% (completa) | escorregadio (~2 m) |
| 500 | ~184 cm | 91% | — |
| **600** ← default | ~153 cm | 76% | equilíbrio |
| 700 | ~131 cm | 65% | mais responsivo |
| 2048 (UE) | ~6 cm | 0% | brusco (o bug) |

> O landing brake (`LandingBrakingDeceleration`, 4096) ainda sobrepõe esse valor durante a janela de pouso e restaura para `WalkStopBrakingDeceleration` depois — sem conflito (ver doc 17 Jump). **Sem foot-sliding**: como a cápsula percorre exatamente a distância da curva, o distance-matching casa os pés.

#### Fix 3 — Catch-up cedo elimina a câmera lenta

O `explicit time` da anim Stop avança proporcional à distância consumida por frame. Na faixa de desaceleração (`Speed 130–370`) pouca distância é consumida → o tempo rasteja → **slow motion**. O catch-up que corrige isso só ligava em `Speed < StopTailCatchUpSpeedThreshold` (era **25**), tarde demais. Como o braking (600) também para a cápsula **antes** dos 202 cm autorais, o catch-up é obrigatório para a anim completar.

Novos defaults (`UDFAnimInstance.h`, categoria `DF|Locomotion|DistanceMatching` — **editáveis na instância, sem recompilar**):

| Parâmetro | Antes | Agora | Efeito |
|---|---|---|---|
| `StopTailCatchUpSpeedThreshold` | 25 | **220** | catch-up engata cedo (cobre a banda de desaceleração ≈ run speed) → mata o slow motion |
| `StopTailCatchUpSeconds` | 0.35 | **0.20** | a anim Stop completa em ritmo natural (~1×) |

Afinação: ainda lento → suba o threshold (250–300) / baixe o seconds (0.15). Snap rápido demais no fim → suba o seconds (0.25–0.30).

> **Eixos independentes:** `WalkStopBrakingDeceleration` = *quão longe* desliza (game feel). Catch-up = *quão rápido* a anim Stop toca. O slow motion era 100% o segundo eixo.

#### Fix 4 — Saída Stop → Idle no motion end (sem travadinha) + alinhar as 2 SMs

**4a — C++:** `bTransition_StopToIdle` agora também dispara assim que a anim Stop atinge o **motion end** (não só em `Speed < deadband`). O catch-up leva o explicit time ao fim quando `Speed ≈ 50–70`; esperar `Speed≈0` congelava o último frame por ~0.1 s. Em `UUDFAnimInstance::UpdateDistanceMatching` (fim do bloco de Stop):

```cpp
const bool bStopClipFinished = MotionEndTime > KINDA_SMALL_NUMBER
    && DistanceMatchingStopExplicitTime >= (MotionEndTime - KINDA_SMALL_NUMBER);
const bool bReaccelerating = DFCharacterMovement
    && DFCharacterMovement->GetLastInputVector().SizeSquared2D() > 0.01f;
if (bStopClipFinished && !bReaccelerating)
{
    bInLocomotionStopPhase = false;
    bTransition_LoopToStop = false;
    bTransition_StopToIdle = true;
}
```

**4b — AnimBP (alinhar os 2 níveis):** existem **duas** state machines indo para idle e elas precisam disparar **juntas**:

| State Machine | Transição | Regra **correta agora** |
|---|---|---|
| **Layer** (`ABP_TestLayerBase`) | Stop → Idle Pose | `bTransition_StopToIdle` |
| **Main** (`ABP_JSHeroCharacter`) | Walk/Run → Idle | `bTransition_StopToIdle` |

> ⚠️ **Mudança de semântica importante.** Versões anteriores deste guia (e a §2.4 original) diziam para o layer Stop → Idle usar `GetRelevantAnimTimeRemaining() <= 0.05` e o main usar `NOT bTransition_LoopToStop AND Speed <= 10`, **porque na época o `bTransition_StopToIdle` disparava no primeiro frame parado** (`Speed < deadband`). Com o Fix 4a ele passou a disparar no **motion end** — então agora **ambas** as transições devem usar `bTransition_StopToIdle`. Isso elimina o descompasso (o main saía em `Speed<=10`, o layer em `Speed<5`) que misturava o pose do Stop com a Idle durante o blend.

Se ainda houver um micro-pop após alinhar, suba a **Duração** da transição Stop → Idle Pose de `0.10` para `0.15`.

#### Semântica atualizada dos flags (substitui a tabela da §1)

| Flag | Quando fica **true** (após os fixes) |
|---|---|
| `bTransition_LoopToStop` | Da soltada de input até a fase de Stop encerrar. Vira **false** no mesmo frame em que `bTransition_StopToIdle` vira true (mutuamente exclusivos). |
| `bTransition_StopToIdle` | (a) anim Stop atingiu o **motion end** sem novo input, **ou** (b) `Speed < IdleSpeedDeadband && !input`. **Use este flag nas DUAS transições** (layer Stop→Idle Pose **e** main Walk/Run→Idle). |

#### Ordem de aplicação / teste

1. **Recompile** (Fixes 1, 2, 4a mexeram em `.cpp`; o header novo do Fix 2 exige rebuild completo, não só Live Coding).
2. Class Defaults do AnimBP → `DF|Locomotion|DistanceMatching`: confirme `StopTailCatchUpSpeedThreshold = 220`, `StopTailCatchUpSeconds = 0.20` (Fix 3 — ajustável sem recompilar).
3. Class Defaults do CMC → `DF|Movement`: `WalkStopBrakingDeceleration = 600` (Fix 2 — ajuste o trade-off).
4. AnimBP: as **duas** transições para idle usam `bTransition_StopToIdle` (Fix 4b).
5. `df.LocomotionDebug 4`: o `Time=` deve subir `0 → 0.77 (end)` acompanhando o deslize, sem rastejar; `L>P` e `P>I` nunca true juntos; ao parar, layer **e** main vão a Idle no mesmo instante.

---

## 6. Ordem final do AnimGraph (resumo)

```
[Main States / Locomotion SM]   ← §2.2 (Walk_Run_SM) + §5.1 Distance Matching (dentro do Start)
        ↓
[Stride Warping]                ← §5.2
        ↓
[Layered Blend per Bone]        ← §5.5 Aim Offset (Branch: spine_01)
        ↓
[Slot 'DefaultSlot']            ← montages (attack / dodge / hit) por cima
        ↓
[Control Rig]                   ← §5.3 foot IK (Should Do IKTrace = NOT Is Falling) — VOCÊ JÁ TEM
        ↓
[Output Pose]
```

> **Esta é a ordem que casa com o seu grafo real** (ver §2.0-B). O `Control Rig` no fim faz o foot IK — não troque por Two Bone IK. Se você ainda não usa Control Rig, a alternativa com `Two Bone IK: foot_l/foot_r` (§5.3) vai **antes** do `Slot 'DefaultSlot'`.
>
> **TIP (§5.4)** vive **dentro** do `Walk_Run_SM` (sub-state machine do `Idle Pose`), não no graph principal — pois só faz sentido quando estamos parados.

---

## 7. Checklist de testes

- [ ] **Start/Loop/Stop básico:** parar, andar W, soltar W. Cada transição deve usar a animação correta.
- [ ] **Direção:** Start aciona a direção correta dependendo do input no momento do takeoff (W = Forward, A = Left, etc.).
- [ ] **Loop direcional:** rodar em círculo com input contínuo — o Loop deve trocar de asset suavemente.
- [ ] **Stop direcional:** soltar input durante uma curva — o Stop deve respeitar a direção que estava sendo cursada (não a direção do frame seguinte).
- [ ] **Distance Matching:** pés colados ao chão durante toda a aceleração.
- [ ] **Stride Warping:** trocar `MaxWalkSpeed` entre 300 e 600 — passos encolhem/esticam.
- [ ] **Foot Locker:** parar em uma rampa — pés fixos sem deslizar.
- [ ] **Turn-In-Place:** parado, girar mouse 90° — personagem toca animação de giro e re-alinha.
- [ ] **Aim Offset:** entrar em lockon — corpo segue câmera no Pitch/Yaw com `AimOffsetAlpha → 1`.

---

## 8. Tuning sugerido (Class Defaults do `ABP_JSHeroCharacter`)

| Categoria | Variável | Default sugerido |
|-----------|----------|------------------|
| Locomotion | `WalkSpeedThreshold` | 50 cm/s |
| Locomotion | `RunSpeedThreshold` | 350 cm/s |
| Locomotion | `IdleSpeedDeadband` | 5 cm/s |
| Locomotion | `StartMaxPlayTime` | 0.80 s |
| DistanceMatching | `StopTailCatchUpSpeedThreshold` | **220** cm/s (anti-slow-motion no Stop — §5.6) |
| DistanceMatching | `StopTailCatchUpSeconds` | **0.20** s (§5.6) |
| DistanceMatching | `StopCurveNearZeroCm` | 8 cm |
| **CMC** (`DF\|Movement`) | `WalkStopBrakingDeceleration` | **600** cm/s² (deslize na frenagem — §5.6) |
| StrideWarping | `AuthoredLoopSpeed` | medir do `Loop_F` (geralmente 350–500) |
| StrideWarping | `StrideWarpingMinSpeed` | 150 cm/s |
| TurnInPlace | `TurnInPlaceTriggerYaw` | 60° |
| TurnInPlace | `TurnInPlaceYawInterpSpeed` | 360°/s |
| AimOffset | `AimOffsetInterpSpeed` | 8.0 (~0.13 s para travel completo) |
| FootIK | `FootPlantCurveLeft / Right` | `FootPlant_L` / `FootPlant_R` |

---

## 9. Erros comuns e como corrigir

| Sintoma | Causa provável | Fix |
|---------|----------------|-----|
| `Start` não dispara ao iniciar movimento | `bUseControllerRotationYaw=true` desalinhando | Defina como `false` no CharacterMovement |
| `Loop` não troca de direção ao esterçar | Pin `Sequence` do Sequence Player **não foi exposto** | Botão direito → Expose As Pin → Sequence |
| `Stop` toca na direção errada | Está lendo `MovementDirection` em tempo real | Use `Get Locomotion Stop Anim` (já snapshota) |
| TIP nunca dispara | `bUseControllerRotationYaw=true` zerando `AimYaw` | Defina como `false` |
| TIP dispara mas actor não roda | Animação sem Root Motion na rotação | Habilite Root Motion → Rotation no asset |
| Stride Warping snaps bruscos | `AuthoredLoopSpeed` muito longe da velocidade real | Re-calibre medindo no `Loop_F` |
| Distance Matching trava no início | Curva `Distance` ausente/zerada, ou nome diferente do passado em `Distance Curve Name` | Adicione o **Distance Curve Modifier** no asset e **Apply** (§5.1, Passo 2); confirme `Distance Curve Name = "Distance"` |
| `Advance Time by Distance Matching` não encontrado no AnimBP | Plugin `AnimationLocomotionLibrary` desabilitado, ou função buscada no Event Graph | Confirme o plugin habilitado; a função só existe dentro de uma **Anim Node Function** ("On Update" de um Sequence Evaluator), não no Event Graph |
| Clip de Start/Stop sem curva de distância | Root motion desabilitado no asset | Habilite Root Motion no asset antes de aplicar o `Distance Curve Modifier` (a curva mede o deslocamento do root) |
| Foot Locker faz pop visual | Rampa da curva muito abrupta | Aumente a rampa 0→1 / 1→0 para 3+ frames |
| **Start** some cedo (~0,45 s) | `StartMaxPlayTime` baixo ou `Start→Loop` só com `bTransition_StartToLoop` | Suba para **0,80 s** (Class Defaults); opcionalmente OR `Time Remaining < 0.08` |
| **Stop** brusco / não aparece (cápsula para em ~6 cm) | Braking padrão do UE freia antes da distância autoral | Braking deslizante no CMC: `WalkStopBrakingDeceleration = 600` (§5.6 Fix 2) |
| **Stop** em **câmera lenta** | Catch-up do distance-match engata tarde (`Speed < 25`) | `StopTailCatchUpSpeedThreshold = 220`, `StopTailCatchUpSeconds = 0.20` (§5.6 Fix 3) |
| **Stop** travado / SM presa (anim congela) | `bInLocomotionStopPhase` true junto com `bTransition_StopToIdle` (deadlock) | Recompile C++: force-clear da fase de Stop (flags mutuamente exclusivos — §5.6 Fix 1) |
| Transição **Stop → Idle** estranha (travadinha / pose mistura) | StopToIdle disparava em `Speed<5` + descompasso entre layer e main | Recompile C++ (StopToIdle no motion end); **layer e main** usam `bTransition_StopToIdle` (§5.6 Fix 4) |
| Loop “pisca” a cada ~0,67 s | Transição `Loop → Start` ativa | **Remova** `Loop → Start`; use só `Loop → Stop` |
| HUD: `Stop: … (no Distance curve)` | `Distance Curve Modifier` com **Axis = Z** (ou modifier não aplicado/salvo) | **Axis = XY** → Apply → Save; assign **Uniform Indexable** curve compression (§5.1 Passo 2b) |
| `StopTarget=0` ao soltar W; `Walk_Stop` em run | CMC já abaixo de `RunSpeedThreshold` no frame do release; curva Run ausente | Recompile C++ (peak speed snapshot + latch); corrija `Run_Stop_F_0` (curva −202→0) |
| Evaluator no Stop sem pose | Saída do nó não ligada ao **Output Animation Pose** | Conecte `Sequence Evaluator` → `Result` (§5.1 Passo 4) |
| Personagem **congelado** no Stop (perna no ar) | **Explicit Time = 0** no Evaluator bloqueia o scrub | Remova o `0`; use só `DistanceMatchingStopExplicitTime` |
| Anda com **perna no ar** depois do Stop | Falta **`Stop → Start`** com `bTransition_StopToMove`; Evaluator no último frame | Adicione a transição; recompile C++ |
| `DistanceCurveModifier` **gerou erros** | Root lock / sem deslocamento XY no root | §5.1 Passo 2a — desligue Force Root Lock ao aplicar |
| SmartName **"Could not find … Distance … remove the curve"** | Curva `Distance` órfã no seq.; nome não registrado no Skeleton | §5.1 Passo 2a-b — `Não` no dialog; registrar `Distance` no Skeleton; apagar curva; reaplicar |
| `StopTarget` desce mas animação não anda | Mesmo caso: tempo explícito travado em 0 | Ver linha acima; no HUD deve subir `StopTime=0.00→1.50` |

---

## 10. Mapeamento do pacote Fab → sistemas

Seu pacote (Fighter / Action-RPG: *Idle / Attack / Walk / Run / Jump / Dodge / Roll / Hit / Turn*, armado e desarmado) cobre **todas** as entradas do sistema. A tabela abaixo diz **onde cada categoria entra** e **qual struct/asset** recebe o quê. "Desarmado" = `DefaultAnimSet` do `ABP_JSHeroCharacter`; "Combat/Armado" = `WeaponAnimSet` aplicado via weapon anim layer (`ActiveAnimSet`).

### 10.1 Locomoção (Walk / Run) — o coração deste doc

| Animações do pacote | Destino no sistema | Slots |
|---|---|---|
| **Walk normal 8 ways** | `DefaultAnimSet.WalkSet` (desarmado) | `Loop_F … Loop_FL_45` (8) |
| **Walk combat 8 ways** | `WeaponAnimSet.WalkSet` (layer armado) | `Loop_*` (8) |
| **Run normal 8 ways** | `DefaultAnimSet.RunSet` (desarmado) | `Loop_*` (8) |
| **Run combat 8 ways** | `WeaponAnimSet.RunSet` (layer armado) | `Loop_*` (8) |
| **2 Run speed** | Run (mais lento) → `RunSet`; o mais rápido → gait **Sprint** reusando `RunSet` + Stride Warping (§5.2/§8). Não há `SprintSet` separado (ver §1). | — |
| **Walk equip / Run equip** | Montage no slot `UpperBody`/`DefaultSlot` disparada pela ação de equipar (não é estado de locomoção). Toca por cima do Loop. | — |
| **Walk block 8 ways** | **Não** vai na `Walk_Run_SM`. É um overlay/estado de bloqueio: ou um segundo `FUDLocomotionAnimSet` no weapon layer ativado pela tag `State.Blocking`, ou um `Layered Blend per Bone` (upper body block) sobre o Loop. Veja §10.5. | — |

> **Start/Stop:** se o pacote não traz `walk_start_*` / `run_stop_*` dedicados, deixe os slots `Start_*`/`Stop_*` **vazios**. O resolver retorna `nullptr` e a `Walk_Run_SM` entra direto no `Loop` (transição `Start→Loop` por `Time Remaining`), funcionando como antes. Conforme você gravar/comprar Starts e Stops, preencha incrementalmente (ordem recomendada: Run primeiro — §0 "Migração gradual").

### 10.2 Idle

| Animação | Destino |
|---|---|
| **Idle normal** | `DefaultAnimSet.IdleAnimation` |
| **Idle combat** | `WeaponAnimSet.IdleAnimation` (layer armado) |

### 10.3 Turn (combat 90° / 180° L&R) → Turn-In-Place (§5.4)

| Animação do pacote | Slot TIP (§5.4) | Disparo |
|---|---|---|
| **Turn 90° L** / **Turn combat 90° L** | `TIP_L_90` | `−135° < RootYawOffset < −45°` |
| **Turn 90° R** / **Turn combat 90° R** | `TIP_R_90` | `+45° < RootYawOffset < +135°` |
| **Turn 180° L&R** / **Turn combat 180°** | `TIP_180` | `|RootYawOffset| > 135°` |

Use as variantes **combat** no `TurnInPlace` do weapon layer (armado) e as normais no layer desarmado. Habilite **Root Motion → Rotation** nesses clips (§5.4, Opção A). Seu pacote já entrega 90° e 180° em ambos os lados — cobertura total.

### 10.4 Jump (4 ways) → `FUDJumpAnimSet`

A struct de jump em C++ é **4-way + idle** (`Start_Idle/Forward/Backward/Left/Right`, `Loop`, `Land_*`), exatamente o formato do seu pacote (**Jump 4 ways**).

| Animação | Slot (`DefaultAnimSet.JumpSet`) |
|---|---|
| Jump start (parado) | `Start_Idle` |
| Jump start F/B/L/R | `Start_Forward` / `Start_Backward` / `Start_Left` / `Start_Right` |
| Apex / queda em loop | `Loop` |
| Land (parado + F/B/L/R) | `Land_Idle` / `Land_Forward` / `Land_Backward` / `Land_Left` / `Land_Right` |

Getters BP: `Get Jump Start Anim`, `Get Jump Loop Anim`, `Get Jump Land Anim` (já existem). A máquina de Jump usa as flags `bTransition_*Jump*` do AnimInstance (ver header). Diagonais caem em cardinal automaticamente (`FUDJumpAnimSet::ResolveStart`).

### 10.5 Dodge / Roll (4 ways → resolver 8-way) — sistema GAS, **não** locomoção

Dodge e Roll **não** entram na `Walk_Run_SM`. Eles são **montages** tocadas pela `UDFAbility_Dodge` (GAS), via `FDFDodgeAnimSet` (8 slots, fallback diagonal→cardinal→Backward). Guia completo: [`docs/improvements/15_DodgeAbility_4Way.md`](../improvements/15_DodgeAbility_4Way.md).

| Animação do pacote | Destino |
|---|---|
| **Roll 4 ways** | `UDFAbility_Dodge` → `UnarmedAnimSet` (F/B/L/R); diagonais ficam nulas (fallback) |
| **Roll 4 ways (combat/armado)** | `UDFAbility_Dodge` → `ArmedAnimSet` (F/B/L/R) |
| **Dodge 4 ways** | Se quiser dodge (passo curto) distinto de roll: segunda ability ou segundo set; mesma struct `FDFDodgeAnimSet` |

> O sistema resolve **armado vs desarmado** automaticamente por `IsOwnerArmed()` (slot Weapon do `UDFEquipmentComponent`). Cada montage deve usar o slot que casa com o AnimBP (`DefaultSlot` no chão).

### 10.6 Dash Air (ataque/esquiva aérea) → `UDFAbility_AirDash`

| Animação do pacote | Destino |
|---|---|
| **Dash Air** / **Dash Air Attack** | `UDFAbility_AirDash` → `UnarmedAnimSet` / `ArmedAnimSet` (`FDFDodgeAnimSet`) |

`UDFAbility_AirDash` usa o slot **`FullBody`** por default (`MontageSlotName`), `bLockAltitudeDuringDash` e `bSuppressAnimRootMotionDuringDash` (CMC dirige o deslocamento). Confirme um nó de slot **`FullBody`** no AnimGraph para o dash aéreo se sobrepor à queda. O AnimInstance expõe `bIsAirDashing` para o AnimBP.

### 10.7 Attacks / Hit / Block / Parry → montages (combate)

Estes não fazem parte da locomoção 8-way; são montages tocadas pelo combate (combo component / GAS) no slot `UpperBody` ou `FullBody`:

| Categoria do pacote | Sistema | Observação |
|---|---|---|
| **Attacks combo / Charge / Buff / Execution / Run attacks / Air attacks** | `UDFComboComponent` + GAS (`Ability.Attack.Melee`) | Slot `UpperBody` (mantém locomoção embaixo) ou `FullBody` (ataque aéreo/execução) |
| **Parry counter** | GAS (`Ability.Parry`) | Cancel windows via `ANS_DFAbilityCancelWindow` |
| **Hit / Hit air / Knock down / Getup** | Hit-react (montage por direção / severidade) | Slot `FullBody`; interrompe locomoção |
| **Block (Walk block / Block)** | Estado/overlay de bloqueio | `Layered Blend per Bone` (upper body) sobre o Loop, ou set de locomoção de bloqueio no weapon layer; gate por `State.Blocking` |

> Para o detalhamento do combate (combos, cancel windows, montages) veja os docs de combate do projeto (ex.: `docs/improvements/01_GameFeel.md` e os guias `*_CombatSystem.md`). Este documento foca na **locomoção 8-way + refinamentos AAA**.

### 10.8 Armado vs Desarmado — como o sistema troca

- **Desarmado:** `DefaultAnimSet` (preenchido direto no `ABP_JSHeroCharacter`, §2.1).
- **Armado / Combat:** quando o `UDFEquipmentComponent` equipa uma arma, `SyncEquippedWeaponAnimLayerFromOwner()` aplica o `WeaponAnimSet` do item (via `ApplyAnimSet`) em `ActiveAnimSet` e linka o weapon anim layer. Os getters `Get Locomotion *` leem o `ActiveAnimSet`, então **as mesmas instruções de §2 valem** — você só preenche um segundo conjunto de 8-way (combat) no asset/data do item.
- No AnimGraph, leia `ActiveAnimSet` via **Break ActiveAnimSet** quando precisar do Blend Space legado; os getters 8-way já usam `ActiveAnimSet` internamente.

---

## 11. Referências externas (validação UE 5.4)

Fontes oficiais usadas para garantir a precisão dos nomes de nós/funções e do fluxo de Distance Matching:

- **AnimDistanceMatchingLibrary** (plugin `AnimationLocomotionLibrary`, módulo `AnimationLocomotionLibraryRuntime`) — `advance_time_by_distance_matching(update_context, sequence_evaluator, distance_traveled, distance_curve_name, play_rate_clamp)` e `distance_match_to_target(sequence_evaluator, distance_to_target, distance_curve_name)`. Requer curva via `UDistanceCurveModifier`.
  [Epic — AnimDistanceMatchingLibrary](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/AnimDistanceMatchingLibrary)
- **Advance Time by Distance Matching** (Blueprint API) — pinos `Update Context`, `Sequence Evaluator`, `Distance Traveled`, `Distance Curve Name`, `Play Rate Clamp`.
  [Epic — Advance Time by Distance Matching](https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/DistanceMatching/AdvanceTimebyDistanceMatching)
- **Animation Warping** (Stride / Orientation / Slope Warping) e **Motion Trajectory** (`UCharacterTrajectoryComponent`) — base dos refinamentos §5.2 e §5.1.
  [Epic — Motion Matching / Trajectory](https://dev.epicgames.com/documentation/en-us/unreal-engine/motion-matching-in-unreal-engine)

> Nota de versão: a doc pública mais recente referencia 5.5/5.7, mas as funções e o fluxo (Sequence Evaluator + anim node function + Distance Curve Modifier) **existem e são idênticos no 5.4**. Os nomes acima são os corretos para a sua engine.

---

**Status:** C++ compila limpo e **confere com este documento** (verificado em `DFAnimSetTypes.h/.cpp`, `UDFLocomotionTypes.h`, `UDFAnimInstance.h`, `DungeonForged.uproject`, `DungeonForged.Build.cs`). Próximo passo é executar §2.1 (preencher o `DefaultAnimSet`) e §2.2 (criar a sub-state machine `Walk_Run_SM`) — esses dois já entregam Start/Loop/Stop funcional. Os refinamentos §5.x podem ser aplicados incrementalmente conforme polish, com a §5.1 (Distance Matching) seguindo o fluxo corrigido de Sequence Evaluator + `AnimDistanceMatchingLibrary`. O mapeamento do seu pacote Fab está na §10.
