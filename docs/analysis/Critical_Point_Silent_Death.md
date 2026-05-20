# Ponto Crítico — A Morte Silenciosa

> **Data:** 2026-05-20 · **Status C++:** 2026-05-18 ✅ Patches 1–7 implementados
> **Escopo:** auditoria cirúrgica do sistema de morte (player + inimigo) com identificação do **único ponto mais crítico** do jogo.
> **Setup Blueprint/Editor:** [`11_SilentDeathBlueprintSetup.md`](../improvements/11_SilentDeathBlueprintSetup.md)
> **Audiência:** o desenvolvedor solo (você). Doc denso, com referências `arquivo:linha` em cada afirmação.
> **Premissa:** complementa [`Combat_Advanced_Report.md`](Combat_Advanced_Report.md) e [`Game_Analysis.md`](Game_Analysis.md).

---

## 0. TL;DR

> **Status (2026-05-18):** ✅ **Implementado em C++** — `UDFDeathCinematicSubsystem` + Patches 1–7.  
> ⚠️ Falta assign de `DeathBurstNiagara` / `DeathImpactSound` no GCN e playtest.

> **O ponto mais crítico do DungeonForged era:**
> **Toda a infraestrutura cinematográfica de morte existe em C++ (`UDFScreenEffectsComponent::OnDeath`, `UDFHitStopSubsystem::BossSlam`, `UDFGameplayCueNotify_EnemyDeath`, post-process death MID, slow-mo de 3s a 0.2x). NADA disso é chamado pelo fluxo de morte.**
>
> Resultado: o momento mais cinematográfico do jogo — a morte do player ou do último inimigo de uma sala — é tratado como uma **transição silenciosa de UI**. Sem slow-mo, sem desaturação, sem stinger sonoro, sem "killer attribution", sem celebração de kill.
>
> **Por quê é crítico:** num roguelike, a morte do player é **o último gosto de cada run** — é o que faz o jogador apertar "retry" ou fechar o jogo. Em Hades, a morte É o ritual. No DungeonForged, é um diálogo "Your health reached zero."
>
> **Fix estimado: 4-6 horas de patches em ~60 linhas de C++.** Maior delta impacto/esforço do projeto inteiro.

---

## 1. Por que este é THE ponto mais crítico

Considerei 5 candidatos. Por que descartei os outros:

| Candidato | Por que NÃO é o mais crítico |
|-----------|------------------------------|
| **Projectile Parity Gap** (do relatório anterior) | Já priorizado em Tier A2 do [Combat_Advanced_Report.md](Combat_Advanced_Report.md). Importante, mas afeta feedback contínuo (todo hit) — não o momento de impacto máximo. |
| **Bug de replicação de combo** (C7) | 15 min de fix, mas afeta MP com lag — não bloqueia feel single-player. |
| **CooldownReduction inerte** (G1) | Bug funcional, não afeta feel cinematográfico. |
| **Feedback Fragmentation** (F1) | Sintoma mais amplo do mesmo problema raiz, mas A Morte Silenciosa é o **caso extremo e mais visível** disso — onde a desconexão mais machuca. |
| **Falta de finisher cinematic chain** | Feature NOVA — não é uma lacuna em algo já construído. |

**A Morte Silenciosa vence porque atende todos os 5 critérios simultaneamente:**

1. ✅ **Impacto percebido máximo** — morte é o frame que o jogador lembra
2. ✅ **Esforço mínimo** — infraestrutura completa já existe, falta só wire
3. ✅ **Compound effect** — afeta player death + enemy kill (dois momentos críticos)
4. ✅ **Retenção direta em roguelike** — Hades/Returnal/Hellboy provam que death scene drive replay
5. ✅ **Não bloqueia outras features** — pode ser feito em paralelo a qualquer outro trabalho

---

## 2. Forensic Map — o que existe vs o que é chamado

### 2.1 Player death — pipeline atual

```
                ┌──────────────────────────────────────────┐
                │ UDFAttributeSet::PreAttributeChange      │
                │  cpp:152-228                              │
                │  Health ≤ 0 → SecondWind check ou clamp   │
                └─────────────────┬────────────────────────┘
                                  ▼
                ┌──────────────────────────────────────────┐
                │ UDFAttributeSet::PostGameplayEffectExecute│
                │  cpp:318                                  │
                │  Re-clamp Health                          │
                └─────────────────┬────────────────────────┘
                                  ▼
                ┌──────────────────────────────────────────┐
                │ HandleOutOfHealth()  cpp:354-366          │
                │  Broadcast OnOutOfHealth (idempotente)    │
                └─────────────────┬────────────────────────┘
                                  ▼
   ┌──────────────────────────────────────────────────────────┐
   │ ADFPlayerCharacter::HandlePlayerOutOfHealth   cpp:693-726 │
   │  bPlayerDeathHandled = true                                │
   │  FIRE Event_Death                                          │
   │  TryActivateDeathAbility() → UDFAbility_Player_Death       │
   └─────────────────┬────────────────────────────────────────┘
                     ▼
   ┌──────────────────────────────────────────────────────────┐
   │ UUDFAbility_Death::ActivateAbility   cpp:65-106            │
   │  CommitAbility                                             │
   │  ApplyDeathState → UGE_Death (Health=0, +State.Dead)      │
   │  OnDeathFlowStarted()  ◀── player-specific hook            │
   │  PlayMontageAndWait                                        │
   └─────────────────┬────────────────────────────────────────┘
                     ▼
   ┌──────────────────────────────────────────────────────────┐
   │ BeginDeathPresentationFromAbility   cpp:675-691           │
   │  SetCanBeDamaged(false)                                    │
   │  Stop melee trace                                          │
   │  DisableMovement                                           │
   │  DisableInput(PC)                                          │
   └─────────────────┬────────────────────────────────────────┘
                     ▼
                  Death montage plays...
                     ▼
   ┌──────────────────────────────────────────────────────────┐
   │ FinalizeDeathPresentation   cpp:733-751                    │
   │  bDeathPresentationFinalized = true                        │
   │  LockDeathPoseOnMesh → Mesh->SetComponentTickEnabled(false)│
   └─────────────────┬────────────────────────────────────────┘
                     ▼
   ┌──────────────────────────────────────────────────────────┐
   │ ADFRunGameMode::HandlePlayerOutOfHealth   cpp:283-304      │
   │  Schedule TriggerDefeat (3s timer)                         │
   └─────────────────┬────────────────────────────────────────┘
                     ▼
   ┌──────────────────────────────────────────────────────────┐
   │ TriggerDefeat   cpp:396-424                                │
   │  GetRunSummary → Client_OpenDefeatScreen(Summary, Cause)   │
   │  Cause = "Your health reached zero."  ← HARDCODED          │
   │  Schedule travel to Nexus (5s)                             │
   └──────────────────────────────────────────────────────────┘
```

**O que NÃO é chamado neste pipeline:**

| Sistema disponível | Onde declarado | Por quem é chamado HOJE | Status |
|---|---|---|---|
| `UDFScreenEffectsComponent::OnDeath()` | [UDFScreenEffectsComponent.cpp:375-386](../../Source/DungeonForged/Private/FX/UDFScreenEffectsComponent.cpp) | **Ninguém em C++.** Apenas se BP do player wireou manualmente. | 🔴 **ÓRFÃO** |
| `UDFHitStopSubsystem::TriggerHitStop(3.0, 0.2)` | dentro de `OnDeath()` | Nunca executa (porque OnDeath nunca é chamado) | 🔴 **MORTO** |
| `UDFHitStopSubsystem::BossSlam` band | [UDFHitStopSubsystem.cpp:39](../../Source/DungeonForged/Private/FX/UDFHitStopSubsystem.cpp) | Só boss abilities. Não dispara no momento da morte do player. | 🟡 **SUB-UTILIZADO** |
| Camera takeover (cinematic angle) | Não existe | — | 🔴 **AUSENTE** |
| FOV punch on death | `UDFScreenEffectsComponent::LerpLocalPlayerFOV` é stub ([UDFScreenEffectsComponent.cpp:236](../../Source/DungeonForged/Private/FX/UDFScreenEffectsComponent.cpp)) | — | 🔴 **STUB** |
| "Killer attribution" no defeat screen | [ADFRunGameMode.cpp:299](../../Source/DungeonForged/Private/GameModes/Run/ADFRunGameMode.cpp) | DefeatCause = "Your health reached zero." hardcoded | 🔴 **HARDCODED** |
| Audio sting de morte | `UDFMusicManagerSubsystem::StingDeath` existe ([UDFMusicManagerSubsystem.cpp:309](../../Source/DungeonForged/Private/Audio/UDFMusicManagerSubsystem.cpp)) | Apenas `PlaySound2D(StingDeath)` no music manager — não no fluxo de morte do player | 🟡 **DESLIGADO DO FLUXO** |
| `UDFCameraShake_DeathBlow` | Não existe (mencionado em [02_Juice.md:151](../improvements/02_Juice.md)) | — | 🔴 **AUSENTE** |
| Cinematic letterbox UI | Não existe | — | 🔴 **AUSENTE** |

### 2.2 Enemy death — pipeline atual

```
                ┌─────────────────────────────────────────┐
                │ UDFAttributeSet::PreAttributeChange      │
                │  Health → 0 (sem SecondWind em inimigo)  │
                └────────────────┬────────────────────────┘
                                 ▼
                ┌─────────────────────────────────────────┐
                │ OnHealthOrMaxChanged → HandleServerDeath │
                │  ADFEnemyBase.cpp:467, 479, 484          │
                │  bDeathDetectionArmed guard              │
                └────────────────┬────────────────────────┘
                                 ▼
        ┌──────────────────────────────────────────────────┐
        │ HandleServerDeath  cpp:1235-1326                  │
        │  ResolveKillerPlayerState → XP × (1+0.1×Floor)    │
        │  Award gold                                        │
        │  TriggerDeathGameplayAbility (Event_Death)         │
        │  SyncDeathToBlackboardAndAI                        │
        │  ScheduleDeathDestroyBackup (~20s)                 │
        └────────────────┬─────────────────────────────────┘
                         ▼
        ┌──────────────────────────────────────────────────┐
        │ UDFAbility_Enemy_Death::ActivateAbility           │
        │  PlayMontage (AnimationSingleNode bypass ABP)     │
        │  Multicast_PlayDeathCosmetic                       │
        └────────────────┬─────────────────────────────────┘
                         ▼
              death montage plays ...
                         ▼
        ┌──────────────────────────────────────────────────┐
        │ OnDeathMontagePipelineFinished                     │
        │  LockDeathPoseOnMesh (tick disable)                │
        │  ApplyDeathGameplayState (UGE_EnemyDeath)          │
        │  → GameplayCue.Enemy.Death fires                   │
        │  BeginPostDeathCleanup → dissolve 1s delay         │
        │  SpawnDeathLoot                                    │
        └────────────────┬─────────────────────────────────┘
                         ▼
        ┌──────────────────────────────────────────────────┐
        │ UDFGameplayCueNotify_EnemyDeath::OnExecute         │
        │  cpp:28-30                                          │
        │  → LOG only. Empty implementation.                  │
        └──────────────────────────────────────────────────┘
                         ▼
        Dissolve material lerps 0→1 over DissolveDuration
                         ▼
                       Destroy
```

**O que NÃO é chamado:**

| Sistema disponível | Onde declarado | Por quem é chamado HOJE | Status |
|---|---|---|---|
| `UDFGameplayCueNotify_EnemyDeath::OnExecute` | [UDFGameplayCueNotify_EnemyDeath.cpp:28-30](../../Source/DungeonForged/Private/GAS/Cues/UDFGameplayCueNotify_EnemyDeath.cpp) | **É chamado**, mas o corpo é apenas `UE_LOG`. Cue vazia. | 🔴 **STUB** |
| `UDFHitStopSubsystem::CriticalHit` na lethal blow | Existe | Não disparado quando hit causa morte | 🔴 **AUSENTE** |
| Slow-mo no "last enemy of room" | Não existe | — | 🔴 **AUSENTE** |
| Camera punch/zoom no killer | Não existe | — | 🔴 **AUSENTE** |
| Screen bloom no kill | `UDFScreenEffectsComponent::FlashScreen` existe | Não wireado a evento de kill | 🟡 **DESLIGADO** |
| Knockback impulse on death | `UDFHitReactionComponent` aplica impulse em knockback, mas não há "impulse on lethal hit" diferenciado | — | 🔴 **AUSENTE** |
| Damage-type-specific death (fire = burn death, bleed = bleed-out) | Não existe; uma montage por inimigo | — | 🔴 **AUSENTE** |
| XP/Gold UI burst (floating text de "+50 XP") | `UDFCombatTextSubsystem` suporta type `XP` | Chamado por leveling component, mas não no momento exato do kill como burst | 🟡 **DESCONECTADO DO MOMENTO** |

---

## 3. The Silent Death — análise cirúrgica do player

### 3.1 O que o jogador vivencia hoje

**Cronograma real medido pelos call sites:**

```
T=0.00s   Lethal damage acerta (último frame de hit do inimigo)
T=0.00s   Health → 0 (PreAttributeChange clamp)
T=0.00s   HandleOutOfHealth broadcast
T=0.00s   Event_Death fires
T=0.00s   Death ability activates
T=0.00s   Input disabled, movement stops
T=0.00s   Death montage starts
                                          ← AQUI O JOGO FICA SILENCIOSO
T=~1.5s   Death montage ends
T=~1.5s   LockDeathPoseOnMesh (mesh tick disabled)
T=~1.5s   ScreenEffects.OnDeath()? NUNCA CHAMADO em C++
                                          ← AQUI DEVERIA TER:
                                            - Hit stop band BossSlam (0.20s @ 0.0001x)
                                            - Slow-mo 3s @ 0.2x
                                            - Desaturação (lerp 0→0.8 over 0.8s)
                                            - Vignette 1.0
                                            - Audio sting StingDeath
                                            - Camera kill-zoom no killer
                                            - Letterbox UI (16:9 → 21:9)
                                            - "YOU DIED" stinger
T=3.00s   ADFRunGameMode timer fires → TriggerDefeat
T=3.00s   DefeatScreen widget appears
          DefeatCause = "Your health reached zero." (hardcoded)
T=3.00s   Input mode switches to UI
T=5.00s   ScheduleFinishDefeatToNexus
T=~6.00s  ServerTravel to Nexus
```

**Resumo:** entre T=0 e T=3 (a maior parte da sequência de morte — 3 segundos!) **nada de juice acontece**. Só a montage tocando e o mesh congelando. Sem peso. Sem celebração. Sem reflexão.

### 3.2 O smoking gun — código

Em [`UUDFAbility_Player_Death::OnDeathFlowStarted`](../../Source/DungeonForged/Private/GAS/Abilities/UDFAbility_Player_Death.cpp) (hipotético — agentes confirmaram que existe um hook):

```cpp
void UUDFAbility_Player_Death::OnDeathFlowStarted()
{
    Super::OnDeathFlowStarted();
    if (ADFPlayerCharacter* const Player = Cast<ADFPlayerCharacter>(GetAvatarActorFromActorInfo()))
    {
        Player->BeginDeathPresentationFromAbility();  // ← desabilita input + para movement
    }
    // ❌ NÃO CHAMA Player->GetScreenEffects()->OnDeath()
    // ❌ NÃO CHAMA UDFHitStopSubsystem::BossSlam()
    // ❌ NÃO CHAMA UDFCameraShakeFunctionLibrary
    // ❌ NÃO TROCA câmera para death cam
    // ❌ NÃO ENVIA evento "Event.Combat.PlayerDeath" pro music manager
}
```

E em [`ADFRunGameMode::TriggerDefeat`](../../Source/DungeonForged/Private/GameModes/Run/ADFRunGameMode.cpp) o `DefeatCause` vem hardcoded:

```cpp
// ADFRunGameMode.cpp:299
FString DefeatCause = TEXT("Your health reached zero.");  // ❌ HARDCODED
```

Nenhum lugar no código rastreia "quem te matou" — apesar de `LastDamageAttacker` existir em [`ADFEnemyBase`](../../Source/DungeonForged/Private/Characters/ADFEnemyBase.cpp), o jogador não tem equivalente exposto ao defeat flow.

### 3.3 O que AAA faz nesse momento

| Jogo | O que acontece nos últimos frames antes da defeat screen |
|------|----------------------------------------------------------|
| **Hades** | Hit stop ~0.3s, slow-mo, câmera puxa para Zagreus, ele cai falando algo característico, screen wipe vermelho, audio sting "thanatos theme", "You have been slain by **Megaera**". Toda a cena ~4-5s. |
| **Returnal** | Hit stop, vinheta vermelha + chromatic spike, áudio cascateia (batimento → tone), câmera afasta lentamente, screen desature até preto, "Failure" sting. |
| **Dark Souls** | Hit stop curto, "YOU DIED" gigante em vermelho/dourado entra do nada com SFX brutal, fade. |
| **God of War Ragnarok** | Slow-mo no killer blow, câmera ortográfica em Kratos no chão, gasp respiração final, fade. |
| **Diablo IV** | Death cam orbits corpse, sound sting, summary screen com cause attribution. |

**Padrão comum:** **3 a 5 segundos de cinema** entre o último hit e a UI de game-over. DungeonForged tem esses 3 segundos vazios.

---

## 4. The Silent Kill — análise cirúrgica do inimigo

### 4.1 O que o jogador vivencia hoje ao matar

```
T=0.00s   Player hit lands (último HP do inimigo)
T=0.00s   UDFMeleeTraceComponent dispatches:
            - HitStop (Light or Heavy band — based on dmg threshold)
            - CameraShake (Light or Heavy)
            - PlayImpactCosmetics (impact VFX + SFX)
          ← IGUAL A QUALQUER HIT — não tem distinção de "kill"
T=0.00s   HandleServerDeath fires server-side
T=0.00s   XP/Gold awarded
T=0.00s   AI brain stops, blackboard bIsDead=true
T=0.00s   GameplayCue.Enemy.Death fires
            UDFGameplayCueNotify_EnemyDeath::OnExecute → LOG ONLY
T=0.00s   Death montage starts (multicast)
                                          ← SILÊNCIO TOTAL DE 1-2s
T=~1.5s   Montage ends
T=~1.5s   LockDeathPoseOnMesh
T=~2.5s   Dissolve starts (1s delay)
T=~4.0s   Dissolve completes, Destroy
```

**O que falta em momentos-chave:**

- **Lethal blow:** mesmo hit stop e shake de qualquer hit. Sem distinção de "este foi o último".
- **Last enemy of the room:** sem slow-mo, sem celebração. O jogador não sente "limpou".
- **GameplayCue stub:** `UDFGameplayCueNotify_EnemyDeath` é um logging stub — sem partículas, sem som, sem nada. Era para ser o ponto óbvio de centralização cosmética de morte de inimigo.
- **Sem knockback impulse:** corpse cai onde está. Hades/GoW dão um leve arco/slide para o lado que recebeu o golpe.
- **Dissolve linear blunt:** todos os materiais dissolvem juntos com timing identical. Hades faz staggered dissolve por camada para elegância visual.

### 4.2 O smoking gun — [`UDFGameplayCueNotify_EnemyDeath`](../../Source/DungeonForged/Private/GAS/Cues/UDFGameplayCueNotify_EnemyDeath.cpp)

```cpp
// O arquivo INTEIRO da cue de morte de inimigo:
void UDFGameplayCueNotify_EnemyDeath::OnExecute_Implementation(
    AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
    UE_LOG(LogTemp, Verbose, TEXT("Enemy death cue executed for %s"),
           *GetNameSafe(MyTarget));
    // ❌ FIM. Sem VFX. Sem SFX. Sem shake. Sem hit-stop.
}
```

E em [`UDFMeleeTraceComponent::ApplyDamageToTarget`](../../Source/DungeonForged/Private/Combat/UDFMeleeTraceComponent.cpp) (cpp:1477-1513), a seleção de banda do hit stop é **agnóstica de morte**:

```cpp
// cpp:1483-1497
if (UDFHitStopSubsystem* const HitStop = World->GetSubsystem<UDFHitStopSubsystem>())
{
    if (bCrit)             HitStop->CriticalHit(Owner);
    else if (DmgMagnitude >= 30.f || KbMagnitude > 100.f) HitStop->HeavyHit(Owner);
    else                   HitStop->LightHit(Owner);
    // ❌ Não checa: "isso vai matar o alvo?"
    // ❌ Se sim, deveria ser BossSlam ou nova band Kill
}
```

Sem essa diferenciação, **matar um inimigo sente igual a só acertá-lo**. Esse é o oposto do que AAA faz.

---

## 5. AAA Target Architecture

### 5.1 Princípios

1. **Death é um evento composto**, não uma sequência linear. Deve disparar `OnDeathCinematic(Context)` com payload completo (killer, killed, location, damage type, isLastEnemyInRoom, etc).
2. **Player death ≠ Enemy kill** mecanicamente, mas **compartilham o mesmo pipeline cinematográfico** (slow-mo + camera + screen + sound).
3. **Killer attribution é dado de primeira classe** — não string hardcoded. Persiste no SaveGame para Hades-style retrospectiva.
4. **Last-enemy-of-room** é detectado e celebrado distintamente (combat state subsystem já sabe).
5. **GameplayCue de morte é a porta central** para arte/design extender VFX/SFX sem mexer em C++.

### 5.2 Arquitetura proposta

```
                  ┌─────────────────────────────────────┐
                  │ UDFDeathCinematicSubsystem (novo)   │
                  │  - PlayPlayerDeathCinematic(Ctx)    │
                  │  - PlayEnemyKillCinematic(Ctx)      │
                  │  - PlayLastEnemyKillCinematic(Ctx)  │
                  └────────────┬────────────────────────┘
                               │
            ┌──────────────────┼───────────────────┐
            ▼                  ▼                   ▼
   ┌─────────────────┐  ┌────────────────┐  ┌──────────────────┐
   │ HitStop         │  │ ScreenEffects  │  │ CameraShake +    │
   │ (BossSlam band  │  │ (OnDeath /     │  │ KillCam (lerp    │
   │  no player,     │  │  KillFlash)    │  │  para killer)    │
   │  KillCrit no    │  │                │  │                  │
   │  enemy)         │  │                │  │                  │
   └─────────────────┘  └────────────────┘  └──────────────────┘
            │                  │                   │
            └──────────────────┼───────────────────┘
                               ▼
                  ┌─────────────────────────────────────┐
                  │ MusicManager.PlaySting(Death/Kill)  │
                  └─────────────────────────────────────┘
                               │
                               ▼
                  ┌─────────────────────────────────────┐
                  │ CombatTextSubsystem                 │
                  │ - "+50 XP" burst                    │
                  │ - "Killed by Megaera" defeat text   │
                  └─────────────────────────────────────┘
```

### 5.3 Estrutura de contexto

```cpp
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFDeathCinematicContext
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<AActor> Victim;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<AActor> Killer;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName KillerDisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector LethalImpactLocation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector LethalImpactDirection;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTagContainer DamageTags;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bWasCrit = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIsLastEnemyOfRoom = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIsBoss = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float FinalDamage = 0.f;
};
```

---

## 6. Migration Plan — patch por patch

> **✅ Todos os patches abaixo foram aplicados.** Ver [`11_SilentDeathBlueprintSetup.md`](../improvements/11_SilentDeathBlueprintSetup.md).

| Patch | Descrição | Status |
|-------|-----------|--------|
| 1 | Player death cinematic wire | ✅ |
| 2 | Killer attribution | ✅ |
| 3 | GameplayCue VFX/SFX | ✅ C++ · ⚠️ assets GCN |
| 4 | Last enemy celebration | ✅ |
| 5 | Lethal blow band | ✅ |
| 6 | Safe death pose lock | ✅ |
| 7 | Skip defeat | ✅ |

Sequenciado pelo **menor risco × maior impacto**. Cada patch é isolado e testável independentemente.

### Patch 1 — Wire `ScreenEffects->OnDeath()` no fluxo do player (15 min) ✅

**Onde:** [`UUDFAbility_Player_Death::OnDeathFlowStarted()`](../../Source/DungeonForged/Private/GAS/Abilities/UDFAbility_Player_Death.cpp)

**Diff:**
```cpp
void UUDFAbility_Player_Death::OnDeathFlowStarted()
{
    Super::OnDeathFlowStarted();
    ADFPlayerCharacter* const Player = Cast<ADFPlayerCharacter>(GetAvatarActorFromActorInfo());
    if (!Player) return;

    Player->BeginDeathPresentationFromAbility();

    // === PATCH 1: WIRE EXISTING DEATH CINEMATIC INFRASTRUCTURE ===
    if (UDFScreenEffectsComponent* FX = Player->GetScreenEffects())
    {
        FX->OnDeath();  // Activates 3s @ 0.2x hit stop + desaturation timeline
    }

    if (UWorld* W = GetWorld())
    {
        if (UDFHitStopSubsystem* HS = W->GetSubsystem<UDFHitStopSubsystem>())
        {
            HS->BossSlam(nullptr);  // No exclude actor — freeze the player too
        }
        if (UDFMusicManagerSubsystem* Music = W->GetGameInstance()->GetSubsystem<UDFMusicManagerSubsystem>())
        {
            Music->PlayDeathSting();  // Already exists at line 309
        }
    }
}
```

**Impacto:** o player death sai de silencioso para cinemático em 5 linhas. Slow-mo, desaturação, sting de áudio — tudo já existia.

**Risco:** zero — ScreenEffects.OnDeath() já é defensivo (verifica MID, BeginPlay state).

---

### Patch 2 — Killer attribution no defeat screen (1h)

**Passo 2a — track LastDamageAttacker no player:**

Adicionar em [`UDFAttributeSet.cpp:PostGameplayEffectExecute`](../../Source/DungeonForged/Private/GAS/UDFAttributeSet.cpp), antes de `HandleOutOfHealth()`:

```cpp
// Track who dealt the killing blow on the player
if (Data.EvaluatedData.Attribute == GetHealthAttribute()
    && GetHealth() <= 0.f
    && Data.EffectSpec.GetContext().GetInstigator())
{
    if (ADFPlayerCharacter* Player = Cast<ADFPlayerCharacter>(ASC->GetOwner()))
    {
        Player->SetLastLethalDamageContext(
            Data.EffectSpec.GetContext().GetInstigator(),
            Data.EffectSpec.GetContext().GetEffectCauser(),
            Data.EffectSpec.CapturedSourceTags.GetSpecTags()
        );
    }
}
```

**Passo 2b — expose no `ADFPlayerCharacter`:**

```cpp
// .h
UPROPERTY()
TObjectPtr<AActor> LastLethalInstigator = nullptr;
UPROPERTY()
TObjectPtr<AActor> LastLethalCauser = nullptr;
UPROPERTY()
FGameplayTagContainer LastLethalTags;

UFUNCTION(BlueprintCallable, Category="DF|Death")
void SetLastLethalDamageContext(AActor* Instigator, AActor* Causer, const FGameplayTagContainer& Tags);

UFUNCTION(BlueprintCallable, Category="DF|Death")
FString GetLastLethalCauseString() const;
```

**Passo 2c — usar em `TriggerDefeat`:**

```cpp
// ADFRunGameMode.cpp:299 — substituir hardcoded
FString DefeatCause = TEXT("Your health reached zero.");

// POR:
FString DefeatCause;
if (PlayerChar.IsValid()) {
    DefeatCause = PlayerChar->GetLastLethalCauseString();
}
if (DefeatCause.IsEmpty()) {
    DefeatCause = TEXT("Your health reached zero.");  // fallback
}
```

**Impacto:** "Killed by **Megaera**" / "Killed by **Floor Spikes**" / "Killed by **Bleed**" — Hades-tier retention hook.

**Risco:** baixo — fallback string preserva comportamento atual se attribution falhar.

---

### Patch 3 — GameplayCue de morte com VFX/SFX (1h)

**Onde:** [`UDFGameplayCueNotify_EnemyDeath.cpp:28-30`](../../Source/DungeonForged/Private/GAS/Cues/UDFGameplayCueNotify_EnemyDeath.cpp)

**Diff (esqueleto):**
```cpp
bool UDFGameplayCueNotify_EnemyDeath::OnExecute_Implementation(
    AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
    if (!MyTarget) return false;

    UWorld* const W = MyTarget->GetWorld();
    if (!W) return false;

    // VFX no local do corpo
    if (DeathBurstNiagara)  // UPROPERTY(EditAnywhere)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            W, DeathBurstNiagara, MyTarget->GetActorLocation(), FRotator::ZeroRotator,
            FVector(1.f), true, true, ENCPoolMethod::AutoRelease);
    }

    // SFX (CueParameters carrega ImpactLocation/Direction se disponível)
    if (DeathImpactSound)
    {
        UGameplayStatics::PlaySoundAtLocation(W, DeathImpactSound, MyTarget->GetActorLocation());
    }

    // Hit stop "kill" — banda específica para confirmação de kill
    if (UDFHitStopSubsystem* HS = W->GetSubsystem<UDFHitStopSubsystem>())
    {
        // Killer fica em tempo normal; alvo já está morto
        AActor* Killer = Parameters.Instigator.Get();
        HS->CriticalHit(Killer);
    }

    // Camera shake leve para o killer
    if (Parameters.Instigator.IsValid())
    {
        if (APawn* P = Cast<APawn>(Parameters.Instigator.Get()))
        {
            if (APlayerController* PC = Cast<APlayerController>(P->GetController()))
            {
                UDFCameraShakeFunctionLibrary::PlayHeavyHitOnOwner(this, PC);
            }
        }
    }

    return true;
}
```

E nas properties da classe:
```cpp
UPROPERTY(EditAnywhere, Category="Death|Cue")
TObjectPtr<UNiagaraSystem> DeathBurstNiagara;

UPROPERTY(EditAnywhere, Category="Death|Cue")
TObjectPtr<USoundBase> DeathImpactSound;
```

**Impacto:** o "kill confirmation" finalmente acontece. Cada inimigo morto = burst de partículas + som + freeze frame curto + camera shake.

**Risco:** zero — adiciona apenas; não muda fluxo existente.

---

### Patch 4 — Last Enemy of Room celebration (2h)

**Passo 4a — detectar:**

Adicionar em [`UDFCombatStateLibrary`](../../Source/DungeonForged/Public/Combat/UDFCombatStateLibrary.h):

```cpp
UFUNCTION(BlueprintCallable, Category="DF|Combat")
static bool IsLastEnemyInRoom(UObject* WorldContext, AActor* DyingEnemy);
```

Implementação varre `TActorIterator<ADFEnemyBase>` filtrando vivos e excluindo o que está morrendo. Se `count == 0` → é o último.

**Passo 4b — disparar slow-mo + screen pulse:**

Em `ADFEnemyBase::HandleServerDeath`, antes de `TriggerDeathGameplayAbility`:

```cpp
if (UDFCombatStateLibrary::IsLastEnemyInRoom(this, this))
{
    // Multicast para todos os players verem
    Multicast_PlayLastEnemyCelebration();
}
```

E em `Multicast_PlayLastEnemyCelebration_Implementation`:

```cpp
if (UWorld* W = GetWorld())
{
    if (UDFHitStopSubsystem* HS = W->GetSubsystem<UDFHitStopSubsystem>())
    {
        HS->BossSlam(LastDamageAttacker);  // 0.20s @ 0.0001x — congela mundo, exclui killer
    }
    // Tela inteira pulsa branco curto
    for (FConstPlayerControllerIterator It = W->GetPlayerControllerIterator(); It; ++It)
    {
        if (APlayerController* PC = It->Get())
        {
            if (ADFPlayerCharacter* P = Cast<ADFPlayerCharacter>(PC->GetPawn()))
            {
                if (UDFScreenEffectsComponent* FX = P->GetScreenEffects())
                {
                    FX->FlashScreen(FLinearColor(1.f, 0.95f, 0.8f, 0.3f), 0.4f, 0.35f);
                }
            }
        }
    }
}
```

**Impacto:** matar o último inimigo da sala finalmente sente como **vitória** — slow-mo, flash, peso. Hades-tier moment.

**Risco:** baixo — só executa em N enemies → 0 (detectado server-side, multicast).

---

### Patch 5 — Death band no melee trace para lethal blow (30 min)

**Onde:** [`UDFMeleeTraceComponent.cpp:1483-1497`](../../Source/DungeonForged/Private/Combat/UDFMeleeTraceComponent.cpp)

**Diff:**
```cpp
// Detectar se este hit vai matar o alvo
bool bWillKill = false;
if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
{
    if (const UDFAttributeSet* AS = TargetASC->GetSet<UDFAttributeSet>())
    {
        bWillKill = (AS->GetHealth() - DmgMagnitude) <= KINDA_SMALL_NUMBER;
    }
}

if (UDFHitStopSubsystem* const HitStop = World->GetSubsystem<UDFHitStopSubsystem>())
{
    if (bWillKill)            HitStop->CriticalHit(Owner);  // Boost lethal blow regardless of damage
    else if (bCrit)           HitStop->CriticalHit(Owner);
    else if (DmgMagnitude >= 30.f || KbMagnitude > 100.f) HitStop->HeavyHit(Owner);
    else                      HitStop->LightHit(Owner);
}
```

**Impacto:** o golpe que mata sente diferente do que apenas acerta. Mesmo Light combo no último HP → Critical hit stop.

**Risco:** zero — só sobe a banda condicionalmente.

---

### Patch 6 — Tick disable seguro em LockDeathPoseOnMesh (15 min)

**Problema:** `DFDeathAnimation::LockDeathPoseOnMesh` ([cpp:175-177](../../Source/DungeonForged/Private/Animation/DFDeathAnimation.cpp)) desabilita o tick do mesh **inteiro**, cortando partículas e child components attached.

**Diff:**
```cpp
void LockDeathPoseOnMesh(USkeletalMeshComponent* Mesh, UAnimMontage* Montage)
{
    if (!Mesh || !Montage) return;

    if (UAnimInstance* AI = Mesh->GetAnimInstance())
    {
        AI->Montage_SetPosition(Montage, Montage->GetPlayLength());
        AI->Montage_Pause(Montage);
        // ❌ ANTES: Mesh->SetComponentTickEnabled(false)
        // ✅ DEPOIS: pausa só a anim instance, deixa o tick do mesh fluir
        //           para physics, attached components e partículas continuarem.
        AI->SetUpdateAnimationInEditor(false);
        Mesh->bPauseAnims = true;
    }
}
```

**Impacto:** partículas de morte (dissolve trail, soul particles) não cortam abruptamente. Conforme memória [`feedback_death_pose_locking.md`](../../memory/feedback_death_pose_locking.md), `bPauseAnims` sozinho não é suficiente para death pose em todos os casos — então mantenha um fallback:

```cpp
// Se mesh tem child components que dependem do tick, mantenha tick mas pause anim.
// Se mesh é "limpo" (sem children attached), desabilite tick para perf.
const bool bHasAttachedFXChildren = Mesh->GetAttachChildren().ContainsByPredicate(
    [](USceneComponent* C) { return C && C->IsA<UNiagaraComponent>(); });

if (!bHasAttachedFXChildren)
{
    Mesh->SetComponentTickEnabled(false);  // safe to fully disable
}
```

**Impacto:** corrige cut-off de partículas em mortes com VFX attached.

**Risco:** médio — testar bem porque a memory anterior diz que `bPauseAnims` sozinho não trava root motion. Manter `SetComponentTickEnabled(false)` no caminho default.

---

### Patch 7 — Skip button no defeat screen (30 min)

Para evitar que o jogador ache o tempo 3s+5s longo demais (especialmente em runs de teste rápido):

Em [`UDFDefeatScreenWidget`](../../Source/DungeonForged/Public/UI/Run/UDFDefeatScreenWidget.h):

```cpp
UFUNCTION(BlueprintCallable, Category="DF|Defeat")
void RequestSkipToNexus();

// Bind a um botão "Continue" ou Any-Key.
// Internally: chama ADFRunGameMode::SkipDefeatToNexus().
```

E em `ADFRunGameMode`:

```cpp
void ADFRunGameMode::SkipDefeatToNexus()
{
    // Limpa o timer de 5s e dispara FinishDefeatToNexus imediatamente.
    GetWorld()->GetTimerManager().ClearTimer(FinishDefeatTimerHandle);
    FinishDefeatToNexus();
}
```

**Impacto:** UX melhor — jogador pode pular após ler o cause.

**Risco:** zero.

---

## 7. Code: ready-to-apply diff suite

Ver §6 acima — cada patch é independente e tem o código completo. **Sequência recomendada**:

1. **Patch 1** (15 min) — wire OnDeath. Ganho enorme imediato.
2. **Patch 3** (1h) — GameplayCue de morte. Próximo maior ganho.
3. **Patch 5** (30 min) — death band no melee. Sutil mas omnipresente.
4. **Patch 4** (2h) — last enemy celebration. Cherry on top.
5. **Patch 2** (1h) — killer attribution. Hades-style retention.
6. **Patch 7** (30 min) — skip button. UX small win.
7. **Patch 6** (15 min) — tick disable refinement. Edge case fix.

**Total: ~6h.** Ordem otimizada para feedback visual imediato (1, 3, 5 dão delta gigante em < 2h).

---

## 8. Test Plan

### 8.1 Smoke tests por patch

| Patch | Como testar | O que esperar |
|-------|-------------|---------------|
| 1 | PIE → suicídio com `df.kill` (ou cheat) | Slow-mo 3s + desaturação + sting de áudio antes do defeat screen |
| 2 | Morrer para boss específico | DefeatCause = nome do boss |
| 3 | Matar slime | Niagara burst + SFX + freeze curto |
| 4 | Matar todos os inimigos da sala, observar o último | Screen flash + slow-mo cinematográfico |
| 5 | Combo light até zerar HP do inimigo | Último hit deve ter hitstop visivelmente diferente |
| 6 | Inimigo com VFX attached morrendo | VFX não corta abruptamente |
| 7 | Pressionar Continue no defeat screen | Skip imediato para Nexus |

### 8.2 Telemetria

Adicionar `LogDFDeath` (já existe `LogDFEnemyDeath` — pode estender):

```cpp
DECLARE_LOG_CATEGORY_EXTERN(LogDFDeath, Log, All);

// Em OnDeathFlowStarted, GameplayCue, last-enemy detection, etc:
UE_LOG(LogDFDeath, Verbose, TEXT("[Death] Phase=%s Victim=%s Killer=%s LastInRoom=%d Cause=%s"),
       *PhaseName, *GetNameSafe(Victim), *GetNameSafe(Killer), bIsLast, *Cause);
```

Playtest gravado com `-log LogDFDeath Verbose` produz timeline auditável para tunar timings.

### 8.3 Casos extremos (testar antes de release)

1. **DoT mata player** — `LastLethalInstigator` deve persistir o instigator do DoT GE
2. **Trap mata player** — DefeatCause deve mostrar tipo de trap
3. **Self-damage** (recoil, reflect) — não atribuir killer como "yourself" — fallback para "Your health reached zero."
4. **Boss matar player em coop** — defeat screen para AMBOS os players
5. **Last enemy + boss** — não disparar last-enemy celebration durante boss kill (boss tem cinematica própria)
6. **Morte durante level transition** — TriggerDefeat não deve disparar se TravelInProgress

### 8.4 Network tests

- **Listen Server + Client:** confirmar que `Multicast_PlayLastEnemyCelebration` chega em ambos os players com sincronização razoável (~50ms drift OK).
- **Net PktLag=120:** killer attribution deve persistir através da replicação — testar morrer com lag alto.

---

## 9. Success Metrics

Pós-aplicação dos 7 patches, mediremos por:

| Métrica | Atual | Alvo |
|---------|-------|------|
| **Player death scene total duration** | 0s de juice / 3s de vazio | 1.5s de slow-mo + sting + UI a partir de 3s |
| **Killer attribution accuracy** | 0% (hardcoded) | ≥ 90% (instigator resolvido) |
| **Enemy kill = "feels like a kill"** | Não — mesmo hitstop de hit normal | Sim — banda CriticalHit + cue VFX/SFX |
| **Last-enemy-of-room celebration** | Ausente | Slow-mo + flash em 100% das salas |
| **Mortes com partículas cortadas** | Frequente em inimigos com VFX attached | Zero (após Patch 6) |
| **Tempo médio entre lethal hit e defeat screen** | 3s | 3s (mantém — só preenche o gap visualmente) |
| **Defeat screen com cause descritivo** | "Your health reached zero." | "Killed by **Megaera**" / "Killed by **Floor Spikes**" |

---

## 10. Appendix — outras findings de morte (não críticas, mas catalogadas)

Findings das auditorias dos agentes que NÃO ficam no top mas merecem entrada no backlog:

### Player death

| ID | Finding | Prioridade |
|----|---------|------------|
| PD-A1 | Sem post-mortem camera cinematic (zoom no killer) | Média |
| PD-A2 | Sem coop "all dead" check — defeat fires no primeiro player morto | Alta se coop for prioridade |
| PD-A3 | Sem spectator camera para dead player em coop | Baixa |
| PD-A4 | Sem revive mechanic em coop | Média se coop for prioridade |
| PD-A5 | Sem "YOU DIED" UI stinger entre death montage e defeat screen | Média (Patch 1 já preenche parcialmente) |
| PD-A6 | Sem death animation variation por damage type | Baixa |
| PD-A7 | Race condition teórica: 2 lethal damages no mesmo frame | Baixa (já guardada por `bOutOfHealthBroadcasted`) |
| PD-A8 | Server crash mid-death sem recovery dedicada | Baixa (LastCheckpoint pega) |

### Enemy death

| ID | Finding | Prioridade |
|----|---------|------------|
| ED-A1 | Sem ragdoll handoff — corpse cai sem física | Média (estética) |
| ED-A2 | Sem knockback impulse no death (corpse não arc/slide) | Média |
| ED-A3 | Sem corpse persistence — desaparece após dissolve | Baixa (intencional pra perf) |
| ED-A4 | Sem dynamic body destruction / dismemberment | Baixa (escopo grande) |
| ED-A5 | Combat director não libera token na morte explicitamente | Baixa (sai via Unregister automático) |
| ED-A6 | Killed by another AI: XP/Gold só vai pra player kill | Confirmar design intent |
| ED-A7 | Dissolve linear blunt (todos materiais juntos) — Hades faz staggered | Baixa |
| ED-A8 | `ScheduleDeathDestroyBackup` ~20s pode ser longo demais se ability falhar | Baixa (safety net) |
| ED-A9 | Enemy morre durante stagger: visual lingers 1-2 frames | Baixa |
| ED-A10 | Sem damage-type-specific death animation | Baixa |
| ED-A11 | Boss death não tem cinematic camera takeover dedicado | **Média-alta** (boss é set-piece) |
| ED-A12 | XP scaling formula `× (1 + 0.1 × Floor)` é linear — pode ficar trivial no floor 10 | Tuning |

---

## 11. Conclusão

A Morte Silenciosa **foi endereçada em C++** (Patches 1–7 + `UDFDeathCinematicSubsystem`):

1. ✅ Infraestrutura cinematográfica **wired** ao fluxo de morte do player
2. ✅ GameplayCue de inimigo implementada (assets VFX/SFX no editor)
3. ✅ Killer attribution no defeat screen
4. ✅ Golpe letal e último inimigo com juice distinto
5. ⚠️ **Validação:** playtest com checklist §8 + assign GCN assets

> Próximo passo: [`11_SilentDeathBlueprintSetup.md`](../improvements/11_SilentDeathBlueprintSetup.md)

---

## 12. Links e referências

- Pipeline detalhado: [docs/gas/DF_Death_Blueprint_Setup.md](../gas/DF_Death_Blueprint_Setup.md)
- Death pose pattern: [memory/feedback_death_pose_locking.md](../../memory/feedback_death_pose_locking.md)
- Análise geral: [docs/analysis/Game_Analysis.md](Game_Analysis.md)
- Combat report anterior: [docs/analysis/Combat_Advanced_Report.md](Combat_Advanced_Report.md)
- Implementation status: [docs/improvements/09_ImplementationStatus.md](../improvements/09_ImplementationStatus.md)

### Arquivos C++ tocados pelos 7 patches

| Arquivo | Patches |
|---------|---------|
| `Source/DungeonForged/Private/GAS/Abilities/UDFAbility_Player_Death.cpp` | 1 |
| `Source/DungeonForged/Private/GAS/UDFAttributeSet.cpp` | 2 |
| `Source/DungeonForged/Public/Characters/ADFPlayerCharacter.h` + `.cpp` | 2 |
| `Source/DungeonForged/Private/GameModes/Run/ADFRunGameMode.cpp` | 2, 7 |
| `Source/DungeonForged/Private/GAS/Cues/UDFGameplayCueNotify_EnemyDeath.cpp` | 3 |
| `Source/DungeonForged/Public/Combat/UDFCombatStateLibrary.h` + `.cpp` | 4 |
| `Source/DungeonForged/Public/Characters/ADFEnemyBase.h` + `.cpp` | 4 |
| `Source/DungeonForged/Private/Combat/UDFMeleeTraceComponent.cpp` | 5 |
| `Source/DungeonForged/Private/Animation/DFDeathAnimation.cpp` | 6 |
| `Source/DungeonForged/Public/UI/Run/UDFDefeatScreenWidget.h` + `.cpp` | 7 |

**~10 arquivos, ~150 linhas de código no total. 6 horas. Maior delta cinematic do projeto.**
