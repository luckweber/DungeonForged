# DungeonForged — Relatório Avançado do Sistema de Combate

> **Data:** 2026-05-20
> **Escopo:** auditoria técnica profunda de combate, combos, habilidades, juice, animação e replicação. Comparativo direto com referências AAA (Sekiro, God of War Ragnarok, DMC5, Nioh 2, Hi-Fi Rush, Returnal, Black Myth Wukong).
> **Objetivo:** identificar as lacunas concretas entre o estado atual e a sensação "fluida, responsiva, juicy, feeling AAA" que você quer.
> **Premissa de leitura:** este doc presume familiaridade com [`Game_Analysis.md`](Game_Analysis.md) e a série [`docs/improvements/`](../improvements/00_Overview.md).

---

## TL;DR — onde você está vs. AAA

**Posição atual: ~7.5/10** — engenharia bem acima da média indie, com **toda a infraestrutura de juice já existente em C++** (HitStop, camera shakes, screen FX, motion warping, anim notify states de cancel/parry/telegraph). O que falta para chegar a AAA **não é arquitetura nova**, é:

1. **Centralizar o dispatch de feedback** — hoje hitstop, shake, VFX, SFX, screen FX são chamados de lugares diferentes; AAA chama tudo de um único `OnHitConfirmed` event-driven.
2. **Fechar o "Projectile Parity Gap"** — projetéis (Knife, Fireball, Frostbolt, Arcane Missile) **pulam** o pipeline de hit reaction, hitstop e camera shake do melee. Isso é a lacuna mais grande percebida pelo jogador.
3. **Polir input-feel em 4 pontos**: buffer com pausa em hitstop, combo refresh on-hit, directional input (não velocity), commit-grade (frames não-canceláveis).
4. **Fechar 1 bug de replicação** confirmado no combo chain.
5. **Aplicar CooldownReduction** — a stat existe, é capturada, mas nunca é lida ao aplicar o cooldown. Inútil hoje.
6. **Adicionar juice em dodge** — i-frames funcionam mas não têm nenhum feedback de câmera/screen → sente passivo.

Com essas 6 frentes resolvidas (~3–5 semanas de trabalho focado), o feel sai de "ARPG indie polido" para "AAA-tier action RPG".

---

## 1. Mapa da arquitetura (referência rápida)

```
                        ┌───────────────────────────────────────┐
                        │  Input → DFInputConfig + Enhanced     │
                        └────────────────┬──────────────────────┘
                                         ▼
        ┌────────────────────────────────────────────────────────┐
        │           UDFComboComponent (núcleo do melee)          │
        │  ─ AttackInputBufferDuration = 0.15s                   │
        │  ─ ComboWindowDuration = 0.45s                          │
        │  ─ HeavyChargeThreshold 0.55s / MaxHeavy 1.4s          │
        │  ─ Directional resolve (vel.X / vel.Y / default)        │
        │  ─ Server_ChainMeleeComboStep RPC                      │
        └──────┬───────────────────────────┬─────────────────────┘
               ▼                           ▼
   ┌───────────────────────┐   ┌──────────────────────────────┐
   │ UDFMeleeAimComponent  │   │ GAS: UDFAbility_*MeleeSwing  │
   │ ─ ManualTarget > Lock │   │ ─ activate → PlayMontage     │
   │ ─ Lock > AI BB        │   │ ─ NetExec: LocalPredicted    │
   │ ─ Soft cone sweep     │   └──────────────┬───────────────┘
   │ ─ SnapYaw 15°         │                  ▼
   └──────────┬────────────┘     ┌─────────────────────────────┐
              │                  │ AnimMontage timeline:       │
              │                  │  [DF Melee Warp Target]     │
              │                  │  [AN_TraceStart]            │
              ▼                  │  [AN_TraceEnd]              │
   ┌──────────────────────┐      │  [DF Cancel Window]         │
   │ UMotionWarpingComp   │◀─────│  [AN_ComboWindowOpen]       │
   └──────────────────────┘      └────────────┬────────────────┘
                                              ▼
              ┌──────────────────────────────────────────────────┐
              │  UDFMeleeTraceComponent — server-only swept-sphere│
              │   ─ HitActorsThisSwing (dedup por swing)         │
              │   ─ BuildDamageSpec → SetByCaller Data.Damage    │
              │   ─ Damage GE → UDFDamageCalculation             │
              └───────────────────┬──────────────────────────────┘
                                  ▼
              ┌──────────────────────────────────────────────────┐
              │  UDFDamageCalculation (Exec)                     │
              │   ─ Physical: Str × 0.5 → Armor/(Armor+K)        │
              │   ─ Magic:    Int × 0.5 × (1+SpellAmp) → MR%     │
              │   ─ Crit roll (DR > 0.5)                          │
              │   ─ State.BossVulnerable × 1.5                   │
              └──────┬──────────────────────┬────────────────────┘
                     ▼                      ▼
   ┌──────────────────────────┐   ┌────────────────────────────┐
   │ UDFAttributeSet          │   │ UDFHitReactionComponent    │
   │  ─ PostGEExecute:        │   │  ─ Direction-aware montage │
   │    spawn combat text,    │   │  ─ Light/Heavy/Knockback   │
   │    DispatchHitReceived   │   │  ─ Damage.Source variants  │
   └──────────────────────────┘   └──────────────┬─────────────┘
                                                 ▼
                       ┌─────────────────────────────────────┐
                       │ UDFStaggerComponent                 │
                       │  ─ Sliding window (3s)              │
                       │  ─ Poise threshold → stun GE        │
                       └──────────────┬──────────────────────┘
                                      ▼
              ┌───────────────────────────────────────────────┐
              │  FEEL DISPATCH (FRAGMENTADO HOJE)             │
              │   ─ UDFHitStopSubsystem (Light/Heavy/Crit/    │
              │       BossSlam)                               │
              │   ─ UDFCameraShakeFunctionLibrary             │
              │   ─ UDFScreenEffectsComponent                 │
              │   ─ UDFCombatFeedbackLibrary (só HS + shake)  │
              │   ─ Trail VFX, Niagara impacts (per ability)  │
              └───────────────────────────────────────────────┘
```

**Subsistemas adjacentes em uso:**
- [`UDFCombatDirectorSubsystem`](../../Source/DungeonForged/Public/Combat/UDFCombatDirectorSubsystem.h) — tokens de ataque (max 2)
- [`UDFCombatStateLibrary`](../../Source/DungeonForged/Public/Combat/UDFCombatStateLibrary.h) — tag `State.InCombat`
- [`UDFElementalReactionSubsystem`](../../Source/DungeonForged/Public/GAS/Elemental/UDFElementalReactionSubsystem.h) — Melt/Electrocute/Steam
- [`UDFStaminaExhaustionComponent`](../../Source/DungeonForged/Public/Combat/UDFStaminaExhaustionComponent.h) — `State.Exhausted` por 0.5s
- [`UDFLockOnComponent`](../../Source/DungeonForged/Public/Camera/UDFLockOnComponent.h) — soft lock + Q/E cycle
- [`UDFCameraComponent`](../../Source/DungeonForged/Public/Camera/UDFCameraComponent.h) — Default 400 / Combat 300 / LockOn 350 arm length

---

## 2. Diagnóstico por domínio

### 2.1 Combo, input buffer, fluidez

**Pontos fortes (já tem):**
- Buffer em dois níveis: `bSwingInputBuffered` (durante swing) + `bComboInputBuffered` (durante combo window). Ambos sobrevivem a transições entre seções de montage.
- Combo data-driven via [`FDFComboStep`](../../Source/DungeonForged/Public/Data/DFDataTableStructs.h) (light + heavy finisher branch).
- Heavy attack em 3 tiers (tap = 2.2×, max charge 1.4s = 3.5× dmg).
- Variantes direcionais resolvidas em [`UDFComboComponent::ResolveDirectionalComboMontage`](../../Source/DungeonForged/Private/Combat/UDFComboComponent.cpp).
- Cancel window via `UANS_DFCancelWindow` adiciona tag `State.Combat.CancelWindow.Open` — heavy gateia, dodge sempre passa.

**Gaps AAA concretos:**

| # | Gap | Referência AAA | Impacto |
|---|-----|----------------|---------|
| C1 | **Input buffer = 150ms.** Sekiro/GoW usam 200–250ms. | Sekiro = 220ms; GoW Ragnarok = 200ms | ALTO — entradas no fim do recovery são engolidas |
| C2 | **Buffer não pausa durante hitstop.** Mundo congela mas o timer do buffer continua tickando no tempo dilatado. | DMC5/Sekiro pausam buffer durante hit-lag | MÉDIO — sensação de "comeu meu input" |
| C3 | **Combo não refresca on-hit.** Após o passo 3, sempre expira mesmo acertando inimigo fraco. | GoW/DMC5 estendem combo se acerta | ALTO — limita combos indefinidos em mobs |
| C4 | **Directional combos resolvem por `owner.Velocity.local`, não por stick input.** | Sekiro lê stick direto (mesmo parado), permitindo cross-up | MÉDIO — perde expressão de skill |
| C5 | **Sem "commit grade".** Todo swing é cancelável se cancel-window estiver aberta. | Dark Souls/GoW têm "no return frames" — recovery puro 60–120ms onde nada cancela | ALTO — golpes perdem "peso" |
| C6 | **`ComboChainMontageBlendInTime = 0.08s`** (hardcoded), `ComboChainMontageStopBlendOutTime = 0.0f`. | AAA: 120–200ms blend para fluidez visual; cuts instantâneos só em "stinger" frames | MÉDIO — chains parecem jerky |
| C7 | **Bug de replicação confirmado**: `bComboChainAdvancePending` é usado pelo `Server_ChainMeleeComboStep_Implementation` mas **não é replicado** (não está em [`UDFComboComponent.h:39`](../../Source/DungeonForged/Public/Combat/UDFComboComponent.h) `UPROPERTY(Replicated)`). Cliente chama `Server_ChainMeleeComboStep()` e na sequência `TryActivatePrimaryMeleeGameplayAbility()` localmente — race condition possível. | — | ALTO em multiplayer |

**Validação do bug C7:**
```
UDFComboComponent.h:39   bool bComboChainAdvancePending = false;        ← sem UPROPERTY(Replicated)
UDFComboComponent.cpp:28 SetIsReplicatedByDefault(true);                 ← component replica
UDFComboComponent.cpp:588 if (Owner && Owner->HasAuthority()) { … }     ← gateia escrita no server
```

Como o flag é só server-side mas o cliente já avança o passo localmente (`LockedComboActivationStep` esse sim é replicado), **em rede com lag > 100ms o cliente pode disparar a ability antes do server confirmar o step → step mismatch ou double-activation**. Reproduzir com `Net PktLag=120`.

---

### 2.2 Detecção de hit, hitbox, dano

**Pontos fortes:**
- Server-authoritative (`bServerOnlyTraces = true`).
- Multi-sphere sweep por tick com fallback hand+forward se sockets ficarem stale (>350cm do owner).
- Dedup por swing via `HitActorsThisSwing` (TWeakObjectPtr).
- Override per-swing de damage/knockback (`SetBaseDamageForNextSwing`).
- Hit reactions direcionais (front/back/left/right via dot 2D) e por banda (Light/Heavy/Knockback) + variantes por `Damage.Source.*` tag.
- Stagger com sliding window 3s + threshold + cooldown (4.5s) — bem desenhado.

**Gaps AAA concretos:**

| # | Gap | Impacto |
|---|-----|---------|
| H1 | **Sem interpolação de trace entre ticks.** Sample-at-tick puro. Em fast attacks (heavy = 0.10s impact frame), com 30 FPS = 3 amostras; com lag de servidor 50ms, pode haver miss entre amostras. | ALTO em ataques curtos |
| H2 | **Sphere única por swing.** Sem capsule (overhead chop), sem cone (whirlwind), sem multi-zone (shoulder + tip). | ALTO — todas armas parecem ter o mesmo "reach feel" |
| H3 | **Sem multi-hitbox.** Wide slash não tem 2–3 esferas overlapping (shoulder + meio + ponta) para garantir cobertura. | MÉDIO |
| H4 | **Sem body-part-specific reactions.** O hit impact point é gravado mas não é usado para selecionar montage. Cabeça vs. perna vs. torso = mesma reação. | MÉDIO — leitura visual genérica |
| H5 | **Projectile Parity Gap (MAIOR LACUNA DO PROJETO).** Knife/Fireball/Frostbolt/Arcane Missile fazem `OnHit()` direto, **não chamam** `UDFHitReactionComponent::OnHitReceived()`, **não disparam** `UDFHitStopSubsystem`, **não chamam** `UDFCameraShakeFunctionLibrary`. | **CRÍTICO** — projetéis sentem "desconectados" do melee |
| H6 | **Sem dedup de tempo em projétil.** Se a trajetória cruza o capsule duas vezes (passar reto), pode aplicar dano duplo. | BAIXO |
| H7 | **Sem damage-source tagging consistente em projétil.** Fireball lê Strength direto em vez de via SetByCaller. | BAIXO |

**Evidência do H5** (referências dos cpps):
- [`ADFKnifeProjectile.cpp:74-95`](../../Source/DungeonForged/Private/Combat/ADFKnifeProjectile.cpp) — aplica `PhysicalDamageEffect` + spawn local de VFX, sem hit reaction.
- [`DFFireballProjectile.cpp:48-107`](../../Source/DungeonForged/Private/Combat/DFFireballProjectile.cpp) — mesmo padrão.
- Comparar com [`UDFMeleeTraceComponent.cpp:1474`](../../Source/DungeonForged/Private/Combat/UDFMeleeTraceComponent.cpp) — chama `PlayImpactCosmeticsAt()` + `HitStopSubsystem::LightHit/HeavyHit/CriticalHit()` + `UDFCameraShakeFunctionLibrary::PlayLightHitOnOwner()` + `Hit->OnHitReceived()` após GE apply.

---

### 2.3 GAS — abilities, attributes, effects

**Pontos fortes:**
- AttributeSet limpo: 3 vitals + 6 primárias + 2 mitigação + 2 secondary offensive + utility — todas replicadas com REPNOTIFY_Always e clamps em `PreAttributeChange`.
- SecondWind rescue mechanic em `PostAttributeChange` (resgata em 25% HP se tag `State.Universal.SecondWindAvailable` ativa).
- Damage pipeline tem **single source of truth**: `UDFDamageCalculation` (Execution).
- Tag taxonomy excelente: `Ability.*`, `State.*`, `Event.*`, `Effect.*`, `Data.*` — hierárquica e consistente.
- 34+ abilities autorais bem diferenciadas por classe (Warrior melee/CC, Mage range/haste, Rogue mobility/DoT). Cada uma tem `CanActivateAbility` próprio + traces customizados.
- Passives auto-grant + auto-activate via `OnGiveAbility`. NetExecutionPolicy = ServerOnly nelas (corretíssimo).
- Elemental reactions (Melt / Electrocute / Steam) com affinity matrix por inimigo + GE optional.

**Gaps AAA concretos:**

| # | Gap | Impacto |
|---|-----|---------|
| G1 | **`CooldownReduction` attribute NUNCA é aplicado.** Existe em [`UDFAttributeSet.h`](../../Source/DungeonForged/Public/GAS/UDFAttributeSet.h), é capturado em `UDFDamageCalculation`, é setado por `TimeWarp` buff — mas o `ApplyCooldown` no [`UDFGameplayAbility.cpp`](../../Source/DungeonForged/Private/GAS/UDFGameplayAbility.cpp) aplica `UGE_Cooldown_Base` com `BaseCooldown` puro. Sem leitura da stat. **Stat inerte.** | ALTO — todo o sistema de CDR é placebo |
| G2 | **Sem Global Cooldown (GCD).** Cada ability tem cooldown próprio, sem layer global de 0.3–0.5s. Sente "spammy". | MÉDIO — questão de gosto, mas referências (Lost Ark, WoW, Diablo) usam |
| G3 | **Sem Status Resist / Tenacity.** Stun/Slow stackam sem diminishing returns. `Fortitude` é manual, não DR scaling. | MÉDIO — bosses ficam frágeis a CC chain |
| G4 | **Sem Lifesteal / SpellVamp** como atributo. TimeWarp + ManaShield + HealPotion são as únicas formas de healing in-combat. | MÉDIO |
| G5 | **Sem Dodge%/Block% como atributos.** Só shield buffs e i-frames hardcoded. | BAIXO — design intencional? Confirmar |
| G6 | **Damage event scattered.** Hit triggers ficam em 3 lugares: `PostGameplayEffectExecute` (combat text), `UDFPassivesGASEvents::DispatchHitReceived` (passive listeners), montage notifies (anim reactions). Sem `OnDamageDealt(Source, Target, Magnitude, Crit, Tags)` único delegate. | MÉDIO — replicar features novas vira shotgun de patches |
| G7 | **Sem rollback de prediction.** Cliente preditivamente checa cost/cooldown, server valida no `ApplyCooldown` — se server rejeita, sem rollback visível. | BAIXO até alguém abusar |
| G8 | **Ability cancel windows entre abilities.** Hoje cancel window vai só para heavy/dodge a partir do swing. Não há "chainear FrostBolt → ArcaneBarrage" via cancel window genérica. | MÉDIO — depth para builds combo-ability |

**Sugestão para G1 (fix de 5 min):**

```cpp
// UDFGameplayAbility.cpp — em ApplyCooldown(), antes do BuildSpec:
const float Raw = BaseCooldown;
float CDR = 0.f;
if (UDFAttributeSet const* AS = ASC->GetSet<UDFAttributeSet>())
{
    CDR = AS->GetCooldownReduction();
}
const float CDRCapped = FMath::Min(CDR, 0.4f);              // [CONFIG] cap em 40%
const float Excess    = FMath::Max(0.f, CDR - 0.4f);
const float ExtraDR   = Excess / (Excess + 0.6f) * 0.1f;    // assintótico 0.5
const float Effective = Raw * (1.f - (CDRCapped + ExtraDR));
// usa Effective no SetByCaller Data.Cooldown
```

---

### 2.4 Game feel — juice, câmera, motion warping, lock-on

**Pontos fortes (impressionante para projeto solo):**
- `UDFHitStopSubsystem` com **real-world time** (FPlatformTime, immune à time dilation) — 4 bandas (Light 0.06s / Heavy 0.10s / Critical 0.14s / BossSlam 0.20s) com exclusão de actor.
- 4 camera shakes (LightHit, HeavyHit, BossSlam, Explosion) com playback scale + accessibility intensity.
- `UDFScreenEffectsComponent` com vignette + chromatic + flash + saturation + grain + blur + death slowmo + low-health pulse.
- Motion warping plenamente integrado: `UDFMeleeAimComponent` resolve target (ManualTarget > LockOn > AI BB > soft cone) e `UANS_DFMeleeWarp` aplica warp via `UMotionWarpingComponent`.
- 4 AnimNotifyStates de combate (`Cancel`, `Parry`, `MeleeWarp`, `EnemyTelegraph`) cobrindo o ciclo windup → impact → recovery.
- Lock-on com Q/E cycle, soft search 1500cm/60°, smooth camera lag (0.12s).
- Trail VFX com prune por tag (`WeaponTrailVFX`).
- Combat text pool de 30, abreviação k/M, crit escala 1.4×.

**Gaps AAA concretos:**

| # | Gap | Impacto |
|---|-----|---------|
| F1 | **`UDFCombatFeedbackLibrary` só dispatcha HitStop + Shake.** VFX, SFX, screen effects, combat text, anim notify são chamados de **lugares diferentes** (ability BPs, anim notifies, attribute set, hit reaction). Não há `OnHitConfirmed(Band, Magnitude, Location, Direction, Tags)` único. | **CRÍTICO** — qualquer feature nova de feedback vira shotgun de patches |
| F2 | **HitStop não escala com damage magnitude.** Banda é selecionada por % de HP perdido mas duration/dilation são fixos por banda. | MÉDIO — finishers de boss não "frizam" mais que um light hit do crit |
| F3 | **Sem dodge juice.** I-frames funcionam (0.35s) mas **zero** FOV punch, chromatic, vignette, lag ramp, shake. Dodge sente burocrático. | ALTO — referência (Returnal) usa FOV pop + chromatic + slight desat |
| F4 | **Sem parry/block shake catalog.** Só Hit/Heavy/Critical/Knockback. Parry perfeito poderia ter shake staccato curto + flash. | MÉDIO — parry sente menos especial do que deveria |
| F5 | **Lock-on sem Z-anchor para inimigos aéreos.** Z-pinning ausente; voadores ou pulando ficam fora do framing. | BAIXO até voadores aparecerem |
| F6 | **Camera sem FOV punch.** `LerpLocalPlayerFOV` é stub no `UDFCameraComponent.h:82`. Sprint, dodge, hit, dash não têm punch. | MÉDIO |
| F7 | **Trail VFX não pooled.** Reativação é OK mas em combos rápidos pode gerar spawn extra. | BAIXO |
| F8 | **Sem attack-type tag nos triggers de hitstop/shake.** "Heavy slash" vs "light jab" vs "magic spell" usam mesma banda. Sem identidade por arma. | MÉDIO |
| F9 | **Lag entre vignette e hitstop.** Screen effect fade independente do hitstop end time — no light hit, vignette dim já terminou antes do unfreeze. | BAIXO |
| F10 | **Sem finisher cinematic chain.** Finishing blow é passivo (threshold 20% HP). Sem multi-hit input prompt, sem cinematic grab. | MÉDIO — feature signature ausente |
| F11 | **Sem on-kill spectacle.** Enemy death = cosmetic montage + corpse. Sem screen bloom, sem time-stretch cascade no último kill da sala. | MÉDIO — Returnal/Hi-Fi Rush celebram |

**Para F1, um patch de centralização (referência):**

```cpp
// UDFCombatFeedbackLibrary.h — adicionar
USTRUCT(BlueprintType)
struct FDFHitConfirmedContext
{
    GENERATED_BODY()
    UPROPERTY() AActor* Instigator = nullptr;
    UPROPERTY() AActor* Victim = nullptr;
    UPROPERTY() FVector Location = FVector::ZeroVector;
    UPROPERTY() FVector Normal = FVector::UpVector;
    UPROPERTY() float Magnitude = 0.f;
    UPROPERTY() float DamagePercent = 0.f;
    UPROPERTY() bool bIsCrit = false;
    UPROPERTY() FGameplayTagContainer Tags;
    UPROPERTY() EDFHitFeedbackBand Band = EDFHitFeedbackBand::Light;
};

UFUNCTION(BlueprintCallable, Category="DF|Feel")
static void DispatchOnHitConfirmed(UObject* WorldContext, const FDFHitConfirmedContext& Ctx);
// Implementação interna: HitStop + Shake + ScreenFX + Niagara impact + SFX layer + CombatText.
// Toda chamada de hit (melee, projectile, AoE, status proc) passa por aqui.
```

E mapear cada ponto de chamada atual (`UDFMeleeTraceComponent::ApplyDamageToTarget`, `ADFKnifeProjectile::OnHit`, `DFFireballProjectile::OnHit`, `DFBlizzardZone::TickDamage`, etc.) para usar essa função única.

---

### 2.5 Animação — notifies, montages, layers

**Pontos fortes:**
- 4 AnimNotifyStates de combate bem desenhadas, com side-effects via GAS events e loose tags. Replicação automática por anim system.
- 4 AnimNotifies não-state (`AN_ComboWindowOpen`, `AN_TraceStart`, `AN_TraceEnd`, `AN_SendGameplayEvent`) com fallback de timer no [`UDFMeleeTraceComponent::ScheduleAuthorityTraceWindowsFromMontage`](../../Source/DungeonForged/Private/Combat/UDFMeleeTraceComponent.cpp) — robusto contra LocalPredicted ability não disparar notifies no server.
- Armed/unarmed layers per-weapon ([`Player_Armed_Unarmed_Layers.md`](../animation/Player_Armed_Unarmed_Layers.md)).
- Footstep notify com surface detection.
- Trail VFX notify com auto-prune.
- Death pose lock pattern (já confirmado em [`feedback_death_pose_locking.md`](../../memory/feedback_death_pose_locking.md)).

**Gaps AAA concretos:**

| # | Gap | Impacto |
|---|-----|---------|
| A1 | **Sem `AN_DodgeCancelWindow` montagens.** Dodge cancela qualquer coisa hoje porque NÃO é gated. Ok como design, mas perde a opção de "dodge só na recovery" (Souls-style). | BAIXO (escolha) |
| A2 | **Sem `AN_AbilityCancelWindow` cross-ability.** Já documentado em [`03_Combat.md §6`](../improvements/03_Combat.md). | MÉDIO |
| A3 | **Sem `AN_HitConfirm` notify** para sincronizar feedback no exato frame de impacto da montage (hoje feedback dispara quando trace acerta, que pode ser 1 frame off da montage). | BAIXO |
| A4 | **Sem `AN_RootMotionScaleOverride`** durante recoveries (para "drag forward" controlado sem usar root motion completo). | BAIXO |
| A5 | **Blend in/out de combo montage hardcoded** em 0.08s/0.0s. Designer não consegue tunar por montage. | MÉDIO |

---

### 2.6 Replicação — server authority, prediction, anti-cheat surface

**Pontos fortes:**
- `bServerOnlyTraces = true` no melee — exploits client impossíveis.
- ASC em PlayerState (Mixed mode); em Enemy (Minimal). Correto para action ARPG.
- Loose tags do ANS (Cancel, Parry, Telegraph) são locais — não tem custo de replication.
- GAS events via `SendGameplayEventToActor` propagam corretamente.
- `Client_HitFeedback` no `ADFPlayerCharacter` para feedback local em co-op.
- Boss `Multicast_BossLocalAttackFX` com inner/outer radius — atenuação correta.
- Projéteis com `HasAuthority()` em todos os `OnHit`.
- Stagger `bServerAuthoritative = true`.
- HitReaction `if (!GetOwner()->HasAuthority()) return` no `OnHitReceived`.

**Gaps AAA concretos:**

| # | Gap | Impacto |
|---|-----|---------|
| N1 | **Bug C7 confirmado** (`bComboChainAdvancePending` não-replicado, race condition). | ALTO em multiplayer |
| N2 | **Sem client prediction de hit registration.** Server-only traces significam que feedback de hit pode atrasar ~RTT/2. Para Listen Server invisível; para Client com 80ms ping = 40ms de delay percebido. | MÉDIO — masking via hit-feedback predito (sem aplicar dano) seria AAA |
| N3 | **Sem rollback de Resource cost.** Documentado em G7. | BAIXO |
| N4 | **Sem replicação do `bComboHeavyFinisherPending`** — designar property como replicada se cliente precisa exibir HUD diferenciado durante a janela. | BAIXO |
| N5 | **Decisões de random event em co-op:** sistema atual "first lock-in wins" pode frustrar. Sugerido em [`Game_Analysis.md §10`](Game_Analysis.md). | MÉDIO em co-op |

---

## 3. Prioridade — matriz impacto × esforço

> Ordenado por **impacto/horas**. Cada linha referencia o gap ID das seções 2.x.

### Tier S — ganho percebido enorme, esforço baixo (fazer essa semana)

| # | Ação | Gaps | Esforço | Por quê |
|---|------|------|---------|---------|
| S1 | **Fix C7** — marcar `bComboChainAdvancePending` como `UPROPERTY(Replicated)` + adicionar em `GetLifetimeReplicatedProps` | C7 | 15 min | Fix de bug confirmado de replicação em combo chain — silencioso hoje, vira regressão visível com latency mais alta |
| S2 | **Aplicar CooldownReduction** com cap 0.4 + DR | G1 | 30 min | Stat hoje inerte; atributos `TimeWarp` e equipment ficam funcionais |
| S3 | **Aumentar input buffer 0.15 → 0.20s** | C1 | 5 min (tunable) | Mais responsivo, menos "comeu input" — exporta `UPROPERTY(EditAnywhere)` |
| S4 | **Combo refresh on-hit** — adicionar 0.30s de extensão do `ComboWindowExpireTime` quando trace acerta inimigo | C3 | 1h | DMC5/GoW pattern; chains indefinidos em mobs fracos = sensação de "rolando" |
| S5 | **Dodge juice** — chromatic pulse 0.30/0.6 + flash azul + camera shake suave + FOV punch (95→101→95 em 0.35s) | F3, F6 | 2h | Maior delta percebido para o investimento — dodge vira "sente bem" |
| S6 | **Buffer pause em hitstop** — checar `UDFHitStopSubsystem::IsCurrentlyDilated()` no buffer update e segurar timestamp do buffer enquanto dilatado | C2 | 1h | Inputs durante hit-lag preservados |

**Total Tier S: ~5h, ~80% do "ganho percebido" para o jogador casual.**

### Tier A — alto impacto, esforço médio (próximas 2 semanas)

| # | Ação | Gaps | Esforço | Por quê |
|---|------|------|---------|---------|
| A1 | **Centralizar `OnHitConfirmed`** — refatorar `UDFCombatFeedbackLibrary` para receber `FDFHitConfirmedContext` único e despachar HitStop + Shake + ScreenFX + Niagara + SFX + CombatText | F1, G6 | 6–8h | Próxima feature de feedback (ex.: Niagara per element) vira 1 linha em vez de 5 |
| A2 | **Projectile Parity** — Knife/Fireball/Frostbolt/Arcane Missile passam pelo mesmo `OnHitConfirmed` que melee. Adicionar chamada a `UDFHitReactionComponent::OnHitReceived()` após GE apply em cada projétil | H5, H6, H7 | 4h | Projetéis sentem `igualmente impactantes` quanto melee — fecha a maior dissonância do jogo |
| A3 | **Damage-magnitude HitStop scaling** — lerp `Duration` e `Dilation` dentro da banda baseado em % de MaxHP | F2 | 2h | Boss slam em max charge sente DIFERENTE de slam normal |
| A4 | **Attack-type tags em HitStop/Shake** — passar `FGameplayTag` (`Impact.Heavy.Slash`, `Impact.Light.Pierce`) e dispatch VFX/SFX por tag | F8, A3 | 4h | Identidade por arma — espada vs machado sentem diferente sem novo código |
| A5 | **`AN_AbilityCancelWindow` genérico** + tag-based — documentado em [`03_Combat.md §6`](../improvements/03_Combat.md) | G8, A2 | 4h | Builds combo-ability (FrostBolt → ArcaneBarrage chain) viram viáveis |
| A6 | **Directional input por stick** ao invés de velocity (`MovementInputVector` do CMC) | C4 | 2h | Skill expression real — cross-up no combo |
| A7 | **Commit-grade nas montages** — adicionar `AN_NoCancelFrames` que bloqueia cancel até liberar | C5 | 3h | Golpes ganham peso |
| A8 | **Status Resist attribute + DR para CC** | G3 | 4h | Bosses não morrem em chain de stuns |

**Total Tier A: ~30h, fecha ~90% das lacunas para AAA-feel.**

### Tier B — esforço maior, impacto especializado (mês 2)

| # | Ação | Gaps | Esforço |
|---|------|------|---------|
| B1 | **Trace interpolation** — sub-stepping no `TickTrace` (lerp socket positions entre amostras) | H1 | 6h |
| B2 | **Per-weapon trace shape** (capsule/cone) selecionado por weapon tag | H2 | 8h |
| B3 | **Multi-hitbox por swing** (lista de overlapping shapes) | H3 | 6h |
| B4 | **Body-part-specific reactions** — bone-name resolve no impact + montage map | H4 | 4h |
| B5 | **Stagger DR + per-attack tag multipliers** | "Charge" = 3× poise damage | 4h |
| B6 | **Passive poise regen** — regen rate por archetype | — | 2h |
| B7 | **Finisher cinematic chain** — multi-hit input prompt em threshold 20% HP | F10 | 16h |
| B8 | **On-kill spectacle** — slowmo + bloom + zoom no último kill da sala (detectar via `UDFCombatStateLibrary::OnRoomCleared`) | F11 | 8h |
| B9 | **Client prediction de hit feedback** (sem dano, só feedback) | N2 | 12h |
| B10 | **Lock-on Z-anchor** dinâmico para inimigos aéreos | F5 | 3h |
| B11 | **GCD (Global Cooldown) layer** opcional | G2 | 4h |
| B12 | **Lifesteal / Dodge% / Block% attributes** + integrações | G4, G5 | 8h |
| B13 | **Trail VFX pool** (pre-spawn 2–3 componentes por arma) | F7 | 2h |
| B14 | **Per-attack `AN_HitConfirm`** notify sincronizado com trace | A3 | 2h |

**Total Tier B: ~85h, completa polish AAA.**

---

## 4. Plano de execução — 4 semanas para feel AAA

```
┌─────────────────────────────────────────────────────────────────┐
│ SEMANA 1 — Tier S (5h) + start Tier A1                          │
│  Seg: S1, S2, S3 (45 min cada incluindo testes)                 │
│  Ter: S4 (combo refresh on-hit) + S5 (dodge juice) — 3h         │
│  Qua: S6 (buffer pause em hitstop) + começa A1                  │
│  Qui-Sex: A1 (OnHitConfirmed central) — 6-8h                    │
│  Checkpoint: gravar 2min de gameplay e comparar lado a lado     │
│              com versão pré-mudanças                            │
├─────────────────────────────────────────────────────────────────┤
│ SEMANA 2 — Tier A2-A4                                            │
│  Seg-Ter: A2 (Projectile Parity) — 4h                           │
│  Qua: A3 (Hit Stop scaling) — 2h                                │
│  Qui-Sex: A4 (attack-type tags) — 4h                            │
│  Checkpoint: testar com 3 armas diferentes (Sword, Axe, Dagger) │
│              — devem sentir distintas sem novos assets          │
├─────────────────────────────────────────────────────────────────┤
│ SEMANA 3 — Tier A5-A8                                            │
│  Seg-Ter: A5 (AbilityCancelWindow) — 4h                         │
│  Qua: A6 (directional input por stick) — 2h                     │
│  Qui-Sex: A7 (commit-grade) + A8 (StatusResist) — 7h            │
│  Checkpoint: playtest 30 min focado em combo expression         │
├─────────────────────────────────────────────────────────────────┤
│ SEMANA 4 — Polish + Tier B picks                                 │
│  Seg-Qua: B7 (Finisher cinematic chain) — 16h spread             │
│  Qui: B8 (on-kill spectacle) — 8h spread                         │
│  Sex: B13 (Trail pool) + B10 (Lock-on Z) — 5h                   │
│  Checkpoint: deliver build "AAA candidate"                      │
└─────────────────────────────────────────────────────────────────┘
```

**Critério de "AAA feel" alcançado:**
- [ ] Hit-confirmation latency < 50ms do input ao primeiro feedback A/V (medir com `LogDFFeel`).
- [ ] 100% dos hits (melee + projétil + AoE) disparam HitStop + Shake + Niagara + SFX layer + CombatText pelo mesmo caminho.
- [ ] Dodge sente "evasivo": FOV punch + chromatic visíveis, shake suave, sem delay perceptível.
- [ ] Combo de 3-hit dura 1.0–1.4s sem mash; combo refresha em hit e estende por +0.3s.
- [ ] Buffer aceita input no fim do recovery (0.20s); inputs durante hitstop não são engolidos.
- [ ] Cancel-into-ability funciona em pelo menos 5 abilities.
- [ ] Projétil acerta inimigo: hit reaction direcional + camera shake + hitstop disparam idênticos ao melee.
- [ ] Crit hit visualmente distinto: chromatic spike + shake forte + text dourado escala 1.4×.
- [ ] Net test (Listen + Client com Net PktLag=120ms): combo chain não double-activa nem mismatch step.

---

## 5. Validação contínua

### 5.1 Telemetria — adicionar `LogDFFeel`

Já existe `LogDFTuning`; adicionar `LogDFFeel` para auditar dispatch de juice:

```cpp
// DungeonForgedLog.h
DECLARE_LOG_CATEGORY_EXTERN(LogDFFeel, Log, All);

// Em DispatchOnHitConfirmed (depois do A1):
UE_LOG(LogDFFeel, Verbose,
       TEXT("[Hit] Band=%s Mag=%.1f Crit=%d Inst=%s Vic=%s Loc=%s Tags=[%s]"),
       *UEnum::GetValueAsString(Ctx.Band), Ctx.Magnitude, Ctx.bIsCrit,
       *GetNameSafe(Ctx.Instigator), *GetNameSafe(Ctx.Victim),
       *Ctx.Location.ToString(), *Ctx.Tags.ToStringSimple());
```

Playtest gravado com `-log LogDFFeel Verbose -LogDFTuning Verbose -LogGameplayCues Verbose` produz timeline auditável: pegar 60s de gameplay, calcular:
- Hits totais
- % com HitStop disparado (alvo: 100%)
- % com Camera Shake (alvo: ≥ 90%)
- Latency média input → primeiro log de feedback (alvo: < 50ms)

### 5.2 Cenário de teste — `L_CombatRange`

Documentado em [`00_Overview.md §C`](../improvements/00_Overview.md). Implementar como [`03_Combat.md §10`](../improvements/03_Combat.md):
- 3 training dummies HP infinito
- 1 elite dummy
- 1 boss dummy
- Console buttons: spawn N grunts, gold N, floor N, give all abilities, heal full, toggle inf stamina
- HUD com FPS + frame time + active montages + buffer state

Acessível via `open L_CombatRange` — iteração de feel em 1 minuto, não em uma run completa.

### 5.3 Network testing

- Listen Server + Client com `Net PktLag=120` `Net PktLagVariance=20`:
  - Combo chain LP → SLP (server confirma) sem double-activate.
  - Aim snap acontece no client primeiro (LocalPredicted) e server confirma.
  - HitStop é local-only (cliente A em hitstop não congela cliente B).
- Dedicated Server smoke: 4 players atacando em sequência, sem desync de cooldown.

---

## 6. Apêndice — Diffs de exemplo (não-aplicados)

### 6.1 Fix do bug C7 (replicação de combo)

```cpp
// UDFComboComponent.h — linha ~39
UPROPERTY(Replicated)
bool bComboChainAdvancePending = false;

// UDFComboComponent.cpp — adicionar em GetLifetimeReplicatedProps (criar se não existe)
void UDFComboComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UDFComboComponent, bComboChainAdvancePending, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UDFComboComponent, LockedComboActivationStep, COND_OwnerOnly);
}
```

### 6.2 Cooldown Reduction aplicado (G1)

```cpp
// UDFGameplayAbility.cpp — em ApplyCooldown(), antes do GE spec:
float EffectiveCooldown = BaseCooldown;
if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
{
    if (const UDFAttributeSet* AS = ASC->GetSet<UDFAttributeSet>())
    {
        const float CDR = FMath::Clamp(AS->GetCooldownReduction(), 0.f, 1.f);
        const float Cap = 0.4f;
        const float Hard = FMath::Min(CDR, Cap);
        const float Soft = (CDR > Cap)
            ? ((CDR - Cap) / ((CDR - Cap) + 0.6f) * 0.1f)
            : 0.f;
        EffectiveCooldown = BaseCooldown * (1.f - (Hard + Soft));
    }
}
// Aplica EffectiveCooldown como SetByCaller Data.Cooldown
```

### 6.3 Combo refresh on-hit (S4)

```cpp
// UDFComboComponent.cpp — adicionar método público
void UDFComboComponent::NotifyOwnerHitConfirmed(float ExtensionSeconds /*= 0.30f*/)
{
    if (!bComboWindowActive) return;
    if (UWorld* W = GetWorld())
    {
        const float NewExpire = W->GetTimeSeconds() + ExtensionSeconds;
        if (NewExpire > ComboWindowExpireTime)
        {
            ComboWindowExpireTime = NewExpire;
            UE_LOG(LogDFFeel, Verbose, TEXT("[Combo] Refresh on-hit +%.2fs"), ExtensionSeconds);
        }
    }
}

// UDFMeleeTraceComponent::ApplyDamageToTarget — após bAppliedDamage = true:
if (Owner && bAppliedDamage)
{
    if (UDFComboComponent* Combo = Owner->FindComponentByClass<UDFComboComponent>())
    {
        Combo->NotifyOwnerHitConfirmed(ComboRefreshOnHitSeconds /*UPROPERTY default 0.30f*/);
    }
}
```

### 6.4 OnHitConfirmed central (A1) — esqueleto

```cpp
// UDFCombatFeedbackLibrary.h
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFHitConfirmedContext
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<AActor> Instigator;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<AActor> Victim;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector Location = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector Normal = FVector::UpVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector HitDirection2D = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Magnitude = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float DamagePercent = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIsCrit = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTagContainer Tags;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EDFHitFeedbackBand Band = EDFHitFeedbackBand::Light;
};

UFUNCTION(BlueprintCallable, Category="DF|Feel", meta=(WorldContext="WorldContext"))
static void DispatchOnHitConfirmed(UObject* WorldContext, const FDFHitConfirmedContext& Ctx);
```

```cpp
// UDFCombatFeedbackLibrary.cpp — esqueleto da implementação
void UDFCombatFeedbackLibrary::DispatchOnHitConfirmed(
    UObject* WorldContext, const FDFHitConfirmedContext& Ctx)
{
    if (!WorldContext) return;
    UWorld* W = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
    if (!W) return;

    // 1. HitStop com magnitude scaling
    if (UDFHitStopSubsystem* HS = W->GetGameInstance()->GetSubsystem<UDFHitStopSubsystem>())
    {
        const float MagFactor = FMath::Clamp(Ctx.DamagePercent / 0.15f, 0.5f, 1.5f);
        HS->RequestHitStop(Ctx.Band, Ctx.Instigator, MagFactor);
    }

    // 2. Camera shake por banda + atenuação radial
    UDFCameraShakeFunctionLibrary::PlayBandShake(WorldContext, Ctx.Band, Ctx.Instigator);

    // 3. Screen effects no victim
    if (Ctx.Victim)
    {
        if (UDFScreenEffectsComponent* FX = Ctx.Victim->FindComponentByClass<UDFScreenEffectsComponent>())
        {
            FX->ApplyHitFromCombat(Ctx.Band, Ctx.DamagePercent, Ctx.Instigator);
            if (Ctx.bIsCrit) FX->ChromaticAberrationPulse(0.10f, 1.2f);
        }
    }

    // 4. Niagara impact por tag
    SpawnHitImpactByTags(W, Ctx);

    // 5. SFX layer (impact + tail + crit sting opcional)
    PlayHitSoundByTags(W, Ctx);

    // 6. Combat text
    if (UDFCombatTextSubsystem* CT = W->GetSubsystem<UDFCombatTextSubsystem>())
    {
        const ECombatTextType T = Ctx.bIsCrit ? ECombatTextType::Crit : ECombatTextType::Damage;
        CT->SpawnFloatingText(Ctx.Location + FVector(0,0,120), Ctx.Magnitude, T, Ctx.HitDirection2D);
    }

    UE_LOG(LogDFFeel, Verbose,
        TEXT("[OnHit] Band=%s Mag=%.1f Crit=%d Tags=[%s]"),
        *UEnum::GetValueAsString(Ctx.Band), Ctx.Magnitude, Ctx.bIsCrit,
        *Ctx.Tags.ToStringSimple());
}
```

E refatorar todas as chamadas atuais para usar essa função única — mapping mínimo:

| Local atual | Substituir por |
|-------------|---------------|
| `UDFMeleeTraceComponent::ApplyDamageToTarget` (cpp:1483-1510) | `DispatchOnHitConfirmed` |
| `ADFKnifeProjectile::OnHit` (cpp:74-100) | `DispatchOnHitConfirmed` |
| `DFFireballProjectile::OnHit` (cpp:48-107) | `DispatchOnHitConfirmed` |
| `DFFrostBoltProjectile::OnHit` | `DispatchOnHitConfirmed` |
| `DFArcaneMissileProjectile::OnHit` | `DispatchOnHitConfirmed` |
| `DFBlizzardZone::TickDamage` | `DispatchOnHitConfirmed` |

---

## 7. Conclusão

DungeonForged está **arquitetonicamente pronto para sentir AAA**. Toda a infraestrutura crítica existe — o que separa o jogo da meta é uma sequência de patches focados em:

1. **Centralização** (OnHitConfirmed → fim do feedback shotgun)
2. **Paridade** (projéteis no mesmo pipeline do melee)
3. **Refinamento de input-feel** (buffer maior, pause em hitstop, refresh on-hit, commit grade)
4. **Polish de dodge / cooldown / status** (estes mexem no nível visceral do feel)
5. **Um bug de replicação** que custa 15 minutos para resolver

Total estimado para **alcançar feel AAA percebido pelo jogador casual**: 35–50 horas focadas (Tier S + Tier A completos). Os ~85h de Tier B são polish e features signature.

> O maior risco hoje é continuar adicionando features verticais (mais habilidades, mais inimigos) antes de fechar essas lacunas horizontais de feedback. Toda nova feature herda os mesmos buracos — vale priorizar a fundação.

---

## 8. Próximos documentos sugeridos

- `docs/analysis/Combat_NetSim_Plan.md` — plano de testes com `Net PktLag` para validar S1 e tier A.
- `docs/analysis/OnHitConfirmed_Migration.md` — checklist de cada call site (melee + projéteis + AoE + status ticks) para refator de A1.
- `docs/improvements/11_FinisherSystem.md` — design completo da cinematic finisher chain (B7).
- `docs/analysis/Game_Balance_Tuning.md` — números pós-CooldownReduction-fix para todas as abilities.
