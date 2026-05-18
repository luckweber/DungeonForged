# 03 — Combate

> **Objetivo:** dar **depth** ao combate sem refazer sistemas — usando o que já existe (GAS, combo, melee trace) e adicionando heavy attack, dodge cancel, e ajustes finos nas fórmulas.

---

## Sumário rápido

| Eixo | Mudança | Esforço |
|---|---|---|
| Combo window | 0.60s → 0.45s | 5min |
| Heavy attack | novo: charge 0.55s → 2.2× dmg, knockback 1.6× | 4h |
| Dodge cancel | recovery cancelável por dodge | 1h |
| Soft caps | Armor diminishing returns, CDR cap 0.4 | 2h |
| Crit upgrade band | tag `Effect.Critical` → upgrade hit feedback band | 1h |
| Trace por arma | `MeleeTraceRadiusOverride` em `FDFItemRow` | 2h |
| Stamina recovery | curva — recovery lento dentro de combate | 1h |
| Ability cancel | janelas de cancel-into entre abilities | 3h |

---

## 1. Combo window — `[CONFIG]` <a id="combo-window"></a>

**Onde:** [`Source/DungeonForged/Public/Combat/UDFComboComponent.h:27`](../../Source/DungeonForged/Public/Combat/UDFComboComponent.h#L27)

```cpp
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo")
float ComboWindowDuration = 0.45f;   // antes 0.6f
```

**Por quê:** 0.6s convida a mash; 0.45s força ritmo. Comparação:
- Hades = ~0.30–0.40s
- DMC5 = ~0.40s
- Dark Souls 3 = ~0.50s
- Elden Ring = ~0.55s

### Variantes por arma `[CODE]`

Adicionar em [`FDFItemRow`](../../Source/DungeonForged/Public/Data/DFDataTableStructs.h):

```cpp
USTRUCT(BlueprintType)
struct FDFItemRow : public FTableRowBase
{
    // ... existente ...
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat")
    float ComboWindowOverride = 0.f;  // 0 = usa default; > 0 = override
};
```

Em `UDFComboComponent::Refresh*` (chamado quando equipamento muda):

```cpp
const float Override = CurrentWeaponRow ? CurrentWeaponRow->ComboWindowOverride : 0.f;
EffectiveComboWindow = (Override > 0.f) ? Override : ComboWindowDuration;
```

Tuning sugerido:
- Dagger: 0.32s
- Sword (default): 0.45s
- Mace / Axe: 0.55s
- Greatsword / Claymore: 0.65s

---

## 2. Heavy Attack — `[CODE/ASSET]` <a id="heavy-attack"></a>

**Estado atual:** apenas combo de 3 light hits idênticos. Sem branching.

### 2.1 Design

| Aspecto | Light Attack | Heavy Attack |
|---|---|---|
| **Input** | LMB tap | LMB held > 0.55s (charge) ou Shift+LMB |
| **Damage** | base | base × **2.2** |
| **Trace radius** | 20cm | **40cm** |
| **Knockback** | base | base × **1.6** |
| **Stamina cost** | 0 | **15** |
| **Recovery** | 0.25s | **0.6s** |
| **Hit stop band** | Light | **Heavy** |
| **Pode interromper** | combo light | combo light **a partir do hit 1** |
| **Combo branch** | continua | finaliza combo (não tem hit 2 heavy) |

### 2.2 Implementação

Em [`UDFComboComponent`](../../Source/DungeonForged/Public/Combat/UDFComboComponent.h):

```cpp
public:
    UPROPERTY(EditAnywhere) float HeavyChargeThreshold = 0.55f;
    UPROPERTY(EditAnywhere) float HeavyDamageMultiplier = 2.2f;
    UPROPERTY(EditAnywhere) float HeavyKnockbackMultiplier = 1.6f;
    UPROPERTY(EditAnywhere) float HeavyStaminaCost = 15.f;
    UPROPERTY(EditAnywhere) float HeavyTraceRadiusBonus = 20.f;

    UPROPERTY(EditAnywhere) TObjectPtr<UAnimMontage> HeavyAttackMontage;

    void OnHeavyAttackPressed();
    void OnHeavyAttackReleased();

private:
    float HeavyChargeStart = -1.f;
    bool bHeavySwingPending = false;
```

```cpp
// .cpp
void UDFComboComponent::OnHeavyAttackPressed()
{
    HeavyChargeStart = World->GetTimeSeconds();
    // tocar VFX de charge (Niagara nos braços)
    if (UNiagaraComponent* C = UNiagaraFunctionLibrary::SpawnSystemAttached(
            ChargeNiagara, Char->GetMesh(), "hand_r", ...))
    {
        ChargeFX = C;
    }
}

void UDFComboComponent::OnHeavyAttackReleased()
{
    const float Held = World->GetTimeSeconds() - HeavyChargeStart;
    HeavyChargeStart = -1.f;
    if (ChargeFX) { ChargeFX->Deactivate(); ChargeFX = nullptr; }
    if (Held < HeavyChargeThreshold) { return; }   // fizzle se solta cedo

    // Commit
    if (!ASC->HasEnoughResources(...)) return;
    SetBaseDamageForNextSwing(BaseDamage * HeavyDamageMultiplier);
    SetBaseKnockbackForNextSwing(BaseKnockback * HeavyKnockbackMultiplier);
    bHeavySwingPending = true;
    PlayMontage(HeavyAttackMontage);
    ASC->ApplyGameplayEffectToSelf(GE_Stamina_Cost_Heavy, ...);  // consome 15 stamina
}
```

No `UDFMeleeTraceComponent::PerformTrace`, ler `bHeavySwingPending`:

```cpp
const float EffectiveRadius = bHeavySwingPending
    ? (TraceRadius + UDFCombo->HeavyTraceRadiusBonus)
    : TraceRadius;
```

E ao terminar a montage, resetar `bHeavySwingPending = false`.

### 2.3 Charge feedback visual

Durante o charge:

```cpp
// Pulse de stamina meter (HUD bordas em laranja com intensity crescente)
// Niagara nos braços (sparks orbitando)
// Camera zoom-in leve (-3 FOV)
// SFX low rumble crescente
```

Se chega ao threshold, **um "ping" visual** (flash + click sonoro) indica que está pronto. Se o jogador continua segurando após "fully charged", apenas mantém — não escala dano além.

### 2.4 Animação `[ASSET]`

Cada classe precisa do seu `HeavyAttackMontage`:
- Warrior: overhead slam (~1.0s total)
- Mage: arcane wave (~0.9s)
- Rogue: dual-blade cross slash (~0.7s)

Frame breakdown:
```
0.00 - 0.45  : windup (lento, leitura)
0.45 - 0.55  : swing (rápido — TraceStart aqui)
0.55 - 0.70  : impact (TraceEnd, hit notify, camera shake)
0.70 - 1.00  : recovery (cancelável por dodge a partir de 0.85)
```

---

## 3. Dodge Cancel — `[CODE]`

Permitir cancelar a montage de heavy/light na **recovery** apertando dodge.

### 3.1 Janela cancelável

Em cada montage, adicionar `AN_DodgeCancelWindow_Open` no início da recovery e `AN_DodgeCancelWindow_Close` no fim:

```cpp
class UAN_DodgeCancelWindow_Open : public UAnimNotifyState
{
    GENERATED_BODY()
    virtual void NotifyBegin(...) override { Combo->SetDodgeCancelable(true); }
    virtual void NotifyEnd(...) override { Combo->SetDodgeCancelable(false); }
};
```

No `GA_Dodge::CanActivateAbility`:

```cpp
if (Combo && Combo->IsAnyMontagePlaying() && !Combo->IsDodgeCancelable())
{
    return false;  // só cancela na janela
}
```

E ao ativar, `Combo->ForceStopMontage(0.05f)` antes de tocar a dodge montage.

### 3.2 Stamina extra

Dodge cancel **custa 10 stamina extra** (fica em 30 total) para não virar spam.

```cpp
if (bIsCancelingFromCombo) { StaminaCost += 10.f; }
```

---

## 4. Soft caps — `[CODE]`

### 4.1 Armor — diminishing returns

**Onde:** [`Source/DungeonForged/Private/GAS/Calculation/UDFDamageCalculation.cpp`](../../Source/DungeonForged/Private/GAS/Calculation/UDFDamageCalculation.cpp)

Fórmula atual (presumida pelo `Game_Analysis.md`):
```
final = Damage × (1 - Armor / 100)
```

Problema: Armor 100 = 0 dano. Tank invencível.

**Sugestão (WoW-style):**
```cpp
const float Level = SourceLevel.GetValueOrDefault(1.f);
const float K = 400.f + Level * 85.f;  // ~485 no level 1, ~3000 no 30
const float Mitigation = Armor / (Armor + K);
final *= (1.f - Mitigation);
```

Resultado:
- Armor 100, Level 1 → mitigation 17%
- Armor 200, Level 1 → 29%
- Armor 500, Level 1 → 51%
- Armor 1000, Level 1 → 67% (cap soft)

Nunca chega a 100%. Builds tank ainda fortes mas não invencíveis.

### 4.2 Cooldown Reduction — cap em 0.4

**Onde:** mesma calc.

```cpp
const float CDRRaw = Caster->GetCooldownReduction();
const float CDRCapped = FMath::Min(CDRRaw, 0.4f);   // [CONFIG] cap em 40%
const float EffectiveCooldown = BaseCooldown * (1.f - CDRCapped);
```

Acima de 0.4, **diminishing returns**:
```cpp
const float Excess = FMath::Max(0.f, CDRRaw - 0.4f);
const float ExtraReduction = Excess / (Excess + 0.6f) * 0.1f;  // assintótico para 0.5 cap
const float TotalCDR = 0.4f + ExtraReduction;
```

50% é o ceiling absoluto.

### 4.3 Crit chance — diminishing returns acima de 50%

```cpp
const float CritRaw = Caster->GetCritChance();
const float CritEff = (CritRaw <= 0.5f) ? CritRaw
                                          : 0.5f + (CritRaw - 0.5f) * 0.6f;  // 60% efetivo
```

Crit 100% (raw) → 80% efetivo. Mantém builds crit fortes mas não auto-crit-everything.

---

## 5. Fórmula de dano consolidada — `[CODE]`

Em [`UDFDamageCalculation::Execute_Implementation`](../../Source/DungeonForged/Private/GAS/Calculation/UDFDamageCalculation.cpp):

```cpp
// 1. Base
const float Base = SetByCallerDamage;                 // do GE spec

// 2. Attribute scaling
const float StatBonus = bUsesStrength      ? STR * 0.8f
                       : bUsesIntelligence ? INT * 1.0f * (1.f + SpellDamageAmp)
                                            : AGI * 0.6f;
const float Raw = Base + StatBonus;

// 3. Crit roll
const float CritEffective = ApplyCritDR(CritChance);
const bool bIsCrit = FMath::FRand() < CritEffective;
const float CritMul = bIsCrit ? CritMultiplier : 1.f;
const float Postcrit = Raw * CritMul;

// 4. Mitigation
const float K = 400.f + TargetLevel * 85.f;
const float Mitigation = (DamageType == Physical) ? TargetArmor / (TargetArmor + K)
                                                  : TargetMagicResist / (TargetMagicResist + K);
const float Mitigated = Postcrit * (1.f - Mitigation);

// 5. Elemental reaction multiplier
const float ReactionMul = ResolveElementalReaction(SourceTags, TargetTags);   // 1.0 / 1.5 / 2.0
const float Final = Mitigated * ReactionMul;

// 6. Apply
OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
    HealthAttribute, EGameplayModOp::Additive, -Final));

// 7. Tag for hit feedback
if (bIsCrit) Context->AddTag(FDFGameplayTags::Effect_Critical);
if (Final >= 2.0f * Raw) Context->AddTag(FDFGameplayTags::Effect_Massive);
```

**Output extras** que o `UDFHitReactionComponent` pode ler:
- `Effect.Critical` (1.0 multiplier) → hit feedback band Critical
- `Effect.Massive` (≥ 2× raw) → forçar Knockback montage no target
- `Effect.Reaction.Melt/Steam/Electrocute` → tag adicional

---

## 6. Ability cancel windows — `[CODE]`

Atualmente abilities tocam até o fim e bloqueiam outros inputs. Sugestão: **window de "early cancel into" para outras abilities** (não combo light).

### 6.1 Tag-based

Em cada AbilityMontage, adicionar `AN_AbilityCancelWindow_Open(CancelTags)`:

```cpp
class UAN_AbilityCancelWindow_Open : public UAnimNotifyState
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere) FGameplayTagContainer AllowedCancelTags;
    virtual void NotifyBegin(...) override
    {
        Combo->SetAbilityCancelWindow(AllowedCancelTags);
    }
    virtual void NotifyEnd(...) override { Combo->ClearAbilityCancelWindow(); }
};
```

No `GA_Ability::CanActivateAbility`:

```cpp
if (ASC->HasAnyMatchingGameplayTags(ActivationBlockedTags))
{
    // Permitir se a tag bloqueadora é uma "ability cancel" window aberta
    if (!Combo->IsAbilityCancellable(this->AbilityTags)) return false;
}
```

### 6.2 Exemplo prático

- Warrior `ShieldBash` (recovery 0.4s): janela cancel para `Charge` ou `Execute` (combos).
- Mage `FrostBolt`: janela cancel para `ArcaneBarrage` (chain cast).
- Rogue `Backstab`: janela cancel para `Eviscerate` (se tiver combo points).

Cria **combos de habilidades** em vez de wait-for-recovery.

---

## 7. Stamina recovery curve — `[CODE]`

**Onde:** GE de stamina regen (provavelmente `GE_StaminaRegen`).

### 7.1 Combat vs out-of-combat

```cpp
// UDFAttributeSet::PostGameplayEffectExecute, ou em GE custom calc:
const bool bInCombat = ASC->HasMatchingGameplayTag(FDFGameplayTags::State_InCombat);
const float RegenRate = bInCombat ? 8.f : 25.f;  // [CONFIG] /s
SetStamina(FMath::Min(GetMaxStamina(), GetStamina() + RegenRate * DeltaTime));
```

**`State.InCombat`** já existe? Confirmar e que é setada quando:
- Player ataca/é atacado → adiciona `State.InCombat`
- Sem ações por 4s → remove `State.InCombat`

Sem isso, regen é igual sempre = dodge spam viável.

### 7.2 Stamina-locked window

Se `Stamina < 10`, o player **não pode sprintar nem dodgar por 0.5s** (exausto):

```cpp
if (Stamina < 10.f && !bExhaustedActive)
{
    ASC->AddLooseGameplayTag(FDFGameplayTags::State_Exhausted, 1);
    World->GetTimerManager().SetTimer(ExhaustHandle, [this]() {
        ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Exhausted, 1);
    }, 0.5f, false);
}
```

`State.Exhausted` bloqueia `GA_Sprint`, `GA_Dodge`. Visual: HUD stamina bar pisca vermelho.

---

## 8. Combo points (Rogue) — refinement `[CODE]`

`UDFComboPointsComponent` existe. Sugestões de polish:

### 8.1 Visual em 5 pips

HUD widget pequeno acima da stamina bar:
```
○ ○ ○ ○ ○     ← 0 points
● ○ ○ ○ ○     ← 1
● ● ● ○ ○     ← 3
● ● ● ● ●     ← 5 (golden glow + sparkle)
```

### 8.2 Decay timer

Combo points decaem em **8s sem ação ofensiva**:

```cpp
UPROPERTY(EditAnywhere) float ComboPointDecayTime = 8.f;
// Em OnHitDealt: timer reset
// Em timer expire: ComboPoints = 0, "stale" SFX + pip empty animation
```

Cria pressão: gastar antes que perca.

### 8.3 Eviscerate damage scaling

Se ainda for fixo, escalar:
```
1 point  → 1.0× damage
2 points → 1.4×
3 points → 1.8×
4 points → 2.3×
5 points → 2.9× + crit garantido + Heavy hit feedback band
```

---

## 9. Status effects / debuffs — `[CODE/CONFIG]`

### 9.1 Standardizar duração e tier

Sugestão de baseline (cada um em GE separado, mas tunings consistentes):

| Status | Duração | Dano/s | Stacks max | Tag |
|---|---|---|---|---|
| **Burn** | 5s | 4 (× INT/4) | 5 | `Effect.Status.Burn` |
| **Bleed** | 6s | 3 (× AGI/4) | 5 | `Effect.Status.Bleed` |
| **Poison** | 8s | 2 (× AGI/4) | 8 | `Effect.Status.Poison` |
| **Freeze** | 1.5s | (slow 80%) | 1 | `Effect.Status.Freeze` |
| **Stun** | 1.0-1.5s | (sem ações) | 1 | `Effect.Status.Stunned` |
| **Slow** | 3s | (slow 40%) | 1 | `Effect.Status.Slowed` |
| **Vulnerable** | 4s | (+25% dmg taken) | 1 | `State.Vulnerable` |
| **Berserk** (buff) | 8s | (+30% dmg, -20% defense) | 1 | `State.Berserk` |

### 9.2 Reações elementais (já existem)

Fórmula sugerida para `UDFElementalLibrary::ResolveReaction`:

```
Fire   + Ice    = Melt        (×1.5 damage, removes Freeze, brief Wet)
Ice    + Lightning = Electrocute (×1.8, stuns 0.5s)
Fire   + Lightning = Overload (×2.0, AOE burst on target)
Water  + Lightning = ChainLightning (jumps to 2 nearby enemies)
Water  + Fire     = Steam      (× damage 1.3, AoE smaller decals)
Arcane + Any      = Amplify   (×1.4 of the other element)
```

Cada reação dispara `UDFCombatTextSubsystem` com `Type::Status` + label colorido + Niagara específico.

---

## 10. Test scene — `L_CombatRange` `[BP]`

Para iterar combate rapidamente, criar um nível dedicado:

```
L_CombatRange/
  ├─ Player Start
  ├─ TrainingDummy × 3 (HP 99999, on-screen damage taken counter)
  ├─ EliteDummy × 1 (HP 99999, mostra resistência por tipo)
  ├─ BossDummy × 1 (com phase transition stub)
  ├─ ButtonPanel:
  │   • "Spawn 5 grunts"
  │   • "Spawn 1 elite"
  │   • "Toggle infinite stamina/mana"
  │   • "Set floor: 1 / 5 / 10"
  │   • "Give all abilities"
  │   • "Heal full"
  ├─ FPS/GPU widget canto superior
  └─ Console: stat unit, showdebug abilitysystem
```

Acessível via `open L_CombatRange` no console. **Iterar feel em 1 minuto** em vez de uma run completa.

---

## 11. Checklist de "pronto"

- [ ] Combo window 0.45 default + override por arma testado.
- [ ] Heavy attack implementado com charge visual + hit stop Heavy + animations das 3 classes.
- [ ] Dodge cancel funciona na recovery window (custa stamina extra).
- [ ] Armor segue curva DR (testar: Armor 1000 = 67% mitigation, não 100%).
- [ ] CDR cap 0.4 + soft cap para 0.5.
- [ ] Crit chance > 50% sofre DR (50% raw = 50% eff; 100% raw = 80% eff).
- [ ] Ability cancel windows configuradas em 5+ habilidades.
- [ ] Stamina regen diferencia combat (8/s) vs out-of-combat (25/s).
- [ ] Combo points têm decay 8s e pip widget visual.
- [ ] Status effects padronizados em 8 tipos com durações tabeladas.
- [ ] `L_CombatRange` criado para iterar feel.

---

## Apêndice — Builds esperados (sanity check)

Após as mudanças, três builds devem ser claramente distintos no andar 10:

| Build | Stats foco | DPS | Survivability | Estilo |
|---|---|---|---|---|
| **Warrior Tank** | STR 30, Armor 800 | 350/s | 70% mit + heal | Stand-and-deliver, heavy attacks |
| **Mage Burst** | INT 30, SpellAmp 60% | 600/s burst | 30% mit + shield | Cooldown management, kite |
| **Rogue Crit** | AGI 30, Crit 60% | 500/s sustained | 40% mit + dodge | Combo points + execute |

Se um build domina trivialmente, **os números acima precisam re-tuning**. Caso prático: se Rogue mata todos os bosses em < 30s, baixar Crit DR para começar em 40% raw em vez de 50%.
