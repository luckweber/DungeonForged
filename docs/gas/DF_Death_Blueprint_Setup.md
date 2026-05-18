# DungeonForged — Configurar morte no Blueprint (Player + Inimigos)

Guia para configurar animação de morte, loot e VFX usando o fluxo GAS (`UUDFAbility_Death` e subclasses).  
Classes C++: `UUDFAbility_Death`, `UUDFAbility_Enemy_Death`, `UUDFAbility_Player_Death`.

**Referência de projeto:** `C:\Users\Thiago\Documents\Unreal Projects\ElderLore` — `UElderGA_Death`, `Event.Death`, `AElderCharacterBase::HandleDeath`.

---

## Comparação ElderLore ↔ DungeonForged

| ElderLore | DungeonForged |
|-----------|-----------------|
| Uma `UElderGA_Death` (player + enemy) | `UUDFAbility_Enemy_Death` + `UUDFAbility_Player_Death` (subclasses de `UUDFAbility_Death`) |
| `Die()` → `HandleDeath()` → `Event.Death` | `OnHealthOrMaxChanged` → `HandleServerDeath` → `Event.Death` |
| `bDeathSequenceCommitted` | `bHasDied` (replicado) |
| `PlayMontageAndWait` + freeze mesh tick | Igual (após alinhar com ElderLore) |
| `OnBlendOut` → mesmo handler que `OnCompleted` | Igual |
| Destroy inimigo: delay na GA (`EnemyDestroyDelay`) | Igual em `UUDFAbility_Enemy_Death` |
| Montage no BP do personagem | `DeathMontage` no `BP_DeepSeaLizard` / player BP |

Não precisas de Blueprint na ability se usares as classes C++ e `Death Montage` no pawn.

---

## Visão geral

| Quem | Ability (auto-grant) | Montage | Fim da morte |
|------|----------------------|---------|--------------|
| **Inimigo** | `UUDFAbility_Enemy_Death` | `DeathMontage` no BP do inimigo | `Destroy()` após o montage |
| **Player** | `UUDFAbility_Player_Death` | `DeathMontage` no BP do player | Freeze no último frame ou ragdoll |

Fluxo automático (não precisa ligar Blueprint em “On Death”):

1. Vida chega a 0 → C++ chama `TryActivateDeathAbility()`.
2. A death ability aplica `State.Dead` (via Gameplay Effect).
3. Toca o montage (`PlayMontageAndWait` no player; inimigo usa bypass do ABP).
4. **Inimigo:** opcionalmente `Event.Death.Loot` no montage → `SpawnDeathLoot`.
5. **Inimigo:** destroy no fim; **Player:** `FinalizeDeathPresentation` (pose ou ragdoll).

Se a ability não ativar, existe **fallback** (montage manual + timer de destroy no inimigo).

---

## Tags usadas

| Tag | Uso |
|-----|-----|
| `Ability.Death` | Pai (filtro geral) |
| `Ability.Death.Enemy` | Ability de morte do inimigo |
| `Ability.Death.Player` | Ability de morte do player |
| `State.Dead` | Bloqueia outras abilities / ABP |
| `Event.Death.Loot` | Notify no montage → spawn de loot (inimigo) |
| `GameplayCue.Enemy.Death` | VFX/SFX opcional (não toca montage) |

---

## 1. Inimigo — Blueprint do personagem

Abra o BP filho de `ADFEnemyBase` (ex.: `BP_DeepSeaLizard`).

### 1.1 Class Defaults — GAS | Enemy

| Propriedade | O que fazer |
|-------------|-------------|
| **Death Montage** | Arraste o `AnimMontage` de morte (ex. `A_DeepSeaLizard_Death_Montage`). **Obrigatório** para ver animação. |
| **Death Gameplay Effect Class** | Deixe `GE_EnemyDeath` (default) salvo em recompilar C++. |
| **Death Ability Class** | Use **`UDFAbility_Enemy_Death`** (C++). Se criar `GA_*_Death` em Blueprint, o parent **tem de ser** `UDFAbility_Enemy_Death` e **não** overrides `ActivateAbility` sem chamar o parent — senão o inimigo fica com HP 0 mas não morre/destrói. |

### 1.1b Blackboard + Behavior Tree

No asset **`BB_EnemyBlackboard`**, a chave bool **tem de se chamar exatamente** `bIsDead` (não `bisDead` nem outro nome).

No **`BT_EnemyBase`**, o ramo superior do Selector deve ser:

- Decorator: `Blackboard Key Query` → `bIsDead` **Is Set** → Task `UDFBTTask_Die` (ou Wait)

Sem isto, o AI continua em `Chase` com HP 0.

### 1.2 Class Defaults — UI (opcional)

| Propriedade | O que fazer |
|-------------|-------------|
| **Health Bar Widget Class** | Widget da barra de HP (ex. `WBP_EnemyHealthBar`). Se vazio, não há barra 3D. |

### 1.3 Data Table (`DT_Enemies`)

Na linha do inimigo, confira:

- **Loot Table Rows** — usado em `SpawnDeathLoot` (servidor).
- **Experience Reward / Gold** — XP e ouro no kill (C++ em `HandleServerDeath`).

A death ability é **concedida automaticamente** em `BeginPlay` (`GrantDeathAbility`). Não é preciso mapear tag na tabela para a ability de morte.

### 1.4 Override de loot (opcional)

No BP do inimigo: **Event Graph → Override → Spawn Death Loot** se precisar de lógica extra além da tabela.

O default em C++ só roda no **servidor** e usa `CachedLootTableRowNames` da linha do DT.

---

## 2. Inimigo — Montage de morte

Abra o **AnimMontage** atribuído em `Death Montage`.

### 2.1 Skeleton

O skeleton do montage deve ser o **mesmo** do mesh do inimigo (ex. `DeepSeaLizard_Skeleton`).

### 2.2 Notify de loot (recomendado)

No frame em que o corpo deve soltar loot:

1. Clique na timeline do montage → **Add Notify**.
2. Escolha **`DF Send Gameplay Event`** (`UAN_SendGameplayEvent`).
3. **Event Tag** = `Event.Death.Loot`.
4. **Event Magnitude** = `0` (não usado para loot).

Sem este notify, o loot spawna **no fim do montage** (fallback).

### 2.3 Slot do montage

O C++ usa bypass de ABP (`AnimationSingleNode`) no inimigo — o slot do montage importa menos que antes, mas mantenha um slot coerente (ex. `DefaultGroup`) para preview no editor.

### 2.4 VFX de morte (opcional)

`GE_EnemyDeath` dispara **`GameplayCue.Enemy.Death`**. Configure um asset de cue no projeto se quiser partículas/som; o cue **não** reproduz o montage (isso é da ability).

---

## 3. Player — Blueprint do personagem

Abra o BP filho de `ADFPlayerCharacter` (classe do run / personagem).

### 3.1 Class Defaults — Combat | Death

| Propriedade | O que fazer |
|-------------|-------------|
| **Death Montage** | Montage de morte do player. |
| **Use Ragdoll On Death** | `false` = congela no último frame do montage; `true` = ragdoll no fim. |

A ability `UUDFAbility_Player_Death` é concedida em **`InitializeGAS`** (servidor). Não precisa input binding.

### 3.2 Montage do player

- Mesmo skeleton do mesh do player.
- Pode usar notifies `DF Send Gameplay Event` para outros efeitos no futuro; a ability de morte do player **não** escuta `Event.Death.Loot` por default.
- O montage é tocado via **ASC** (`PlayMontageAndWait`), então o **ABP do player** deve permitir morte com `State.Dead` / slot de montage (ver [ABP authoring](../animation/Player_Armed_Unarmed_Layers.md) e `Source/DungeonForged/Public/Animation/ABP_DungeonForged_Authoring.md`).

### 3.3 Morte e UI

A tela de derrota e música continuam no fluxo do run (`HandlePlayerOutOfHealth` → GAS). Só a **apresentação** (montage + pose/ragdoll) passou pela death ability.

---

## 4. Animation Blueprint

### Inimigo

Com `UUDFAbility_Enemy_Death`, o mesh entra em **`AnimationSingleNode`** durante a morte — o ABP de locomotion **não precisa** de um estado Dead perfeito para a animação aparecer.

Ainda é boa prática ter transição com **`bIsDead`** (`UUDFAnimInstance` lê `HasDied()` no inimigo) para parar IK/locomotion antes do kill, se o ABP herdar `UUDFAnimInstance`.

### Player

Recomendado: estado **Dead** no ABP quando `bIsDead` ou montage de morte ativo, para não voltar a idle no fim do blend.

---

## 5. (Opcional) Blueprint filho da Death Ability

Só se quiser tunar no editor sem mudar C++:

1. **Add → Blueprint Class** → parent `UDFAbility_Enemy_Death` ou `UDFAbility_Player_Death`.
2. No BP do inimigo/player, em **Death Ability Class**, selecione esse BP.
3. No CDO da ability (Class Defaults):
   - **Death Event Tag** — default inimigo: `Event.Death.Loot`.
   - **Ability Montage** — fallback se o pawn não tiver `Death Montage` no BP.

Prioridade do montage: **`DeathMontage` no pawn** → depois `Ability Montage` na ability.

---

## 6. Debug

No console (PIE):

```
df.EnemyDeathDebug 2
```

Filtro no Output Log: **`LogDFEnemyDeath`**.

| Mensagem | Significado |
|----------|-------------|
| `HandleServerDeath START` | Morte iniciada no servidor |
| `GA_Enemy_Death: montage finished -> Destroy` | Fluxo GAS OK |
| `TryActivateDeathAbility -> 0` | GA não ativou — ver Death Ability Class no BP |
| `Destroy backup scheduled` | Timer de segurança (~4s); inimigo deve sumir mesmo se GA falhar |
| `FallbackDeathPresentation` | Ability não ativou; checar ASC / spec |
| `AnimationSingleNode (bypass ABP)` | Montage inimigo via bypass |
| `GameplayCue.Enemy.Death OnExecute` | Cue disparou (só VFX) |

---

## 7. Checklist rápido

### Inimigo (`BP_DeepSeaLizard` etc.)

- [ ] `Death Montage` preenchido no Class Defaults  
- [ ] Skeleton do montage = skeleton do mesh  
- [ ] (Opcional) Notify `Event.Death.Loot` no montage  
- [ ] Linha em `DT_Enemies` com loot/XP se aplicável  
- [ ] (Opcional) `Health Bar Widget Class`  
- [ ] Recompilar C++ após pull  

### Player

- [ ] `Death Montage` no BP do personagem  
- [ ] `Use Ragdoll On Death` conforme design  
- [ ] ABP com suporte a `State.Dead` / montage slot  
- [ ] Testar morte em listen server (montage replica via ASC)  

---

## 8. Problemas comuns

| Sintoma | Causa provável | Correção |
|---------|----------------|----------|
| Inimigo some sem animação | `Death Montage` vazio | Atribuir montage no BP |
| Montage “toca” no log mas não vê | ABP sobrescrevia (antigo) | Recompilar; deve usar `AnimationSingleNode` |
| Loot no ar / cedo demais | Notify mal posicionado | Ajustar frame de `Event.Death.Loot` |
| Sem loot | `Loot Table Rows` vazio no DT | Preencher tabela |
| Player volta a idle | ABP sem estado Dead | Transição `bIsDead` no ABP |
| `FallbackDeathPresentation` no log | GA não ativou | Verificar ASC no pawn, recompilar, `GrantDeathAbility` |

---

## 9. Referência C++ (para programadores)

| Arquivo | Responsabilidade |
|---------|------------------|
| `Source/DungeonForged/Private/GAS/Abilities/UDFAbility_Death.cpp` | Montage + event + fim da ability |
| `Source/DungeonForged/Private/GAS/Abilities/UDFAbility_Enemy_Death.cpp` | GE, corpse, loot, destroy |
| `Source/DungeonForged/Private/GAS/Abilities/UDFAbility_Player_Death.cpp` | GE, input off, finalize pose |
| `Source/DungeonForged/Private/Characters/ADFEnemyBase.cpp` | `HandleServerDeath`, grant/activate GA |
| `Source/DungeonForged/Private/Characters/ADFPlayerCharacter.cpp` | `HandlePlayerOutOfHealth`, grant/activate GA |
| `Source/DungeonForged/Public/Combat/AN/AN_SendGameplayEvent.h` | Notify → Gameplay Event no montage |
| `Source/DungeonForged/Private/GAS/Effects/UGE_EnemyDeath.cpp` | `State.Dead` + cue VFX |

Documentação de animação: `Source/DungeonForged/Public/Animation/ABP_DungeonForged_Authoring.md`.
