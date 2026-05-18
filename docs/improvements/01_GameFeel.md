# 01 — Game Feel

> **Objetivo:** que cada input pareça responder em ≤2 frames, que o personagem comunique velocidade/peso, e que combate em lock-on tenha leitura clara.

---

## Sumário rápido

| Item | Atual | Alvo | Esforço |
|---|---|---|---|
| **Combo window** | 0.60s | 0.45s | 5min |
| **Dodge i-frames** | 0.25s | 0.35s | 10min |
| **Dodge feedback visual** | nenhum | chromatic + vignette azul | 30min |
| **Camera lag em combate** | sem | spring damping ↑ no lock-on | 1h |
| **Sprint shake** | sem | sub-perceptual 2D shake | 30min |
| **Landing impact** | sem | shake leve + dust niagara > 200cm queda | 1h |
| **Input buffer** | desconhecido | 0.15s para ability + combo input | 2h |
| **Strafe blendspace** | 8-way | confirmar transição idle↔strafe ≤ 0.15s | 30min |
| **Footstep tempo** | fixo | escalar com `MaxWalkSpeed` real | 1h |

---

## 1. Combo window — `[CONFIG]`

**Onde:** [`Source/DungeonForged/Public/Combat/UDFComboComponent.h:27`](../../Source/DungeonForged/Public/Combat/UDFComboComponent.h#L27)

```cpp
float ComboWindowDuration = 0.6f;  // ← reduzir para 0.45f
```

**Razão:**
- Hades = ~0.30–0.40s, DMC = ~0.40s, Dark Souls = ~0.50s.
- 0.60s convida a mash; 0.45s força timing.

**Validação:**
- Bater 3-hit consecutivos no dummy sem perder combo (rítmico, não mash).
- Esperar > 0.5s entre hits deve **resetar** para hit 1 (testar visualmente).

**Variante por arma:** expor override em `FDFItemRow::ComboWindowOverride` (se = 0, usa default). Armas pesadas (claymore) podem ficar 0.55s; daggers 0.35s.

---

## 2. Dodge — `[CONFIG] + [CODE]`

### 2.1 i-frame duration

**Onde:** procurar `IFrameDuration` ou similar em `Source/DungeonForged/Public/GAS/Abilities/DFAbility_Dodge.h`.

Atualmente **0.25s** (do `Game_Analysis.md`). Sugerido:

```cpp
UPROPERTY(EditAnywhere, Category="Dodge")
float IFrameDuration = 0.35f;       // antes: 0.25
UPROPERTY(EditAnywhere, Category="Dodge")
float DodgeCooldown = 0.7f;         // antes: 0.8
UPROPERTY(EditAnywhere, Category="Dodge")
float DodgeStaminaCost = 20.f;      // mantém
```

**Razão:** 0.35s é o sweet spot Souls/Bloodborne. 0.25 deixa o player "comer" hits que parecem evitáveis (frustração).

### 2.2 Feedback visual `[ASSET]`

No `GA_Dodge::ActivateAbility`, disparar:

```cpp
if (UDFScreenEffectsComponent* FX = Character->FindComponentByClass<UDFScreenEffectsComponent>())
{
    FX->ChromaticAberrationPulse(0.30f, 0.6f);   // duração, intensidade
    FX->FlashScreen(FLinearColor(0.6f, 0.8f, 1.f, 0.12f), 0.15f, 0.3f);
}
UDFCameraShakeFunctionLibrary::PlayDodgeShake(Character);  // shake suave novo
```

**Atalho:** `ChromaticAberrationPulse` já existe em `UDFScreenEffectsComponent`.

### 2.3 Camera shake do dodge `[ASSET]`

Criar `UDFCameraShake_Dodge` em [`Source/DungeonForged/Public/FX/UDFCameraShakes.h`](../../Source/DungeonForged/Public/FX/UDFCameraShakes.h):

```cpp
UCLASS(BlueprintType)
class UDFCameraShake_Dodge : public UCameraShakeBase
{
    GENERATED_BODY()
public:
    UDFCameraShake_Dodge();
};

// .cpp
UDFCameraShake_Dodge::UDFCameraShake_Dodge()
{
    OscillationDuration = 0.18f;
    OscillationBlendInTime = 0.04f;
    OscillationBlendOutTime = 0.10f;
    LocOscillation.X.Amplitude = 2.0f;
    LocOscillation.X.Frequency = 30.f;
    LocOscillation.Y.Amplitude = 1.5f;
    LocOscillation.Y.Frequency = 25.f;
    // muito sutil — apenas para reforçar "moveu rápido"
}
```

### 2.4 Trail VFX

Já existe `UDFAnimNotify_SpawnTrailVFX`. Acrescentar notify no `DodgeMontage` para fizz-trail nas mãos/pés (Niagara já está pronto). **`[BP]`**

---

## 3. Camera — `[CODE]`

**Onde:** [`Source/DungeonForged/Public/Camera/UDFCameraComponent.h`](../../Source/DungeonForged/Public/Camera/UDFCameraComponent.h)

### 3.1 Damping em lock-on

Quando entra modo Combat (lock-on), aumentar `SpringArm->CameraLagSpeed` de 10 → **18** para perseguir o pivot mais rápido (alvo se move = câmera mantém pace) e diminuir overshoot.

### 3.2 FOV bump em sprint

```cpp
// UDFCameraComponent::Tick / TickComponent
const float BaseFOV = 90.f;
const float SprintBonusFOV = 8.f;        // [CONFIG]
const float Interp = 6.f;                // [CONFIG]
const float TargetFOV = bIsSprinting ? BaseFOV + SprintBonusFOV : BaseFOV;
CameraComp->SetFieldOfView(FMath::FInterpTo(CameraComp->FieldOfView, TargetFOV, DeltaTime, Interp));
```

Reforça sensação de velocidade sem motion sickness. **Cap a +10 FOV.**

### 3.3 Vertical kick em landing > 200cm `[CODE]`

```cpp
// ADFPlayerCharacter::Landed
const float FallHeight = LastJumpStartZ - GetActorLocation().Z;
if (FallHeight > 200.f)
{
    const float Intensity = FMath::Clamp(FallHeight / 800.f, 0.f, 1.f);  // 0..1
    UDFCameraShakeFunctionLibrary::PlayLandingShake(this, Intensity);
    if (HitStop) HitStop->LightHit(this);
    if (LandDustNiagara) UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        this, LandDustNiagara, GetActorLocation() - FVector(0,0,90));
}
```

**Critério "pronto":** queda de plataforma a 600cm sente pesada e ouve thump grave.

---

## 4. Movement — `[CODE]`

### 4.1 Strafe transition timing

Em [`UDFCharacterMovementComponent`](../../Source/DungeonForged/Public/Characters/UDFCharacterMovementComponent.h):

- Confirmar `bUseControllerDesiredRotation = true` em modo Combat.
- `BrakingFrictionFactor` = 2.0 em modo Combat (parada rápida quando solta input).
- `MaxAcceleration` = 2048 (default) é razoável; em modo Strafe pode ir a 2400 para reagir.

### 4.2 Sprint shake

Adicionar shake sub-perceptual de 1Hz quando bSprintActive:

```cpp
UDFCameraShake_Sprint::UDFCameraShake_Sprint()
{
    OscillationDuration = -1.f;       // infinito (parado quando termina sprint)
    LocOscillation.Z.Amplitude = 0.6f;
    LocOscillation.Z.Frequency = 4.f;
    LocOscillation.Y.Amplitude = 0.4f;
    LocOscillation.Y.Frequency = 3.f;
}
```

Tocar em `OnSprintStart`, `Stop` em `OnSprintEnd`. **Bem sutil** — quase imperceptível mas sentido.

### 4.3 Footstep tempo escalado `[BP]`

No AnimBP, `BlendSpace_Walk_Run` deve usar a velocidade real; o `UDFAnimNotify_FootStep` já dispara por animação. Confirmar que **playrate da run montage = 1.0** (não interpolada para uma velocidade fictícia).

Adicionar **leve filter** em material de footstep por surface: pedra / madeira / metal — usa `LineTrace` para `PhysicalMaterial` do chão e cruza com `USoundCue` tabelado em `UDFSoundLibrary::FootstepPerSurface`.

---

## 5. Animação — `[CODE/ASSET]`

### 5.1 Hit reaction com root motion

`UDFHitReactionComponent` decide montagem por severity. Se a montage **tem root motion**, garantir que `bEnableRootMotionTranslationScaling` no AnimBP esteja com escala 1.0 (para conseguir afastar o player ao receber knockback) e cancelar movement input do CMC durante a montage para não ficar "andando sob hit".

### 5.2 Combo finisher visual

Hit 3 do combo deve:
- Tocar **HeavyHit shake** (não Light).
- Disparar `ChromaticAberrationPulse(0.10, 1.2)`.
- VFX no hit (Niagara `NS_ComboFinisher_Slash`) com tail mais longo.

Centralizar em `UDFMeleeTraceComponent::ApplyDamageToTarget` lendo `CurrentComboIndex` do `UDFComboComponent`:

```cpp
if (Combo && Combo->GetComboIndex() == Combo->GetMaxComboIndex() - 1)
{
    if (HitStop) HitStop->HeavyHit(Owner);
    if (Screen) Screen->ChromaticAberrationPulse(0.10f, 1.2f);
}
else
{
    if (HitStop) HitStop->LightHit(Owner);
}
```

### 5.3 Anim curve "Stride"

Expor curva `Stride` no AnimBP que vai 0 (idle) a 1 (full run). O `CapsuleHalfHeight` pode ser leve push-down quando Stride alto (3cm), criando "weight in motion" sem mexer no CMC.

---

## 6. Input buffer — `[CODE]`

**Problema clássico em ARPG**: jogador aperta LMB no fim do recovery; sem buffer, é "engolido".

### 6.1 Strategy

Em `UDFComboComponent`, manter um **buffered input** com timestamp:

```cpp
UPROPERTY(EditAnywhere) float InputBufferWindow = 0.15f;  // [CONFIG]

void UDFComboComponent::TryStartNextSwing()
{
    if (bIsPlayingMontage)
    {
        BufferedAttackTime = GetWorld()->GetTimeSeconds();
        return;
    }
    PlayNextMontage();
}

void UDFComboComponent::OnMontageEnded(...)
{
    bIsPlayingMontage = false;
    const float Now = GetWorld()->GetTimeSeconds();
    if (BufferedAttackTime > 0.f && Now - BufferedAttackTime < InputBufferWindow)
    {
        BufferedAttackTime = -1.f;
        PlayNextMontage();
    }
}
```

Aplicar **mesmo padrão para ability triggers** (Q/E/R/F).

### 6.2 Lock-on switch buffer

Ao apertar Q/E durante uma animação não-cancelável, **mesmo padrão** — não engolir o switch.

---

## 7. Strafe / lock-on feel — `[CODE]`

**Onde:** [`Source/DungeonForged/Public/Camera/UDFLockOnComponent.h`](../../Source/DungeonForged/Public/Camera/UDFLockOnComponent.h)

### 7.1 Suavização da rotação para o alvo

Quando lock-on, a câmera olha para o alvo via `SetControlRotation` direto. Adicionar interp:

```cpp
const FRotator Desired = (TargetLoc - PawnLoc).Rotation();
const FRotator Current = PC->GetControlRotation();
const FRotator Smoothed = FMath::RInterpTo(Current, Desired, DeltaTime, 12.f);  // [CONFIG]
PC->SetControlRotation(FRotator(Smoothed.Pitch, Smoothed.Yaw, 0.f));
```

Reduz snap quando alvo dá hit-reaction (move 50cm subitamente).

### 7.2 Auto-break em distância

Se o alvo está a > `LockOnRange + 200cm` por > 0.4s, break automático. Já existe? Confirmar em `UDFLockOnComponent::Tick`.

### 7.3 Indicador visual de break iminente

Quando alvo está nos últimos 15% do range, **pulsar o `UDFLockOnWidget`** em amarelo. Mostra que vai perder o lock se não fechar.

---

## 8. Checklist de "pronto"

- [ ] Combo 3-hit completo entre 1.0 e 1.4s, sem mash.
- [ ] Dodge dá invuln de 0.35s — testar: enemy projectile mid-dodge não acerta.
- [ ] Dodge tem chromatic pulse + shake leve + flash azul → **sente** evasivo.
- [ ] Sprint: leve FOV bump + shake sub-perceptual; ao parar, retorno suave.
- [ ] Landing > 200cm: shake + dust + thump SFX; > 600cm: shake forte.
- [ ] LMB no fim do recovery não é engolido (buffer 0.15s funciona).
- [ ] Lock-on switch (Q/E) funciona durante montages (buffer 0.15s).
- [ ] Camera em lock-on persegue alvo sem overshoot perceptível.
- [ ] Hit 3 do combo dispara HeavyHit (não Light) e tem VFX maior.

---

## Apêndice — anti-padrões observados

1. **`PrimaryActorTick.bCanEverTick = true` em actor que só ouve eventos** — desligar. Reduz overhead em runs com 30+ inimigos.
2. **`SetActorRotation` direto** sem Interp para girar player — causa snap. Sempre `FMath::RInterpTo`.
3. **`Multicast` de FX em vez de `Client`** — em co-op causa duplicate FX. Para FX local (hit feedback do que TU bateste), usar `Client_HitFeedback` (já existe).
4. **Hit stop global sem exclude actor** — congelava o próprio attacker; o `UDFHitStopSubsystem` já faz isso certo (`ExcludeActor`).
