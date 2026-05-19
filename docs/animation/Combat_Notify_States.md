# DungeonForged — AnimNotifyStates de combate (referência de designer)

> Referência rápida das 4 notify states que controlam **aim**, **telegraph**, **cancel** e **parry**.
> Cada uma é arrastada na timeline da montage em pontos específicos. Para a teoria completa, ver [`10_AAA_AimWarpCombat.md`](../improvements/10_AAA_AimWarpCombat.md).

---

## Pré-requisito (uma vez)

No `ABP_Warrior`, `ABP_Enemy_*` e qualquer outro AnimBlueprint de personagem:

1. Open AnimGraph.
2. Insere o nó **`Motion Warping`** entre `Slot 'DefaultSlot'` e `Output Pose`.
3. Sem isto, `UANS_DFMeleeWarp` **não tem efeito**.

Na montage:
- **Habilita `Enable Root Motion`** (Asset Details).
- No character: `Mesh.Animation > AnimGraph` → `Root Motion From Montages Only`.

---

## 1. `DF Melee Warp Target` — motion warping

**Quando usar:** windup de qualquer ataque que precise rotacionar / aproximar do alvo.

**Onde colocar:** start do windup → antes do `AN_TraceStart` (geralmente cobre 30–50% do início da montage).

**Cor na timeline:** `Orange` (#FF8C00)

| Property | Default | Quando mudar |
|---|---|---|
| `WarpTargetName` | `MeleeTarget` | Deixar — match com Motion Warping node |
| `DesiredStopDistance` | 150 cm | Espada=150, lança=220, machado 2H=180, fist=80 |
| `MaxWarpDistance` | 800 cm | Aumenta para abilities de gap-close (charge=2000) |
| `bRotationOnly` | **false** | **TRUE em enemies** (só roda, sem deslizar) |
| `bMatchTargetZ` | false | True só em ataques áereos / verticais |
| `bUpdateEveryTick` | true | False = warp congela no início (alvos parados) |
| `bSnapYawOnBegin` | false | True = snap brusco no início (fallback) |
| `bDrawDebug` | false | Ligar pra ver linhas no editor |

**Quando ele **não** funciona:**
- Sem nó Motion Warping no AnimGraph → ignorado silenciosamente.
- Montage sem root motion → não há translação pra redirecionar.
- Sem `UDFMeleeAimComponent` no owner → não resolve target.
- Alvo > `MaxWarpDistance` → warp clamped pra zero.

---

## 2. `DF Enemy Telegraph` — windup ground warning

**Quando usar:** ataques de inimigos médios/pesados que devem ser legíveis (player tem que ver chegando).

**Onde colocar:** início do windup, cobre ~50–70% do windup todo. Termina junto com `AN_TraceStart` (ou um pouco antes).

**Cor na timeline:** `Red` (#DC1E1E)

| Property | Default | Notas |
|---|---|---|
| `GroundWarningVFX` | nullptr | **OBRIGATÓRIO:** Niagara anel vermelho no chão sob o alvo |
| `WeaponChargeVFX` | nullptr | Opcional: faíscas anexadas ao socket da arma |
| `WeaponChargeSocketName` | `weapon_r` | Match com o socket do mesh do inimigo |
| `WindupSound` | nullptr | Som de "growl" no início (one-shot) |
| `LocationOffset` | (0,0,5) | Eleva o FX 5cm pra não z-fight com o chão |
| `bUpdateTargetEveryTick` | true | False = telegraph "trava" onde o player estava |
| `bUseAttackerForwardInsteadOfTarget` | false | **True para ataques AOE** (telegrafa onde o ataque vai LANDAR, não onde o player está) |
| `ForwardOffsetCm` | 180 cm | Quando `bUseAttackerForwardInsteadOfTarget` |
| `bAddStateTag` | true | Adiciona `State.Combat.Telegraph.Active` (UI lê) |
| `bSendGameplayEvents` | true | Fire Event.Combat.Telegraph.Begin/End |

**Side effects:**
- `ASC->AddLooseGameplayTag(State.Combat.Telegraph.Active)` — UI ouvinte mostra ícone de perigo.
- Eventos `Event.Combat.Telegraph.Begin/End` — boss listeners reagem.

**Net:** roda em todas as máquinas (sem multicast manual), porque AnimNotifyState dispara em qualquer machine que toque a montage.

---

## 3. `DF Cancel Window` — janela de cancelamento

**Quando usar:** recovery do swing do player (último 25–40% da montage).

**Onde colocar:** depois do `AN_TraceEnd`, até o final da montage (ou no momento que o jogador "deveria" poder chainear).

**Cor na timeline:** `Green` (#1EC85A)

| Property | Default | Notas |
|---|---|---|
| `bSendGameplayEvents` | true | Fire Event.Combat.CancelWindow.Open/Close |

Side effects:
- `ASC->AddLooseGameplayTag(State.Combat.CancelWindow.Open)` — heavy attack checa pra ativar.
- Dodge **não** é gated por isto (sempre cancela).

**Como heavy reage:**

[`UDFAbility_Warrior_HeavyAttack::CanActivateAbility`](../../Source/DungeonForged/Private/GAS/Abilities/Warrior/UDFAbility_Warrior_HeavyAttack.cpp#L38):
- Fora de swing → ativa normal.
- Em swing **sem** `State.Combat.CancelWindow.Open` → bloqueia.
- Em swing **com** `State.Combat.CancelWindow.Open` → ativa + cancela light via `CancelAbilitiesWithTag(Ability.Warrior.MeleeSwing)`.

---

## 4. `DF Parry Window` — janela de parry

**Quando usar:** primeiros 0.2–0.4s do windup de inimigos (sweet spot é "depois do anticipation, antes do swing acelerar").

**Onde colocar:** no início do windup, **dentro** do `DF Enemy Telegraph` (telegraph é o aviso visual, parry window é a janela real onde o golpe perfeito conta).

**Cor na timeline:** `Gold` (#FFD700)

| Property | Default | Notas |
|---|---|---|
| `bSendGameplayEvents` | true | Fire Event.Combat.ParryWindow.Open/Close |

Side effects:
- `ASC->AddLooseGameplayTag(State.Combat.ParryWindow.Open)` no inimigo.
- Player melee trace detecta a tag → aplica `ParryReactionGE` (stun) + boost de dmg (1.5×).
- Dispara `Event.Combat.Parry.Triggered` nos 2 lados (HUD pode mostrar "PARRY!").

**Não dá parry sem config:** o player precisa ter `ParryReactionGameplayEffect` setado no `MeleeTrace`. Caso contrário, só o dmg boost acontece (sem stun).

---

## Quadro resumo — ordem na timeline

### Player combo montage (1.2s)

```
                                                         Combo
0.00 ──────── 0.18 ────────── 0.45 ──────── 0.75 ──────  next
   │             │               │              │           │
[ DF Melee Warp ]                                            │
                  │                                          │
                  [AN_TraceStart]                            │
                                  [AN_TraceEnd]              │
                                                │            │
                                  [DF Cancel Window]         │
                                                             │
                              [AN_ComboWindowOpen]
```

### Enemy attack montage (1.5s)

```
0.00 ────── 0.20 ────── 0.55 ────── 0.85 ────── 1.30
   │           │           │           │           │
[ DF Enemy Telegraph                  ]
   │
[DF Parry Window]
   │   │
   [ DF Melee Warp (bRotationOnly=true) ]
                          │
                          [AN_TraceStart]
                                    [AN_TraceEnd]
```

### Heavy attack do player (1.8s)

```
0.00 ──── 0.35 ──── 0.95 ──── 1.30 ──── 1.80
   │         │         │         │         │
[ DF Melee Warp (StopDistance=200) ]
                       │
                       [AN_TraceStart]
                              [AN_TraceEnd]
                                       │
                                       [DF Cancel Window]
```

---

## Verificação no editor

1. Open the montage → toolbar tem botão **"Show Notify Tracks"**.
2. Drag notify state da palette esquerda pra timeline (filtra por "DF").
3. Click pra selecionar — Details panel mostra as properties.
4. Right-click no notify → "Set Notify Color" se quiser destacar visualmente.
5. Para testar in-editor com debug, abre o BP do char → seta `bDrawDebug = true` nas componentes; em PIE vê linhas/spheres do warp + cone + telegraph.

---

## Cheat sheet de troubleshooting

| Sintoma | Diagnóstico |
|---|---|
| Player não vira pro inimigo no swing | (1) `MeleeAim` ausente no BP? (2) `bConsiderPlayerLockOn` está false? (3) Lock-on ativo mas alvo já morto? |
| Enemy ataca de costas | (1) Aim component sem target? Cast em log na ativação da ability. (2) `bRotationOnly=false` no warp do enemy = ele desliza em vez de girar |
| Warp não move | (1) Falta nó Motion Warping no AnimGraph. (2) Montage sem root motion. (3) Character `Root Motion Mode` não está em `Root Motion From Montages Only` |
| Heavy não ativa durante swing | Funcionando como esperado — só entra dentro do `DF Cancel Window`. Verifica a notify foi colocada no recovery |
| Parry stuna mas não dá dmg bônus | `ParryDamageMultiplier=1` no MeleeTrace. Setar 1.5 |
| Stagger nunca triggera | (1) `Poise` muito alta. (2) `StaggerWindow` muito curto. (3) ASC não está bindando — ativa `bLogVerbose` no Stagger e olha o output log |
| Telegraph FX não aparece | (1) `GroundWarningVFX` é nullptr. (2) Niagara não está marcado `bAutoActivate`. (3) Sistema Niagara mal-configurado (Local Space ao invés de World) |
