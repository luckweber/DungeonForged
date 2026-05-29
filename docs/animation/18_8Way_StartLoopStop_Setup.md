# 8-Way Locomotion — Start / Loop / Stop Setup

Suporte C++ para alimentar animações direcionais 8-way com **Start → Loop → Stop** por gait (Walk / Run / Sprint), incluindo os 5 refinamentos AAA: Distance Matching, Stride Warping, Foot Locker, Turn-In-Place e Aim Offset.

> **Status do código:** compila limpo (`Rebuild All: 1 succeeded`). Todo o trabalho restante é wiring no AnimBP + curvas nos assets.

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

**Segredo:** o CMC continua dirigindo a cápsula via `MaxWalkSpeed` (não a animação). A animação **acompanha** via Distance Matching (§5.1) no Start e Stride Warping (§5.2) no Loop. No Stop, na maior parte dos casos, o playrate normal é suficiente — Stops são curtos (~0.4 s) e a discrepância é pequena.

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

Fallback de gait: `Sprint` vazio → tenta `Run`; `Run` vazio → tenta `Walk`.

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
| `bTransition_LoopToStop` | `bool` | **Sustained** — alto enquanto `!bHasInput && bMoving` |
| `bTransition_StopToIdle` | `bool` | **Sustained** — alto enquanto `!bMoving && !bHasInput` |
| `LocomotionStartElapsed` | `float` | Tempo no Start (s); zera ao ficar parado |

> **Semântica edge vs sustained:** AnimBP transitions disparam apenas na borda de subida da condição, então flags sustentadas funcionam bem. Não tente fazer edge-detection manual no AnimBP — deixe a state machine cuidar.

### Tuning (Class Defaults do `ABP_JSHeroCharacter` → `DF | Locomotion | Directional`)

| Variável | Default | Função |
|----------|---------|--------|
| `WalkSpeedThreshold` | **50** cm/s | Acima → `Walk`; abaixo → `Idle` |
| `RunSpeedThreshold` | **350** cm/s | Acima → `Run` (ou `Sprint` se flag) |
| `IdleSpeedDeadband` | **5** cm/s | Velocidade abaixo deste valor = considerado parado |
| `StartMaxPlayTime` | **0.45** s | Tempo máximo no Start antes de forçar Start→Loop |

### Getters Blueprint (`BlueprintPure`)

```
Get Locomotion Start Anim → UAnimSequenceBase*
Get Locomotion Loop Anim  → UAnimSequenceBase*
Get Locomotion Stop Anim  → UAnimSequenceBase*
```

`Start` e `Stop` usam suas direções snapshot; `Loop` usa `MovementDirection` em tempo real (atualiza enquanto o jogador esterça).

---

## 2. Setup AnimBP

### 2.1 Preencher `DefaultAnimSet` no `ABP_JSHeroCharacter`

Asset: `Content/DungeonForged/Character/JSHero/Animation/BPAnim/ABP_JSHeroCharacter.uasset`

**Class Defaults → DefaultAnimSet → DF | Anim Set - Locomotion | Directional:**

1. Expanda **WalkSet** → arraste as 8 Start, 8 Loop e 8 Stop de Walk.
2. Expanda **RunSet** → arraste as 8 Start, 8 Loop e 8 Stop de Run.

> Use a pasta real do seu projeto (provavelmente `Content/DungeonForged/Character/JSHero/Animation/...`). Ajuste o caminho conforme seu layout.

Slots vazios são OK — o resolver cai no Forward correspondente.

### 2.2 Substituir o estado `Walk/Run` no Locomotion SM

O state machine de locomoção atualmente delega para o linked anim layer (`ABP_JCHero_UnArmed_Layer` / `ABP_JCHero_Armed_Layer`). Você tem duas opções:

**Opção A — Substituir o conteúdo da função `FullBody_LocomotionState` do linked layer.**
**Opção B — Criar uma nova função `FullBody_LocomotionState_8Way` no linked layer e apontar o estado `Walk/Run` para ela.**

A estrutura interna é a mesma. Use Opção B se quiser manter Blend Space como fallback alternativo.

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
| `Start` | Sequence Player com pin `Sequence` exposto, ligado a **Get Locomotion Start Anim** | **false** |
| `Loop` | Sequence Player com pin `Sequence` exposto, ligado a **Get Locomotion Loop Anim** | **true** |
| `Stop` | Sequence Player com pin `Sequence` exposto, ligado a **Get Locomotion Stop Anim** | **false** |

> **Importante sobre o pin Sequence:** clique com botão direito no Sequence Player → **Expose As Pin → Sequence**. Isso converte a propriedade `Sequence` (que é "bind" estático) em **pin de entrada** ligável. Ligue o getter `Get Locomotion Loop Anim` nesse pin. Cada frame o Sequence Player re-resolve qual asset tocar — assim trocar de direção durante o Loop atualiza a animação instantaneamente sem precisar transicionar de estado.

#### Transições

| Transição | Condição | Blend (s) | Mode |
|-----------|----------|-----------|------|
| `Entry → Idle Pose` | (default) | — | — |
| `Idle Pose → Start` | `bTransition_IdleToStart` | 0.10 | Hermite |
| `Start → Loop` | `bTransition_StartToLoop` **OR** `Time Remaining (ratio) < 0.10` | 0.12 | Hermite |
| `Loop → Stop` | `bTransition_LoopToStop` | 0.15 | Hermite |
| `Stop → Idle Pose` | `bTransition_StopToIdle` **OR** `Time Remaining (ratio) < 0.10` | 0.10 | Hermite |
| `Stop → Start` | `bTransition_IdleToStart` (re-pressed durante Stop) | 0.08 | Hermite |
| `Loop → Start` *(opcional)* | mudança brusca de direção — ver §2.3 | 0.10 | Hermite |

#### Re-direct durante o Loop (opcional)

O Loop já segue `MovementDirection` em tempo real, então uma rotação suave (ex.: virar enquanto corre) é absorvida pelo próprio Loop player. A transição `Loop → Start` só é útil se você quiser tocar a **animação de Start** novamente em um redirect ≥ 90°:

```
Condição: AbsDelta(Yaw com MovementDirection trocando de half-plane) > 90  AND  bIsAccelerating
```

Implementar como uma flag custom no AnimInstance se quiser esse efeito; para 99% dos casos pode pular.

### 2.3 Idle (turn-in-place opcional)

Se quiser TIP, veja §6.4. Para começar, basta o `Idle Pose` simples.

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
3. **Após `StartMaxPlayTime` (0.45s)** → `bTransition_StartToLoop=true` → SM entra em `Loop`.
4. **Esterçar enquanto corre** → `MovementDirection` muda → `Get Locomotion Loop Anim` retorna outro asset → Sequence Player troca sem mudança de estado.
5. **W solto** → `bTransition_LoopToStop=true` → SM entra em `Stop`; `LocomotionStopDirection` congela na direção atual.
6. **Velocidade < deadband** → `bTransition_StopToIdle=true` → SM volta para `Idle Pose`.

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

| Plugin | Função |
|--------|--------|
| `AnimationWarping` | Nós **Stride Warping** e **Orientation Warping** no AnimGraph |
| `MotionTrajectory` | `UCharacterTrajectoryComponent` (já adicionado em C++ no `ADFPlayerCharacter::CharacterTrajectory`) |
| `AnimationLocomotionLibrary` | Nó **Distance Matching** (`Distance Matching to Time`) e **Foot Placement** |
| `Chooser` *(opcional)* | Seleção data-driven de animações por contexto |

Módulos C++ adicionados ao `DungeonForged.Build.cs`: `AnimationWarpingRuntime`, `MotionTrajectory`.

### Variáveis C++ feeders (AnimInstance → AnimBP)

| Variável | Tipo | Para que serve |
|----------|------|----------------|
| `DistanceMatchingDistance` | `float` | Distância XY acumulada desde a borda Idle→Start (input do nó **Distance Matching**) |
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

### 5.1 Distance Matching — Sincronizar Start Anim com a distância percorrida

**Problema que resolve:** sem isso, o `Start_*` toca em playrate fixo enquanto o personagem acelera de 0→max. Resultado: pés escorregam no chão durante 0.3–0.5s.

#### Editor — Setup

1. **Componente já adicionado em C++** (`ADFPlayerCharacter::CharacterTrajectory` → `UCharacterTrajectoryComponent`). Nada a fazer no Blueprint — basta confirmar na hierarquia do `BP_ThirdPersonCharacter` que o componente `CharacterTrajectory` aparece.
   - O componente é experimental do plugin `MotionTrajectory`. Sampling settings (history/prediction) usam os defaults do CDO; ajuste no header se quiser custom.
   - Acessível em BP via getter `Get Character Trajectory` ou direto via `PlayerCharacter.CharacterTrajectory`.

2. **Curva `DistanceCurve` em cada animação `Start_*`:**
   - Abra a sequence no Persona.
   - Window → **Curves** → Add → **Float Curve**, nome `DistanceCurve`.
   - **Atalho:** se a animação tem Root Motion habilitado, clique direito na curva → **Apply Root Motion → Distance** (UE 5.4 gera automaticamente).
   - Verifique: no frame 0, valor = 0. No último frame, valor = distância total percorrida pelo root (cm).

3. **AnimBP — dentro do estado `Start` da `Walk_Run_SM`:**

   ```
   ┌─[Sequence Evaluator (by Time)]──→ Pose Output
   │      ↑ Explicit Time
   │      │
   │  ┌─[Distance Matching to Time]
   │  │    ↑ Distance      ↑ Sequence
   │  │    │                │
   └──┘  DistanceMatchingDistance    Get Locomotion Start Anim
   ```

   - **Substitua** o Sequence Player por: `Sequence Evaluator (by Time)` + nó `Distance Matching to Time` (do plugin `AnimationLocomotionLibrary`).
   - **Distance Curve Name:** `DistanceCurve`
   - **Distance:** ligue `DistanceMatchingDistance`
   - **Strict Mode:** ON (clamp ao endpoint)

4. **Transição `Start → Loop`:** mantenha a condição existente. Distance Matching apenas controla o **timing interno** do Start; quando a animação chega no fim (ou `StartMaxPlayTime` expira), transiciona normalmente.

#### Debug
- Anim Preview → observe `DistanceMatchingDistance` crescendo de 0.
- Esperado: pés colados no chão durante toda a aceleração.

---

### 5.2 Stride Warping — Esticar passos pela velocidade real

**Problema que resolve:** suas Loops são autoradas a uma velocidade fixa (ex.: 400 cm/s). Se o `MaxWalkSpeed` for 500, o Loop original arrasta os pés. Com warping, o passo é esticado/encolhido para casar com a velocidade atual.

#### Editor — Setup

1. **AnimBP — após o estado `Walk_Run_SM` (de volta no AnimGraph principal), antes do Output Pose:**

   ```
   [Walk_Run_SM] → [Stride Warping] → (... próximos estágios ...)
                          ↑
                          │
       StrideWarpingAlpha    (Alpha)
       Speed                 (Locomotion Speed)
       AuthoredLoopSpeed     (Locomotion Speed at Authored)
   ```

2. **Configuração do nó `Stride Warping`:**

   | Pino / Setting | Valor |
   |----------------|-------|
   | **Mode** | `Manual` |
   | **Stride Direction** | `Velocity` |
   | **Locomotion Speed** | `Speed` (do AnimInstance) |
   | **Locomotion Speed at Authored** | `AuthoredLoopSpeed` |
   | **Alpha** | `StrideWarpingAlpha` |
   | **Pelvis Bone** | `pelvis` |
   | **IK Foot Root** | `ik_foot_root` |
   | **IK Foot Bones** | `[ik_foot_l, ik_foot_r]` |
   | **Foot Definitions** | `foot_l` (FK) + `ik_foot_l` (IK); idem para `foot_r` |
   | **Min/Max Root Stretch** | `0.5` / `1.5` (clamp seguro) |

3. **Calibrar `AuthoredLoopSpeed`:**
   - Abra `Loop_F`, observe a velocidade do root motion (Persona → Asset Details → Root Motion).
   - Defina `AuthoredLoopSpeed` no Class Defaults igual a esse valor.

#### Debug
- Console: `a.AnimNode.StrideWarping.Debug 1`
- Esperado: passos longos > 400 cm/s, encolhidos < 400 cm/s. Pelvis subtle scaling visível.

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

Adicione **Two Bone IK** para cada pé após o Stride Warping:

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
   - Para controle manual de gameplay:

     ```cpp
     UDFAnim->SetAimOffsetEnabled(true);   // ao pressionar aim
     UDFAnim->SetAimOffsetEnabled(false);  // ao soltar
     ```

#### Debug
- Anim Preview → mexa `AimPitch` / `AimYaw` manualmente; tronco deve apontar.
- Esperado: corpo segue câmera no Pitch/Yaw apenas durante strafe / lockon; em exploração livre, `AimOffsetAlpha = 0` e o aim offset não aparece.

---

## 6. Ordem final do AnimGraph (resumo)

```
[Locomotion State Machine]
        ↓
[State: Walk_Run_SM]            ← §2.2 + §5.1 Distance Matching (dentro do Start)
        ↓
[Stride Warping]                ← §5.2
        ↓
[Two Bone IK: foot_l]           ← §5.3 Foot Locker
        ↓
[Two Bone IK: foot_r]           ← §5.3
        ↓
[Layered Blend per Bone]        ← §5.5 Aim Offset (Branch: spine_01)
        ↓
[Output Pose]
```

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
| Locomotion | `StartMaxPlayTime` | 0.45 s |
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
| Distance Matching trava no início | Curva `DistanceCurve` ausente ou zerada | Re-gere via Persona → Apply Root Motion → Distance |
| Foot Locker faz pop visual | Rampa da curva muito abrupta | Aumente a rampa 0→1 / 1→0 para 3+ frames |

---

**Status:** C++ compila limpo. Próximo passo é executar §2.1 (preencher o DefaultAnimSet) e §2.2 (criar a sub-state machine) — esses dois já entregam Start/Loop/Stop funcional. Os refinamentos §5.x podem ser aplicados incrementalmente conforme polish.
