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
| Escolha 90 vs 180 | `\|RootYawOffset\| >= TurnInPlace180Threshold` (default **100°**) |
| Abort durante turn | Só **ar** (`AbortAir`) ou **input de movimento** (`AbortInput`) — não por Stop phase nem `Speed` fantasma |

### Modo exploração (ALS) — o que você quer

| Situação | Cápsula (corpo) | Mesh / AnimBP |
|----------|-----------------|---------------|
| Idle, câmera **&lt; 60°** (`TurnInPlaceTriggerYaw`) | **Não gira** com a câmera | `RootYawOffset` → **Aim Offset / spine** só |
| Idle, câmera **≥ 60°** | **Não** segue câmera frame a frame | Entra estado **Turn** + clip 90°/180° |
| Durante **Turn** (`bInTurnInPlacePhase`) | Gira pela curva `RotationYawSpeed` | `RootYawOffset` travado no C++ |
| Andando (WASD) | Locomoção normal | TIP desligado |

**Não é bug** ver o torso/olhar acompanhar um pouco a mira abaixo de 60° — isso é Aim Offset. **É bug** a cápsula inteira rodar aos 30° sem clip de Turn.

Condição de idle no C++: `IsIdleForTurnInPlace()` = no chão, `Speed` ≤ deadband, **sem stick de movimento** (Stop phase permitida se `spd=0`).

Quando `|RootYawOffset| > TurnInPlaceTriggerYaw` (default **60°**) **e** idle → `bTransition_TurnInPlace` + `bInTurnInPlacePhase`, clip e `RotationYawSpeed`.

### Câmera vs corpo durante o Turn

**Comportamento intencional:** enquanto `bInTurnInPlacePhase` (estado **Turn** na SM), o **player não gira com a câmera** em tempo real.

| O que acontece | Detalhe |
|----------------|---------|
| Mover mouse / stick | A **câmera** (control rotation) continua livre; o **corpo** não acompanha frame a frame |
| `RootYawOffset` | **Travado** no início do turn — o C++ **não** faz `RootYawOffset = AimYaw` a cada tick durante a fase |
| `AimYaw` no debug | Pode divergir de `RootYawOffset` enquanto você gira a mira no meio do clip — é esperado |
| Rotação da **cápsula** | Só com `phase=1` (`ShouldDriveCapsuleYawFromTurn`) via `RotationYawSpeed` — **nunca** com `phase=0` |
| AnimBP | `RootYawOffset` no **Aim Offset**; não use Rotate Root Bone na cápsula fora do Turn |

Isso evita cancelar o one-shot, impede trocar de clip 90↔180 no meio da animação e mantém o **Sequence Evaluator** estável. Se precisar reorientar de novo, espere o turn terminar (`phase=0`) ou reduza o offset da câmera abaixo de `TurnInPlaceRetriggerYaw` (default **45°**) antes de um novo trigger.

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
| `IsIdleForTurnInPlace()` | Parado ALS (sem WASD, spd ~0) |
| `ShouldPlayTurnInPlaceTransition()` | Idle + \|off\| > trigger |
| `ShouldDriveCapsuleYawFromTurn()` | Só true em `phase=1` |
| `bTransition_IdleToStart` | Idle/Turn → Start quando W |

---

## 4. Tuning (Class Defaults do AnimInstance)

| Propriedade | Default | Efeito |
|-------------|---------|--------|
| `TurnInPlaceTriggerYaw` | 60 | Mínimo de offset para iniciar turn |
| `TurnInPlace180Threshold` | 100 | Usa clip 180° em vez de 90° |
| `TurnInPlaceCompleteYaw` | 8 | Encerra turn quando offset residual ≤ isto |
| `TurnInPlaceYawInterpSpeed` | 360 | Bleed do offset enquanto **anda** |
| `bTurnInPlaceDriveYawFromRotationCurve` | true | Delta da curva Rotation/Yaw por frame (`curve=1`) |
| `bTurnInPlaceApplyActorYawFromCode` | false | Linear legado — deixar off |
| `TurnInPlaceRotationCurveName` | (vazio) | Nome fixo da curva; vazio = auto |

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
| `spd` / `accel` | `Speed` e `bIsAccelerating` — turn ativo só aborta se `accel=1` (W) |
| Dump `air` / `stop` | `bIsInAir` / `bInLocomotionStopPhase` (Stop **não** cancela turn em andamento) |
| `armed=0` + `ready=1` | offset alto mas `\|off\| < TurnInPlaceRetriggerYaw` (45°) ainda não — espera câmera baixar OU recompile (retry após abort) |
| `spd` alto + `idle=0` + `phase=1` | RM ou snap anterior — C++ zera XY ao **iniciar** e ao **abortar/snap** turn |
| `dir` / `deg` | L/R e 90/180 **travados** no início do turn |
| `end` | Último fim: `ClipDone`, `OffsetDone`, `AbortAir`, `AbortInput`, `NoClip` — **limpo** ao iniciar turn novo; vazio durante `phase=1` |
| `codeYaw=1` | `bTurnInPlaceApplyActorYawFromCode` — default C++ **true** (Opção A) |

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

## 6. Rotação do pawn — RotationYawSpeed (ALS Refactored)

**Defaults C++ (recompile):**

| Propriedade | Default | Log |
|-------------|---------|-----|
| `bTurnInPlaceDriveYawFromRotationCurve` | **true** | `curve=1` + `clipCurve=1` |
| `bTurnInPlaceApplyActorYawFromCode` | **false** | `codeYaw=0` |
| `TurnInPlacePostTurnSnapMaxYaw` | **0** | sem snap automático pós-turn |
| Enable Root Motion nos `Turn_*_Seq` | **Off** | `RM=0` |

Como [ALS-Refactored](https://github.com/Sixze/ALS-Refactored) (`ApplyRotationYawSpeedAnimationCurve`): o clip leva **`RotationYawSpeed`** (°/s). `UDFCharacterMovementComponent::PhysicsRotation` aplica `GetCurveValue(RotationYawSpeed) * DeltaTime` enquanto `bInTurnInPlacePhase`. O `UpdateTurnInPlace` **não** chama `SetActorRotation` — evita girar sozinho / em duplicata com curva absoluta `Rotation` 0→180.

### Inválido

| Combo | Sintoma |
|-------|---------|
| `codeYaw=1` sem curva | Rotação linear fixa (deg/s) desincronizada da câmera; spikes de `spd` após o turn |
| `RM=1` + qualquer rotação C++ | Double-rotate / velocidade fantasma |

### Sem curva no Fab (só pose)

Se `curve=0` no dump: o clip só alimenta o **Sequence Evaluator**; no **fim** do clip um snap único fecha até `TurnInPlaceAnimDegrees`. Adicione a curva no editor (§10) para sincronizar grau ↔ tempo.

### RM

Manter **RM Off** nos Turn clips do Fab; rotação vem da curva + Evaluator, não da cápsula RM.

---

## 7. Checklist de teste

1. PIE, olhar fixo, mover mouse / stick para girar câmera **sem** W → entra em **Turn**.
2. **Durante o Turn:** continuar girando a câmera — o corpo **não** deve seguir a mira em tempo real; só o clip (e, no fim, o snap de yaw do C++).
3. `|RootYawOffset|` passa de 60° → transição para **Turn**, clip L ou R.
4. Offset ≥ 100° (`TurnInPlace180Threshold`) → **Turn_180_***.
5. Log: `curve=1`, `codeYaw=0`, `off` caindo com `t`, `end=ClipDone` (sem `actorYaw` saltando com `phase=0`).
6. Ao terminar, `bInTurnInPlacePhase=0`, volta **Idle Pose**.
7. Pressionar W → `bTransition_IdleToStart` → **Start** (não ficar preso no Turn).

---

## 8. Problemas comuns

| Sintoma | Causa provável | Correção |
|---------|----------------|----------|
| `Turn=None`, `TIP=0`, warning no **main** | Turn Set só no **layer** (`Anim Graph Overrides` do `ABP_Test_UnArmed_Layer`); o C++ roda TIP no main | Recompile: o main **herda** Turn Set dos linked layers. Confirme `ABP_TestLayerBase` → Parent Class = `UDFAnimInstance`. Transições Idle↔Turn no **layer** usam `bTransition_TurnInPlace` / `bInTurnInPlacePhase` (se aparecerem em vermelho, recompile o C++ e **Refresh** o AnimBP). |
| Vira para o lado errado | Nome L/R do pacote vs convenção UE | Toggle **`bInvertTurnInPlaceDirection`** no Class Defaults |
| “Ponto cego” ~10–35° após 90° | Um `Turn_90` não fecha offset > 90°; retrigger só acima de 60° | Baixar **`TurnInPlace180Threshold`** (default **100**) ou subir snap **`TurnInPlacePostTurnSnapMaxYaw`** (default 35) |
| Gira sozinho sem `phase=1` | Curva `Rotation` 0→180 + C++ antigo, RM, ou `bOrientRotationToMovement` com `spd` fantasma | Recompile; reaplicar modificador (revision 2 → `RotationYawSpeed`); `clipCurve=1`; RM Off |
| `curve=0` / `clipCurve=0` | Turn_* sem `RotationYawSpeed` ou modifier Out of Date | §10 — Apply Modifier em todos os Turn |
| `RM=1` + rotação C++ | Combo inválido (§6) | RM Off nos Turn clips |
| Personagem “estranho” / `off=0` com `aim` grande após abort | RM girou actor; offset travado até abort | Opção A; ver §6 |
| `end=NotGrounded` em build antigo | Stop phase / `!bGrounded` abortava turn ativo | Recompile: `AbortAir` / `AbortInput` apenas; `end` limpo ao iniciar turn |
| Idle congelado | `Get Locomotion Idle Anim` null | Preencher **Idle Animation** no Anim Set do layer ativo (não no Turn Set) |
| Nunca entra em Turn | `TurnInPlaceTriggerYaw` alto / sem Aim Offset | Baixar threshold; garantir `CalculateAimOffsets` (strafe/lock-on) |
| Troca 90→180 no meio do clip | Anim escolhido no BP por offset ao vivo | Usar só `Get Locomotion Turn Anim` (C++ trava graus no início) |
| Gira mas câmera não acompanha | Só RM no mesh, sem rotacionar pawn | RM no clip + `ConsumeRootYawOffset` ou rotacionar actor no RM |
| Câmera gira mas o corpo “não segue” **no meio** do Turn | Comportamento **correto** — ver §0 “Câmera vs corpo” | Não force `RootYawOffset = AimYaw` no BP durante `phase=1`; espere `ClipDone` |
| Turn não **inicia** durante Stop | `bIdleEnoughToStartTurn` exige `!bInLocomotionStopPhase` | Normal; turn **em andamento** continua mesmo com `stop=1` no dump |

---

## 9. Referências cruzadas

- **Locomoção 8-way:** [`18_8Way_StartLoopStop_Setup.md`](18_8Way_StartLoopStop_Setup.md) — Start/Loop/Stop, Distance Matching, `df.LocomotionDebug`.
- **Stride Warping:** [`19_StrideWarping_Setup.md`](19_StrideWarping_Setup.md) — desligado automaticamente durante Stop; idle/turn não usam stride warp.
- **C++:** `DFAnimSetTypes.h` (`FUDTurnInPlaceAnimSet`), `UDFAnimInstance.cpp` (`UpdateTurnInPlace`, getters).

---

## 10. Referência ALS — curva de rotação ↔ tempo do clip

O **ALS Refactored** calcula **`RotationYawSpeed`** a partir do twist do osso `root` no sequence (`UAlsAnimationModifier_CalculateRotationYawSpeed`) e o personagem aplica `GetCurveValue(RotationYawSpeed) * DeltaTime` em `PhysicsRotation` quando parado. O DungeonForged replica esse fluxo (modificador + `TryApplyTurnInPlaceRotationYawSpeed`).

### Repositórios de referência

| Projeto | Link | Notas |
|---------|------|--------|
| **ALS Refactored** (C++, UE 5.4+) | [github.com/Sixze/ALS-Refactored](https://github.com/Sixze/ALS-Refactored) | Turn in place, linked anim layers, curvas no skeleton — versão **4.14** do plugin para UE 5.4 |
| **ALS Community** (replicado, UE 5.4) | [github.com/PanicPetal/ALS-Community](https://github.com/PanicPetal/ALS-Community) | Mesma base V4; útil para comparar AnimBP / curvas em `Content/AdvancedLocomotionV4` |

No ALS Refactored, use o wiki/releases do repo e a ação **Setup Als Skeleton** (curvas padrão no skeleton). O ALS também usa **Animation Modifiers** no editor (ex.: foot sync markers no [ALS-Refactored](https://github.com/Sixze/ALS-Refactored)). No DungeonForged há um modificador equivalente para a curva de turn.

### Modificador automático — `DF Turn In Place Rotation Curve` (recomendado)

Classe C++: `UUDFTurnInPlaceRotationCurveModifier` (módulo `DungeonForgedEditor`). Após recompilar o projeto:

1. Abra um `Turn_90_R_Seq` (ou `Turn_180_L_Seq`, etc.) no editor de **Sequence**.
2. Painel **Modificadores de animação** (como na sua captura) → **Adicionar modificador**.
3. Escolha **`DF Turn In Place Rotation Curve`**.
4. Clique **Aplicar todos os modificadores** (ou aplique só este).
5. Aba **Curvas** deve mostrar **`RotationYawSpeed`** (graus/segundo), não `Rotation` 0→180.

| Opção do modificador | Efeito |
|---------------------|--------|
| `CurveName` | `RotationYawSpeed` |
| `RootBoneName` | `root` (troque para `pelvis` se o turn não animar o root) |
| `bFallbackSyntheticYawSpeedIfNoRootMotion` | Deriva °/s do nome `Turn_90_R` etc. se o track do root não girar |
| `bAutoDetectYawFromAssetName` | Lê `Turn_90_R`, `Turn_180_L`, `Turn_Combat_90_R_Seq`, etc. |

**Lote:** abra cada Turn do Fab (`Turn_90_L/R`, `Turn_180_L/R`, variantes Combat), adicione o mesmo modificador e **Aplicar**. Ou copie a pilha de modificadores do primeiro sequence para os outros (menu do painel de modificadores).

**Revert:** remove `RotationYawSpeed` e a curva legada `Rotation`.

**Log:** Output Log filtro `TIP Modifier` — confirma graus e duração aplicados.

### Manual (alternativa)

1. Aba **Curvas** → curva float **`RotationYawSpeed`** (ou use o modificador DF).
2. Valores em **graus por segundo** (não graus acumulados).
3. **Enable Root Motion = Off** no sequence.

O C++ faz:

```text
Δyaw = RotationYawSpeed(T) * DeltaTime   (em PhysicsRotation, como ALS)
pawn.Yaw += Δyaw
RootYawOffset -= Δyaw
```

Assim o grau girado no mundo bate com o grau da curva **naquele** `T` do clip — não com `180°/1.5s` fixo.

### Conferir no PIE

```text
df.TurnDebug 1
```

- `phase=0` → `actorYaw` estável enquanto só mexe a câmera (|off| &lt; 60).
- `phase=1` → `curve=1(Rotation)` (ou nome resolvido), `t` sobe, `off` cai, `actorYaw` segue a curva.
- `phase=0` após `end=ClipDone` → sem `spd` 200+ fantasma.

### Class Defaults (AnimInstance)

| Campo | Valor |
|-------|--------|
| `bTurnInPlaceDriveYawFromRotationCurve` | true |
| `bTurnInPlaceApplyActorYawFromCode` | false |
| `TurnInPlaceRotationCurveName` | vazio (auto) ou `Rotation` |

---

**Status:** Recompile (runtime + editor) → aplicar modificador **DF Turn In Place Rotation Curve** nos Turn Fab → PIE com `curve=1` no `df.TurnDebug`. AnimBP: estado **Turn** só com `bInTurnInPlacePhase`; Idle usa `Get Locomotion Idle Anim` apenas.
