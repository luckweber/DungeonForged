# Combat AAA — configuração Blueprint / Editor

> Complemento ao [`Combat_Advanced_Report.md`](../analysis/Combat_Advanced_Report.md).  
> Todo o C++ restante do relatório foi implementado; esta página lista **o que você precisa configurar no editor** para fechar o feel AAA.

---

## 1. Data Asset — `DA_CombatTuning` (Primary Asset)

Abra o asset referenciado pelo `UDFAssetManager` (`GetCombatTuningData()`).

| Campo | Ação |
|-------|------|
| **ImpactFeedbackByTag** | Preencher chaves `Impact.Light.Slash`, `Impact.Heavy.Blunt`, `Impact.Critical.Pierce`, etc. Cada entrada: `ImpactVFX` (Niagara) + `ImpactSFX` (SoundCue). |
| **TraceShapeByWeaponTag** | Mapear tags de arma → shape: ex. `Weapon.Sword` → Capsule, `Weapon.Axe` → Capsule, `Weapon.Spear` → Cone, `Weapon.Dagger` → Sphere. |
| **FinisherHealthThreshold** | 0.20 (padrão) — alinhado ao Execute. |
| **FinisherMultiHitCount** | 3 |
| **FinisherInputWindowSec** | 0.65 |
| **bEnableGlobalAbilityGCD** | false por padrão; true se quiser GCD global. |
| **GlobalAbilityGCD** | 0.35 |

---

## 2. DT_Items — armas (`FDFItemTableRow`)

Por linha de arma:

| Campo | Ação |
|-------|------|
| **WeaponTags** | Ex.: `Weapon.Sword.1H` |
| **WeaponDamageSourceTag** | Ex.: `Damage.Source.Slash` |
| **bOverrideMeleeTraceShape** | true se quiser forçar shape por item (ignora mapa do tuning). |
| **WeaponMeleeTraceShape** | Sphere / Capsule / Cone |
| **WeaponMeleeComboSteps** | Preferir steps com `ChainBlendInTime` por passo (ex. 0.12, 0.08, 0.15). -1 = usa default do componente. |
| **WeaponMeleeComboMontages** | Montages com notifies (ver §4). |

---

## 3. BP_PlayerCharacter (`ADFPlayerCharacter`)

| Componente / ref | Ação |
|------------------|------|
| **MeleeTrace** | `TraceSubStepCount` = 3; `ExtraTraceZones` opcional (ombro/ponta); `bClientPredictHitFeel` = true. |
| **Combo** | `ComboChainMontageBlendInTime` = 0.12; `AttackInputBufferDuration` vem do tuning em runtime. |
| **ScreenEffects** | Assign `ScreenEffectParentMaterial` (MID com scalars documentados no header). |
| **WeaponTrailPool** | Adicionar `UDFWeaponTrailPoolComponent` se ainda não existir; assign Niagara trail por slot de arma. |

---

## 4. Montages — notifies obrigatórios

Coloque estes **Anim Notifies** nas montages de combo / heavy / execute:

| Notify | Onde | Função |
|--------|------|--------|
| `AN_TraceStart` / `AN_TraceEnd` | Janela de hit melee | Liga trace server-side. |
| `AN_HitConfirm` | Frame exato de impacto | Sincroniza feedback com anim (B14). |
| `UANS_DFCancelWindow` | Recovery de cada swing | Permite cancel para heavy/dodge/outras abilities (tags em `AllowedCancelTags`). |
| `UANS_DFNoCancelWindow` | Wind-up / commit frames | Bloqueia cancel (C5). |
| `UANS_DFAbilityCancelWindow` | Recovery de magias | Tags ex.: `Ability.Mage.FrostBolt`, `Ability.Mage.ArcaneBarrage`. |
| `UAN_RootMotionScaleOverride` | Recovery com drag forward | `TranslationScale` 0.3–0.8 (A4). |
| `AN_SendGameplayEvent` | Execute intro | Tag `Event.Warrior.Execute.Trace` → inicia fase QTE. |

**Mínimo 5 abilities com `UANS_DFAbilityCancelWindow`** (checklist §4 do relatório): ex. FrostBolt, ArcaneBarrage, Dodge, HeavyAttack, ShieldBash.

---

## 5. Ability — Execute / Finisher (`UDFAbility_Warrior_Execute`)

No CDO da ability (ou BP filho):

| Propriedade | Ação |
|-------------|------|
| **ExecuteMontage** | Montage de abertura (grab/cinematic). |
| **FinisherHitMontages** | Array de 3 montages curtos (1 por input QTE). |
| **FinisherMultiHitCount** | 3 (ou override no tuning). |
| **FinisherInputWindowSec** | 0.65 |
| **FinisherHitDamageFractionOfMaxHP** | 0.12 por hit |
| **DeathBlowNiagara** | VFX no kill final |
| **ExecuteKillBonusEffect** | GE de recompensa (mana, etc.) |

**Fluxo in-game:** inimigo < 20% HP → tag `State.Combat.FinisherReady` + evento `Event.Combat.Finisher.Available` → jogador aperta ataque → Execute ativa → após intro, **3 inputs** de ataque dentro da janela (`Event.Combat.Finisher.Input`).

**HUD (Blueprint):** bind em `State.Combat.FinisherReady` ou `Event.Combat.Finisher.Available` para mostrar prompt "FINISHER — [Attack]".

---

## 6. Hit reaction — `UDFHitReactionComponent` (inimigos)

| Campo | Ação |
|-------|------|
| **BoneHitMontages** | Mapa `head` / `spine_03` / `pelvis` → montages de reação direcional (B4). |

---

## 7. Parry (inimigos)

| Onde | Ação |
|------|------|
| Montages de windup inimigo | `UANS_DFParryWindow` na janela vulnerável. |
| **MeleeTrace** (player) | `ParryReactionGameplayEffect` = GE de stun + bonus damage. |

Shake de parry (F4) já dispara em C++ (`PlayParrySuccessOnOwner`).

---

## 8. Combat Text widget

| Onde | Ação |
|------|------|
| **UDFCombatTextSubsystem** (GameMode / GameInstance setup) | `WidgetClass` = `W_DF_CombatText`. |
| **W_DF_CombatText** | Crit usa fonte ~1.5× (já 42 vs 28 no C++). |

Combat text de dano melee/projétil agora sai só de `DispatchOnHitConfirmed` (sem duplicar).

---

## 9. Projéteis (BP filhos)

C++ adiciona `UDFProjectileHitTrackerComponent` automaticamente nos projectiles Knife/Fireball/Frostbolt/Arcane Missile.  
BP filhos: só assign VFX/SFX; dedup H6 já está no componente.

---

## 10. Validação / playtest

Console útil:

```
-log LogDFFeel Verbose -LogDFTuning Verbose
Net PktLag=120 Net PktLagVariance=20
```

Checklist §4 do relatório:

- [ ] Hit feedback < 50ms (LogDFFeel timeline)
- [ ] Melee + projétil passam pelo mesmo `DispatchOnHitConfirmed`
- [ ] Dodge juice visível (FOV + chromatic)
- [ ] Combo refresh on-hit (+0.3s)
- [ ] Buffer 0.20s + pause em hitstop
- [ ] Cancel em 5+ abilities (montages)
- [ ] Crit visual distinto
- [ ] Net: combo sem double-activate

---

## 11. Ainda fora de escopo (editor-only / design)

| Item | Notas |
|------|-------|
| **L_CombatRange** | Level de teste (§5.2) — criar no editor com dummies + console buttons. |
| **Co-op random events (N5)** | Design de gameplay, não C++. |
| **Rollback completo de resource cost (N3)** | RPC de rejeição de montage implementado; refund visual de mana/stamina fino fica para polish net. |

---

## 12. Tags novas (referência)

- `Effect.Combat.FeedbackCentralized` — interno; evita combat text duplicado.
- `Event.Combat.Finisher.Input` — input QTE durante Execute.
- `Event.Combat.Finisher.Completed` — fim da chain; útil para HUD/camera.
