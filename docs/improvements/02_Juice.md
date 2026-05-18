# 02 — Juice

> **Definição prática:** juice = a soma de **microfeedbacks** que faz cada ação parecer impactante. Quando bem feito, o jogador não nota; quando falta, o jogo sente "morto".

DungeonForged já tem **toda a infraestrutura pronta** (`UDFHitStopSubsystem`, `UDFCameraShakes`, `UDFScreenEffectsComponent`, `UDFCombatTextSubsystem`). Este doc é sobre **disparar tudo nos momentos certos**, com banding consistente.

---

## Sumário rápido

| Sistema | Estado | Ação |
|---|---|---|
| Hit Stop | Implementado, sub-utilizado | **Centralizar dispatch por banda** |
| Camera Shake | 4 shakes legacy | Adicionar: Dodge, Sprint, Landing, DeathScreen, BossEnrage |
| Screen Effects | Cobertura excelente | Conectar Second Wind, Boss Intro, Phase Transition |
| Combat Text | Pool 30 | Crit visual (escala + shake), abreviar > 1000 |
| Niagara | Por ability | Hit Spark padronizado por elemento |
| SFX | Por som | Camadas (impact + tail), tier por dano |

---

## 1. Hit Stop — `[CODE]` <a id="hit-stop-dispatch"></a>

**Onde:** [`Source/DungeonForged/Public/FX/UDFHitStopSubsystem.h:29-39`](../../Source/DungeonForged/Public/FX/UDFHitStopSubsystem.h#L29)

```cpp
LightHit:    0.06s @ 0.05× dilation
HeavyHit:    0.10s @ 0.02×
CriticalHit: 0.14s @ 0.01×
BossSlam:    0.20s @ 0.0×   (clamp 0.0001)
```

Valores estão bons. **O problema é: onde está cada chamada?**

### 1.1 Dispatcher central

Criar helper estático em [`UDFCombatFeedbackTypes`](../../Source/DungeonForged/Public/FX/UDFCombatFeedbackTypes.h):

```cpp
// .h
UENUM(BlueprintType)
enum class EDFHitFeedbackBand : uint8
{
    None       = 0,
    Light      = 1,
    Heavy      = 2,
    Critical   = 3,
    BossSlam   = 4
};

UCLASS()
class DUNGEONFORGED_API UDFCombatFeedbackLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="DF|Feel", meta=(WorldContext="WorldContext"))
    static void DispatchHitFeedback(
        UObject* WorldContext,
        EDFHitFeedbackBand Band,
        AActor* Instigator,
        AActor* Victim,
        float DamagePercent = 0.f);
};

// .cpp
void UDFCombatFeedbackLibrary::DispatchHitFeedback(
    UObject* WorldContext, EDFHitFeedbackBand Band,
    AActor* Instigator, AActor* Victim, float DamagePercent)
{
    UWorld* W = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
    if (!W) return;
    UGameInstance* GI = W->GetGameInstance();
    if (!GI) return;
    UDFHitStopSubsystem* HS = GI->GetSubsystem<UDFHitStopSubsystem>();
    if (!HS) return;

    switch (Band)
    {
        case EDFHitFeedbackBand::Light:    HS->LightHit(Instigator);    break;
        case EDFHitFeedbackBand::Heavy:    HS->HeavyHit(Instigator);    break;
        case EDFHitFeedbackBand::Critical: HS->CriticalHit(Instigator); break;
        case EDFHitFeedbackBand::BossSlam: HS->BossSlam(Instigator);    break;
        default: break;
    }

    // Camera shake (radius-based)
    UDFCameraShakeFunctionLibrary::PlayBandShake(WorldContext, Band, Instigator);

    // Screen effects no victim apenas
    if (DamagePercent > 0.f)
    {
        if (UDFScreenEffectsComponent* FX = Victim ? Victim->FindComponentByClass<UDFScreenEffectsComponent>() : nullptr)
        {
            FX->DamageReceived(DamagePercent);
        }
    }
}
```

### 1.2 Mapping ação → banda

| Trigger | Banda |
|---|---|
| Melee combo hit 1, 2 | `Light` |
| Melee combo hit 3 (finisher) | `Heavy` |
| Heavy attack (novo) | `Heavy` |
| Ability comum (FrostBolt, ShieldBash) | `Light` |
| Eviscerate finisher (5 combo points) | `Critical` |
| Execute finisher (Warrior, < 20% HP) | `Critical` |
| Crit roll positivo no `UDFDamageCalculation` | upgrade band em +1 |
| Boss GroundSlam, MeteorImpact | `BossSlam` |
| Boss PhaseTransitionSlam | `BossSlam` (sem exclude — mundo todo congela 0.2s) |
| EnragePulse, Berserk activation | `Heavy` |

### 1.3 Tag-based escalation

No spec aplicado, se `Effect.Critical` está presente, o `UDFDamageCalculation` faz upgrade da banda automaticamente:

```cpp
// UDFDamageCalculation::Execute_Implementation, no fim:
if (Spec.CapturedSourceTags.GetSpecTags().HasTag(FDFGameplayTags::Effect_Critical))
{
    EDFHitFeedbackBand Out = (Band == EDFHitFeedbackBand::Light) ? EDFHitFeedbackBand::Critical
                                                                  : EDFHitFeedbackBand::Critical;
    // anexa ao context para HitReaction ler
}
```

### 1.4 Centralizar via `Client_HitFeedback`

Já existe: [`ADFPlayerCharacter::Client_HitFeedback`](../../Source/DungeonForged/Public/Characters/ADFPlayerCharacter.h). Só receber a `Band` no parâmetro e despachar localmente. **Mantém hit stop client-side em co-op** (cada player tem o seu).

---

## 2. Camera Shakes — `[ASSET] + [CODE]`

**Onde:** [`Source/DungeonForged/Public/FX/UDFCameraShakes.h`](../../Source/DungeonForged/Public/FX/UDFCameraShakes.h)

Atual: `LightHit`, `HeavyHit`, `BossSlam`, `Explosion`. Faltam:

### 2.1 Shakes a criar

```cpp
// Sutil — sentido mas não notado
UDFCameraShake_Sprint        // dur=∞, X/Y amp 0.4-0.6, freq 3-4 Hz
UDFCameraShake_Dodge         // dur 0.18s, X amp 2.0, Y amp 1.5, freq 25-30 Hz
UDFCameraShake_Landing       // dur 0.25s, Z amp 1.5 (1.5 × intensity), freq 18 Hz

// Médio — claramente percebido
UDFCameraShake_DeathBlow     // dur 0.6s, Pitch amp 1.5, Yaw amp 2.0, freq 4 Hz
UDFCameraShake_LowHealth     // dur 0.4s, kick lateral, pulsa quando HP < 20%

// Forte — set-piece
UDFCameraShake_BossEnrage    // dur 1.2s, build-up + sustain, Z+pitch
UDFCameraShake_PhaseTransition // dur 1.5s, queda crescente
UDFCameraShake_Earthquake    // dur 2s+, contínuo durante ability
```

### 2.2 Intensity scaling

`UDFCameraShakeFunctionLibrary::PlayShakeWithScale(Class, Scale)` que multiplica todas as amplitudes pelo `Scale`. Usado em:
- `LandingShake(intensity)` onde intensity = `FallHeight / 800` clamped 0..1
- `LowHealthShake(severity)` onde severity = `(0.2 - HpRatio) / 0.2` clamped 0..1

### 2.3 Atenuação por distância

Já existe em `Multicast_BossLocalAttackFX` (inner/outer radius). **Estender para todos os shakes não-locais:**

```cpp
UDFCameraShakeFunctionLibrary::PlayRadialShake(
    World, ShakeClass, Origin,
    /*InnerRadius=*/ 300.f, /*OuterRadius=*/ 1500.f, Falloff=2.f);
```

Garante que shake de boss longe não é tão forte quanto perto.

### 2.4 Slider de intensidade global

Em `FDFAccessibilitySettings`:

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin=0, ClampMax=1.5))
float CameraShakeIntensity = 1.f;  // [CONFIG]
```

`UDFCameraShakeFunctionLibrary::PlayShake*` multiplica `Scale × CameraShakeIntensity`. Default 1.0; 0 = desliga; 1.5 = exagerado.

---

## 3. Screen Effects — `[CODE]`

**Onde:** [`Source/DungeonForged/Public/FX/UDFScreenEffectsComponent.h`](../../Source/DungeonForged/Public/FX/UDFScreenEffectsComponent.h)

### 3.1 Triggers faltando

| Momento | Efeito sugerido |
|---|---|
| **Second Wind ativado** (`State.Universal.SecondWindAvailable`) | `FlashScreen(branco, 0.3s, 0.7)` + slow-mo global 0.4s @ 0.3× |
| **Boss intro** (`OnBossActivated`) | `Vignette` cinematic 0.5s + `Saturation -0.3` + letterbox UI |
| **Boss phase transition** | `Saturation +0.6` + `ChromaticAberrationPulse(0.5, 1.5)` 1.5s |
| **Boss enrage** | `Vignette` vermelho persistente + grain +0.3 |
| **Player death** | `Saturation -0.8` lento (0.8s) + `Vignette` 1.0 + `BlurAmount` 0.6 |
| **Pickup raro** | `FlashScreen(dourado, 0.2s, 0.4)` |
| **Crit hit dado** | `ChromaticAberrationPulse(0.10, 1.2)` |
| **Mana baixa** (Mage) | `Saturation -0.2` + tilt azulado quando mana < 20% |

### 3.2 Material parent — confirmar `[ASSET]`

`UDFScreenEffectsComponent` depende de um material `M_ScreenEffectsParent` com **scalar/vector params**:

```
VignetteIntensity     (scalar)
VignetteColor         (vector)
ChromaticAberration   (scalar)
BlurAmount            (scalar)
SaturationMult        (scalar)
FlashIntensity        (scalar)
FlashColor            (vector)
GrainAmount           (scalar)
```

**Confirmar que existe em `Content/DungeonForged/FX/Post/M_ScreenEffectsParent`** e que `MID_ScreenEffects` é instanciado em `BeginPlay`. Sem o material, todo o componente é no-op silencioso.

### 3.3 Performance

`UDFScreenEffectsComponent` deve usar **`PostProcess Material` com priority alta** e `BlendWeight 1.0` apenas quando algum effect está ativo. Quando todos os scalars voltam a 0, desligar o blend (`bEnabled = false`) para evitar fullscreen draw call permanente.

---

## 4. Combat Text — `[CODE/ASSET]` <a id="combat-text"></a>

**Onde:** [`Source/DungeonForged/Public/UI/Combat/UDFCombatTextSubsystem.h`](../../Source/DungeonForged/Public/UI/Combat/UDFCombatTextSubsystem.h)

Pool de 30 widgets, tipos `Damage/Crit/Heal/Miss/XP/Status`. Sólido. **Polishes:**

### 4.1 Crit visual

No widget `WBP_CombatText`:

```
Damage normal:  font size 28, white, no stroke
Crit:           font size 40, golden, stroke 2px black, horizontal shake 1.5s
Heal:           font size 28, green
Miss:           font size 22, gray italic
XP:             font size 20, blue, "+25 XP"
Status:         font size 18, colored by status (poison=green, burn=orange)
```

Adicionar **horizontal shake** via UMG animation tied a `Type == Crit`.

### 4.2 Abbreviation

Para evitar números enormes na tela:

```cpp
// UDFCombatTextSubsystem::SpawnFloatingText
FString FormatDamage(float Damage)
{
    if (Damage < 1000.f) return FString::Printf(TEXT("%d"), FMath::RoundToInt(Damage));
    if (Damage < 10000.f) return FString::Printf(TEXT("%.1fk"), Damage / 1000.f);
    return FString::Printf(TEXT("%dk"), FMath::RoundToInt(Damage / 1000.f));
}
```

### 4.3 Cluster / merge

Múltiplos hits no mesmo frame (Whirlwind, BlizzardStorm) viram uma chuva de números. Sugestão:

- Agrupar hits no mesmo alvo no mesmo frame.
- Mostrar "12 × 45" ou "540 (12 hits)" em um único widget.

```cpp
struct FCombatTextPending {
    AActor* Target;
    float TotalDamage;
    int32 HitCount;
    float ExpireTime;  // se passar 0.10s sem novo hit, spawn
};
```

### 4.4 Direção do spawn

Atualmente spawn em worldspace acima do alvo. Adicionar **drift por direção do hit** — se hit veio da direita, texto drifta para a direita 30cm enquanto sobe. Reforça leitura espacial.

```cpp
const FVector Drift = (HitDirection.GetSafeNormal2D()) * 30.f;
const FVector SpawnLoc = TargetLoc + FVector(0, 0, 100.f) + Drift;
```

---

## 5. Niagara — padronização por tipo de hit `[ASSET]`

Cada hit deve disparar **um sistema Niagara consistente** por categoria:

| Categoria | Sistema | Cor | Duração |
|---|---|---|---|
| Melee physical | `NS_HitImpact_Physical` | branco/amarelo | 0.3s |
| Melee crit | `NS_HitImpact_Crit` | dourado, sparks 2x | 0.6s |
| Heavy attack | `NS_HitImpact_Heavy` | branco com shockwave | 0.5s |
| Fire ability | `NS_HitImpact_Fire` | laranja, ember trail | 0.4s |
| Ice ability | `NS_HitImpact_Ice` | ciano, crystal burst | 0.4s |
| Lightning | `NS_HitImpact_Lightning` | branco-azul, arc | 0.3s |
| Arcane | `NS_HitImpact_Arcane` | roxo, fizz | 0.4s |
| Death blow | `NS_DeathBlow` | flash branco + dust 1.5s | 1.5s |

**Centralizar em `UDFCombatFeedbackLibrary::SpawnHitImpact(Type, Location, Normal)`** — todas as habilidades chamam pela enum, não por reference direta.

### 5.1 Decals de sangue

Já existe (8s lifespan no `Game_Analysis`). Confirmar:
- **Material com depth fade** (não Z-fighting em superfícies orgânicas).
- **Cleanup automático** após 8s (já tem).
- **Pool** — em uma sala com 10 inimigos abatidos, 50+ decals podem prejudicar perf. Manter pool máximo 32; o mais antigo é descartado.

---

## 6. SFX — `[ASSET]`

### 6.1 Camadas por hit

Cada hit deve ser **duas camadas**:

```
Impact (curto, transiente):  0-0.15s — definido por material da arma
  + Tail (corpo / energia):  0.05-0.4s — definido por tipo de dano
```

Exemplo Eviscerate:
- Impact: `SFX_Blade_Stab` (0.10s)
- Tail: `SFX_BleedSpray_Tail` (0.30s)

E **layer adicional para crit**:
- + `SFX_CritSting` (0.20s, alta freq)

### 6.2 Tier por dano

Atualmente todo hit usa o mesmo som. Sugiro tier:

```cpp
USoundBase* PickHitSound(float Damage)
{
    if (Damage < 30.f)  return SFX_Hit_Soft;
    if (Damage < 80.f)  return SFX_Hit_Medium;
    if (Damage < 150.f) return SFX_Hit_Hard;
    return SFX_Hit_Crushing;
}
```

Combinado com pitch shift `±0.08` random, evita repetição auditiva.

### 6.3 SFX de "low health"

Quando HP < 20%, tocar `SFX_Heartbeat_Loop` em `UDFAudioComponent` do player (não 3D, só 2D mix). Crossfade quando subir > 25% novamente.

### 6.4 Death sting

Player death = sting épico, 2-3s, fade-out da música. Já existe (`UDFMusicManagerSubsystem`). Confirmar que **não está sendo cortado** pelo carregamento do defeat screen.

---

## 7. Hit feedback consolidado — patch sugerido `[CODE]`

**Onde:** [`Source/DungeonForged/Private/Combat/UDFMeleeTraceComponent.cpp`](../../Source/DungeonForged/Private/Combat/UDFMeleeTraceComponent.cpp)

```cpp
void UDFMeleeTraceComponent::ApplyDamageToTarget(AActor* Target, const FHitResult& Hit)
{
    // ... apply GE_MeleeDamage ...

    // ---- FEEL DISPATCH ----
    const bool bIsFinisher = (Combo && Combo->GetComboIndex() == Combo->GetMaxComboIndex() - 1);
    const bool bIsHeavy = bUseHeavyAttackThisSwing;  // adicionar flag para heavy attack
    EDFHitFeedbackBand Band = EDFHitFeedbackBand::Light;
    if (bIsFinisher || bIsHeavy)  Band = EDFHitFeedbackBand::Heavy;

    // Crit upgrade vem do GE applied: o execution adiciona tag no GE context;
    // ler aqui pode ser feito via callback (UDFDamageCalculation broadcast)

    UDFCombatFeedbackLibrary::DispatchHitFeedback(
        this, Band, GetOwner(), Target, /*DamagePercent=*/ NormalizedDamage);

    // ---- VFX impacto ----
    const EHitImpactType ImpactType = bIsFinisher ? EHitImpactType::Heavy : EHitImpactType::Physical;
    UDFCombatFeedbackLibrary::SpawnHitImpact(ImpactType, Hit.ImpactPoint, Hit.ImpactNormal);

    // ---- Combat text ----
    if (UDFCombatTextSubsystem* CT = World->GetSubsystem<UDFCombatTextSubsystem>())
    {
        const ECombatTextType TextType = bWasCrit ? ECombatTextType::Crit : ECombatTextType::Damage;
        CT->SpawnFloatingText(Target->GetActorLocation() + FVector(0,0,120), FinalDamage, TextType, HitDirection);
    }
}
```

---

## 8. Checklist de "pronto"

- [ ] **Hit stop** dispara em 100% dos melee hits (testar com `LogDFTuning Verbose`).
- [ ] **Camera shake** dispara em 100% dos melee hits (Light/Heavy banding correto).
- [ ] **Crit** tem efeito visual claramente distinto (text dourado escala 1.4, chromatic pulse, shake forte).
- [ ] **Niagara** tem assets por tipo de dano (8 sistemas), centralizados via `SpawnHitImpact`.
- [ ] **SFX** tem 4 tiers por damage threshold e layer crit/normal.
- [ ] **Boss enrage** dispara: shake forte, vignette vermelho, music intensity++.
- [ ] **Second Wind** ativado: slow-mo 0.4s + flash branco + sting.
- [ ] **Decal pool** capado em 32 (não vaza).
- [ ] **Slider de intensity** em settings (camera shake, hit stop, damage numbers on/off).

---

## Apêndice — métricas de juice

Em jogos AAA, mede-se juice por:

- **Hit Confirmation Latency**: tempo entre o frame do input e o primeiro feedback audiovisual. **Alvo: < 50ms (3 frames a 60fps)**.
- **Hit Stop Frequency**: % de hits que disparam hit stop. **Alvo: 100% para hits em inimigos**.
- **Camera Shake Coverage**: % de eventos importantes que têm shake. **Alvo: ≥ 90%**.
- **Combat Text Spawn Rate**: por hit ou aglutinado. Pool 30 deve aguentar 30 hits/s sem reciclar prematuramente.

Adicionar logs:

```cpp
DECLARE_LOG_CATEGORY_EXTERN(LogDFFeel, Log, All);
// hit stop dispatch:
UE_LOG(LogDFFeel, Verbose, TEXT("[HS] Band=%s Dur=%.3f Dil=%.3f Excl=%s"),
       *UEnum::GetValueAsString(Band), Dur, Dil, *GetNameSafe(ExcludeActor));
```

Permite playtest gravado com replay analysis.
