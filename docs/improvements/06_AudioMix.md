# 06 — Audio & Music Mix

> **Objetivo:** áudio que **comunica estado de combate** sem precisar olhar a HUD. Música que escala com tensão; SFX que diferencia hits por banda; mix coerente.

---

## Sumário rápido

| Eixo | Atual | Alvo |
|---|---|---|
| Music states | 7 enum, 3 layers | Manter + intensity tiers dentro de Combat |
| Elite music | enum existe, sem trigger | Auto-trigger via combat director |
| Boss music | crossfade 2s | + enrage variant, + intensity por phase |
| SFX banding | flat | 4 tiers por damage |
| Footsteps | per montage | per surface (PhysicalMaterial) |
| Mix submix | desconhecido | confirmar 5 submixes |
| Ambient | desconhecido | per-floor ambient bed |
| Stinger | death | + victory, + boss intro, + level-up |

---

## 1. Music States — refinement `[CODE]`

**Onde:** [`Source/DungeonForged/Public/Audio/UDFMusicManagerSubsystem.h`](../../Source/DungeonForged/Public/Audio/UDFMusicManagerSubsystem.h)

Estados atuais (7 enum): `Idle`, `Exploration`, `Combat`, `Elite`, `Boss`, `Victory`, `Death`.

### 1.1 Intensity tiers dentro de Combat

Adicionar **subdivisão de Combat** baseada em # de inimigos:

```cpp
UENUM(BlueprintType)
enum class EDFMusicState : uint8
{
    Idle,
    Exploration,
    CombatLow,       // 1-2 inimigos
    CombatHigh,      // 3+ inimigos (ou elite presente)
    Elite,           // elite engagement (acima de high)
    BossPhase1,
    BossPhase2,      // phase transition
    BossPhase3,      // últimas batidas
    BossEnrage,      // enrage active
    Victory,
    Death,
};
```

### 1.2 Lógica de transição

```cpp
void UDFMusicManagerSubsystem::EvaluateCombatIntensity()
{
    if (CurrentState >= EDFMusicState::BossPhase1) return;  // boss music já governa

    const int32 ActiveEnemies = CountEnemiesEngagedWithLocalPlayer(/*radius*/ 2000.f);
    const bool bElitePresent = HasEliteEnemyInRange(/*radius*/ 1500.f);

    EDFMusicState NewState;
    if (ActiveEnemies == 0)        NewState = EDFMusicState::Exploration;
    else if (bElitePresent)        NewState = EDFMusicState::Elite;
    else if (ActiveEnemies >= 3)   NewState = EDFMusicState::CombatHigh;
    else                           NewState = EDFMusicState::CombatLow;

    if (NewState != CurrentState) SetState(NewState);
}
```

Chamado:
- A cada 2s (timer).
- Imediato em `OnEnemySpawned` / `OnEnemyDied`.

### 1.3 Layer scheme

```
Track de cada floor:
┌─ Base layer (ambient pad, sempre tocando)
├─ Combat low (drums + bass, gain 0/1)
├─ Combat high (+ lead, gain 0/1)
├─ Elite layer (variant intense, gain 0/1)
└─ Boss layer (separate composition)

Crossfade entre layers via volume modulation (não cut). 2s default.
```

### 1.4 Per-floor music

Sugestão: cada **floor type** (Cripta / Catacumbas / Profundezas) tem seu próprio track set:

```cpp
USTRUCT(BlueprintType)
struct FDFFloorMusicSet
{
    UPROPERTY(EditAnywhere) USoundBase* AmbientBase;
    UPROPERTY(EditAnywhere) USoundBase* CombatLowLayer;
    UPROPERTY(EditAnywhere) USoundBase* CombatHighLayer;
    UPROPERTY(EditAnywhere) USoundBase* EliteLayer;
};

// FDFDungeonFloorRow
UPROPERTY(EditAnywhere) FDFFloorMusicSet MusicSet;
```

Reforça identidade visual+audio dos bioma sets.

---

## 2. Elite music trigger — `[CODE]` <a id="elite-trigger"></a>

### 2.1 Detection

```cpp
bool UDFMusicManagerSubsystem::HasEliteEnemyInRange(float Radius) const
{
    AActor* P = GetLocalPlayer();
    if (!P) return false;

    TArray<AActor*> Enemies;
    UGameplayStatics::GetAllActorsOfClass(P->GetWorld(), ADFEnemyBase::StaticClass(), Enemies);
    for (AActor* E : Enemies)
    {
        const ADFEnemyBase* En = Cast<ADFEnemyBase>(E);
        if (!En || En->HasDied()) continue;
        if (En->GetEnemyTier() != EEnemyTier::Elite) continue;
        if (FVector::Dist(P->GetActorLocation(), En->GetActorLocation()) > Radius) continue;
        return true;
    }
    return false;
}
```

`UGameplayStatics::GetAllActorsOfClass` é caro — chamar do timer 2s, não cada frame.

### 2.2 Trigger via delegate

Alternativa mais elegante: `ADFEnemyBase` broadcasts:

```cpp
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEliteEngaged, ADFEnemyBase*);

// no BeginPlay do elite, se entrou em combat:
if (Tier == EEnemyTier::Elite) {
    if (auto* Music = World->GetSubsystem<UDFMusicManagerSubsystem>())
        Music->NotifyEliteEngaged(this);
}

// no OnDeath:
if (Tier == EEnemyTier::Elite) {
    if (auto* Music = World->GetSubsystem<UDFMusicManagerSubsystem>())
        Music->NotifyEliteDefeated(this);
}
```

`UDFMusicManagerSubsystem` mantém set de elites ativos, set vazio = volta ao state anterior.

### 2.3 Crossfade timing

Padrão 2s. Para enter Elite: **1.2s** (mais imediato — player precisa saber que mudou). Para exit: 2s (suave).

---

## 3. Boss music — phase variants `[CODE/ASSET]`

### 3.1 Stems por phase

Cada boss tem **3 stems** musicais (phase 1, 2, 3) + 1 enrage variant:

```cpp
USTRUCT(BlueprintType)
struct FDFBossMusicSet
{
    UPROPERTY(EditAnywhere) USoundBase* IntroSting;       // 4s, plays during cinematic intro
    UPROPERTY(EditAnywhere) USoundBase* Phase1Layer;      // 100-60% HP
    UPROPERTY(EditAnywhere) USoundBase* Phase2Layer;      // 60-30%
    UPROPERTY(EditAnywhere) USoundBase* Phase3Layer;      // <30%
    UPROPERTY(EditAnywhere) USoundBase* EnrageLayer;      // overlay durante enrage
    UPROPERTY(EditAnywhere) USoundBase* DefeatedSting;    // 4s, plays on boss death
};

// ADFBossBase
UPROPERTY(EditAnywhere) FDFBossMusicSet MusicSet;
```

### 3.2 Transitions

| Trigger | State change | Crossfade |
|---|---|---|
| Player enters boss room | Exploration → BossIntro (sting) | hard cut |
| Sting ends | BossIntro → BossPhase1 | 0.5s |
| Boss HP < 60% | BossPhase1 → BossPhase2 (phase transition mid) | 2s; **e** overlay phase transition stinger |
| Boss HP < 30% | BossPhase2 → BossPhase3 | 2s |
| Enrage triggers | + EnrageLayer overlaid | 1s ramp |
| Boss defeated | * → Victory (stinger) | hard cut |

### 3.3 Heart-rate effect

Phase 3 (HP < 30%): **tempo da música acelera 10%** (`PitchMultiplier = 1.1`) — sensação de heart-rate elevado, finale.

```cpp
if (CurrentState == EDFMusicState::BossPhase3)
{
    if (BossLayerAudioComponent) BossLayerAudioComponent->SetPitchMultiplier(1.1f);
}
```

---

## 4. SFX banding — `[CODE/ASSET]` <a id="sfx-banding"></a>

### 4.1 Hit tier por damage

```cpp
USoundBase* UDFSoundLibrary::PickHitSound(float Damage, EDFDamageType Type)
{
    int32 Tier = 0;
    if (Damage >= 30.f) Tier = 1;
    if (Damage >= 80.f) Tier = 2;
    if (Damage >= 150.f) Tier = 3;

    switch (Type) {
        case EDFDamageType::Slash:   return SlashHits[Tier];
        case EDFDamageType::Blunt:   return BluntHits[Tier];
        case EDFDamageType::Pierce:  return PierceHits[Tier];
        case EDFDamageType::Magic:   return MagicHits[Tier];
        default:                     return DefaultHits[Tier];
    }
}
```

Sound assets necessários:
- `SFX_Hit_Slash_T0/T1/T2/T3` (4)
- `SFX_Hit_Blunt_T0/T1/T2/T3` (4)
- `SFX_Hit_Pierce_T0/T1/T2/T3` (4)
- `SFX_Hit_Magic_T0/T1/T2/T3` (4)

= 16 sons base. Adicionar **3 variantes por sound** (`Cue` random play) para 48 totais.

### 4.2 Crit layer

Crit = sound normal + **`SFX_CritStinger`** sobreposto.

```cpp
UGameplayStatics::PlaySoundAtLocation(this, HitSound, ImpactPoint);
if (bIsCrit) UGameplayStatics::PlaySoundAtLocation(this, CritStinger, ImpactPoint, /*volume=*/0.8f, /*pitch=*/1.0f);
```

### 4.3 Random pitch

`PlaySoundAtLocation` aceita pitch parameter. Usar `FMath::FRandRange(0.95f, 1.05f)` para evitar repetição auditiva exata.

```cpp
const float RandomPitch = FMath::FRandRange(0.93f, 1.07f);
UGameplayStatics::PlaySoundAtLocation(this, HitSound, ImpactPoint, 1.f, RandomPitch);
```

### 4.4 Damage taken — different sound

Quando **player** leva hit, som distinto + diferenciado por severity:

```cpp
EDFHitTaken { Light, Heavy, Critical };
SFX_PlayerHit_Light  → "thud" curto
SFX_PlayerHit_Heavy  → "thud" grosso + grunt vocal
SFX_PlayerHit_Critical → "shatter" + grunt forte + heartbeat increase
```

---

## 5. Footsteps per surface — `[CODE/ASSET]`

**Onde:** [`Source/DungeonForged/Public/Animation/UDFAnimNotify_FootStep.h`](../../Source/DungeonForged/Public/Animation/UDFAnimNotify_FootStep.h)

### 5.1 Surface detection

```cpp
void UDFAnimNotify_FootStep::Notify(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Anim)
{
    AActor* Actor = Mesh->GetOwner();
    if (!Actor) return;

    FHitResult Hit;
    const FVector Start = Actor->GetActorLocation();
    const FVector End = Start - FVector(0, 0, 200);
    FCollisionQueryParams Params;
    Params.bReturnPhysicalMaterial = true;
    Actor->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

    EPhysicalSurface Surface = SurfaceType_Default;
    if (Hit.PhysMaterial.IsValid())
        Surface = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

    USoundBase* Sound = UDFSoundLibrary::Get()->GetFootstepSound(Surface);
    if (Sound) UGameplayStatics::PlaySoundAtLocation(Actor, Sound, Hit.Location, FootstepVolume);
}
```

### 5.2 Surface map

Definir em `UDFSoundLibrary`:

```cpp
UPROPERTY(EditAnywhere, Category="Footsteps")
TMap<TEnumAsByte<EPhysicalSurface>, USoundBase*> FootstepSounds;
```

| Surface | Sound |
|---|---|
| Stone (default) | SFX_Foot_Stone |
| Wood | SFX_Foot_Wood |
| Metal | SFX_Foot_Metal |
| Sand | SFX_Foot_Sand |
| Water/Wet | SFX_Foot_Wet |
| Snow | SFX_Foot_Snow |
| Grass | SFX_Foot_Grass |

### 5.3 Run vs walk

Player corre = footsteps + 30% volume + 10% pitch up. Walking = baseline. Sneaking (crouch) = -50% volume.

```cpp
const float Vel = Actor->GetVelocity().Size2D();
const float VolMult = Vel > 400.f ? 1.3f : (Vel < 80.f ? 0.5f : 1.f);
```

---

## 6. Submix architecture — `[ASSET/CONFIG]`

Confirmar / criar 5 submixes em `Content/Audio/Submixes/`:

```
MainSubmix
├─ Music (master / layers)
├─ SFX
│   ├─ Combat (hits, abilities, projectiles)
│   ├─ Foley (footsteps, cloth, armor)
│   └─ Ambient (wind, drips, room tone)
├─ UI (button clicks, menus, notifications)
└─ Voice (boss roars, player grunts, NPC dialogue)
```

### 6.1 Gain defaults

| Submix | Default dB | Volume slider in menu |
|---|---|---|
| Music | -6dB | "Music Volume" |
| Combat | -3dB | "SFX Volume" |
| Foley | -10dB | "SFX Volume" |
| Ambient | -15dB | "Ambient Volume" |
| UI | -6dB | "UI Volume" |
| Voice | -3dB | "Voice Volume" |

Cada slider de menu chama `UAudioSettings::SetSubmixVolume` ou via `USoundClass`.

### 6.2 Ducking

Quando UI menu opens (pause / inventory), **duck Music por -6dB** automaticamente:

```cpp
void UDFMusicManagerSubsystem::OnPauseMenuOpened()
{
    SetSubmixVolumeDb(MusicSubmix, -6.f, /*fade=*/0.3f);
}
void UDFMusicManagerSubsystem::OnPauseMenuClosed()
{
    SetSubmixVolumeDb(MusicSubmix, 0.f, /*fade=*/0.3f);
}
```

Quando boss intro plays, **duck Ambient por -10dB** durante 5s.

---

## 7. Ambient — `[ASSET]`

Cada floor type tem seu ambient bed (background loop):

| Floor type | Ambient | Random one-shots |
|---|---|---|
| Cripta (1-3) | distant drips, low wind | bat squeak, distant scream (every 60-120s) |
| Catacumbas (4-6) | torch crackle, hollow boom | rats, stone shift |
| Profundezas (7-9) | deep rumble, magma hiss | distant roar, gem chime |
| Boss room | silence with subtle pad | none (música domina) |

Implementar como `UAudioComponent` no nível (placement) ou via `UDFAmbientSubsystem` (procedural).

### 7.1 3D random one-shots

```cpp
class UDFAmbientSubsystem : public UWorldSubsystem
{
    UPROPERTY(EditAnywhere) TArray<USoundBase*> RandomOneShots;
    UPROPERTY(EditAnywhere) float MinInterval = 60.f;
    UPROPERTY(EditAnywhere) float MaxInterval = 120.f;
    UPROPERTY(EditAnywhere) float RadiusAroundPlayer = 1200.f;

    void TickEvery5s();  // chance roll, spawn at random location
};
```

Cria sensação de mundo vivo.

---

## 8. Stingers — `[ASSET]`

Stinger = SFX curto (1-4s) que **pontua** um momento. Manter discreto, alto impact.

| Trigger | Sound | Duration | Submix |
|---|---|---|---|
| Boss intro | `STG_Boss_Intro` | 4s | Music |
| Boss defeated | `STG_Boss_Victory` | 5s | Music |
| Phase transition | `STG_Phase_Shift` | 2s | Music |
| Phase 3 entered | `STG_Final_Phase` | 3s | Music |
| Player death | `STG_Player_Death` | 4s | Music |
| Player level up | `STG_LevelUp` | 1s | UI |
| Floor cleared | `STG_FloorCleared` | 2s | UI |
| Run victory | `STG_RunVictory` | 6s | Music |
| Heart rate (HP < 20%) | `SFX_Heartbeat_Loop` | loop | UI (low pass) |
| Critical hit dealt | `STG_Crit` | 0.3s | Combat |
| Second Wind triggered | `STG_SecondWind` | 1.5s | Combat |

---

## 9. Player vocal — `[ASSET]`

Cada classe tem **6 vocal lines mínimas**:

```
On dodge        (×3 variants, random)
On heavy attack (×3)
On hit taken    (×3 by severity)
On low HP       (1 loop labored breathing)
On level up     (×2)
On death        (×2)
```

Tone:
- Warrior: grunts grosso, esforço pesado
- Mage: incantation phrases sussurradas, breath of effort
- Rogue: low-volume murmurs, snake-like exhale

= 18 lines × 3 classes = ~54 sound files. Outsourceável (TTS para prototype, voice actor para release).

---

## 10. Music adaptive em co-op `[CODE]`

Em co-op, cada player tem seu próprio estado de música local (não sync).

- Player A está em boss room → Boss music local.
- Player B está em corredor → Exploration local.

Já que o `UDFMusicManagerSubsystem` é WorldSubsystem por player (LocalPlayer scope), cada um avalia o seu state. Confirmar.

Exceção: quando ambos estão no boss room, sync **estado boss phase** para evitar dessincronia em phase transitions (server-driven via boss replicate state).

---

## 11. Mix testing — `[TOOL]`

Criar `L_MixRange`:
- Player walking in 3 surface types
- 3 enemies de archetypes diferentes attacking
- Boss dummy com phase trigger button
- HUD com VU meter por submix
- Toggle: bypass music / SFX / ambient para isolar

Permite iteração rápida sem entrar em run.

---

## 12. Loudness target — `[CONFIG]`

Master loudness alvo: **-16 LUFS** (gaming standard, similar a YouTube/streaming).

Confirmar com `Audio Insights` (UE5.5+) ou `Reaper` external.

- Music: peaks -10 dB FS
- SFX impact: peaks -6 dB FS
- Foley: peaks -20 dB FS
- Voice: peaks -8 dB FS

Master compressor leve (-1dB threshold, ratio 2:1) para garantir que crit clusters não clipam.

---

## 13. Checklist de "pronto"

- [ ] CombatLow / CombatHigh / Elite states implementados.
- [ ] Auto-evaluation de combat intensity per timer 2s.
- [ ] Elite music trigger via delegate broadcasts.
- [ ] Boss music tem 3 phase stems + enrage overlay.
- [ ] Phase 3 toca com PitchMultiplier 1.1 (heart rate).
- [ ] SFX hits têm 4 tiers + 3 random variants.
- [ ] Crit layer (`STG_Crit`) toca em todo crit.
- [ ] Footsteps detectam surface via PhysicalMaterial.
- [ ] 5 submixes configurados com gain defaults.
- [ ] Ducking music quando pause menu.
- [ ] Ambient bed por floor type.
- [ ] Stingers para Boss Intro / Phase / Defeated / Victory / Death / LevelUp.
- [ ] Heart rate loop para HP < 20%.
- [ ] Player vocal lines mínimas (18 × 3 classes).
- [ ] Master loudness em -16 LUFS confirmado.

---

## Apêndice — referências de mix

- **Hades**: SFX layered (impact + flesh), música com tempo crescente nas chambers finais, distinct stingers entre rooms.
- **Returnal**: ambient extremamente rica, weapon SFX layered impact+tail, boss music spatial.
- **Dark Souls**: bosses com tema próprio que loop perfeito, exploration silence (uso ousado), footsteps com 5 surfaces.
- **Devil May Cry 5**: SFX crit/heavy com pitch alto super-saturated, music battle theme dinâmico.

DungeonForged já tem a base. Refinement = banding e adaptação.
