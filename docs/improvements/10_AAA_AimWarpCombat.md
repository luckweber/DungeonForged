# 10 — Combate AAA: Aim, Motion Warping, Telegraph, Parry, Stagger

> **Objetivo:** ligar todo o pipeline AAA-style de combate melee (aim assist, motion warping, cancel windows, parry, stagger). Cobre setup de C++ → AnimGraph → AnimMontage → Blueprint defaults.
>
> **Estado:** Sistemas em C++ prontos (✅) — falta BP/asset wire (🟡).

---

## Sumário do que entrou

| Sistema | C++ | Onde |
|---|---|---|
| Motion Warping (plugin) | ✅ | [`DungeonForged.uproject`](../../DungeonForged.uproject) + [`Build.cs`](../../Source/DungeonForged/DungeonForged.Build.cs) |
| `UDFMeleeAimComponent` | ✅ | [`UDFMeleeAimComponent.h`](../../Source/DungeonForged/Public/Combat/UDFMeleeAimComponent.h) |
| `UDFStaggerComponent` | ✅ | [`UDFStaggerComponent.h`](../../Source/DungeonForged/Public/Combat/UDFStaggerComponent.h) |
| `UANS_DFMeleeWarp` | ✅ | [`ANS_DFMeleeWarp.h`](../../Source/DungeonForged/Public/Combat/AN/ANS_DFMeleeWarp.h) |
| `UANS_DFEnemyTelegraph` | ✅ | [`ANS_DFEnemyTelegraph.h`](../../Source/DungeonForged/Public/Combat/AN/ANS_DFEnemyTelegraph.h) |
| `UANS_DFCancelWindow` | ✅ | [`ANS_DFCancelWindow.h`](../../Source/DungeonForged/Public/Combat/AN/ANS_DFCancelWindow.h) |
| `UANS_DFParryWindow` | ✅ | [`ANS_DFParryWindow.h`](../../Source/DungeonForged/Public/Combat/AN/ANS_DFParryWindow.h) |
| Fix enemy "de costas" | ✅ | [`UDFAbility_Enemy_Melee.cpp:95`](../../Source/DungeonForged/Private/GAS/Abilities/UDFAbility_Enemy_Melee.cpp#L95) |
| Heavy cancela light | ✅ | [`UDFAbility_Warrior_HeavyAttack.cpp:42`](../../Source/DungeonForged/Private/GAS/Abilities/Warrior/UDFAbility_Warrior_HeavyAttack.cpp#L42) |
| Parry detection no trace | ✅ | [`UDFMeleeTraceComponent.cpp:1293`](../../Source/DungeonForged/Private/Combat/UDFMeleeTraceComponent.cpp#L1293) |

---

## 1. Como o sistema funciona em runtime

```
                ┌──────────────────────────────┐
Player ataca ─→ │ UDFAbility_Warrior_MeleeSwing│
                └────┬─────────────────────────┘
                     │
                     ▼
        ┌────────────────────────────────────┐
        │ UDFMeleeAimComponent               │
        │   AcquireAndCommitTarget()         │
        │                                    │
        │   1. ManualTarget? (per-attack)    │
        │   2. LockOn → CurrentTarget?       │
        │   3. AI BB::TargetActor?           │
        │   4. Soft cone sweep (cone + LOS)  │
        │                                    │
        │   SnapYaw se >15° desviado          │
        └────┬───────────────────────────────┘
             │
             ▼
        Montage.Play(Combo[CurrentStep])
             │
             ▼
   ╔═══════════════════════════════════════╗
   ║  WINDUP (frames 0 – swing-start)      ║
   ║  ┌─────────────────────────────────┐  ║
   ║  │ UANS_DFMeleeWarp                │  ║
   ║  │   → MotionWarpingComp           │  ║
   ║  │   → AnimGraph MotionWarping node│  ║
   ║  │   → root motion gira + lunge    │  ║
   ║  └─────────────────────────────────┘  ║
   ╠═══════════════════════════════════════╣
   ║  IMPACT                               ║
   ║   AN_TraceStart → AN_TraceEnd         ║
   ║   trace acerta alvo                   ║
   ║                                        ║
   ║   Se alvo tem State.ParryWindow.Open: ║
   ║     • ParryDamageMultiplier (1.5×)    ║
   ║     • ParryReactionGE (stun + dmg)    ║
   ║     • Event.Combat.Parry.Triggered    ║
   ╠═══════════════════════════════════════╣
   ║  RECOVERY                             ║
   ║  ┌─────────────────────────────────┐  ║
   ║  │ UANS_DFCancelWindow              │  ║
   ║  │   → State.CancelWindow.Open      │  ║
   ║  │   → heavy/dodge podem chainear  │  ║
   ║  └─────────────────────────────────┘  ║
   ╚═══════════════════════════════════════╝
```

---

## 2. Setup obrigatório no Blueprint (BP_Warrior / herói)

### 2.1 AnimGraph do `ABP_Warrior` (e qualquer ABP_<classe>)

> **Sem isto, motion warping não funciona.** Os warp targets seteados pelo notify state vão ser ignorados.

1. Open `ABP_Warrior` (o AnimBlueprint principal).
2. AnimGraph → encontra o `Slot 'DefaultSlot'` (onde montages tocam).
3. **Insere o nó `Motion Warping`** entre o `Slot` e o `Output Pose`.
4. No detail panel do nó Motion Warping → não precisa configurar nada explicitamente; ele lê automaticamente o `UMotionWarpingComponent` do owner.

```
[Locomotion]
     │
     ▼
[Slot 'DefaultSlot']
     │
     ▼
[Motion Warping]  ← INSERE AQUI
     │
     ▼
[Output Pose]
```

> ⚠️ Se a montage não tem **root motion enabled**, motion warping não consegue redirecionar nada (o root está parado). Habilita `Enable Root Motion` na asset da montage E define `Root Motion Mode = Root Motion From Montages Only` no character class (já vem default no `ADFPlayerCharacter`).

### 2.2 `MeleeTrace` component (no `BP_Warrior`)

Para parry funcionar:

| Property | Valor sugerido | Notas |
|---|---|---|
| `ParryReactionGameplayEffect` | `UGE_Debuff_Stun` | GE existente; aplica stun |
| `ParrySetByCallerTag` | `Data.Duration` | SetByCaller pro stun durar 1.5s |
| `ParrySetByCallerMagnitude` | `1.5` | Segundos de stun |
| `ParryDamageMultiplier` | `1.5` | 50% dmg bônus no acerto perfeito |

### 2.3 `MeleeAim` component (já vem no `ADFPlayerCharacter`)

Pode tunar opcionalmente:

| Property | Default | Quando mudar |
|---|---|---|
| `SoftSearchDistance` | 500 cm | Aumenta pra armas longas (lança = 700) |
| `SoftSearchHalfAngle` | 45° | Maior = mais "magnético" |
| `SnapYawTolerance` | 15° | Maior = menos snap, mais "natural" |
| `TargetClassFilter` | nullptr | Setar = `ADFEnemyBase` |
| `bConsiderPlayerLockOn` | true | Sempre true no player |
| `bConsiderAIBlackboard` | true | Pode ser false no player (não tem BB) |

---

## 3. Setup obrigatório no Blueprint (BP_Enemy_<arquetipo>)

### 3.1 AnimGraph do `ABP_Enemy_<arquetipo>`

Mesmo passo da seção 2.1 — adicionar `Motion Warping` node no AnimGraph.

### 3.2 `MeleeAim` component (no `BP_Enemy_*`)

| Property | Default | Para inimigos |
|---|---|---|
| `bConsiderPlayerLockOn` | true | Mudar para **false** (inimigo não tem lock-on) |
| `bConsiderAIBlackboard` | true | Manter true |
| `TargetClassFilter` | nullptr | Setar = `ADFPlayerCharacter` |

### 3.3 `Stagger` component (no `BP_Enemy_*`)

| Property | Slime/Grunt | Brute | Boss | Notas |
|---|---|---|---|---|
| `Poise` | 30 | 80 | 200 | Dano acumulado pra staggar |
| `StaggerWindow` | 2.5s | 3.0s | 4.0s | Janela rolante |
| `StaggerCooldown` | 3.0s | 4.5s | 8.0s | Tempo entre staggers |
| `StaggerGameplayEffect` | `UGE_Debuff_Stun` | mesma | mesma | GE aplicado |
| `StaggerSetByCallerTag` | `Data.Duration` | mesmo | mesmo | |
| `StaggerSetByCallerMagnitude` | 1.0 | 1.5 | 2.5 | Segundos de stagger |
| `StaggerMontage` | `AM_Slime_Stagger` | `AM_Brute_Stagger` | `AM_Boss_Stagger` | Opcional; toca cambaleio |

---

## 4. Setup nas montages

### 4.1 Montage do player — combo light (`AM_Sword_Combo_01/02/03`)

Layout sugerido (frames aproximados para um swing de 1.2s):

```
0.00s ────── 0.15s ────── 0.55s ─── 0.85s ─── 1.20s
   │            │            │         │         │
   │ WINDUP     │  IMPACT    │ RECOVRY │         │
   │            │            │         │         │
   ▼            ▼            ▼         ▼         ▼

  [DF Melee Warp Target]
  ├──────────┤
              │
              [AN_TraceStart]
                           │
                           [AN_TraceEnd]
                                       │
                                       [DF Cancel Window]
                                       ├─────────────────┤
                                                         │
                                              [AN_ComboWindowOpen] (combo chain)
```

Properties do `DF Melee Warp Target`:
- `WarpTargetName` = `MeleeTarget`
- `DesiredStopDistance` = 150–180 (espada)
- `bRotationOnly` = **false** (queremos lunge)
- `bUpdateEveryTick` = true

### 4.2 Montage do player — heavy (`AM_Sword_Heavy`)

```
0.00s ────── 0.35s ────── 0.95s ─── 1.30s ─── 1.80s
   │            │            │         │         │
   │  WINDUP    │  IMPACT    │ RECOVRY │         │
   │ (longo)    │            │         │         │
```

- `DF Melee Warp Target`: `DesiredStopDistance=200` (heavy reach maior)
- `AN_TraceStart` / `AN_TraceEnd` no impact
- `DF Cancel Window` no recovery (permite chainear dodge cancel)

### 4.3 Montage do enemy — bite/slash (`AM_Slime_Bite`, `AM_Brute_Slam`)

```
0.00s ────── 0.40s ────── 0.75s ─── 1.20s
   │            │            │         │
   │  WINDUP    │  IMPACT    │ RECOVRY │
   │ (telegrafado)
```

Frames de notify state:

```
   ┌────────────────────────────┐
   │ [DF Enemy Telegraph]       │  → ground FX no player + weapon flash
   ├──────────────────────┤
   │   ┌──────────────────┐     │
   │   │ [DF Parry Window] │     │  → primeiros 0.2-0.4s = janela de parry
   │   ├───────┤            │
   │   │                    │     │
   │   │ [DF Melee Warp]    │     │  → bRotationOnly = TRUE no enemy
   │   ├──────────────┤     │
   │   │                    │     │
   │                  [AN_TraceStart]
   │                                [AN_TraceEnd]
```

Properties recomendadas:

**`DF Enemy Telegraph`:**
- `GroundWarningVFX` = `NS_GroundWarning_Red` (criar Niagara: anel vermelho expansivo)
- `WeaponChargeVFX` = `NS_WeaponCharge_Red` (faíscas na arma)
- `WeaponChargeSocketName` = `weapon_r` (ou socket apropriado)
- `WindupSound` = `S_Enemy_Windup_Growl`
- `bUseAttackerForwardInsteadOfTarget` = false (marca onde o player **está**)

**`DF Parry Window`:** sem properties extras — só drop nos primeiros 0.2–0.4s.

**`DF Melee Warp Target`:**
- `bRotationOnly` = **true** (enemy só roda, não desliza)
- `DesiredStopDistance` = 100–150
- `bSnapYawOnBegin` = false (motion warping cuida do giro)

---

## 5. Sistema de Combo Light + Heavy — análise

### 5.1 Estado atual — variação por arma **funciona**

| Eixo | Fonte de dados | Override | Status |
|---|---|---|---|
| Combo light (3 montages) | `FDFClassTableRow::ArmedMeleeComboMontagesFallback` | `FDFItemTableRow::WeaponMeleeComboMontages` (per-weapon) | ✅ Por arma |
| Heavy attack (1 montage) | `FDFClassTableRow::ArmedHeavyAttackMontageFallback` | `FDFItemTableRow::WeaponHeavyAttackMontage` (per-weapon) | ✅ Por arma |
| Combo unarmed | `FDFClassTableRow::UnarmedMeleeComboMontages` | — | ✅ |
| Base damage | `UDFMeleeTraceComponent::BaseDamage` | `FDFItemTableRow::WeaponMeleeBaseDamage` (per-weapon) | ✅ |
| Damage GE | `UDFMeleeTraceComponent::MeleeDamageGameplayEffect` | `FDFItemTableRow::WeaponMeleeDamageGameplayEffect` | ✅ |
| Ability class | `Ability.Warrior.MeleeSwing` | `FDFItemTableRow::WeaponMeleeGameplayAbility` (per-weapon) | ✅ |
| Anim Layer (idle / locomoção) | classe BP | `FDFItemTableRow::WeaponLinkedAnimLayerClass` | ✅ |

**Resolução (em [`UDFAbility_Warrior_MeleeSwing.cpp:74`](../../Source/DungeonForged/Private/GAS/Abilities/Warrior/UDFAbility_Warrior_MeleeSwing.cpp#L74)):**

```
ComboMontages[CurrentComboStep] → cair de volta pra AbilityMontage
```

E **`Combo.ComboMontages`** é populado a cada equip via `RefreshMeleeLoadoutFromClassAndEquipment` (em [`ADFPlayerCharacter.cpp`](../../Source/DungeonForged/Private/Characters/ADFPlayerCharacter.cpp)) — busca primeiro do weapon row, depois fallback da classe, depois unarmed.

**Heavy** (em [`UDFComboComponent::ResolveHeavyAttackMontage`](../../Source/DungeonForged/Private/Combat/UDFComboComponent.cpp#L430)):

```
HeavyAttackMontage (set por refresh) → ComboMontages[0] (fallback final)
```

### 5.2 Status atual das variações

| Variação | Status | Notas |
|---|---|---|
| **Tiered heavy** (3 tiers: tap, heavy hold, max hold) | ✅ | `UDFComboComponent::MaxHeavyChargeThreshold` (default 1.4s) — ver §5.3 |
| **Directional attacks** (forward / back / side) | ✅ | `BackwardComboMontages`, `SideComboMontages` — ver §5.4 |
| Combo branches (light → heavy finisher) | ❌ | Adicionar struct `FDFComboStep` com `LightMontage`, `HeavyBranchMontage` |
| Charge windup montage | ❌ | Tocar `ChargeWindupMontage` em loop durante hold; `ChargeReleaseMontage` no release |
| Weapon-type tags (`Weapon.Sword.1H`, `Weapon.Axe`) | ❌ | Adicionar `FGameplayTagContainer WeaponTags` a `FDFItemTableRow` |
| Stamina cost por tier | ✅ | Heavy=15, MaxHeavy=30; combo light continua sem stamina cost |
| Hit reaction por weapon kind | ❌ | `UGE_Damage_Physical` poderia incluir tag `Damage.Source.Blunt/Slash/Pierce` |

### 5.3 Tiered heavy — como configurar

**Threshold timeline (input do player):**

```
0.00s ────── 0.55s ────────── 1.40s ──────── ∞
   │            │                │
   │  TAP       │  HEAVY HOLD    │  MAX HOLD
   │            │                │
   ▼            ▼                ▼
 Light combo  Heavy attack    Max heavy
 chain        (2.2× dmg)      (3.5× dmg, 2.2× kb)
              StaminaCost=15  StaminaCost=30
```

**Tuning no combo component (defaults):**

| Property | Default | Notas |
|---|---|---|
| `HeavyChargeThreshold` | 0.55s | Tempo pra entrar em heavy tier 1 |
| `MaxHeavyChargeThreshold` | 1.4s | Tempo pra entrar em max tier (acima de Heavy) |
| `HeavyDamageMultiplier` | 2.2× | Tier 1 |
| `MaxHeavyDamageMultiplier` | 3.5× | Tier 2 |
| `HeavyKnockbackMultiplier` | 1.6× | |
| `MaxHeavyKnockbackMultiplier` | 2.2× | |
| `HeavyStaminaCost` | 15 | |
| `MaxHeavyStaminaCost` | 30 | |
| `HeavyTraceRadiusBonus` | 20 cm | |
| `MaxHeavyTraceRadiusBonus` | 35 cm | |
| `HeavyAttackMontage` | nullptr | Setado por `RefreshMeleeLoadoutFromClassAndEquipment` |
| `MaxHeavyAttackMontage` | nullptr | Setado por refresh; **null = fallback pra HeavyAttackMontage** |

**Per-weapon (`FDFItemTableRow` em `DT_Items`):**

| Field | Notas |
|---|---|
| `WeaponHeavyAttackMontage` | Override tier 1 |
| `WeaponMaxHeavyAttackMontage` | Override tier 2 (max) |

**Per-class (`FDFClassTableRow`):**

| Field | Notas |
|---|---|
| `ArmedHeavyAttackMontageFallback` | Fallback se weapon não tem heavy montage |
| `ArmedMaxHeavyAttackMontageFallback` | Fallback se weapon não tem max heavy montage |

**Comportamento se max não existir:** `CommitMaxHeavyAttack` automaticamente cai pra `CommitHeavyAttack`. Sem max montage configurado = jogador sempre faz heavy tier 1. Não há erro.

### 5.4 Directional combos — como configurar

O `UDFComboComponent` resolve a montage do passo atual via `ResolveDirectionalComboMontage(Step)`:

```
                       owner.Velocity (local)
                              │
                    ┌─────────┼─────────┐
                    │         │         │
              vel.X < -T   vel.Y > T  default
                    │         │         │
                    ▼         ▼         ▼
            BackwardCombo  SideCombo  ComboMontages
            [step]         [step]     [step]
```

Onde `T = DirectionalInputThreshold` (default 80 cm/s).

**Resolução:**
- Owner andando pra trás (vel local X < -80) → tenta `BackwardComboMontages[step]`.
- Owner strafing (vel local |Y| > 80) → tenta `SideComboMontages[step]`.
- Owner parado / andando pra frente → usa `ComboMontages[step]` (default).
- **Array directional vazio** → fallback automático pra `ComboMontages` (não há montage substituta = não atrapalha).

**Per-class (`FDFClassTableRow`):**

```cpp
UPROPERTY(...) TArray<TObjectPtr<UAnimMontage>> ArmedBackwardMeleeComboMontagesFallback;
UPROPERTY(...) TArray<TObjectPtr<UAnimMontage>> ArmedSideMeleeComboMontagesFallback;
```

Designer popula os mesmos N steps no mesmo índice (ex: 3 montages each).

**Tuning sugerido por classe:**

| Class | Forward | Backward | Side |
|---|---|---|---|
| Warrior | `AM_Sword_Combo_*` | `AM_Sword_BackThrust` | `AM_Sword_SideSlash` |
| Rogue | `AM_Dagger_Combo_*` | `AM_Dagger_BackRoll` | `AM_Dagger_Spin` |
| Mage | (sem combo melee) | — | — |

> **Per-weapon directional** não foi adicionado pra manter o escopo minimal. Designer pode override por classe (todas as armas daquela classe compartilham o pattern). Se precisar per-weapon, adicione `WeaponMeleeBackwardComboMontages` / `WeaponMeleeSideComboMontages` ao `FDFItemTableRow` + setter no `RefreshMeleeLoadoutFromClassAndEquipment` — 5 min de código.

---

## 6. Test plan

### 6.1 Smoke test (PIE)

1. **Compila** o projeto (rebuild after .uproject change).
2. **Lock-on**: aperta lock (tab por default) → ataca → o player vira pro inimigo (snap se >15°) e a swing acerta.
3. **Soft cone**: sem lock-on, com 2 inimigos no cone à frente, ataca → o mais alinhado / próximo é selecionado.
4. **Enemy ataca**: cria spawner com `BP_Enemy_Slime` → posiciona o player atrás do slime → quando ele inicia bite, vira para o player (snap + warp).
5. **Cancel window**: durante o último 30% da combo do player, segura LMB → heavy ativa cancelando o swing.
6. **Sem cancel window**: durante 0–70% da combo, segura LMB → heavy não ativa (input descartado).
7. **Dodge cancel**: a qualquer momento, dodge cancela o swing (já funcionava).
8. **Parry**: ataca o slime durante os primeiros 0.3s do windup → slime stuna + dmg boostado.
9. **Stagger**: bate seguidamente num brute (Poise=80) → após 80 de dano acumulado → stagger triggera.

### 6.2 Network test (Listen server + Client)

- **Server-only abilities** (`UDFAbility_Enemy_Melee`): aim snap acontece no server → client vê via replication.
- **LocalPredicted** (`UDFAbility_Warrior_MeleeSwing`): aim snap acontece no client primeiro, depois server confirma.
- **Loose tags**: `State.Combat.*` são adicionadas localmente em todas as máquinas (multiplicador são via GE — replicam normalmente).

---

## 7. Tuning recomendado

```ini
; Combat tuning (DataAsset UDFCombatTuningData ou em UDFComboComponent defaults)
ComboWindowDuration        = 0.45
HeavyChargeThreshold       = 0.55
HeavyDamageMultiplier      = 2.20
HeavyKnockbackMultiplier   = 1.60
HeavyStaminaCost           = 15.0
HeavyTraceRadiusBonus      = 20.0
AttackInputBufferDuration  = 0.15

; Aim (no MeleeAim defaults)
SoftSearchDistance         = 500
SoftSearchHalfAngle        = 45
SoftSearchSphereRadius     = 250
SoftSearchDistanceWeight   = 0.55
SnapYawTolerance           = 15

; Parry (no MeleeTrace do player)
ParryDamageMultiplier      = 1.5
ParrySetByCallerMagnitude  = 30        ; dmg bônus, ou duration se for Data.Duration

; Stagger por archetype
Slime    Poise=30  Cooldown=3.0  StaggerDuration=1.0
Brute    Poise=80  Cooldown=4.5  StaggerDuration=1.5
Boss     Poise=200 Cooldown=8.0  StaggerDuration=2.5
```

---

## 8. Links rápidos

- [Combat_Notify_States.md](../animation/Combat_Notify_States.md) — referência rápida dos 4 AnimNotifyStates
- [03_Combat.md](03_Combat.md) — combat improvements anterior (heavy attack, combo window, dodge cancel)
- [Player_Armed_Unarmed_Layers.md](../animation/Player_Armed_Unarmed_Layers.md) — anim layers per weapon
