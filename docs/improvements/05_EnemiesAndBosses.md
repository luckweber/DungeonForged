# 05 — Enemies & Bosses

> **Objetivo:** transformar combate de "swing-or-spam" em **leitura de inimigo** — cada inimigo tem uma assinatura, telegraphs claros, e fraquezas exploráveis.

---

## Sumário rápido

| Eixo | Atual | Alvo |
|---|---|---|
| Enemy archetypes | 1-2 tipos por andar | 4-6 archetypes rotativos |
| Group AI | independent | role-based (Tank/Skirmisher/Caster) |
| Telegraphs | algumas montages | 100% das abilities têm tell visual + audio |
| Elite tier | enum existe, sem bonus | HP 2.5×, dmg 1.5×, 1 ability extra |
| Boss phases | 3 + enrage | manter + vulnerability windows |
| Boss telegraphs | warning decals em alguns | 100% das phase abilities |
| Death visual | corrigido nesta PR | manter |

---

## 1. Enemy Archetypes — `[CODE/ASSET]`

### 1.1 Tabela master

Cada enemy row em `DT_Enemies` ganha um campo `EnemyArchetype`:

```cpp
UENUM(BlueprintType)
enum class EDFEnemyArchetype : uint8
{
    Grunt,           // baseline melee
    Tank,            // HP++, slow, taunt aura
    Skirmisher,      // fast, kiting, weak HP
    Caster,          // ranged spells, fragile
    Berserker,       // low HP, high dmg, leaps
    Healer,          // priest-style, heals allies
    Spawner,         // summons minions
    Shielder,        // blocks frontal damage
    Sniper,          // long-range projectile, slow windup
    Bomber,          // suicide explosion on death
};

USTRUCT(BlueprintType)
struct FDFEnemyTableRow : public FTableRowBase
{
    // ... existente ...
    UPROPERTY(EditAnywhere) EDFEnemyArchetype Archetype = EDFEnemyArchetype::Grunt;
    UPROPERTY(EditAnywhere) FGameplayTagContainer AISignatureTags;  // tells, behaviors
};
```

### 1.2 Comportamentos por archetype

| Archetype | HP × | Speed × | Damage × | Special |
|---|---|---|---|---|
| Grunt | 1.0 | 1.0 | 1.0 | baseline (3-hit melee combo) |
| Tank | 2.0 | 0.7 | 1.2 | `State.AggroAura` força threat em 800cm |
| Skirmisher | 0.6 | 1.4 | 0.8 | dash backward após hit; kite |
| Caster | 0.7 | 0.9 | 1.5 (spell) | range 1500cm, cast time 1.2s (interruptible) |
| Berserker | 0.8 | 1.3 | 1.6 | leap attack (warning decal); engages first |
| Healer | 0.6 | 0.9 | 0.5 | heals nearest ally per 6s; priority kill |
| Spawner | 1.2 | 0.6 | 0.7 | summons 2 grunts per 15s (cap 4) |
| Shielder | 1.5 | 0.8 | 1.0 | front 90° block, vulnerable from sides |
| Sniper | 0.8 | 0.7 | 2.0 (single shot) | line-of-sight required; 2s windup |
| Bomber | 0.5 | 1.1 | n/a | explosion on death (telegraphed 1.5s) |

### 1.3 Behavior Tree por archetype

Hoje cada enemy tem `AIBehaviorTree` da row. Sugestão: **5-6 BTs reusáveis** por archetype, não 1 por inimigo.

```
BT_Grunt.uasset          → grunt + tank + shielder
BT_Kite.uasset           → skirmisher + sniper + bomber
BT_Caster.uasset         → caster + healer
BT_Spawner.uasset        → spawner
BT_Berserk.uasset        → berserker
```

`FDFEnemyTableRow::AIBehaviorTree` aponta para um destes. Reduz manutenção e garante consistência de role.

### 1.4 Tells visuais por archetype

Cada archetype tem **um destaque visual sutil** para leitura imediata:

| Archetype | Visual marker |
|---|---|
| Tank | aura outline grossa azul (`PostProcess > Outline`) |
| Caster | glow azul nas mãos quando casting |
| Berserker | tint vermelho no corpo + saliva (Niagara) |
| Healer | aura verde + flutuação leve (idle Z bob) |
| Spawner | crachá pulsante em cima |
| Shielder | shield prop visível, normal map metálico |
| Sniper | scope glint (Niagara) quando aiming |
| Bomber | fuse spark contínuo (Niagara) |

Player aprende: "verde = healer mata primeiro", "vermelho = berserker abre com leap".

---

## 2. Group AI — `[CODE]`

### 2.1 Problema

Atualmente cada inimigo é independente — 5 inimigos = 5 chargers no player simultaneamente = chaos.

### 2.2 Solução: Combat Director

Adicionar **`UDFCombatDirectorSubsystem`** (WorldSubsystem) que coordena papéis:

```cpp
class UDFCombatDirectorSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    void RegisterEnemy(ADFEnemyBase* Enemy);
    void UnregisterEnemy(ADFEnemyBase* Enemy);

    /** Returns true se o enemy pode atacar agora (token sistema). */
    bool RequestAttackToken(ADFEnemyBase* Enemy);
    void ReleaseAttackToken(ADFEnemyBase* Enemy);

    /** Max enemies attacking simultaneously. */
    UPROPERTY(EditAnywhere, Category="Director")
    int32 MaxAttackTokens = 2;   // [CONFIG]

private:
    TArray<TWeakObjectPtr<ADFEnemyBase>> ActiveAttackers;
    TArray<TWeakObjectPtr<ADFEnemyBase>> Registered;
};
```

### 2.3 Uso no BT

Antes de qualquer `BTTask_MeleeAttack` ou `BTTask_RangedAttack`, fazer `Service` que consulta o Director:

```cpp
// UDFBTService_RequestAttackToken
void TickNode(...)
{
    ADFEnemyBase* Self = ...;
    auto* Director = World->GetSubsystem<UDFCombatDirectorSubsystem>();
    BB->SetValueAsBool("HasAttackToken", Director->RequestAttackToken(Self));
}

// Decorator: only attack se HasAttackToken == true
```

Resultado: **max 2 inimigos atacam o player simultaneamente**, outros fazem strafing/reposicionar.

### 2.4 Role priority

Caster e Sniper têm **prioridade** sobre Grunt para token attack (eles têm cast time = não bloqueia melee). Tank tem **prioridade alta** para forçar threat.

```cpp
int32 GetPriority(EDFEnemyArchetype A)
{
    switch (A) {
        case EDFEnemyArchetype::Tank:       return 100;
        case EDFEnemyArchetype::Sniper:     return 90;
        case EDFEnemyArchetype::Caster:     return 80;
        case EDFEnemyArchetype::Berserker:  return 70;
        default:                            return 50;
    }
}
```

### 2.5 Aggro / threat

Adicionar `UDFThreatComponent` no player. Damage dealt by player to enemy = `Threat += damage × 0.5`. Tanks fazem `Threat += 30/s` passive em vez de damage. Em room, **enemy persegue o player com mais threat acumulado**.

Em single player isso é trivial (sempre 1 player). Em co-op, **distribui aggro entre players**.

---

## 3. Elite tier — `[CODE/ASSET]`

`EEnemyTier::Elite` está no enum mas sem bonus implementado.

### 3.1 Bonificação

```cpp
void ADFEnemyBase::ApplyEliteModifiers()
{
    if (Tier != EEnemyTier::Elite) return;

    UDFAttributeSet* S = ASC->GetSet<UDFAttributeSet>();
    S->SetMaxHealth(S->GetMaxHealth() * 2.5f);
    S->SetHealth(S->GetMaxHealth());
    S->SetStrength(S->GetStrength() * 1.5f);
    S->SetArmor(S->GetArmor() * 1.3f);

    // Visual: outline + niagara aura
    if (EliteAuraNiagara) UNiagaraFunctionLibrary::SpawnSystemAttached(
        EliteAuraNiagara, GetMesh(), NAME_None, ...);
    if (UMeshComponent* M = GetMesh()) M->SetCustomDepthStencilValue(252);  // outline shader

    // Grant 1 extra ability
    if (EliteBonusAbility)
    {
        ASC->GiveAbility(FGameplayAbilitySpec(EliteBonusAbility, 1));
    }
}
```

### 3.2 Spawn rules

Por andar:
- Floor 1-3: 0% chance
- Floor 4-6: 1 elite garantido na sala "Elite"
- Floor 7-9: 2 elites (1 em room dedicada + 1 random)
- Floor 10: bosses (não elite)

### 3.3 Elite loot

Elites dropam:
- 1 item raridade Uncommon+ (garantido)
- 50-200 gold extra
- 25 XP extra

### 3.4 Elite music

Trigger `UDFMusicManagerSubsystem::SetState(EDFMusicState::Elite)` quando elite enters in player range 1500cm. Restore `Combat` quando elite dies ou exit range. Ver [doc 06](06_AudioMix.md#elite-trigger).

---

## 4. Boss phases — refinamentos `[CONFIG/CODE]`

**Onde:** [`Source/DungeonForged/Public/Boss/ADFBossBase.h:72`](../../Source/DungeonForged/Public/Boss/ADFBossBase.h#L72)

Atual: 3 phases (HP 0.6 / 0.3), enrage 120s. Manter base e adicionar:

### 4.1 Vulnerability windows <a id="vulnerability-windows"></a>

Após phase transition (stun 1.5s já existe), aplicar **`State.BossVulnerable`** por mais 2s adicionais:

```cpp
void ADFBossBase::OnPhaseTransitioned()
{
    PlayPhaseTransitionMontage();
    if (ASC)
    {
        // já vinha: stun 1.5s
        ASC->AddLooseGameplayTag(FDFGameplayTags::State_BossVulnerable, 1);
    }
    World->GetTimerManager().SetTimer(VulnTimer, [this]() {
        if (ASC) ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_BossVulnerable, 1);
        OnBossVulnerabilityEnded.Broadcast();
    }, 3.5f, false);  // 1.5 stun + 2 vuln
}
```

E na fórmula de dano:

```cpp
if (TargetTags.HasTag(FDFGameplayTags::State_BossVulnerable))
{
    Final *= 1.5f;   // +50% dano em vulnerability window
}
```

Combat text mostra "VULNERABLE!" + golden hue durante a janela.

### 4.2 Telegraph cinematic

Para abilities devastadoras (PhaseTransitionSlam, MeteorStrike), adicionar **camera cut sequence**:

```cpp
void UDFBossAbility_MeteorStrike::ActivateAbility(...)
{
    // 1. Camera zooms para o boss
    if (UDFCameraComponent* PCam = Player->FindComponentByClass<UDFCameraComponent>())
    {
        PCam->TriggerCinematicLock(BossActor, /*duration=*/0.5f);
    }
    // 2. SFX: low rumble crescente
    UGameplayStatics::PlaySoundAtLocation(this, MeteorWindupSFX, BossLoc);
    // 3. Vignette / saturation
    if (UDFScreenEffectsComponent* FX = Player->FindComponentByClass<UDFScreenEffectsComponent>())
    {
        FX->FlashScreen(FLinearColor(1.f, 0.5f, 0.f, 0.1f), 0.5f, 0.4f);
    }
    // 4. Decal warning (já existe)
    SpawnMeteorWarning();
    // 5. Delay impact
    World->GetTimerManager().SetTimer(ImpactHandle, ...);
}
```

Player aprende: "tela ficou laranja + zoom no boss = AOE chegando".

### 4.3 Enrage timer adaptativo

```cpp
UPROPERTY(EditAnywhere, Category="Boss")
float EnrageTimer_FirstFight = 180.f;   // [CONFIG] 1ª vez deste boss
UPROPERTY(EditAnywhere, Category="Boss")
float EnrageTimer_Normal = 120.f;       // [CONFIG] padrão
UPROPERTY(EditAnywhere, Category="Boss")
float EnrageTimer_Heat = 90.f;          // [CONFIG] heat 5+

float ResolveEnrageTime() const
{
    if (UDFRunManager* RM = World->GetGameInstance()->GetSubsystem<UDFRunManager>())
    {
        const int32 BossKills = RM->GetBossKillCount(BossArchetype);
        const int32 Heat = RM->GetCurrentHeat();
        if (BossKills == 0) return EnrageTimer_FirstFight;
        if (Heat >= 5) return EnrageTimer_Heat;
    }
    return EnrageTimer_Normal;
}
```

### 4.4 Enrage telegraph

10s antes do enrage, **avisar visualmente**:

```cpp
if (TimeLeft < 10.f && !bWarnedEnrage)
{
    bWarnedEnrage = true;
    if (UDFScreenEffectsComponent* FX = Player->FindComponentByClass<UDFScreenEffectsComponent>())
    {
        FX->FlashScreen(FLinearColor(1.f, 0.2f, 0.f, 0.15f), 1.f, 0.5f);
    }
    // HUD bossbar piscando vermelho
    OnEnrageWarning.Broadcast(10.f);
}
```

### 4.5 Phase abilities — confirmar

`PhaseAbilities[NewPhase - 2]` libera abilities por fase. Confirmar que:
- Phase 1 (100-60% HP): 2-3 abilities baseline
- Phase 2 (60-30%): + 1 ability nova
- Phase 3 (< 30%): + 2 abilities, including signature

E que abilities **não tocam todas ao mesmo tempo** — cooldowns staggered no ASC.

### 4.6 Boss intro

Quando player entra na sala do boss:
1. Camera cinematic 4s (orbita o boss).
2. Letterbox 16:9 → 21:9 (UI Vertical Bands fade in).
3. Boss name reveal (`BossDisplayName` em UMG, scale + fade).
4. Boss roar (montage + SFX).
5. Music: `BossLayer` ramp up.
6. Player input released.

Já tem todos os ingredientes? Confirmar em `ADFBossTriggerVolume::OnPlayerEntered`.

---

## 5. Boss abilities — design checklist `[CONFIG]`

Para cada boss ability, validar:

- [ ] **Tell visual** (≥ 0.5s antes do hit) — decal, telegraph VFX, ou animação clara.
- [ ] **Tell audio** distinto (low rumble / high whine / scream).
- [ ] **Escape route** — sempre há ≥ 1 forma de evitar (sidestep, dodge, range out).
- [ ] **Damage cap** — não one-shot do player (max 60% HP por hit, exceto enrage).
- [ ] **Recovery window** ≥ 1.0s onde boss não pode usar outra ability.
- [ ] **Cooldown** ≥ 4s entre repetições da mesma ability.
- [ ] **Camera shake** apropriado (BossSlam para impacts grandes).
- [ ] **Niagara** com inner/outer radius (já tem `Multicast_BossLocalAttackFX`).
- [ ] **Cinematic moment** para signature ability da phase 3.

### 5.1 Audit do roster atual

| Ability | Tell visual | Tell audio | Escape | Cap | Recovery | CD | Status |
|---|---|---|---|---|---|---|---|
| GroundSlam | ⚠ overlap só | ❌ | ✅ sidestep | ? | ? | ? | revisar |
| MeteorStrike | ✅ decal | ⚠ verify | ✅ move out | ? | ? | ? | verify |
| VoidBarrier | ❌ none | ❌ | n/a (defensive) | n/a | n/a | n/a | add visual |
| TerrorShout | ⚠ shout windup | ⚠ verify | ⚠ range out 800cm | ? | ? | ? | telegraph improve |
| ChargeAttack | ⚠ roar pre | ✅ | ✅ sidestep | ? | ✅ vulnerable 1s | ? | mantém |
| SummonMinions | ❌ | ❌ | n/a | n/a | n/a | n/a | add intro |
| PhaseTransitionSlam | ✅ cinematic | ✅ | n/a (forced) | controlled | n/a | once | mantém |
| EnragePulse | ✅ aura | ⚠ verify | ✅ outrange | n/a | persistent | n/a | mantém |

Sem audit completo, criar test scene `L_BossPlayground` para validar 1 a 1.

---

## 6. Boss roster — adicionar variedade `[CONTENT]`

Sugerido 4 bosses distintos (ver [doc 04](04_RunMechanics.md#boss-roster)):

| Boss | Archetype | Phase 1 | Phase 2 | Phase 3 | Enrage |
|---|---|---|---|---|---|
| **Necromancer Lord** | Caster | Frost projectiles | + Summon skeletons | + Meteor / void | Constant void pulse |
| **Iron Knight** | Melee tank | Sword swings | + Shield bash combo | + Berserk leaps | +50% dmg + speed |
| **Hydra Twin** | Dual | Coordinated melee | Phase swap (immune rotation) | Synced AOEs | Both enrage |
| **Voidcaller** | Elemental | Element rotation (fire/ice/lightning) | Element fusion | Reality tear (room-wide telegraphed) | Permanent vortex |

Cada um precisa de:
- Skeletal mesh + AnimBP
- 4-6 montages (idle, walk, attack 1-3, phase transition, enrage, death)
- 4-7 Niagara effects (per ability + intro)
- Boss music tema (3 layers: phase 1/2/3)
- 6-8 SFX (roars, attacks, foley)
- Death loot table

**Estimativa:** ~3-4 semanas por boss completo (com art outsource), 1-2 com art reuse.

---

## 7. Hit reaction — feedback do inimigo `[CONFIG/CODE]`

Já existe `UDFHitReactionComponent`. Confirmar:

- [ ] Hit reaction dispara em **100% dos hits** (não cancelado por outra montage trivialmente).
- [ ] **Severity escala** com damage: Light (<30), Heavy (30-60), Stagger (60-90), Knockback (≥90).
- [ ] **Stunned tag** se damage > StaggerThreshold (30) — 1.5s sem ação.
- [ ] Hit reaction **não interrompe** ability sendo executada (caster termina cast mesmo levando hit leve).
- [ ] **Resistance to stun** após N hits — diminishing returns para evitar perma-stun lock.

### 7.1 Diminishing returns

```cpp
class UDFHitReactionComponent : public UActorComponent
{
    UPROPERTY(EditAnywhere) float StunImmunityWindow = 1.5f;
    UPROPERTY(EditAnywhere) int32 StunBeforeImmunity = 3;

    float LastStunTime = -1.f;
    int32 RecentStunCount = 0;
};

bool ShouldApplyStun(float Damage)
{
    if (Damage < StaggerThreshold) return false;
    const float Now = World->GetTimeSeconds();
    if (Now - LastStunTime < StunImmunityWindow) RecentStunCount++;
    else RecentStunCount = 0;
    LastStunTime = Now;
    return RecentStunCount < StunBeforeImmunity;
}
```

Após 3 stuns em 1.5s, inimigo fica **imune a stun por 2s** (visual: aura branca leve).

---

## 8. Patrol e wandering — `[CODE]`

`PatrolPoints` existe. Sugestão de polish:

### 8.1 Idle behavior

Quando out of combat, inimigos:
- Patrulham seus `PatrolPoints` em loop.
- Cada ponto: pausa 2-4s (random) com `IdleMontage` (look around / scratch / inspect).
- Detectam player via `AIPerception` (1500cm sight, 800cm hearing).

### 8.2 Lost track

Após perder LOS do player por 5s, retornam ao patrol gradualmente:
- 0-3s: investigam last known position
- 3-5s: walk back to patrol
- 5s+: full patrol mode

### 8.3 Alert chain

Quando inimigo A detecta player, **chama allies em 600cm** (broadcast `Event.Enemy.Alert`). Allies recebem `LastKnownPlayerLocation` e juntam-se ao combate.

```cpp
void ADFEnemyBase::OnPlayerDetected(AActor* Player)
{
    LastKnownPlayer = Player;
    EnterCombatState();

    // Alert allies
    TArray<AActor*> Allies;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADFEnemyBase::StaticClass(), Allies);
    for (AActor* A : Allies)
    {
        if (A == this) continue;
        if (FVector::Dist(GetActorLocation(), A->GetActorLocation()) > 600.f) continue;
        Cast<ADFEnemyBase>(A)->OnAlertedByAlly(Player, GetActorLocation());
    }
}
```

Cria momentos "ops, fui visto" → 3 inimigos vêm correndo.

---

## 9. Death loot & XP — `[CONFIG]`

Para cada archetype, gold/XP range diferente:

| Archetype | Gold | XP |
|---|---|---|
| Grunt | 5-15 | 5 |
| Tank | 15-30 | 10 |
| Skirmisher | 10-20 | 8 |
| Caster | 15-25 | 12 |
| Berserker | 12-22 | 10 |
| Healer | 20-35 | 15 |
| Spawner | 20-40 | 18 |
| Elite (any) | × 4 base | × 3 base |
| Mini-boss | 100-300 | 50 |
| Boss | 500-1500 | 150 |

Tunable por `FDFEnemyTableRow::GoldDropMin/Max/ExperienceReward`.

---

## 10. Checklist de "pronto"

- [ ] Enum `EDFEnemyArchetype` adicionado, `FDFEnemyTableRow` atualizada.
- [ ] 6+ behaviors implementados (Grunt, Tank, Caster, Berserker, Healer, Spawner).
- [ ] Visual markers (outline / aura / glow) por archetype.
- [ ] `UDFCombatDirectorSubsystem` limita 2 attackers simultâneos.
- [ ] Elite tier aplica HP 2.5×, dmg 1.5×, +1 ability extra.
- [ ] Boss vulnerability window 2s aplica +50% dmg taken.
- [ ] Boss telegraphs (visual + audio + cinematic) em 100% das phase abilities.
- [ ] Enrage timer adaptativo (180/120/90 por contexto).
- [ ] Enrage warning 10s antes (flash + bar pulse).
- [ ] Hit reaction tem DR de stun (3 em 1.5s = immunity 2s).
- [ ] Alert chain entre allies em 600cm.
- [ ] Boss roster expandido para ≥ 2 bosses únicos (curto prazo) / 4 (longo prazo).

---

## Apêndice — leitura de combate

Após estas mudanças, em uma sala com 5 inimigos o player deve **ler** algo como:

1. "Healer verde no fundo — kill priority"
2. "Tank azul puxando aggro — bloquear/dodge"
3. "Caster com glow — interromper cast"
4. "Berserker leap incoming — sidestep right"
5. "Grunt restante — finisher"

Sem essa leitura, combate vira "swing tudo até morrer". O archetype + tells visuais resolvem isso.
