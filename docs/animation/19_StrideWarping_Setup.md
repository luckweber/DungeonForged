# Stride Warping — Configuração Completa (UE 5.4)

Guia dedicado para configurar o nó **Stride Warping** (plugin `AnimationWarping`) no `ABP_JSHeroCharacter`, eliminando o **WARNING** que aparece quando o nó está sem os bones definidos.

> **Complementa** [`18_8Way_StartLoopStop_Setup.md`](18_8Way_StartLoopStop_Setup.md) §5.2. Engine alvo: **UE 5.4**. Nomes de campos validados na doc oficial *Pose Warping* (ver §10).

---

## 0. O que o Stride Warping resolve

Suas Loops de locomoção são autoradas a uma velocidade fixa (ex.: 400 cm/s). Se o `MaxWalkSpeed` real for 500, os pés **arrastam** (a anim "anda" mais devagar que a cápsula). O Stride Warping **estica/encolhe o passo** (distância entre os pés) para casar a passada com a velocidade real, sem precisar de blend spaces por velocidade.

**Equação interna:** `StrideScale = LocomotionSpeed / RootMotionSpeed`
- `LocomotionSpeed` = velocidade real (cápsula/CMC).
- `RootMotionSpeed` = velocidade "de autoria" da animação (root motion do clip).
- Scale **1.0** = sem mudança; **0.5** = meia passada; **2.0** = passada dobrada.

---

## 1. Diagnóstico do seu nó (por que está com WARNING)

Na sua captura, o painel **Detalhes** mostra:

| Campo | Seu valor | Precisa ser |
|---|---|---|
| **Pelvis Bone** | `None` ❌ | `pelvis` |
| **IK Foot Root Bone** | `None` ❌ | `ik_foot_root` |
| **Foot Definitions** | `0 elementos` ❌ | **2 índices** (perna esquerda + direita) |

> O `WARNING!` no rodapé do nó é **exatamente** por causa desses três campos vazios. Assim que você preencher os três (§4), o warning some ao compilar.

Sua fiação já está **correta**:

```
... → [Local To Component] → [Stride Warping] → [Component To Local] → [Slot 'DefaultSlot'] → ...
```

O Stride Warping trabalha em **Component Space**, por isso ele tem que ficar entre um `Local To Component` (antes) e um `Component To Local` (depois). Você já fez isso. 👍

---

## 2. Pré-requisito CRÍTICO — o skeleton precisa ter IK bones

⚠️ **Este é o motivo nº 1 de o dropdown mostrar só `None`.** O Stride Warping **exige** a cadeia de **IK bones virtuais** do padrão UE:

```
root
└─ ik_foot_root
   ├─ ik_foot_l
   └─ ik_foot_r
```

mais os FK normais (`pelvis`, `thigh_l/r`, `calf_l/r`, `foot_l/r`).

### Como verificar
1. Abra o **Skeleton** do JSHero (`SK_..._Skeleton`).
2. Na **Skeleton Tree**, ative *Options → Show Retargeting Options* e procure por `ik_foot_root`, `ik_foot_l`, `ik_foot_r`.

### Se NÃO existirem (pacotes Fab geralmente só têm FK)
Você tem 3 opções, da melhor para a mais simples:

- **(A) Usar o skeleton do Mannequin UE5** (`SK_Mannequin`/`Manny`) e fazer **retarget** das animações do pacote para ele. O Mannequin já tem toda a cadeia IK. É o caminho AAA padrão.
- **(B) Adicionar as IK bones ao seu skeleton** via **Skeleton Tree → botão direito num bone → Add Virtual Bone**, ou importando um FBX com a hierarquia IK. Crie `ik_foot_root` (filho de `root`) e `ik_foot_l/r` (filhos de `ik_foot_root`), posicionados nos `foot_l/r`.
- **(C) Apontar as Foot Definitions para os FK** (`foot_l/r`) e deixar **IK Foot Root Bone = `root`**. Funciona com qualidade menor; o solver de IK fica limitado. Use só como teste rápido.

> Sem uma cadeia IK válida, o nó **continua com warning ou não warpa**. Resolva isso **antes** de seguir.

---

## 3. Wiring no AnimGraph (confirmação)

Se ainda não montou, o padrão é:

1. Arraste do pino de saída da locomoção → **Local To Component**.
2. Do `Local To Component` → **Stride Warping**.
3. Do `Stride Warping` → **Component To Local** (o editor fecha a conversão automaticamente ao ligar no próximo nó).
4. Continue para `[Slot 'DefaultSlot']` → ... → `[Control Rig]` → `[Output Pose]`.

> **Ordem no graph principal** (casa com o doc 18, §6): `... → Stride Warping → (Aim Offset) → Slot 'DefaultSlot' → Control Rig → Output`. O Stride Warping vem **sobre a pose de locomoção**, antes das montages.

Pinos do nó:

| Pino | Liga em |
|---|---|
| **Component Pose** | saída do `Local To Component` |
| **Alpha** | `StrideWarpingAlpha` (variável do AnimInstance) |
| **Stride Direction** *(só Manual)* | vetor de direção (ver §5) |
| **Stride Scale** *(só Manual)* | `Speed / AuthoredLoopSpeed` (ver §5) |
| **Locomotion Speed** *(só Graph)* | `Speed` (ver §5) |

---

## 4. Preencher os 3 campos obrigatórios (mata o WARNING)

Selecione o nó **Stride Warping** → painel **Detalhes** → header **Settings**:

### 4.1 Pelvis Bone
- **Pelvis Bone** = `pelvis`

### 4.2 IK Foot Root Bone
- **IK Foot Root Bone** = `ik_foot_root`

### 4.3 Foot Definitions (clique no **+** duas vezes — uma por perna)

**Index [0] — perna esquerda:**

| Campo | Valor (Mannequin) |
|---|---|
| **IK Foot Bone** | `ik_foot_l` |
| **FK Foot Bone** | `foot_l` |
| **Thigh Bone** | `thigh_l` |
| **Num Bones In Limb** | `2` (calf + thigh, sem contar o pé) |

**Index [1] — perna direita:**

| Campo | Valor (Mannequin) |
|---|---|
| **IK Foot Bone** | `ik_foot_r` |
| **FK Foot Bone** | `foot_r` |
| **Thigh Bone** | `thigh_r` |
| **Num Bones In Limb** | `2` |

> **`Num Bones In Limb = 2`** porque, do pé até (mas sem incluir) o pelvis, há **calf** e **thigh**. Se seu rig tiver mais ossos na perna, ajuste.

✅ Compile. O `WARNING!` deve sumir.

---

## 5. Mode: Manual vs Graph — como dirigir o warp

O comportamento muda conforme o **Mode** (Evaluation Mode) no painel Settings.

### Opção A — **Graph** (recomendado, mais simples)

O nó calcula o Stride Scale sozinho (`LocomotionSpeed / RootMotionSpeed`). **Requer animações Loop com Root Motion habilitado** (para o nó saber a `RootMotionSpeed` autoral).

1. **Mode** = `Graph`.
2. O pino **Locomotion Speed** aparece → ligue a variável **`Speed`** do AnimInstance.
3. **Min Locomotion Speed Threshold** = `150` (abaixo disso não warpa — evita warp na cauda de desaceleração; bate com `StrideWarpingMinSpeed` do C++).
4. **Alpha** = `StrideWarpingAlpha`.

> `Speed` já existe no `UDFAnimInstance` (use **Get Speed** no AnimBP):
> ```254:255:Source/DungeonForged/Public/Animation/UDFAnimInstance.h
>	UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion")
>	float Speed = 0.f;
> ```

### Opção B — **Manual** (use se suas Loops NÃO têm root motion / são in-place)

Você fornece o scale calculado no AnimBP.

1. **Mode** = `Manual`.
2. **Stride Direction** = `(1, 0, 0)` (Forward em component space) — para locomoção 8-way puxe a direção do movimento; (1,0,0) é um default seguro.
3. **Stride Scale** (pino) = no AnimBP, ligue **`Speed ÷ AuthoredLoopSpeed`**:

```
[Get Speed] ──┐
              ├─▶ ( float / float ) ──▶ Stride Scale
[Get AuthoredLoopSpeed] ─┘
```

4. **Alpha** = `StrideWarpingAlpha`.

> ⚠️ **Por isso seu Stride Scale = 1.0 fixo não faz nada.** `1.0` significa "passada igual à autoral" = sem warp. Em Manual você **precisa** dividir `Speed / AuthoredLoopSpeed`. `AuthoredLoopSpeed` (default 400) já existe no C++:
> ```413:419:Source/DungeonForged/Public/Animation/UDFAnimInstance.h
>	/** Authored speed of Loop animations (cm/s) — Stride Warping divides actual / authored. */
>	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|StrideWarping", meta = (ClampMin = "1.0"))
>	float AuthoredLoopSpeed = 400.f;
>
>	/** Min speed to engage stride warping (avoid warping during deceleration tail). */
>	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|StrideWarping", meta = (ClampMin = "0.0"))
>	float StrideWarpingMinSpeed = 150.f;
> ```

| | **Graph** | **Manual** |
|---|---|---|
| Quem calcula o scale | o nó | você (`Speed/AuthoredLoopSpeed`) |
| Precisa root motion nas Loops? | **Sim** | Não |
| Pino exposto | `Locomotion Speed` | `Stride Direction` + `Stride Scale` |
| Quando usar | clips com root motion | clips in-place |

---

## 6. Stride Scale Modifier (clamp de segurança)

Expanda **Stride Scale Modifier** no painel Settings e ative:

| Sub-propriedade | Valor sugerido | Função |
|---|---|---|
| **Clamp Result** | ✅ ON | Limita o scale final |
| **Clamp Min** | `0.5` | Não encolhe além da metade |
| **Clamp Max** | `1.5` | Não estica além de 1.5× (evita "passos de gigante") |
| **Interp Result** | opcional | Suaviza mudanças bruscas de scale |

---

## 7. Advanced (deixe nos defaults para começar)

Os checkboxes que aparecem na sua captura já vêm bons:

| Propriedade | Recomendado | Nota |
|---|---|---|
| **Orient Stride Direction Using Floor Normal** | ✅ ON | Alinha a passada à inclinação do chão |
| **Compensate IK Using FK Thigh Rotation** | ✅ ON | Preserva o formato da perna |
| **Clamp IK Using FK Limits** | ✅ ON | Evita hiperextensão do joelho |
| **Floor Normal Direction** / **Gravity Direction** | default | Só mexa em casos especiais (gravidade custom) |

---

## 8. Debug (validar visualmente)

No painel **Depurar** do nó:

| Propriedade | Liga |
|---|---|
| **Enable Debug Draw** | seta vermelha = vetor de velocidade (cresce com a velocidade) |
| **Debug Draw IKFoot Origin** | esferas vermelhas nos pés IK |
| **Debug Draw IKFoot Adjustment** | setas azuis = ajuste sendo aplicado |
| **Debug Draw Scale** | escala visual dos guias (1.0) |

Console em PIE: `a.animnode.stridewarping.debug 1` (se disponível na build).

**Esperado:** correndo acima de `AuthoredLoopSpeed`, passos **mais longos**; abaixo, **mais curtos**. Pelvis com leve ajuste vertical visível.

---

## 9. Calibrar `AuthoredLoopSpeed`

O scale só fica certo se `AuthoredLoopSpeed` = velocidade real do root motion da sua `Loop_F`:

1. Abra `Loop_F` (Run) no Persona.
2. **Asset Details → Root Motion** (ou habilite *Process Root Motion* e meça o deslocamento ÷ duração).
3. Defina **`AuthoredLoopSpeed`** (Class Defaults do `ABP_JSHeroCharacter` → `DF | Locomotion | StrideWarping`) com esse valor.

> Se Walk e Run têm velocidades autorais diferentes, calibre para a **Run** (gait mais usado) e deixe o clamp absorver a diferença no Walk; ou exponha um `AuthoredLoopSpeed` por gait se quiser precisão total.

---

## 10. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| `WARNING!` persiste após compilar | Algum dos 3 campos ainda `None`, ou Foot Definitions com bone inválido | Revise §4; confirme nomes exatos dos bones |
| Dropdown não lista `ik_foot_root` | Skeleton sem cadeia IK | §2 — retarget pro Mannequin ou adicione virtual bones |
| Nó sem warning mas **não warpa** | Mode=Manual com `Stride Scale = 1.0` fixo | Ligue `Speed / AuthoredLoopSpeed` no pino Stride Scale (§5-B) ou use Graph (§5-A) |
| Warpa mas dá **snap brusco** | `AuthoredLoopSpeed` longe do real | Recalibre (§9) e ative **Interp Result** no Stride Scale Modifier |
| Joelho hiperestende | `Clamp IK Using FK Limits` OFF | Ligue (§7) e baixe `Clamp Max` para ~1.3 |
| Pés "patinam" mesmo com warp | Foot lock ausente | Trate no `Control Rig` (foot lock por curva `FootPlant_L/R`) — ver doc 18 §5.3 |
| Em Graph, scale fica preso em 1 | Loops sem root motion → `RootMotionSpeed` indefinido | Habilite root motion nos clips ou troque para Manual |

---

## 11. Checklist

- [ ] Skeleton tem `ik_foot_root`, `ik_foot_l`, `ik_foot_r` (§2)
- [ ] Fiação `Local To Component → Stride Warping → Component To Local` (§3)
- [ ] `Pelvis Bone = pelvis` (§4.1)
- [ ] `IK Foot Root Bone = ik_foot_root` (§4.2)
- [ ] Foot Definitions com 2 índices (L/R) preenchidos (§4.3)
- [ ] `WARNING!` sumiu ao compilar
- [ ] Mode definido (Graph se root motion; Manual caso contrário) (§5)
- [ ] `Alpha ← StrideWarpingAlpha`
- [ ] `Stride Scale Modifier` com clamp 0.5–1.5 (§6)
- [ ] `AuthoredLoopSpeed` calibrado pela `Loop_F` (§9)
- [ ] Debug Draw confirma passos esticando/encolhendo (§8)

---

## 12. Referências externas (UE oficial)

- [Pose Warping in Unreal Engine — Stride / Orientation / Slope Warping](https://dev.epicgames.com/documentation/en-us/unreal-engine/pose-warping-in-unreal-engine) — lista oficial de propriedades, equação `StrideScale = LocomotionSpeed / RootMotionSpeed`, Foot Definitions (IK/FK/Thigh + Num Bones In Limb), Manual vs Graph.

---

**Status:** o C++ (`UDFAnimInstance`) já expõe `Speed`, `StrideWarpingAlpha`, `AuthoredLoopSpeed` e `StrideWarpingMinSpeed`. Falta só o setup no nó (campos obrigatórios §4 + Mode §5). Comece pela §2 (IK bones) — sem isso nada warpa.
