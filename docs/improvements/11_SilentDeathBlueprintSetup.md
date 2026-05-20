# Silent Death — Setup Blueprint / Editor

> Complementa [`Critical_Point_Silent_Death.md`](../analysis/Critical_Point_Silent_Death.md).  
> **C++:** Patches 1–7 implementados via `UDFDeathCinematicSubsystem`.

Legenda: **✅ C++ pronto** · **⚠️ configurar no editor** · **❌ opcional / futuro**

---

## 1. Checklist rápido

| Item | Status | Onde |
|------|--------|------|
| Morte do player: slow-mo + desat + sting | ✅ | automático (`Multicast_PlayPlayerDeathCinematic`) |
| Killer attribution no defeat screen | ✅ | automático (`GetLastLethalCauseString`) |
| GameplayCue inimigo: VFX/SFX | ⚠️ | `GCN_EnemyDeath` asset |
| Golpe letal = hitstop Critical | ✅ | `bWasLethal` no pipeline de hit |
| Último inimigo da sala = celebração | ✅ | `UDFDungeonManager` + `Client_PlayCombatSpectacle` |
| Skip defeat → Nexus | ✅ | botão `ReturnNexus` chama `RequestSkipToNexus` |
| Death pose sem cortar partículas | ✅ | `LockDeathPoseOnMesh` (Patch 6) |
| Câmera death cam / letterbox | ❌ | backlog PD-A1 / PD-A5 |

---

## 2. GameplayCue — morte de inimigo (Patch 3)

### 2.1 Asset

1. Content Browser → **Gameplay Cue Notify Static**
2. Parent class: `UDFGameplayCueNotify_EnemyDeath` (Display Name: **DF Enemy Death**)
3. Nome sugerido: `GCN_EnemyDeath`
4. Em **DefaultGameplayTags.ini** / Project Settings, confirme que `GameplayCue.Enemy.Death` aponta para este asset (registro nativo já existe em `DFGameplayCueRegistration`).

### 2.2 Properties no CDO do GCN

| Property | Tipo | Sugestão |
|----------|------|----------|
| `DeathBurstNiagara` | Niagara System | burst de alma/partículas no corpo (~0.5s) |
| `DeathImpactSound` | Sound Base | SFX “thud” ou “soul release” |

### 2.3 Teste

- PIE → matar um grunt → burst + som + hitstop Critical no killer
- Log: `-log LogDFDeath Verbose`

---

## 3. Defeat screen (Patch 2 + 7)

### 3.1 WBP Defeat

O widget C++ base é `UDFDefeatScreenWidget`. No Blueprint filho (ex. `WBP_DefeatScreen`):

| BindWidget | Função |
|------------|--------|
| `CauseText` | mostra `Derrotado por: {Killer}` — preenchido por `SetDefeatData` |
| `ReturnNexus` | já ligado a `HandleReturnNexus` → `RequestSkipToNexus` |

**Opcional:** adicionar botão “Continuar” / Any Key → chamar `RequestSkipToNexus` no graph.

### 3.2 Sting de morte (player)

| Asset | Onde assign |
|-------|-------------|
| `StingDeath` | `UDFMusicManagerSubsystem` no level / game instance BP |

Chamado automaticamente em `PlayPlayerDeathCinematic`.

### 3.3 Desaturação do background

| Property | Onde |
|----------|------|
| `DesaturationPostProcessMaterial` | `UDFDefeatScreenWidget` no BP |

---

## 4. Montages de morte

| Actor | Property | Notas |
|-------|----------|-------|
| Player BP | `DeathMontage` | já usado por `UUDFAbility_Player_Death` |
| Enemy BP | `DeathMontage` | montage via `UUDFAbility_Enemy_Death` |
| Enemy BP | `bDissolveOnDeath` + materiais | dissolve pós-montage |

**Juice durante montage:** Patch 1 dispara slow-mo/desat **no início** da ability de morte, não no fim.

---

## 5. Playtest — §8 do relatório

```text
-log LogDFDeath Verbose -log LogDFFeel Verbose
```

| Teste | Esperado |
|-------|----------|
| Morrer para boss | `CauseText` = “Killed by {EnemyDisplayName}” |
| Matar último inimigo (não-boss) | slow-mo BossSlam + flash + spectacle |
| Light hit letal | hitstop Critical visível |
| Inimigo com Niagara attached na morte | partículas não cortam abruptamente |
| Clicar Return no defeat | viagem imediata ao Nexus (sem esperar 5s) |
| DoT bleed mata | “Killed by Bleed” |
| Self-damage | fallback “Your health reached zero.” |

### Network

```text
Net PktLag=120 Net PktLagVariance=20
```

- Killer attribution persiste no defeat screen
- `Client_PlayCombatSpectacle` chega em listen + client

---

## 6. Backlog (não implementado — §10 do relatório)

| ID | Item | Notas |
|----|------|-------|
| PD-A1 | Death cam zoom no killer | requer `UDFCameraComponent` |
| PD-A2 | Co-op all-dead check | defeat no primeiro morto hoje |
| PD-A5 | “YOU DIED” UI stinger | widget entre montage e defeat |
| ED-A1 | Ragdoll handoff | estética |
| ED-A11 | Boss death cinematic camera | set-piece separado |

---

## 7. Arquivos C++ tocados

| Arquivo | Patch |
|---------|-------|
| `UDFDeathCinematicSubsystem` | central (novo) |
| `UDFAbility_Player_Death.cpp` | 1 |
| `ADFPlayerCharacter` | 1, 2 |
| `UDFAttributeSet.cpp` | 2 |
| `ADFRunGameMode.cpp` | 2, 7 |
| `UDFGameplayCueNotify_EnemyDeath` | 3 |
| `ADFEnemyBase::ExecuteEnemyDeathPresentationCue` | 3 |
| `UDFCombatStateLibrary` + `UDFDungeonManager` | 4 |
| `UDFMeleeTraceComponent` + `UDFCombatFeedbackLibrary` | 5 |
| `DFDeathAnimation.cpp` | 6 |
| `UDFDefeatScreenWidget` | 7 |
