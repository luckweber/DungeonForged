# 02 — Combate & GAS

> Parte da [AAA Technical Review (Maio 2026)](00_Index.md).
> Cobre: AttributeSet, pipeline de dano, abilities, GameplayEffects, elemental,
> cues/tags, combo, melee trace, hit reaction, feel/juice, projéteis, lock-on.

---

## 1. GAS — núcleo

### 1.1 AttributeSet (`UDFAttributeSet`)

**21 atributos** (sem meta-attribute), em categorias claras: vitais
(Health/Mana/Stamina), progressão (CharacterLevel), primários (STR/INT/AGI),
mitigação (Armor/MagicResist), secundários (Crit/CDR/SpellAmp/StatusResist/
Lifesteal/SpellVamp/Dodge/Block), movimento (SpeedMult/SprintDrain).

**Forças**
- Clamps coerentes em `PreAttributeChange` com **hard caps** sensatos:
  CritChance 0.75, CooldownReduction 0.4, StatusResist 0.85, Dodge/Block 0.75
  (`UDFAttributeSet.cpp:224-248`).
- **Second Wind** intercepta dano letal na authority → HP a 25% + shield GE
  (`UDFAttributeSet.cpp:181-188`, `459-498`). Mecânica "alta dopamina".
- `PostGameplayEffectExecute` rico: lifesteal/spellvamp, style rating, passives,
  combat text, mana shield (`296-407`).

**Gaps vs AAA**
| Gap | Detalhe | Ref |
|---|---|---|
| **Sem meta-attribute `IncomingDamage`** | Dano vai direto na Health; dificulta shields/absorção/log/pre-mitigação (padrão Lyra) | `UDFAttributeSet.cpp` |
| **Armor/MR sem softcap no atributo** | Só na execution; atributo é ilimitado → mismatch tooltip vs real | `PreAttributeChange:218-222` |
| **Mana shield em PostExecute** | reescreve health após GE; race com outros listeners; deveria ser atributo de absorção | `358-380` |
| **OnRep de Mana não broadcast** | `OnHealthChanged` dispara, Mana não → UI pode não atualizar | `516-522` |
| **Lifesteal via `ApplyModToAttribute` em PostExecute** | pode recursar/disparar PostExecute extra | `296-407` |

### 1.2 Pipeline de dano

`SetByCaller Data.Damage → UGE_Damage_{Physical|Magic|True} → UDFDamageCalculation
| UDFTrueDamageExecution → modifier na Health → PostGameplayEffectExecute`.

Fluxo do `UDFDamageCalculation` (`DFDamageCalculation.cpp`): Invulnerable early-out
→ Dodge (FRand) → base + STR/INT×0.5 → mitigação → Block → Boss vulnerable ×1.5
→ Crit (FRand, escreve `Data.CriticalHit`).

**Mitigação:** Físico `Armor/(Armor+100)` cap 85%; Mágico `(1+SpellAmp)×(1−MR/100)`.

**Gaps vs AAA**
| Gap | Detalhe | Ref |
|---|---|---|
| 🔴 **Elemental nunca aplicado** | matriz/resist do `UDFElementalReactionSubsystem` não é lida na calc — sistema inteiro dormente (§1.5) | — |
| **Dodge/crit com `FRand()` não-seedado** | sem replicação do roll → desync sob prediction | `:129` |
| **`Effect.Critical` nunca setado** | tag existe mas só `Data.CriticalHit` SetByCaller é usado (split confuso) | `DFGameplayTags.cpp:437` |
| **Armor (curva) ≠ MR (linear %)** | inconsistência de balanceamento | `:100-106` |
| **True damage ignora invuln** | pode ser intencional — documentar/gate | `UDFTrueDamageExecution.cpp` |

### 1.3 Abilities (~41 classes)

Organização por fantasia: Warrior(8)/Mage(6)/Rogue(6)/Boss(5)/Equipment(3)/
Passive(6) + movimento(3) + enemy/death(4). Base `UDFGameplayAbility`:
`InstancedPerActor`, custo Mana/Stamina, cooldown via GE + CDR + GCD opcional,
montage pós-`CommitAbility`, integração com combo-cancel, gate `bSourceObjectMustBeBoss`.

**Net policy:** maioria player = `LocalPredicted`; boss + passives = `ServerOnly`;
death/enemy melee = `ServerInitiated`.

**Gaps vs AAA**
| Gap | Detalhe | Ref |
|---|---|---|
| **Custos burlam cost GE** | `UGE_Cost_*` existe, mas abilities setam Mana/Stamina direto → prediction/immunity mais fraca | `UDFGameplayAbility.cpp:305-323` |
| **Sem `NetExecutionPolicy` default** | fácil esquecer numa ability nova | base class |
| 🟡 **Universal kit é só tag** | HealthPotion, SecondWind, BattleHymn, Siphon, Berserk, CallLightning anunciadas em tags, sem classe GA | `DFGameplayTags.h:87-92` |
| **Regen passives sem GA** | só GE (`UGE_HealthRegen/ManaRegen`), sem `UDFAbility_Passive` que conceda | — |

### 1.4 GameplayEffects (~52) & componentes

Base `UDFGameplayEffect` configura componentes em `PostInitProperties`. Stacking
definido em DoTs (Fire×3, Poison×12, Frost×5) e ArmorBreak×3; SetByCaller
consistente. Só **2** GE components custom: `StatusResistDuration` (escala CC por
StatusResist) e `CancelAbilitiesOnApply`.

**Gaps:** stacking não-uniforme (muitos buffs sem policy explícita); sem registry
data-driven de GE (classe-por-efeito escala mal); sem componente "remove-on-damage"
para shields.

### 1.5 🔴 Elemental — o maior dead code

`UDFElementalComponent` (afinidade por DT), `UDFElementalLibrary` (matriz 5×5
Fire/Ice/Water/Lightning/Earth) e `UDFElementalReactionSubsystem` (Melt/Steam/
Electrocute) estão **bem desenhados**, mas:

- **`OnElementalHit`/`ApplyElementalDamage` não têm caller no combate.** Inimigos
  fazem `InitFromTable` (`ADFEnemyBase.cpp:550-558`) mas nenhum path de dano
  invoca escala/reação elemental.
- `Effect.Element.*` em specs não é lido pelo `UDFDamageCalculation`.
- GEs de reação são editor-assigned → provavelmente null em shipping.

**Recomendação (item #1 do Top 15):** injetar a chamada elemental **antes** de
`ApplyGameplayEffectSpecToTarget` no melee trace / abilities, lendo a tag
`Effect.Element.*` do spec e aplicando multiplicador + reação. É a melhoria de
maior ROI do projeto: ativa um sistema inteiro já construído.

### 1.6 GameplayCues & Tags

- **Cues:** apenas **1** cue nativo (`GameplayCue.Enemy.Death`). Falta hit/block/
  crit/reação/buff. Taxonomia `Impact.*` existe sem mapping para cue.
- **Tags:** ~200 nativas com comentário, hierarquia forte em `State.*`/`Event.*`/
  `Effect.*`. Problemas: árvores duplicadas (`Ability.Fire.*` legacy vs
  `Ability.Mage.*`), `Ability.Slot.1-4` não-wirado, muitas tags sem consumidor C++.

---

## 2. Combate melee

### 2.1 Combo (`UDFComboComponent`)

**Forças (maduro):** resolução de step com prioridade lock/pending/current
(anti-race de montage-end, `:45-56`); `FDFComboStep` com light/heavy branch,
blend in/out, launcher; janelas notify+timer com extensão por hit; **dois buffers**
(swing vs combo); heavy/charge em dois tiers; combos direcionais (fwd/back/side) +
aerial; cancel tag-filtrado; soft-ref montages com cache; replicação owner-only de
step + reconciliação em `OnRep`.

**Gaps vs AAA (DMC/GoW/Hades)**
| Gap | Detalhe | Ref |
|---|---|---|
| **Profundidade baixa out-of-box** | `MaxComboSteps=3`, sem árvores de branching (launcher→juggle→finisher) | `:28-29` |
| **Direcional 3-way, não 8-way** | sem diagonais nem ataque relativo ao lock-on | `:1900-1919` |
| **Aerial é tabela separada, não estado de juggle** | sem hit-count aéreo, bounce, spike finisher | `:351-361` |
| **Prediction parcial** | `bComboWindowActive`/buffers/montage não replicam; rollback só snapa step | `.h:35-39` |
| **`PickComboVariant` com `FRand()` no cliente** | multiplayer pode escolher montage diferente | `:440-449` |
| **Dois code paths** | path legacy montage-only coexiste com GAS | `:1378-1415` |

### 2.2 Melee trace (`UDFMeleeTraceComponent`)

**Forças:** shapes sphere/capsule/cone; sub-stepping (1–8) anti-tunneling;
multi-hit prevention (`HitActorsThisSwing`); server-only authoritative;
finishing blow por HP%; hook de parry; agendamento de trace por parse de montage
(workaround p/ notify não-confiável em LocalPredicted); preview cliente cosmético.

**Gaps:** sem hit prediction real (cliente só cosmético); overlap fallback
"magnético" demais; knockback keyed em `DamageAmount` e não em força; finisher
só HP% (sem backstab/stagger/aéreo); cone aproximado por 3 spheres.

### 2.3 Hit reaction (`UDFHitReactionComponent`)

**Forças:** reações direcionais 4-way + override por AnimBP; overrides por
damage-source e por bone; stagger via GE acima de threshold; impulso de knockback;
retarget de aggro em hit pesado; RPC de feedback ao victim; soft-ref async.

**Gaps:** knockback por threshold de dano (não usa `KnockbackMagnitude`);
sem camadas de hit-stun/super-armor (flinch vs poise-break vs launch);
reações de inimigo só na authority (sem lead-in local → late em latência);
**duplicação stagger** entre `StaggerThreshold` (hit reaction) e `UDFStaggerComponent`
(poise).

---

## 3. Feel / Juice — destaque do projeto

Pipeline centralizado em `UDFCombatFeedbackLibrary::DispatchOnHitConfirmed`:
hit reaction → combo refresh → feel do attacker/victim → combat text → VFX/SFX.

| Sistema | Força | Gap |
|---|---|---|
| **Hit stop** | bands (Light/Heavy/Crit/Knockback), wall-clock, exclusão de attacker, escala de acessibilidade | só dilatação **global**; victim não recebe rate local |
| **Impact framing** | freeze de rate por-attacker independente do hit-stop global | `bRateActive` ignora hits subsequentes durante freeze → multi-hit perde framing |
| **Style rating** | ladder D→SSS, penalidade repeat, hooks parry/dodge | leve: sem score por variedade/aéreo, sem display de multiplicador |
| **Stamina exhaustion** | tag `State.Exhausted` automática + clear por threshold | binário; sem slow/deflect estilo Souls |
| **Launcher** | launch por combo-step + gravity scale de hangtime | sem cap de juggle / ground bounce |
| **Screen FX** | rica (damage/heal/berserk/death/lowHP/teleport/dodge/kill) | `SetBlurAmount` declarado e **nunca implementado**; vignette writers competem na mesma tick |

---

## 4. Lock-on (`UDFLockOnComponent`)

**Forças:** aquisição por sphere+cone+LOS+filtro de classe (nearest first);
maintain dropa o cone (dodge-friendly); grace break; cycle Q/E; integração de
câmera (arm length, slerp aim com fix de Z aéreo); strafe mode; tag `State.Targeting`;
tuning via `UDFCombatTuningData`.

**Gaps:** local-only (não replicado p/ co-op assist); sempre nearest-in-cone (sem
scoring por câmera/ameaça/elevação como Souls/Zelda); sem hard/soft lock; sem
re-target ao acertar; cycle em lista vazia faz `ReleaseLockOn` (duro vs Souls).

---

## 5. Projéteis (knife / fireball / frostbolt)

**Forças:** GAS damage na authority, multi-hit prevention via
`UDFProjectileHitTrackerComponent`, feedback unificado com melee.

**Gaps:** colisão só `OnComponentHit` (sem sweep/substep → tunneling em alvos
rápidos); knife filtra `Cast<ADFEnemyBase>` (sem dummy/PvP); **sem pooling**
(spawn/destroy por tiro → GC spike em horda); fireball dispara feedback com
SetByCaller cru (não pós-mitigação → band pode divergir do HP real).

---

## 6. Recomendações priorizadas — Combate & GAS

| # | Recomendação | Tag | Esforço |
|---|---|---|---|
| 1 | **Wirar elemental no pipeline de dano** (ler `Effect.Element.*`, aplicar mult+reação antes do apply) | 🔴 | M |
| 2 | **Meta-attribute `IncomingDamage`** + execution única → habilita shields/absorção/log | 🔴 | M |
| 3 | **Seed/replicar dodge & crit**; setar `Effect.Critical` no spec | 🟡 | M |
| 4 | **Unificar poise/stagger/juggle** num modelo só (hit reaction + `UDFStaggerComponent` + launcher com cap) | 🟡 | M |
| 5 | **Server resolve `PickComboVariant`** + replicar step/janela p/ co-op determinístico | 🟡 | M |
| 6 | **Implementar Universal abilities** (ou remover tags) + GA para regen passives | 🟡 | M |
| 7 | **Expandir cue registry** (hit/crit/block/reação/buff) mapeando `Impact.*` | 🟡 | M |
| 8 | **Combo depth**: branching light/heavy + 8-way + estado de juggle aéreo | 🟡 | H |
| 9 | **Custos via cost GE** (prediction parity) | 🟢 | M |
| 10 | **Hit-stop/rate no victim** (clareza DMC) + framing que aceita multi-hit | 🟢 | M |
| 11 | **Lock-on scoring** (câmera/ameaça) + soft aim + re-target ao acertar | 🟢 | M |
| 12 | **Pooling de projéteis** + colisão por sweep/substep | 🟢 | M |
| 13 | Implementar/ remover `SetBlurAmount`; resolver competição de vignette | 🟢 | L |

> Arquivos-chave: `Source/DungeonForged/Public/GAS/UDFAttributeSet.h`,
> `Private/GAS/DFDamageCalculation.cpp`, `Public/GAS/UDFGameplayAbility.h`,
> `Private/GAS/Elemental/UDFElementalReactionSubsystem.cpp`,
> `Public/Combat/UDFComboComponent.h`, `UDFMeleeTraceComponent.h`,
> `UDFHitReactionComponent.h`, `Public/FX/UDFCombatFeedbackLibrary.h`,
> `Public/Camera/UDFLockOnComponent.h`.
