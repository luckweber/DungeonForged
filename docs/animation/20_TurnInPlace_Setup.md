# Turn In Place (TIP) — Idle + Rotação 90° / 180°

Guia para usar as animações **Turn_90_L/R** e **Turn_180_L/R** (pasta `09_Turn`) com o C++ do `UUDFAnimInstance`, no mesmo estilo dos getters **Start / Loop / Stop** do doc [`18_8Way_StartLoopStop_Setup.md`](18_8Way_StartLoopStop_Setup.md).

> **Pré-requisitos:** locomoção 8-way configurada (doc 18). Stride Warping opcional (doc [`19_StrideWarping_Setup.md`](19_StrideWarping_Setup.md)). Engine **UE 5.4**.

---

## 0. O que o C++ faz

| Recurso | Função / variável |
|--------|-------------------|
| Idle no chão | `GetLocomotionIdleAnim()` → **`Idle Animation`** do `ActiveAnimSet` (layer define qual set) |
| Virar parado | `GetLocomotionTurnAnim()` → `Turn Set` → `Turn_90_*` ou `Turn_180_*` (L/R) |
| Quando virar | `bTransition_TurnInPlace` (1 frame) + `bInTurnInPlacePhase` (sustained) |
| Tempo do clip | `TurnInPlaceExplicitTime` (alimenta **Sequence Evaluator**) |
| Offset câmera vs corpo | `RootYawOffset` (= `AimYaw` enquanto idle) |
| Escolha 90 vs 180 | `\|RootYawOffset\| >= TurnInPlace180Threshold` (default **120°**) |

Enquanto **parado** (`Speed <= IdleSpeedDeadband`, sem aceleração, sem Stop phase), se `|RootYawOffset| > TurnInPlaceTriggerYaw` (default **60°**), o sistema entra em turn, avança o tempo do clip e consome yaw via `ConsumeRootYawOffset` até o fim do clip ou `TurnInPlaceCompleteYaw`.

---

## 1. Preencher o Anim Set (Class Defaults)

No **Anim Set** que o layer usa (`Default Anim Set` desarmado, **Weapon Anim Set** armado — o linked layer / equipamento troca o `ActiveAnimSet`):

| Campo | O que preencher |
|-------|-----------------|
| **Idle Animation** | Seu idle desse contexto (`Idle_Seq`, `Idle_Combat_Seq`, etc.) — **só este campo** alimenta `Get Locomotion Idle Anim` |
| **Turn Set → Turn 90 L** | `Turn_90_L_Seq` |
| **Turn Set → Turn 90 R** | `Turn_90_R_Seq` |
| **Turn Set → Turn 180 L** | `Turn_180_L_Seq` |
| **Turn Set → Turn 180 R** | `Turn_180_R_Seq` |

Não há `Idle Combat` separado no **Turn Set**. Desarmado vs armado = dois **Anim Sets** (ou o mesmo set com clips diferentes), não um override dentro de Turn.

Turns em: `Content/.../Sword_and_Shield/Animations/Sequence2/09_Turn/01_Turn/`

**Layer (`ABP_Test_UnArmed_Layer` / weapon layer):** escolhe qual Anim Set está ativo; os getters leem sempre `ActiveAnimSet`.

---

## 2. AnimBP — Layer `ABP_TestLayerBase` (Walk_Run_S)

### 2.1 Estado **Idle Pose**

Substitua o **Sequence Player** fixo por:

```
Get Locomotion Idle Anim  →  Sequence Evaluator (ou Player)
                              Should Loop = true
                              →  Output Animation Pose
```

**Condição de permanência:** `NOT bInTurnInPlacePhase` (e opcionalmente `NOT bTransition_TurnInPlace`).

### 2.2 Novo estado **Turn** (recomendado)

Adicione um estado **Turn** entre **Idle Pose** e o restante do SM:

```
Entry → Idle Pose
Idle Pose → Turn     : bTransition_TurnInPlace OR bInTurnInPlacePhase
Turn → Idle Pose     : NOT bInTurnInPlacePhase  (blend ~0.1–0.2 s)
```

Dentro de **Turn**:

```
Get Locomotion Turn Anim  →  Sequence Evaluator
                              Explicit Time = TurnInPlaceExplicitTime
                              Should Loop = false
                              Reinitialization = Explicit Time
                              →  Output Animation Pose
```

Opcional: função **Update Turn Anim** no `On Update` do Evaluator (pode ficar vazio — o C++ já avança `TurnInPlaceExplicitTime`).

**Root motion:** se os clips de Turn tiverem RM, habilite **Root Motion from Everything** no mesh / use o delta de rotação do RM; o C++ também drena `RootYawOffset` proporcionalmente à duração do clip (90° ou 180° / `PlayLength`).

### 2.3 Main ABP `ABP_JSHeroCharacter`

O estado **Locomotion → Idle** que usa **Linked Anim Layer** `FullBody_IdleState` continua válido: garanta que o layer interno implemente §2.1–2.2.

---

## 3. Getters Blueprint (resumo)

| Getter | Uso no AnimBP |
|--------|----------------|
| `Get Locomotion Idle Anim` | Loop idle — vem de **Idle Animation** do set ativo |
| `Get Locomotion Turn Anim` | One-shot 90/180 L ou R |
| `TurnInPlaceExplicitTime` | Pin **Explicit Time** do Evaluator |
| `bTransition_TurnInPlace` | Transição Idle → Turn (edge) |
| `bInTurnInPlacePhase` | Permanecer no estado Turn |
| `bTransition_IdleToStart` | Idle/Turn → Start quando W |

---

## 4. Tuning (Class Defaults do AnimInstance)

| Propriedade | Default | Efeito |
|-------------|---------|--------|
| `TurnInPlaceTriggerYaw` | 60 | Mínimo de offset para iniciar turn |
| `TurnInPlace180Threshold` | 120 | Usa clip 180° em vez de 90° |
| `TurnInPlaceCompleteYaw` | 8 | Encerra turn quando offset residual ≤ isto |
| `TurnInPlaceYawInterpSpeed` | 360 | Bleed do offset enquanto **anda** |

---

## 5. Debug — comando só de Turn (`df.TurnDebug`)

Comando **separado** da locomoção 8-way:

```text
df.TurnDebug 3
```

| Nível | Efeito |
|-------|--------|
| `0` / `off` | Desligado |
| `1` / `log` | Output Log bloco `[TIP]` + linha **`[TIP|1]`** (one-liner) |
| `2` / `hud` | Log + HUD roxo na tela |
| `3` / `draw` | Log + HUD + **desenho no chão** (círculo + arco) |
| `4` / `verbose` | Igual ao 1 (reservado; use `dump` para snapshot instantâneo) |

Outros:

```text
df.TurnDebug dump
df.TurnDebug.CircleRadius 100
```

Filtro no Output Log: **`TIP`** ou **`TIP|1`** (não mistura com `Loco`). Cole a linha `[TIP|1]` no chat para debug.

### Desenho no mundo (nível 3)

| Visual | Significado |
|--------|-------------|
| Círculo cinza | Raio de referência ao redor do personagem (`df.TurnDebug.CircleRadius`, default 85 cm) |
| Arco amarelo/laranja | “Wedge” entre corpo e mira (`RootYawOffset`) |
| Arco ciano | Offset já passou do `TurnInPlaceTriggerYaw` — vai entrar em Turn |
| Seta azul | Frente do **corpo** (actor) |
| Seta laranja | Direção da **mira** (corpo + offset) |
| Linha vermelha | Limite do trigger (±`TurnInPlaceTriggerYaw`) |
| Círculo magenta + arco verde | Turn em andamento (`bInTurnInPlacePhase`) |

Locomoção geral continua em `df.LocomotionDebug` (filtro `Loco`).

### Linha `[TIP|1]` (compacta)

```text
[TIP|1] ABP_JSHeroCharacter_C off=-124.6 aim=-124.6 |off|=125 trans=0 phase=1 armed=1 idle=1 dir=-1 deg=180 t=0.72/1.50 clip=Turn_180_L_Seq end=ClipDone codeYaw=0
```

| Campo | Significado |
|-------|-------------|
| `phase=1` | `bInTurnInPlacePhase` (estado Turn na SM) |
| `trans=1` | `bTransition_TurnInPlace` (1 frame) |
| `armed=0` | bloqueado até `\|off\| < TurnInPlaceRetriggerYaw` (default 45°) |
| `idle=0` | não pode **iniciar** turn (`Speed`, `accel`, ar, Stop) — durante `phase=1` o turn **continua** |
| `spd` / `accel` | `Speed` e `bIsAccelerating` (ver `TurnInPlaceAbortSpeed`, default 80) |
| `armed=0` + `ready=1` | offset alto mas `\|off\| < TurnInPlaceRetriggerYaw` (45°) ainda não — espera câmera baixar OU recompile (retry após abort) |
| `spd` alto + `idle=0` | `SetActorRotation` no fim do turn gerava velocidade falsa — C++ zera XY após snap |
| `dir` / `deg` | L/R e 90/180 **travados** no início do turn |
| `end` | último fim: `ClipDone`, `OffsetDone`, `NoClip` |

### Nível 2/4 — HUD

```text
Anim Idle=Idle_Combat_Seq
Anim Turn =Turn_90_R_Seq (90° L/R=+1.0) Time=0.35/1.10s
TIP RootYawOff=72.0 AimYaw=72.0 Trig>=60 180>=120
Trans: ... TIP=1 Turn=1
```

### Dump instantâneo

```text
df.LocomotionDebug dump
```

Imprime `BuildDirectionalLocomotionDebugString` + bloco Deep no log.

### CVars

| CVar | Valores |
|------|---------|
| `df.DebugLocomotion` | 0 off, 1 log, 2 HUD, 3 draw (setas), 4 deep |

---

## 6. Root Motion e Root Lock nos clips Turn

No seu `Turn_90_R_Seq` o editor mostra **Enable Root Motion desmarcado** — isso **funciona** com este projeto porque o C++ pode girar o actor (`bTurnInPlaceApplyActorYawFromCode`, default **true** no Class Defaults).

### Opção A — Sem root motion no asset (seu caso atual)

| Campo | Valor recomendado |
|-------|-------------------|
| Enable Root Motion | **Off** |
| Force Root Lock | **Off** |
| Root Motion Root Lock | qualquer (ignorado) |
| Class Defaults → `bTurnInPlaceApplyActorYawFromCode` | **true** |

O AnimInstance avança `TurnInPlaceExplicitTime` e aplica `SetActorRotation` no yaw enquanto drena `RootYawOffset`.

### Opção B — Com root motion só na rotação (estilo Epic)

| Campo | Valor recomendado |
|-------|-------------------|
| Enable Root Motion | **On** |
| Root Motion Root Lock | **Anim First Frame** ou **Ref Pose** |
| Force Root Lock | **Off** (evita travar translação errada) |
| Curva RM | Rotação **Y (yaw)** relevante; translação XY mínima ou zerada no DCC |
| `bTurnInPlaceApplyActorYawFromCode` | **false** (evita double-rotate) |

Use **uma** das opções — não marque RM no clip **e** deixe `bTurnInPlaceApplyActorYawFromCode` true ao mesmo tempo.

`df.TurnDebug` / HUD mostram `RM=1` se o sequence tiver root motion habilitado.

---

## 7. Checklist de teste

1. PIE, olhar fixo, mover mouse / stick para girar câmera **sem** W.
2. `|RootYawOffset|` passa de 60° → transição para **Turn**, clip L ou R.
3. Offset > 120° → preferência por **Turn_180_***.
4. Ao terminar, `bInTurnInPlacePhase=0`, volta **Idle Pose**.
5. Pressionar W → `bTransition_IdleToStart` → **Start** (não ficar preso no Turn).

---

## 8. Problemas comuns

| Sintoma | Causa provável | Correção |
|---------|----------------|----------|
| `Turn=None`, `TIP=0`, warning no **main** | Turn Set só no **layer** (`Anim Graph Overrides` do `ABP_Test_UnArmed_Layer`); o C++ roda TIP no main | Recompile: o main **herda** Turn Set dos linked layers. Confirme `ABP_TestLayerBase` → Parent Class = `UDFAnimInstance`. Transições Idle↔Turn no **layer** usam `bTransition_TurnInPlace` / `bInTurnInPlacePhase` (se aparecerem em vermelho, recompile o C++ e **Refresh** o AnimBP). |
| Vira para o lado errado | Nome L/R do pacote vs convenção UE | Toggle **`bInvertTurnInPlaceDirection`** no Class Defaults |
| “Ponto cego” ~10–35° após 90° | Um `Turn_90` não fecha offset > 90°; retrigger só acima de 60° | Baixar **`TurnInPlace180Threshold`** (default **100**) ou subir snap **`TurnInPlacePostTurnSnapMaxYaw`** (default 35) |
| Personagem “estranho” / clip errado | Offset drenado antes do fim do clip; `RootYawOff=0` com `AimYaw` grande | Recompile: durante turn **não** faz `RootYawOffset=AimYaw`; com `CodeYaw=0` o pawn gira **no fim** do clip (1×). Evaluator + RM off = correto |
| Idle congelado | `Get Locomotion Idle Anim` null | Preencher **Idle Animation** no Anim Set do layer ativo (não no Turn Set) |
| Nunca entra em Turn | `TurnInPlaceTriggerYaw` alto / sem Aim Offset | Baixar threshold; garantir `CalculateAimOffsets` (strafe/lock-on) |
| Troca 90→180 no meio do clip | Anim escolhido no BP por offset ao vivo | Usar só `Get Locomotion Turn Anim` (C++ trava graus no início) |
| Gira mas câmera não acompanha | Só RM no mesh, sem rotacionar pawn | RM no clip + `ConsumeRootYawOffset` ou rotacionar actor no RM |
| Turn durante Stop | Fases sobrepostas | C++ ignora TIP se `bInLocomotionStopPhase` |

---

## 9. Referências cruzadas

- **Locomoção 8-way:** [`18_8Way_StartLoopStop_Setup.md`](18_8Way_StartLoopStop_Setup.md) — Start/Loop/Stop, Distance Matching, `df.LocomotionDebug`.
- **Stride Warping:** [`19_StrideWarping_Setup.md`](19_StrideWarping_Setup.md) — desligado automaticamente durante Stop; idle/turn não usam stride warp.
- **C++:** `DFAnimSetTypes.h` (`FUDTurnInPlaceAnimSet`), `UDFAnimInstance.cpp` (`UpdateTurnInPlace`, getters).

---

**Status:** C++ preenche Turn Set vazio a partir do vizinho do `Idle_Seq`, copia do main → layer, e só entra em TIP se existir clip. Próximo passo no editor: confirmar **Turn Set** no layer + estado **Turn** no `Walk_Run_S`.
