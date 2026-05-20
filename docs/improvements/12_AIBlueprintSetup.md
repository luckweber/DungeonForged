# AI System — Setup Blueprint / Editor

> Complementa [`AI_System_Deep_Dive.md`](../analysis/AI_System_Deep_Dive.md).  
> **C++:** Patches 1–8 implementados (subsistemas, BT nodes, notifies, tags).

Legenda: **✅ C++ pronto** · **⚠️ configurar no editor** · **❌ backlog manual**

---

## 1. Checklist rápido

| Patch | Item | Status | Onde |
|-------|------|--------|------|
| 1 | Token liberado no stagger | ✅ | `UDFStaggerComponent` |
| 2 | Parry → BB `bWasParried` + Recover | ⚠️ | BB + BT branch |
| 3 | Aggro em hit pesado | ✅ | `UDFHitReactionComponent` |
| 4 | Música → Exploration na sala limpa | ✅ | `NotifyRoomCleared` → `OnRoomCombatCleared` |
| 5 | Cap de telegraphs simultâneos | ⚠️ | BB `bCanTelegraph` + service no BT |
| 6 | Subtrees por archetype | ⚠️ | BT assets + decorator |
| 7 | Flee volta para Chase | ✅ | `UDFBTService_CheckHealth` |
| 8 | Boss cast interruptível | ⚠️ | Notify no montage do boss |

---

## 2. Blackboard (`BB_EnemyBase`)

Adicione as keys (nomes **exatos** — C++ usa `DFAIKeys`):

| Key | Tipo | Default | Uso |
|-----|------|---------|-----|
| `bWasParried` | Bool | `false` | Branch Recover após parry |
| `bCanTelegraph` | Bool | `true` | `TelegraphCoordinator` pode setar `false` |
| `CombatState` | Enum | Chase | Incluir valor **Recover** (ordem = enum C++) |

Enum `EADFAICombatState` em C++: Idle, Patrol, Chase, Attack, Flee, **Recover**.

---

## 3. Behavior Tree — serviços (Patch 2 + 5)

No **Sequence de combate** do `BT_EnemyBase`, adicione (ordem sugerida):

1. `UDFBTService_UpdateTarget` (já existente)
2. `UDFBTService_CheckHealth`
3. **`UDFBTService_TelegraphCoordinator`** — Radius 1200, Max Concurrent 2
4. **`UDFBTService_CombatEventListener`** — Parry Recovery 1.2s

### Branch parry (Patch 2)

```
Selector (combat root)
├ Decorator: Blackboard bWasParried == true
│   └ Sequence
│       ├ Wait 1.0s (opcional — C++ já limpa após ParryRecoveryDuration)
│       └ BTTask_MoveTo (TargetActor)  // recuo / reposicionar
├ ... ramos existentes (Attack / Chase / Flee) ...
```

O service já seta `CombatState = Recover` e limpa `bWasParried` após `ParryRecoveryDuration`.

---

## 4. Telegraph coordinator (Patch 5)

- Service: `UDFBTService_TelegraphCoordinator`
- `UDFBTTask_MeleeAttack` falha se `bCanTelegraph` estiver setado e for `false`
- Se o service **não** estiver no BT, deixe `bCanTelegraph` **unset** ou `true` (ataque normal)

**Teste PIE:** 4+ inimigos melee — no máximo ~2 telegraphs visíveis no raio de 12m.

---

## 5. Subtrees por archetype (Patch 6)

### 5.1 Decorator

- Classe: `UDFBTDecorator_IsArchetype`
- Ex.: Archetype = `Tank` → filho `BTTask_RunBehavior` → `BT_Sub_Tank`

### 5.2 Assets sugeridos (criar no Content)

| Asset | Archetype | Comportamento alvo |
|-------|-----------|-------------------|
| `BT_Sub_Tank` | Tank | Chase lento, melee prioritário |
| `BT_Sub_Caster` | Caster | Manter distância, ranged |
| `BT_Sub_Sniper` | Sniper | Flank + ranged |
| `BT_Sub_Healer` | Healer | Flee + buff allies (placeholder) |
| `BT_Sub_Default` | Grunt / outros | Comportamento atual |

### 5.3 Árvore principal (esboço)

```
Root → Selector
├ bIsDead → Die
├ Sequence (Combat)
│   ├ Services (UpdateTarget, CheckHealth, TelegraphCoordinator, CombatEventListener)
│   └ Selector
│       ├ IsArchetype Tank → RunBehavior BT_Sub_Tank
│       ├ IsArchetype Caster → RunBehavior BT_Sub_Caster
│       ├ IsArchetype Sniper → RunBehavior BT_Sub_Sniper
│       ├ IsArchetype Healer → RunBehavior BT_Sub_Healer
│       └ RunBehavior BT_Sub_Default
└ Patrol sequence
```

**Rollout:** comece com Tank + Caster; expanda depois.

---

## 6. Boss interrupt window (Patch 8)

1. No montage de cast telegrafado do boss, adicione notify state **`DF Interruptible Cast`** (`UANS_DFInterruptibleCast`) na janela de windup.
2. Shield Bash / stuns chamam `UDFCombatInterruptLibrary::TryInterruptBossCast` (já wired no Shield Bash).
3. Tags nativas: `State.Combat.Casting.Interruptible`, `Event.Combat.Boss.Interrupted`.

**Tuning:** só casts longos e telegrafados — não em ataques rápidos.

---

## 7. Playtest (§7 do relatório)

| Cenário | Esperado |
|---------|----------|
| 3 inimigos, stagger em 2 | 3º consegue token de ataque |
| Parry no windup | Inimigo entra Recover ~1.2s |
| Tank leva 40+ dmg | Aggro vai para o tank |
| Último inimigo da sala | Música volta Exploration (sem esperar 5s do tag) |
| 4 grunts melee | ≤2 telegraphs simultâneos perto do player |
| Inimigo Flee + heal | Volta Chase acima de 60% HP |
| Boss cast + Shield Bash | Montage cancela, stun 2s, tag Vulnerable |

Log útil: `-log LogDFAI Verbose`

---

## 8. Referência C++ nova

| Classe | Pasta |
|--------|-------|
| `UDFAIAwarenessSubsystem` | `AI/` |
| `UDFBTService_CombatEventListener` | `AI/` |
| `UDFBTService_TelegraphCoordinator` | `AI/` |
| `UDFBTDecorator_IsArchetype` | `AI/` |
| `UDFCombatInterruptLibrary` | `Combat/` |
| `UANS_DFInterruptibleCast` | `Combat/AN/` |
