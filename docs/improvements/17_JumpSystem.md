# 17 — Jump System: Guia Completo de Implementação (C++ + Editor)

> **Versão:** 2026-05-22
> **Objetivo:** transformar o `ACharacter::Jump()` nativo num sistema completo com **11 animações direcionais** (5 starts + 1 loop + 5 lands), suporte armado/desarmado, integração com GAS, combate, lock-on, movimento e dodge, mais debug visual e tuning data-driven.
>
> **Animações disponíveis no projeto** (JCHero, 22 totais — 11 unarmed + 11 armed):
> ```
> Unarmed (exploração):                    Armed (combat — Weapon equipped):
>   Jump_Start_0       (idle, sem direção)   Jump_Combat_Start_0
>   Jump_Start_F_0     (forward)             Jump_Combat_Start_F_0
>   Jump_Start_B_180   (backward)            Jump_Combat_Start_B_180
>   Jump_Start_L_90    (left)                Jump_Combat_Start_L_90
>   Jump_Start_R_90    (right)               Jump_Combat_Start_R_90
>   Jump_Loop_0        (in-air loop)         Jump_Combat_Loop_0
>   Jump_End_0         (idle land)           Jump_Combat_End_0
>   Jump_End_F_0       (forward land)        Jump_Combat_End_F_0
>   Jump_End_B_180     (backward land)       Jump_Combat_End_B_180
>   Jump_End_L_90      (left land)           Jump_Combat_End_L_90
>   Jump_End_R_90      (right land)          Jump_Combat_End_R_90
> ```
>
> **Estado atual da base:**
> - Input: `IA_Jump` ([`ADFRunPlayerController.h:50`](../../Source/DungeonForged/Public/GameModes/Run/ADFRunPlayerController.h)) → `Input_JumpStart/End` → `ACharacter::Jump()/StopJumping()`
> - AnimInstance: `bIsInAir`, `Velocity`, `Direction`, `GroundDistance` já calculados
> - `FUDAnimSet`: tem apenas `JumpStartAnim`, `JumpLoopAnim`, `JumpLandAnim` (slots únicos, sem variantes direcionais)
> - **Sem** tags GAS de jump, **sem** AnimNotifyState para apex/landing, **sem** integração com combate (pode-se pular durante ataque)

---

## Sumário

- [1. Diagnóstico — gaps no sistema atual](#1-diagnóstico--gaps-no-sistema-atual)
- [2. Arquitetura proposta](#2-arquitetura-proposta)
- [3. C++ — Expandir `FUDAnimSet` para jump direcional](#3-c--expandir-fudanimset-para-jump-direcional)
- [4. C++ — Estado de jump no `UUDFAnimInstance`](#4-c--estado-de-jump-no-uudfaniminstance)
- [5. C++ — Tuning no `UDFCharacterMovementComponent`](#5-c--tuning-no-udfcharactermovementcomponent)
- [6. C++ — GAS Tags & blocking](#6-c--gas-tags--blocking)
- [7. C++ — Integração combate / dodge / lock-on](#7-c--integração-combate--dodge--lock-on)
- [8. C++ — AnimNotify: apex, landing recovery, footstep](#8-c--animnotify-apex-landing-recovery-footstep)
- [9. C++ — Tuning DataAsset](#9-c--tuning-dataasset)
- [10. C++ — Debug commands `df.JumpDebug`](#10-c--debug-commands-dfjumpdebug)
- [11. Setup Editor passo-a-passo](#11-setup-editor-passo-a-passo)
- [12. AnimBP — state machine de jump](#12-animbp--state-machine-de-jump)
- [13. Checklist de validação](#13-checklist-de-validação)
- [14. Tabela de arquivos](#14-tabela-de-arquivos)
- [15. Próximos passos (AAA polish)](#15-próximos-passos-aaa-polish)
- [16. Combate Aéreo & Combos com Jump](#16-c--combate-aéreo--combos-com-jump)

---

## 1. Diagnóstico — gaps no sistema atual

### O que existe ✅

| Sistema | Onde | Status |
|---|---|---|
| Input → Jump | [`ADFRunPlayerController.cpp:376-390`](../../Source/DungeonForged/Private/GameModes/Run/ADFRunPlayerController.cpp) | ✓ wired |
| `IA_Jump` asset | `Content/DungeonForged/Input/Actions/IA_Jump.uasset` | ✓ existe |
| Native `ACharacter::Jump()` | UE5 base class | ✓ default tuning |
| `bIsInAir` no AnimInstance | [`UDFAnimInstance.h:98`](../../Source/DungeonForged/Public/Animation/UDFAnimInstance.h) | ✓ atualizado por `IsFalling()` |
| Anim slots básicos | [`DFAnimSetTypes.h:64-70`](../../Source/DungeonForged/Public/Animation/DFAnimSetTypes.h) | ✓ 3 slots: Start/Loop/Land |
| 22 animações de jump | `Content/Assets/Animations/JCHero/Animation/` | ✓ todas importadas |
| `Direction` (−180→180) | [`UDFAnimInstance.cpp:79-81`](../../Source/DungeonForged/Private/Animation/UDFAnimInstance.cpp) | ✓ via `CalculateDirection` |
| `OnDFMovementModeChanged` delegate | [`UDFCharacterMovementComponent.cpp:45`](../../Source/DungeonForged/Private/Characters/UDFCharacterMovementComponent.cpp) | ✓ broadcast Walking↔Falling |
| `GroundDistance` (foot IK trace) | [`UDFAnimInstance.h:150`](../../Source/DungeonForged/Public/Animation/UDFAnimInstance.h) | ✓ pode reusar para landing prediction |

### O que falta ❌

| Gap | Impacto | Solução |
|---|---|---|
| `FUDAnimSet` só tem **um** slot para Start/Land | Não dá pra escolher anim direcional | §3 — expandir struct |
| `bIsJumping` (no AnimInstance) não existe | AnimBP não distingue "subindo" de "caindo" | §4 |
| `JumpDirection` capturada no takeoff não persiste | Land usa direção atual em vez da do takeoff | §4 |
| `VerticalVelocity` exposto, mas no AnimInstance só tem `Velocity.Z` indireto | Não dá pra detectar apex sem ler `Velocity.Z` | §4 |
| `AirTime` não tracked | Sem AAA "weight-by-airtime" landing scale | §4 |
| `PredictedLandingDistance` ausente | Não dá pra pré-blendar land animation | §4 |
| Sem tags `State.Jumping` / `State.Falling` / `State.Landing` | Outras systems não sabem do estado aéreo | §6 |
| Pode pular **durante ataque** | Quebra combate fluido | §7 |
| Pode pular **durante dodge** | Quebra invulnerabilidade visual | §7 |
| Pode pular **enquanto exhausted** | Stamina ignorada | §7 |
| Sem AnimNotify `JumpApex` | VFX/SFX no peak não disparáveis | §8 |
| Sem `LandingRecoveryWindow` | Player pode atacar instantâneo no toque (sem peso) | §8 |
| `JumpZVelocity`, `AirControl`, `GravityScale` no default UE | Sem afinação data-driven | §5, §9 |

---

## 2. Arquitetura proposta

### State machine de jump

```
┌──────────────────────────────────────────────────────────────┐
│                       Grounded (Walking)                     │
└─────────────────────────┬────────────────────────────────────┘
                          │ Input IA_Jump (Started)
                          │ + GAS check (no State.Dodging, no State.Attacking)
                          ▼
              ┌──────────────────────────────┐
              │ JumpStart (montage by dir)   │   anim: Jump_Start_{0,F_0,B_180,L_90,R_90}
              │  - Capture JumpDirection     │   tag:  State.Jumping (add)
              │  - Trigger ACharacter::Jump()│   AirTime = 0
              └─────────────┬────────────────┘
                            │ MovementMode → Falling
                            ▼
              ┌──────────────────────────────┐
              │ JumpLoop                      │   anim: Jump_Loop_0
              │  - AirTime += dt              │   VerticalVelocity tracked
              │  - PredictedLandingDistance   │   (line trace down from feet)
              │  - AnimNotify_JumpApex when   │
              │    Velocity.Z crosses 0       │
              └─────────────┬────────────────┘
                            │ GroundDistance < LandPreparationThreshold
                            ▼
              ┌──────────────────────────────┐
              │ JumpLand (montage by dir)    │   anim: Jump_End_{0,F_0,B_180,L_90,R_90}
              │  - Resolve land anim:        │   tag:  State.Jumping (remove)
              │    JumpDirection at takeoff  │   tag:  State.Landing (add, with window)
              │  - AnimNotifyState_LandRecovery
              │    blocks attack/dodge for X frames
              └─────────────┬────────────────┘
                            │ Notify_LandingRecoveryEnd
                            ▼
              ┌──────────────────────────────┐
              │      Grounded (Walking)      │   tag:  State.Landing (remove)
              └──────────────────────────────┘
```

### Decisões de design

| Decisão | Escolha | Razão |
|---|---|---|
| GAS ability ou native? | **Native** (`ACharacter::Jump()`) com gate via tags | Pulo precisa ser ultra-responsivo (1 frame). GAS adiciona latência via prediction. |
| Direcional capturada quando? | **No takeoff**, persistir no `LastJumpDirection` | Aerial control não muda a anim, evita popping |
| Quando trocar para Loop? | Quando `Velocity.Z` < 0 OU AirTime > `StartDuration` | Suaviza transição start→loop |
| Quando antecipar Land? | `GroundDistance < 250cm && IsFalling()` | Pre-blend land anim antes do toque (Lyra pattern) |
| Armed/Unarmed split? | **`FUDAnimSet` por stance** (já existe via WeaponAnimSet vs DefaultAnimSet) | Aproveita o sistema existente |
| Anims direcionais — slots ou BlendSpace? | **Slots** (5 starts + 5 lands) | Blend de 11 anims no BS é menos clean; o switch por enum é mais previsível |
| Cancel out of jump? | **Não** durante landing recovery (200ms grace) | Dá peso e impede combo-spam aéreo |

---

## 3. C++ — Expandir `FUDAnimSet` para jump direcional

### Patch — `DFAnimSetTypes.h`

**Arquivo:** [`Source/DungeonForged/Public/Animation/DFAnimSetTypes.h`](../../Source/DungeonForged/Public/Animation/DFAnimSetTypes.h)

Substituir os 3 slots singulares (`JumpStartAnim`, `JumpLoopAnim`, `JumpLandAnim`) por uma sub-struct direcional. Manter o legacy como fallback para não quebrar conteúdo antigo.

```cpp
// Add near top of file
#include "Animation/UDFLocomotionTypes.h"  // for EDFMovementDirection

/**
 * Per-direction jump animations.
 *
 * Direction order matches takeoff input snapped to cardinals at takeoff time:
 *   None     → idle takeoff (no input)
 *   Forward  → moving forward when Jump pressed
 *   Backward → moving backward
 *   Left     → strafing left
 *   Right    → strafing right
 *
 * Diagonals fall back to nearest cardinal (handled in DFResolveJumpDirection).
 *
 * The Loop is direction-agnostic (one anim covers all in-air poses).
 */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FUDJumpAnimSet
{
    GENERATED_BODY()

    /** Idle takeoff (no movement input). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Start")
    TObjectPtr<UAnimSequenceBase> Start_Idle;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Start")
    TObjectPtr<UAnimSequenceBase> Start_Forward;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Start")
    TObjectPtr<UAnimSequenceBase> Start_Backward;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Start")
    TObjectPtr<UAnimSequenceBase> Start_Left;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Start")
    TObjectPtr<UAnimSequenceBase> Start_Right;

    /** In-air loop (single, direction-agnostic). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Loop")
    TObjectPtr<UAnimSequenceBase> Loop;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Land")
    TObjectPtr<UAnimSequenceBase> Land_Idle;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Land")
    TObjectPtr<UAnimSequenceBase> Land_Forward;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Land")
    TObjectPtr<UAnimSequenceBase> Land_Backward;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Land")
    TObjectPtr<UAnimSequenceBase> Land_Left;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Land")
    TObjectPtr<UAnimSequenceBase> Land_Right;

    /** Resolve start anim by movement direction, with cardinal fallback for diagonals. */
    UAnimSequenceBase* ResolveStart(EDFMovementDirection Dir) const;
    UAnimSequenceBase* ResolveLand(EDFMovementDirection Dir) const;

    bool IsValid() const
    {
        return Start_Idle || Start_Forward || Start_Backward || Start_Left || Start_Right || Loop;
    }
};
```

**Integrar no `FUDAnimSet`** (mesmo arquivo) — adicionar o novo campo e manter os legacy:

```cpp
// Inside FUDAnimSet struct, replace the three Jump fields with:

/** Per-direction jump set (preferred). Falls back to legacy Jump*Anim when empty. */
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Jump")
FUDJumpAnimSet JumpSet;

/** Legacy single jump anims (kept for backward compatibility). */
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Jump|Legacy")
TObjectPtr<UAnimSequenceBase> JumpStartAnim;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Jump|Legacy")
TObjectPtr<UAnimSequenceBase> JumpLoopAnim;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Anim Set - Jump|Legacy")
TObjectPtr<UAnimSequenceBase> JumpLandAnim;
```

**Helpers no .cpp (criar `DFAnimSetTypes.cpp` se não existir):**

```cpp
// Source/DungeonForged/Private/Animation/DFAnimSetTypes.cpp
#include "Animation/DFAnimSetTypes.h"

UAnimSequenceBase* FUDJumpAnimSet::ResolveStart(const EDFMovementDirection Dir) const
{
    switch (Dir)
    {
    case EDFMovementDirection::Forward:        return Start_Forward       ? Start_Forward  : Start_Idle;
    case EDFMovementDirection::Backward:       return Start_Backward      ? Start_Backward : Start_Idle;
    case EDFMovementDirection::Left:           return Start_Left          ? Start_Left     : Start_Idle;
    case EDFMovementDirection::Right:          return Start_Right         ? Start_Right    : Start_Idle;
    case EDFMovementDirection::ForwardLeft:    return Start_Forward       ? Start_Forward  : (Start_Left  ? Start_Left  : Start_Idle);
    case EDFMovementDirection::ForwardRight:   return Start_Forward       ? Start_Forward  : (Start_Right ? Start_Right : Start_Idle);
    case EDFMovementDirection::BackwardLeft:   return Start_Backward      ? Start_Backward : (Start_Left  ? Start_Left  : Start_Idle);
    case EDFMovementDirection::BackwardRight:  return Start_Backward      ? Start_Backward : (Start_Right ? Start_Right : Start_Idle);
    default:                                   return Start_Idle;
    }
}

UAnimSequenceBase* FUDJumpAnimSet::ResolveLand(const EDFMovementDirection Dir) const
{
    switch (Dir)
    {
    case EDFMovementDirection::Forward:        return Land_Forward  ? Land_Forward  : Land_Idle;
    case EDFMovementDirection::Backward:       return Land_Backward ? Land_Backward : Land_Idle;
    case EDFMovementDirection::Left:           return Land_Left     ? Land_Left     : Land_Idle;
    case EDFMovementDirection::Right:          return Land_Right    ? Land_Right    : Land_Idle;
    case EDFMovementDirection::ForwardLeft:    return Land_Forward  ? Land_Forward  : (Land_Left  ? Land_Left  : Land_Idle);
    case EDFMovementDirection::ForwardRight:   return Land_Forward  ? Land_Forward  : (Land_Right ? Land_Right : Land_Idle);
    case EDFMovementDirection::BackwardLeft:   return Land_Backward ? Land_Backward : (Land_Left  ? Land_Left  : Land_Idle);
    case EDFMovementDirection::BackwardRight:  return Land_Backward ? Land_Backward : (Land_Right ? Land_Right : Land_Idle);
    default:                                   return Land_Idle;
    }
}
```

---

## 4. C++ — Estado de jump no `UUDFAnimInstance`

### Patch — `UDFAnimInstance.h`

**Arquivo:** [`Source/DungeonForged/Public/Animation/UDFAnimInstance.h`](../../Source/DungeonForged/Public/Animation/UDFAnimInstance.h)

Adicionar novas variáveis junto com `bIsInAir`:

```cpp
// ── Jump state ──────────────────────────────────────────────────────
/** True while ACharacter is going up (Velocity.Z > 0 and IsFalling). */
UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump")
bool bIsJumping = false;

/** True while in-air after apex (Velocity.Z <= 0 and IsFalling). */
UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump")
bool bIsFalling = false;

/** True for LandingRecoveryWindow seconds after touching ground. */
UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump")
bool bIsLanding = false;

/** Direction captured at takeoff (kept stable for the entire jump). */
UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump")
EDFMovementDirection LastJumpDirection = EDFMovementDirection::None;

/** Seconds airborne since takeoff. Resets when grounded. */
UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump")
float AirTime = 0.f;

/** Vertical component of velocity, exposed for apex / falling detection. */
UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump")
float VerticalVelocity = 0.f;

/** Distance to ground straight down (cm). Capped by trace length. Used to start landing blend early. */
UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump")
float PredictedLandingDistance = 0.f;

/** Distance below which AnimGraph starts blending towards JumpLand. */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Jump", meta = (ClampMin = "0.0"))
float LandPreparationThreshold = 250.f;

/** Trace distance for PredictedLandingDistance (cm). */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Jump", meta = (ClampMin = "0.0"))
float LandPredictionTraceMax = 1000.f;

/** Land anim plays for the captured direction. Use Break ActiveAnimSet → JumpSet.ResolveLand(LastJumpDirection). */
```

### Patch — `UDFAnimInstance.cpp`

**Arquivo:** [`Source/DungeonForged/Private/Animation/UDFAnimInstance.cpp`](../../Source/DungeonForged/Private/Animation/UDFAnimInstance.cpp)

No `NativeUpdateAnimation`, depois do bloco que atualiza `bIsInAir`, adicionar:

```cpp
// ── Jump state derivation ────────────────────────────────────────────
const bool bWasInAir = bIsJumping || bIsFalling;
const bool bNowInAir = bIsInAir;

VerticalVelocity = Velocity.Z;
bIsJumping = bNowInAir && VerticalVelocity > 1.f;
bIsFalling = bNowInAir && VerticalVelocity <= 1.f;

if (!bWasInAir && bNowInAir)
{
    // Takeoff frame: capture direction.
    if (Speed > 50.f)
    {
        // Movement direction was already determined this frame by DetermineMovementDirection().
        LastJumpDirection = MovementDirection;
    }
    else
    {
        LastJumpDirection = EDFMovementDirection::None;
    }
    AirTime = 0.f;
}
else if (bNowInAir)
{
    AirTime += DeltaSeconds;
}
else if (bWasInAir && !bNowInAir)
{
    // Just landed.
    bIsLanding = true;
    AirTime = 0.f;
    // bIsLanding cleared by the AnimNotify_LandingRecoveryEnd (added in AnimBP) — see §8.3.
}

// Predict landing distance for early blend (only while falling).
if (bIsFalling && OwningCharacter)
{
    PredictedLandingDistance = LandPredictionTraceMax;
    UWorld* const W = OwningCharacter->GetWorld();
    if (W)
    {
        const FVector Origin = OwningCharacter->GetActorLocation();
        const FVector Down   = Origin - FVector(0.f, 0.f, LandPredictionTraceMax);
        FHitResult Hit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(JumpLandTrace), false, OwningCharacter.Get());
        if (W->LineTraceSingleByChannel(Hit, Origin, Down, ECC_Visibility, Params))
        {
            PredictedLandingDistance = FMath::Max(0.f, (Origin - Hit.ImpactPoint).Z - OwningCharacter->GetSimpleCollisionHalfHeight());
        }
    }
}
else
{
    PredictedLandingDistance = LandPredictionTraceMax;
}
```

> **Nota:** O `bIsLanding = false` é setado por um `AnimNotify` no final do `Land` montage (§8). Como fallback, se a anim não disparar o notify, adicione um timer no `NativeUpdateAnimation` que zera após `LandingRecoveryWindow` segundos.

---

## 5. C++ — Tuning no `UDFCharacterMovementComponent`

### Patch — header

**Arquivo:** [`Source/DungeonForged/Public/Characters/UDFCharacterMovementComponent.h`](../../Source/DungeonForged/Public/Characters/UDFCharacterMovementComponent.h)

Adicionar categoria `"DF|Movement|Jump"`:

```cpp
// ── Jump tuning ─────────────────────────────────────────────────────
/** Initial Z velocity on jump (cm/s). Override of ACharacter default 420. */
UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump", meta = (ClampMin = "0.0"))
float DFJumpZVelocity = 550.f;

/** Air control (0=none, 1=full). Override of UE default 0.05. */
UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump", meta = (ClampMin = "0.0", ClampMax = "1.0"))
float DFAirControl = 0.35f;

/** Gravity scale during normal air. 1.0 = world gravity. */
UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump", meta = (ClampMin = "0.0"))
float DFGravityScale = 1.7f;

/** Extra gravity multiplier applied after apex (jumping → falling). Creates a snappier arc. */
UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump", meta = (ClampMin = "1.0"))
float DFFallGravityMultiplier = 1.25f;

/** Stamina cost per jump (drained on takeoff). Use 0 to disable. */
UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump", meta = (ClampMin = "0.0"))
float DFJumpStaminaCost = 10.f;

/** Min seconds between jumps even if grounded again instantly (anti-spam). */
UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump", meta = (ClampMin = "0.0"))
float DFJumpCooldown = 0.20f;

/** Landing recovery window — duration of State.Landing tag (gates attack/dodge). */
UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump", meta = (ClampMin = "0.0"))
float DFLandingRecoveryWindow = 0.20f;

UFUNCTION(BlueprintPure, Category = "DF|Movement|Jump")
float GetJumpCooldownRemaining() const;

protected:
    float TimeLastJump = -1.f;
```

### Patch — .cpp

No construtor, aplicar os valores ao `ACharacter` quando possível:

```cpp
UDFCharacterMovementComponent::UDFCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // ... existing init ...
    JumpZVelocity = DFJumpZVelocity;
    AirControl   = DFAirControl;
    GravityScale = DFGravityScale;
    MaxAcceleration = 2048.f; // keep — already snappy
}
```

E no `TickComponent`, aplicar fall gravity boost depois do apex:

```cpp
// ── Apex gravity boost ────────────────────────────────────────────
if (MovementMode == MOVE_Falling)
{
    const float ZVel = Velocity.Z;
    GravityScale = (ZVel < 0.f) ? (DFGravityScale * DFFallGravityMultiplier) : DFGravityScale;
}
```

### `GetJumpCooldownRemaining` + helper para abortar pulo

```cpp
float UDFCharacterMovementComponent::GetJumpCooldownRemaining() const
{
    if (TimeLastJump < 0.f || !GetWorld()) return 0.f;
    return FMath::Max(0.f, DFJumpCooldown - (GetWorld()->GetTimeSeconds() - TimeLastJump));
}
```

### Override `DoJump` para custo de estamina e cooldown

```cpp
// header: override
virtual bool DoJump(bool bReplayingMoves) override;

// cpp:
bool UDFCharacterMovementComponent::DoJump(const bool bReplayingMoves)
{
    if (GetJumpCooldownRemaining() > 0.f) return false;

    // Stamina gate (use ASC if available)
    if (DFJumpStaminaCost > 0.f && CharacterOwner)
    {
        if (IAbilitySystemInterface* const IAS = Cast<IAbilitySystemInterface>(CharacterOwner))
        {
            if (UAbilitySystemComponent* const ASC = IAS->GetAbilitySystemComponent())
            {
                if (UDFAttributeSet* const Attrs = const_cast<UDFAttributeSet*>(ASC->GetSet<UDFAttributeSet>()))
                {
                    if (Attrs->GetStamina() < DFJumpStaminaCost) return false;
                    Attrs->SetStamina(Attrs->GetStamina() - DFJumpStaminaCost);
                }
            }
        }
    }
    const bool bOk = Super::DoJump(bReplayingMoves);
    if (bOk && GetWorld())
    {
        TimeLastJump = GetWorld()->GetTimeSeconds();
    }
    return bOk;
}
```

---

## 6. C++ — GAS Tags & blocking

### 6.1 Adicionar tags

**Arquivo:** [`Source/DungeonForged/Public/GAS/DFGameplayTags.h`](../../Source/DungeonForged/Public/GAS/DFGameplayTags.h) — declarar:

```cpp
// ── Jump ────────────────────────────────────────────
static FGameplayTag State_Jumping;
static FGameplayTag State_Falling;
static FGameplayTag State_Landing;

/** Ability tag if you decide later to wrap jump in a GAS ability. */
static FGameplayTag Ability_Movement_Jump;
```

**`DFGameplayTags.cpp`:**

```cpp
FGameplayTag FDFGameplayTags::State_Jumping;
FGameplayTag FDFGameplayTags::State_Falling;
FGameplayTag FDFGameplayTags::State_Landing;
FGameplayTag FDFGameplayTags::Ability_Movement_Jump;

// Inside RegisterGameplayTags():
DF_TAG(State_Jumping)(FName("State.Jumping"), FString("Character is going up after a jump."));
DF_TAG(State_Falling)(FName("State.Falling"), FString("Character is falling (post-apex or off ledge)."));
DF_TAG(State_Landing)(FName("State.Landing"), FString("Character is in landing recovery window."));
DF_TAG(Ability_Movement_Jump)(FName("Ability.Movement.Jump"), FString("Jump ability identifier (reserved)."));
```

### 6.2 Setar/limpar tags no CMC

No `UDFCharacterMovementComponent::OnMovementModeChanged`:

```cpp
void UDFCharacterMovementComponent::OnMovementModeChanged(
    const EMovementMode PreviousMovementMode, const uint8 PreviousCustomMode)
{
    Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
    OnDFMovementModeChanged.Broadcast(MovementMode, PreviousMovementMode, PreviousCustomMode);

    if (!CharacterOwner) return;
    IAbilitySystemInterface* const IAS = Cast<IAbilitySystemInterface>(CharacterOwner);
    UAbilitySystemComponent* const ASC = IAS ? IAS->GetAbilitySystemComponent() : nullptr;
    if (!ASC) return;

    // Walking → Falling
    if (PreviousMovementMode == MOVE_Walking && MovementMode == MOVE_Falling)
    {
        ASC->AddLooseGameplayTag(FDFGameplayTags::State_Jumping);  // assume upward first
    }
    // Falling → Walking (landed)
    else if (PreviousMovementMode == MOVE_Falling && MovementMode == MOVE_Walking)
    {
        ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Jumping, 0);
        ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Falling, 0);
        ASC->AddLooseGameplayTag(FDFGameplayTags::State_Landing);
        // Auto-clear after recovery window.
        if (UWorld* const W = GetWorld())
        {
            W->GetTimerManager().SetTimer(TimerHandle_EndLanding, [this]()
            {
                if (!CharacterOwner) return;
                if (IAbilitySystemInterface* const I = Cast<IAbilitySystemInterface>(CharacterOwner))
                {
                    if (UAbilitySystemComponent* const A = I->GetAbilitySystemComponent())
                    {
                        A->RemoveLooseGameplayTag(FDFGameplayTags::State_Landing, 0);
                    }
                }
            }, DFLandingRecoveryWindow, false);
        }
    }
}
```

**`UDFCharacterMovementComponent::TickComponent`** — promover `State.Jumping` para `State.Falling` no apex:

```cpp
// After existing tick logic:
if (MovementMode == MOVE_Falling && CharacterOwner)
{
    IAbilitySystemInterface* const IAS = Cast<IAbilitySystemInterface>(CharacterOwner);
    if (UAbilitySystemComponent* const ASC = IAS ? IAS->GetAbilitySystemComponent() : nullptr)
    {
        const bool bGoingUp = Velocity.Z > 1.f;
        const bool bHasJumping = ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Jumping);
        if (!bGoingUp && bHasJumping)
        {
            ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Jumping, 0);
            ASC->AddLooseGameplayTag(FDFGameplayTags::State_Falling);
        }
    }
}
```

Header — adicionar `FTimerHandle TimerHandle_EndLanding;` em `protected`.

---

## 7. C++ — Integração combate / dodge / lock-on

> ⚠️ **Importante:** o projeto usa **combos aéreos e combos que incluem pulo como step**. Por isso, os blockers **não bloqueiam atacar ou pular cegamente** — usam tags mais finas (`State_Attacking_Aerial`, `State_Combat_AbilityCancelWindow_Open`) que permitem jump-cancel e ar attack. Ver detalhes na [§16 — Combate Aéreo & Combos com Jump](#16-c--combate-aéreo--combos-com-jump).

### 7.1 Bloqueio do jump em estados inválidos (refinado)

Em `ADFPlayerCharacter::Jump` (override) — **somente** bloqueia em estados verdadeiramente incapacitantes:

```cpp
void ADFPlayerCharacter::Jump()
{
    if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponent())
    {
        // Hard blockers — estados onde nem pular nem atacar faz sentido.
        static const FGameplayTagContainer HardBlockers = []{
            FGameplayTagContainer C;
            C.AddTag(FDFGameplayTags::State_Dead);
            C.AddTag(FDFGameplayTags::State_Stunned);
            C.AddTag(FDFGameplayTags::State_Dodging);
            C.AddTag(FDFGameplayTags::State_Exhausted);
            C.AddTag(FDFGameplayTags::State_Landing);   // landing recovery
            return C;
        }();
        if (ASC->HasAnyMatchingGameplayTags(HardBlockers)) return;

        // Soft blocker — durante ataque, só permite jump se houver cancel window aberta.
        // (Jump-cancel: o combo cancel window é setado pelo ANS_DFAbilityCancelWindow
        //  e inclui Ability.Movement.Jump nos AllowedCancelTags — ver §16.2.)
        if (ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Attacking))
        {
            const bool bCancelable = ASC->HasMatchingGameplayTag(
                FDFGameplayTags::State_Combat_AbilityCancelWindow_Open);
            if (!bCancelable) return;
            // Cancela o combo step atual antes de pular.
            if (UDFComboComponent* const Combo = this->Combo) { Combo->CancelCurrentMontage(); }
        }
    }
    Super::Jump();
}
```

### 7.2 Combo no takeoff — **NÃO** resetar incondicional

Substitui o reset cego por uma decisão baseada na intenção:

```cpp
// UDFCharacterMovementComponent::OnMovementModeChanged, Walking→Falling:
if (ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(CharacterOwner))
{
    if (UDFComboComponent* const Combo = PC->Combo)
    {
        // Se o combo está em janela de cancel ou se está em estado aéreo previsto,
        // preserva o combo counter. Senão, decide pelo tempo no chão (curto = preserva).
        const bool bPreserve = Combo->IsInCancelWindow() || Combo->HasAerialContinuation();
        if (!bPreserve)
        {
            // Não reseta imediatamente — dá grace window para conectar aerial attack.
            Combo->RequestDeferredReset(/*GraceSeconds=*/0.35f);
        }
    }
}
```

> O `RequestDeferredReset` é um método novo a adicionar no `UDFComboComponent` — ver §16.3.

### 7.3 Dodge integration (refinada)

Air dodge **sim** ou **não**? Decisão do projeto:

- **Air dodge desabilitado** (Souls-like): bloqueie via `BlockAbilitiesWithTag` (`State_Jumping`, `State_Falling`)
- **Air dodge habilitado** (DMC / Bayonetta): permita; ajuste o `UDFAbility_Dodge` para usar `LaunchCharacter` em vez de root-motion no ar

Sugestão para este projeto (intermediário): **um air dodge por pulo**.

```cpp
// UDFAbility_Dodge::PostInitProperties — NÃO adicionar State_Jumping/Falling em BlockAbilitiesWithTag.
// Em vez disso, controlar via UDFAbility_Dodge::CanActivateAbility:
bool UDFAbility_Dodge::CanActivateAbility(...) const
{
    if (!Super::CanActivateAbility(...)) return false;
    if (ASC && ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Falling))
    {
        // Permite 1 air dodge — controla via attribute "AirDodgeChargesRemaining".
        if (bIsAirDodgeUsedThisJump) return false;
    }
    return true;
}
```

O flag `bIsAirDodgeUsedThisJump` resetaria em `OnMovementModeChanged` quando volta a `MOVE_Walking`.

### 7.4 Lock-on integration

Jump em lock-on deve **manter** o lock — não solta automaticamente. O `UDFLockOnComponent::Tick` já solta só se `IsTargetValid` falhar. Pular não invalida o target.

**Comportamento adicional sugerido:** durante `State.Jumping` ou `State.Falling`, suprimir o `UpdateLockOnRotation` no `UDFCameraComponent` para evitar câmera "tonta" durante o arco do pulo. Patch em `UDFCameraComponent::TickComponent`:

```cpp
if (CurrentState == ECameraState::LockOn && LockOnTarget.IsValid())
{
    bool bSuppress = false;
    if (const APawn* const Owner = Cast<APawn>(GetOwner()))
    {
        if (IAbilitySystemInterface* const I = Cast<IAbilitySystemInterface>(Owner))
        {
            if (UAbilitySystemComponent* const ASC = I->GetAbilitySystemComponent())
            {
                bSuppress = ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Jumping)
                         || ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Falling);
            }
        }
    }
    if (!bSuppress) UpdateLockOnRotation(DeltaTime);
}
```

### 7.5 Strafe & jump direction

Quando lock-on ativo, `bShouldStrafe = true`, e a `MovementDirection` é 8-way relativa ao alvo. O `LastJumpDirection` capturada no takeoff já reflete isso → land anim é coerente com a direção em relação ao inimigo (jump-back enquanto faceando o boss → `Land_Backward`).

---

## 8. C++ — AnimNotify: apex, landing recovery, footstep

### 8.1 `UAnimNotify_JumpApex` (single event)

**Novo arquivo:** `Source/DungeonForged/Public/Animation/AN/AnimNotify_JumpApex.h`

```cpp
#pragma once
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_JumpApex.generated.h"

UCLASS()
class DUNGEONFORGED_API UAnimNotify_JumpApex : public UAnimNotify
{
    GENERATED_BODY()
public:
    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;
};
```

**Cpp:**

```cpp
#include "Animation/AN/AnimNotify_JumpApex.h"
#include "DungeonForgedModule.h"

void UAnimNotify_JumpApex::Notify(USkeletalMeshComponent* const MeshComp,
    UAnimSequenceBase* const Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);
    UE_LOG(LogDungeonForged, Log, TEXT("[Jump] Apex notify on %s"),
        *GetNameSafe(MeshComp ? MeshComp->GetOwner() : nullptr));
    // VFX/SFX hook — connect via Blueprint event "OnJumpApex" on the character.
}
```

> **Uso:** em **Jump_Loop_0**, no frame que corresponde ao peak (geralmente ~50% do loop), adicionar este Notify. Conecta no BP para shake leve / dust trail / sound subtle.

### 8.2 `UAnimNotifyState_LandingRecovery` (window)

**Novo arquivo:** `Source/DungeonForged/Public/Animation/AN/AnimNotifyState_LandingRecovery.h`

```cpp
#pragma once
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "AnimNotifyState_LandingRecovery.generated.h"

/**
 * During this notify state window, applies State.Landing tag to the character's ASC.
 * Blocks attack/dodge/jump for the duration. Place over the end of the Land_* anim.
 */
UCLASS()
class DUNGEONFORGED_API UAnimNotifyState_LandingRecovery : public UAnimNotifyState
{
    GENERATED_BODY()
public:
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;
};
```

**Cpp:**

```cpp
#include "Animation/AN/AnimNotifyState_LandingRecovery.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/DFGameplayTags.h"

void UAnimNotifyState_LandingRecovery::NotifyBegin(USkeletalMeshComponent* const MeshComp,
    UAnimSequenceBase* const Animation, const float TotalDuration,
    const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
    if (UAbilitySystemComponent* const ASC =
            UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp ? MeshComp->GetOwner() : nullptr))
    {
        ASC->AddLooseGameplayTag(FDFGameplayTags::State_Landing);
    }
}

void UAnimNotifyState_LandingRecovery::NotifyEnd(USkeletalMeshComponent* const MeshComp,
    UAnimSequenceBase* const Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);
    if (UAbilitySystemComponent* const ASC =
            UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp ? MeshComp->GetOwner() : nullptr))
    {
        ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Landing, 0);
    }
}
```

### 8.3 `UAnimNotify_JumpFootstep` (já pode ter)

Se o projeto já tem `UAnimNotify_DFFootstep` ou similar, usar nos Land_* para som de "thump" no toque. Verificar em `Source/DungeonForged/Public/Animation/AN/`.

---

## 9. C++ — Tuning DataAsset

### Patch — `UDFCombatTuningData.h`

Adicionar categoria `"Jump"`:

```cpp
// ── Jump ────────────────────────────────────────────────────────
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump", meta = (ClampMin = "0.0"))
float JumpZVelocity = 550.f;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump", meta = (ClampMin = "0.0", ClampMax = "1.0"))
float JumpAirControl = 0.35f;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump", meta = (ClampMin = "0.0"))
float JumpGravityScale = 1.7f;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump", meta = (ClampMin = "1.0"))
float JumpFallGravityMultiplier = 1.25f;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump", meta = (ClampMin = "0.0"))
float JumpStaminaCost = 10.f;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump", meta = (ClampMin = "0.0"))
float JumpCooldown = 0.20f;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump", meta = (ClampMin = "0.0"))
float JumpLandingRecoveryWindow = 0.20f;
```

**`UDFCharacterMovementComponent::BeginPlay`** — pull do DataAsset:

```cpp
if (const UDFCombatTuningData* const Tuning = UDFAssetManager::GetCombatTuningDataSafe())
{
    DFJumpZVelocity         = Tuning->JumpZVelocity;
    DFAirControl            = Tuning->JumpAirControl;
    DFGravityScale          = Tuning->JumpGravityScale;
    DFFallGravityMultiplier = Tuning->JumpFallGravityMultiplier;
    DFJumpStaminaCost       = Tuning->JumpStaminaCost;
    DFJumpCooldown          = Tuning->JumpCooldown;
    DFLandingRecoveryWindow = Tuning->JumpLandingRecoveryWindow;
    JumpZVelocity = DFJumpZVelocity;
    AirControl    = DFAirControl;
    GravityScale  = DFGravityScale;
}
```

---

## 10. C++ — Debug commands `df.JumpDebug`

### Patch — `UDFCheatManager.cpp`

```cpp
// Inside the anonymous namespace, after Cmd_df_dodgedebug:

#if !UE_BUILD_SHIPPING

static TAutoConsoleVariable<int32> CVarDF_DebugJump(
    TEXT("df.DebugJump"),
    0,
    TEXT("DungeonForged jump debug.\n")
    TEXT(" 0: Off\n")
    TEXT(" 1: Log [Jump|...] (takeoff, apex, land, recovery)\n")
    TEXT(" 2: Log + on-screen HUD (AirTime, VerticalVel, PredictedLand, JumpDirection)"),
    ECVF_Cheat);

static void Cmd_df_jumpdebug(TArray<FString> const& Args)
{
    IConsoleVariable* const Cv = IConsoleManager::Get().FindConsoleVariable(TEXT("df.DebugJump"));
    if (!Cv) { DF_LOG(Warning, "df.JumpDebug: CVar não encontrado"); return; }

    if (Args.Num() > 0)
    {
        const FString A = Args[0].ToLower();
        if (A == TEXT("dump"))
        {
            UWorld* const W = GetCheatWorld();
            ADFPlayerCharacter* const P = GetLocalDFPawn(W);
            if (P)
            {
                if (UUDFAnimInstance* const Anim = Cast<UUDFAnimInstance>(P->GetMesh()->GetAnimInstance()))
                {
                    DF_LOG(Log, "[Jump|Dump] bIsJumping=%d bIsFalling=%d bIsLanding=%d AirTime=%.2f VertVel=%.0f PredLand=%.0f JumpDir=%d",
                        Anim->bIsJumping ? 1 : 0, Anim->bIsFalling ? 1 : 0, Anim->bIsLanding ? 1 : 0,
                        Anim->AirTime, Anim->VerticalVelocity, Anim->PredictedLandingDistance,
                        static_cast<int32>(Anim->LastJumpDirection));
                }
                if (UDFCharacterMovementComponent* const CMC =
                        Cast<UDFCharacterMovementComponent>(P->GetCharacterMovement()))
                {
                    DF_LOG(Log, "[Jump|Dump] JumpZVel=%.0f AirControl=%.2f Gravity=%.2f CooldownRem=%.2f",
                        CMC->JumpZVelocity, CMC->AirControl, CMC->GravityScale, CMC->GetJumpCooldownRemaining());
                }
            }
            return;
        }
        if (A == TEXT("0") || A == TEXT("off"))      { Cv->Set(0, ECVF_SetByConsole); }
        else if (A == TEXT("1") || A == TEXT("log")) { Cv->Set(1, ECVF_SetByConsole); }
        else if (A == TEXT("2") || A == TEXT("hud") || A == TEXT("on"))
                                                     { Cv->Set(2, ECVF_SetByConsole); }
        else
        {
            DF_LOG(Warning, "df.JumpDebug: use [0|1|2|dump|log|hud|on|off]");
            return;
        }
    }
    else
    {
        const int32 Next = Cv->GetInt() >= 2 ? 0 : (Cv->GetInt() + 1);
        Cv->Set(Next, ECVF_SetByConsole);
    }

    DF_LOG(Log, "df.JumpDebug: df.DebugJump=%d (0=off 1=log 2=log+hud)", Cv->GetInt());
}

static FAutoConsoleCommand GCmdJumpDebug(
    TEXT("df.JumpDebug"),
    TEXT("Jump debug: toggle df.DebugJump (0/1/2). Args: dump | 0 | 1 | 2 | log | hud | on | off."),
    FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_jumpdebug));

#endif
```

### HUD on-screen — opcional no AnimInstance

Em `NativeUpdateAnimation`, no final, debug condicional:

```cpp
#if !UE_BUILD_SHIPPING
if (const IConsoleVariable* const Cv = IConsoleManager::Get().FindConsoleVariable(TEXT("df.DebugJump")))
{
    if (Cv->GetInt() >= 2 && GEngine && OwningCharacter && OwningCharacter->IsLocallyControlled())
    {
        GEngine->AddOnScreenDebugMessage(0x100, 0.f, FColor::Cyan,
            FString::Printf(TEXT("Jump: J=%d F=%d L=%d Air=%.2fs Vz=%.0f Land=%.0f Dir=%d"),
                bIsJumping ? 1 : 0, bIsFalling ? 1 : 0, bIsLanding ? 1 : 0,
                AirTime, VerticalVelocity, PredictedLandingDistance,
                static_cast<int32>(LastJumpDirection)));
    }
}
#endif
```

---

## 11. Setup Editor passo-a-passo

### Passo 1: Recompilar C++

Live Coding ou rebuild via VS. As novas structs / variáveis / notify classes precisam aparecer no editor.

### Passo 2: Configurar `BP_JCHero_Character` (ou onde quer que esteja `UDFCharacterMovementComponent`)

Em **Class Defaults → DF | Movement | Jump**:

| Param | Valor sugerido |
|---|---|
| `DF Jump Z Velocity` | **550** (default 420 = pulinho weak) |
| `DF Air Control` | **0.35** (default 0.05 = quase nada) |
| `DF Gravity Scale` | **1.7** (default 1.0 = floaty) |
| `DF Fall Gravity Multiplier` | **1.25** |
| `DF Jump Stamina Cost` | **10** |
| `DF Jump Cooldown` | **0.20** |
| `DF Landing Recovery Window` | **0.20** |

### Passo 3: Configurar `FUDAnimSet` no `BP_JCHero_Character` (DefaultAnimSet — unarmed)

**ABP_JCHero → Class Defaults → Default Anim Set → Jump Set**:

| Slot | Animação |
|---|---|
| Start Idle    | `Jump_Start_0_Seq_Retarged` |
| Start Forward | `Jump_Start_F_0_Seq_Retarged` |
| Start Backward| `Jump_Start_B_180_Seq_Retarged` |
| Start Left    | `Jump_Start_L_90_Seq_Retarged` |
| Start Right   | `Jump_Start_R_90_Seq_Retarged` |
| Loop          | `Jump_Loop_0_Seq_Retarged` |
| Land Idle     | `Jump_End_0_Seq_Retarged` |
| Land Forward  | `Jump_End_F_0_Seq_Retarged` |
| Land Backward | `Jump_End_B_180_Seq_Retarged` |
| Land Left     | `Jump_End_L_90_Seq_Retarged` |
| Land Right    | `Jump_End_R_90_Seq_Retarged` |

### Passo 4: Configurar `WeaponAnimSet` no `DT_Items` (Armed)

Para cada arma no DataTable `DT_Items`, em **Weapon Anim Set → Jump Set**:

| Slot | Animação |
|---|---|
| Start Idle    | `Jump_Combat_Start_0_Seq_Retarged` |
| Start Forward | `Jump_Combat_Start_F_0_Seq_Retarged` |
| Start Backward| `Jump_Combat_Start_B_180_Seq_Retarged` |
| Start Left    | `Jump_Combat_Start_L_90_Seq_Retarged` |
| Start Right   | `Jump_Combat_Start_R_90_Seq_Retarged` |
| Loop          | `Jump_Combat_Loop_0_Seq_Retarged` |
| Land Idle     | `Jump_Combat_End_0_Seq_Retarged` |
| Land Forward  | `Jump_Combat_End_F_0_Seq_Retarged` |
| Land Backward | `Jump_Combat_End_B_180_Seq_Retarged` |
| Land Left     | `Jump_Combat_End_L_90_Seq_Retarged` |
| Land Right    | `Jump_Combat_End_R_90_Seq_Retarged` |

### Passo 5: Adicionar Notifies às anims

1. Abrir `Jump_Loop_0_Seq_Retarged` (e versão Combat)
   - No timeline, **~50% da duração**, adicionar Notify: `UAnimNotify_JumpApex`
   - Hook em BP no `OnJumpApex` → spawn dust particle / play `SFX_Jump_Apex`

2. Abrir cada um dos **10 `Jump_End_*`** (5 unarmed + 5 armed)
   - No final do timeline, **últimos 0.20s**, adicionar Notify State: `UAnimNotifyState_LandingRecovery`
   - Isso aplica `State.Landing` tag durante esse intervalo → bloqueia attack/dodge

3. (Opcional) Adicionar `UAnimNotify_DFFootstep` (se existir) no frame de contato dos `Jump_End_*` para som de "thump" no impacto

### Passo 6: Configurar `IMC_DFDefault` (já existe)

Verificar `IA_Jump` está bindado a `SpaceBar` (PC) e `Gamepad Face Bottom` (controller).

### Passo 7: Configurar `UDFCombatTuningData`

Abrir `DA_CombatTuning.uasset` (ou o asset usado em `UDFAssetManager::CombatTuningDataAsset`). Setar valores da categoria "Jump" conforme §9.

### Passo 8: Testar

PIE com `df.JumpDebug 2` ativo. Checklist:

1. Parado + Space → `Jump_Start_0` (idle takeoff) → ✓
2. Andando + W + Space → `Jump_Start_F_0` (forward takeoff) → ✓
3. Andando + A + Space → `Jump_Start_L_90` (left takeoff) → ✓
4. Andando + D + Space → `Jump_Start_R_90` (right takeoff) → ✓
5. Recuando + S + Space → `Jump_Start_B_180` (backward takeoff) → ✓
6. No ar: pular não responde (cooldown 200ms) → ✓
7. No ar: tag `State.Jumping` ativa subindo, troca para `State.Falling` no apex → ✓
8. No ar: AnimNotify_JumpApex dispara no peak → ✓
9. Aterrissar: anim de Land usa direção do takeoff → ✓
10. Durante landing recovery: attack/dodge bloqueados → ✓
11. Em combate (armado): usa Jump_Combat_* — pulo "mais pesado" → ✓
12. Em lock-on + W + Space + W (face inimigo): land é `Jump_End_F_0` → ✓
13. Sem stamina (< 10): pulo é negado → ✓
14. `df.JumpDebug dump` no console mostra estado completo → ✓

---

## 12. AnimBP — state machine de jump

### State machine sugerida

```
                      [Locomotion (Grounded)]
                                │
                                │ bIsInAir == true
                                ▼
                       [JumpStart Substate]
                       Anim: ResolveStart(LastJumpDirection)
                       Exit: AnimAtEnd OR Velocity.Z <= 0
                                │
                                ▼
                       [JumpLoop Substate]
                       Anim: JumpSet.Loop
                       Exit: PredictedLandingDistance < LandPreparationThreshold
                                │
                                ▼
                  [JumpLand Pre-blend Substate]
                  Anim: ResolveLand(LastJumpDirection)  (cross-fade in)
                  Exit: bIsInAir == false (touched ground)
                                │
                                ▼
                       [Landed Substate]
                       Anim: continue Land anim from current frame
                       Exit: AnimAtEnd OR Notify_LandingRecoveryEnd
                                │
                                ▼
                      [Locomotion (Grounded)]
```

### Nodes principais

**Para resolver start:**
```
Break ActiveAnimSet → JumpSet → ResolveStart (input: LastJumpDirection) → Sequence Player
```

**Para pre-blend land:**
```
Layered Blend per Bone (root)
   ├── Pose 1: JumpLoop (full body)
   └── Pose 2: JumpLand pre-blend (full body)
        Alpha = 1.0 - (PredictedLandingDistance / LandPreparationThreshold)
```

### Variáveis críticas no AnimBP

Todas já estão em `UUDFAnimInstance` após §4:

- `bIsInAir`, `bIsJumping`, `bIsFalling`, `bIsLanding`
- `LastJumpDirection` (enum)
- `AirTime`, `VerticalVelocity`, `PredictedLandingDistance`
- `LandPreparationThreshold` (config)

---

## 13. Checklist de validação

### Funcionalidade
- [ ] Space → personagem pula com `JumpZVelocity = 550` (mais snappy que UE default)
- [ ] Anim direcional resolve corretamente para cada cardinal (F/B/L/R)
- [ ] Diagonais fazem fallback para cardinal mais próxima (sem null anim)
- [ ] Armed vs unarmed troca anim set automaticamente (via WeaponAnimSet)
- [ ] `LastJumpDirection` é capturada no takeoff e **não muda no ar**
- [ ] Land anim é a da direção do takeoff (não da direção atual no ar)

### GAS / tags
- [ ] `State.Jumping` ativa subindo → vira `State.Falling` no apex (Velocity.Z <= 0)
- [ ] `State.Landing` ativa no toque, expira após 0.20s (ou no `NotifyEnd`)
- [ ] Tags impedem: jump durante dodge/attack/cast/stun/exhausted/dead
- [ ] Combo reseta no takeoff
- [ ] Dodge é bloqueado durante `State.Jumping` / `State.Falling`

### Movimento
- [ ] Air control ≈ 0.35 — dá controle aéreo perceptível mas não voador
- [ ] Fall gravity multiplier 1.25× faz arco snappy (não floaty)
- [ ] Anti-spam: 2º jump dentro de 0.20s do anterior é ignorado
- [ ] Stamina drena 10 por pulo; com < 10, pulo é negado

### Anim feedback
- [ ] `AnimNotify_JumpApex` dispara no peak do `Jump_Loop_0` (~50%)
- [ ] `AnimNotifyState_LandingRecovery` mantém `State.Landing` durante a window
- [ ] Pre-blend de land começa antes do toque (`GroundDistance < 250cm`)

### Lock-on
- [ ] Câmera não roda durante o arco do pulo (suprime `UpdateLockOnRotation`)
- [ ] Lock-on persiste através do pulo (target válido permanece)
- [ ] Strafing + jump → land coerente com direção relativa ao alvo

### Debug
- [ ] `df.JumpDebug 1` faz log de takeoff / apex / land no Output Log
- [ ] `df.JumpDebug 2` mostra HUD on-screen com AirTime, Vz, PredLand, JumpDir
- [ ] `df.JumpDebug dump` imprime estado completo do AnimInstance + CMC
- [ ] `showdebug AbilitySystem` confirma transição `State.Jumping → State.Falling → State.Landing`

### Frame target ideal
- **Takeoff response**: < 50ms do press até `bIsJumping = true`
- **Apex altitude**: ~120cm (com JumpZVel=550 e GravityScale=1.7)
- **Air time total**: ~0.8s (start→loop→land)
- **Landing recovery**: 200ms — input de attack durante esse intervalo é **buffered** (ver doc 06 / input buffer)

---

## 14. Tabela de arquivos

| Arquivo | Status | O que muda |
|---|---|---|
| [`DFAnimSetTypes.h`](../../Source/DungeonForged/Public/Animation/DFAnimSetTypes.h) | 🔧 Modificar | Adicionar `FUDJumpAnimSet`, expandir `FUDAnimSet::JumpSet`, manter legacy fields |
| `DFAnimSetTypes.cpp` (novo) | ✅ Criar | Implementar `ResolveStart()` / `ResolveLand()` com fallback cardinal |
| [`UDFAnimInstance.h`](../../Source/DungeonForged/Public/Animation/UDFAnimInstance.h) | 🔧 Modificar | Adicionar `bIsJumping`, `bIsFalling`, `bIsLanding`, `LastJumpDirection`, `AirTime`, `VerticalVelocity`, `PredictedLandingDistance`, `LandPreparationThreshold` |
| [`UDFAnimInstance.cpp`](../../Source/DungeonForged/Private/Animation/UDFAnimInstance.cpp) | 🔧 Modificar | Lógica de derivação no `NativeUpdateAnimation` (takeoff capture, AirTime, line trace) |
| [`UDFCharacterMovementComponent.h`](../../Source/DungeonForged/Public/Characters/UDFCharacterMovementComponent.h) | 🔧 Modificar | `DFJumpZVelocity`, `DFAirControl`, `DFGravityScale`, `DFFallGravityMultiplier`, `DFJumpStaminaCost`, `DFJumpCooldown`, `DFLandingRecoveryWindow`, `TimerHandle_EndLanding`, `DoJump()` override, `GetJumpCooldownRemaining()` |
| [`UDFCharacterMovementComponent.cpp`](../../Source/DungeonForged/Private/Characters/UDFCharacterMovementComponent.cpp) | 🔧 Modificar | `DoJump()` override, apex gravity boost no Tick, tag set/clear em `OnMovementModeChanged` |
| [`DFGameplayTags.h/.cpp`](../../Source/DungeonForged/Public/GAS/DFGameplayTags.h) | 🔧 Modificar | Adicionar `State_Jumping`, `State_Falling`, `State_Landing`, `Ability_Movement_Jump` |
| [`ADFPlayerCharacter.h/.cpp`](../../Source/DungeonForged/Public/Characters/ADFPlayerCharacter.h) | 🔧 Modificar | Override `Jump()` com gate de tags |
| `AnimNotify_JumpApex.h/.cpp` (novo) | ✅ Criar | Notify único disparado no peak |
| `AnimNotifyState_LandingRecovery.h/.cpp` (novo) | ✅ Criar | NotifyState que aplica `State.Landing` durante a window |
| [`UDFCombatTuningData.h`](../../Source/DungeonForged/Public/Data/UDFCombatTuningData.h) | 🔧 Modificar | Categoria `"Jump"` com 7 params |
| [`UDFCheatManager.cpp`](../../Source/DungeonForged/Private/Debug/UDFCheatManager.cpp) | 🔧 Modificar | `df.JumpDebug` cheat + CVar `df.DebugJump` |
| [`UDFCameraComponent.cpp`](../../Source/DungeonForged/Private/Camera/UDFCameraComponent.cpp) | 🔧 Modificar | Suprimir `UpdateLockOnRotation` durante `State.Jumping`/`State.Falling` |
| [`DFAbility_Dodge.cpp`](../../Source/DungeonForged/Private/GAS/Abilities/DFAbility_Dodge.cpp) | 🔧 Modificar | Adicionar `State_Jumping`, `State_Falling` ao `BlockAbilitiesWithTag` |
| Anims `Jump_*` (existem) | 🔧 Editor | Adicionar notifies (Apex no Loop, LandingRecovery no End) |
| `BP_JCHero_Character` | 🔧 Editor | Setar JumpSet do DefaultAnimSet com os 11 unarmed |
| `DT_Items` (rows de armas) | 🔧 Editor | Setar `WeaponAnimSet.JumpSet` com os 11 combat |
| `DA_CombatTuning.uasset` | 🔧 Editor | Setar valores da categoria "Jump" |
| `ABP_JCHero.uasset` | 🔧 Editor | State machine usa `ResolveStart`/`ResolveLand` + `Layered Blend per Bone` no pre-blend |

---

## 15. Próximos passos (AAA polish)

> **Nota:** combate aéreo (ataques no ar, combos com jump, plunge, launcher, jump-cancel) **não é "próximo passo" — é requisito do jogo**. Ver [§16](#16-c--combate-aéreo--combos-com-jump) para spec completa. Os itens abaixo são polish secundário, não core mechanics.

1. **Double jump** — `JumpMaxCount = 2` + ability `Ability_Movement_DoubleJump` que requer item/passive
2. **Wall jump** — sweep horizontal no apex; se hit em parede, re-apply impulse perpendicular
3. **Roll-into-jump** — se buffer pressionar jump no último frame de `State.Landing`, executar pequeno "spring jump" sem stamina cost
4. **Long jump** — segurar `IA_Jump` para horizontalizar a velocidade (steeper arc with hold)
5. **Coyote time** — permite jump até 100ms depois de sair de uma plataforma (lenient ledge)
6. **Jump curve** — substituir gravity scale linear por curve asset (`UCurveFloat`) para arcos com ease-in/ease-out
7. **VFX trail** — Niagara emissor no socket `root` durante `State.Falling`
8. **Camera vertical kick** no land se `VerticalVelocity < -500` (heavy land) — já tem reference no doc 01

---

## 16. C++ — Combate Aéreo & Combos com Jump

> Sistema que permite **ataques aéreos**, **combos que incluem pulo como step** (e.g., 3 hits no chão → launcher → 3 hits no ar → plunge), **jump-cancel** (cancelar qualquer step de combo em pulo para juggle setup) e **air dodge**. Inspiração: DMC5, Bayonetta, Stranger of Paradise.

### 16.1 Pilares do sistema

```
┌─────────────────────────────────────────────────────────────────────────┐
│  GROUND COMBOS                          AERIAL COMBOS                   │
│  ──────────────                         ──────────────                  │
│  Light: L L L L                         Air Light:  AL AL AL            │
│  Heavy: H                               Air Heavy:  AH                  │
│  Launcher: L L H↑  (uppercut → both     Plunge:     AH↓ (downward       │
│            popam pro ar)                            slam, com slam      │
│                                                     damage AOE)         │
│                                                                         │
│  CONNECTIONS                                                            │
│  ──────────────                                                         │
│  Jump-cancel: durante cancel window de QUALQUER attack ground,          │
│               pode-se pular → continua combo no ar                      │
│  Launcher → Air: Heavy[3] no fim do combo terrestre lança ambos         │
│                  pro ar; entra no aerial mode automaticamente            │
│  Plunge → Ground: Air Heavy↓ no ar bate forte no chão, AOE radial,      │
│                   reseta combo counter para extensão terrestre          │
│  Land-cancel: Air attack que aterrissa no meio do swing continua        │
│               como ground attack equivalente (continuidade visual)      │
└─────────────────────────────────────────────────────────────────────────┘
```

### 16.2 Tags GAS adicionais

**Arquivo:** [`DFGameplayTags.h/.cpp`](../../Source/DungeonForged/Public/GAS/DFGameplayTags.h)

```cpp
// Aerial state
static FGameplayTag State_Attacking_Aerial;       // ataque aéreo em curso
static FGameplayTag State_Aerial_ComboActive;     // combo aéreo em janela ativa
static FGameplayTag State_Launching;              // launcher pop-up em execução

// Aerial ability tags
static FGameplayTag Ability_Attack_Melee_Aerial;        // pai genérico
static FGameplayTag Ability_Attack_Melee_Aerial_Light;
static FGameplayTag Ability_Attack_Melee_Aerial_Heavy;
static FGameplayTag Ability_Attack_Melee_Aerial_Plunge;
static FGameplayTag Ability_Attack_Melee_Launcher;       // ground attack que pop-up
```

**Em `DFGameplayTags.cpp::RegisterGameplayTags`:**
```cpp
DF_TAG(State_Attacking_Aerial)(FName("State.Attacking.Aerial"), FString("Aerial attack montage active."));
DF_TAG(State_Aerial_ComboActive)(FName("State.Aerial.ComboActive"), FString("Aerial combo window open."));
DF_TAG(State_Launching)(FName("State.Launching"), FString("Launcher upward impulse in progress."));
DF_TAG(Ability_Attack_Melee_Aerial)(FName("Ability.Attack.Melee.Aerial"), FString("Generic aerial melee attack."));
DF_TAG(Ability_Attack_Melee_Aerial_Light)(FName("Ability.Attack.Melee.Aerial.Light"), FString());
DF_TAG(Ability_Attack_Melee_Aerial_Heavy)(FName("Ability.Attack.Melee.Aerial.Heavy"), FString());
DF_TAG(Ability_Attack_Melee_Aerial_Plunge)(FName("Ability.Attack.Melee.Aerial.Plunge"), FString());
DF_TAG(Ability_Attack_Melee_Launcher)(FName("Ability.Attack.Melee.Launcher"), FString());
```

### 16.3 Patches no `UDFComboComponent`

**Header — `UDFComboComponent.h`:**

```cpp
// ── Aerial mode ─────────────────────────────────────────────────────
/** True when current combo state lives in the air (between Launcher and Land). */
UPROPERTY(BlueprintReadOnly, Category = "DF|Combat|Combo|Aerial")
bool bIsAerialComboActive = false;

/** True if the next step of the combo should switch to aerial montages
 *  (set when a Launcher fires; cleared on Land or combo reset). */
UPROPERTY(BlueprintReadOnly, Category = "DF|Combat|Combo|Aerial")
bool bHasAerialContinuation = false;

UFUNCTION(BlueprintPure, Category = "DF|Combat|Combo")
bool HasAerialContinuation() const { return bHasAerialContinuation; }

UFUNCTION(BlueprintPure, Category = "DF|Combat|Combo")
bool IsInCancelWindow() const { return bAbilityCancelWindowOpen; }

/** Defer the combo reset by GraceSeconds — lets player chain into aerial attack
 *  after takeoff without losing the combo counter. Cancelled if an aerial attack
 *  activates within the grace window. */
UFUNCTION(BlueprintCallable, Category = "DF|Combat|Combo")
void RequestDeferredReset(float GraceSeconds);

/** Cancel the current playing montage (used by jump-cancel). */
UFUNCTION(BlueprintCallable, Category = "DF|Combat|Combo")
void CancelCurrentMontage();

/** Called by aerial ability activate; cancels pending deferred reset. */
UFUNCTION(BlueprintCallable, Category = "DF|Combat|Combo")
void ConfirmAerialContinuation();

/** Called when the character lands. If combo was aerial-active, decides whether
 *  to chain to ground variant (land-cancel) or reset. */
UFUNCTION(BlueprintCallable, Category = "DF|Combat|Combo")
void OnLanded();

protected:
    FTimerHandle TimerHandle_DeferredReset;
    bool bAbilityCancelWindowOpen = false;
```

**Cpp — pontos chave:**

```cpp
void UDFComboComponent::RequestDeferredReset(const float GraceSeconds)
{
    if (UWorld* const W = GetWorld())
    {
        W->GetTimerManager().SetTimer(TimerHandle_DeferredReset, this,
            &UDFComboComponent::ResetCombo, GraceSeconds, false);
    }
}

void UDFComboComponent::ConfirmAerialContinuation()
{
    bIsAerialComboActive = true;
    bHasAerialContinuation = true;
    if (UWorld* const W = GetWorld())
    {
        W->GetTimerManager().ClearTimer(TimerHandle_DeferredReset);
    }
    if (UAbilitySystemComponent* const ASC = GetOwnerASC())
    {
        ASC->AddLooseGameplayTag(FDFGameplayTags::State_Aerial_ComboActive);
    }
}

void UDFComboComponent::OnLanded()
{
    if (UAbilitySystemComponent* const ASC = GetOwnerASC())
    {
        ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Aerial_ComboActive, 0);
    }

    if (bIsAerialComboActive && bIsPlayingMontage)
    {
        // Land-cancel: anim aérea ainda tocando — não reseta, deixa a anim terminar
        // ou transiciona para a continuação terrestre (handled pelo AnimBP).
        bIsAerialComboActive = false;
        return;
    }

    // Sem combo aéreo ativo → reseta após uma pequena grace (permite re-entrada).
    RequestDeferredReset(0.25f);
}

void UDFComboComponent::CancelCurrentMontage()
{
    if (USkeletalMeshComponent* const Mesh = GetOwnerMesh())
    {
        if (UAnimInstance* const Anim = Mesh->GetAnimInstance())
        {
            Anim->Montage_Stop(0.05f); // 50ms blend out — preserva fluidez
        }
    }
    bIsPlayingMontage = false;
}
```

### 16.4 Cancel window do attack permite Jump

**Arquivo:** [`ANS_DFAbilityCancelWindow.cpp`](../../Source/DungeonForged/Private/Combat/AN/ANS_DFAbilityCancelWindow.cpp)

Adicionar `Ability_Movement_Jump` no `AllowedCancelTags` por default (como já está feito com `Ability_Movement_Dodge`):

```cpp
UANS_DFAbilityCancelWindow::UANS_DFAbilityCancelWindow()
{
    // ... existing ...
    if (FDFGameplayTags::Ability_Movement_Dodge.IsValid())
    {
        AllowedCancelTags.AddTag(FDFGameplayTags::Ability_Movement_Dodge);
    }
    if (FDFGameplayTags::Ability_Movement_Jump.IsValid())   // ← NOVO
    {
        AllowedCancelTags.AddTag(FDFGameplayTags::Ability_Movement_Jump);
    }
    if (FDFGameplayTags::Ability_Attack_Melee_Aerial.IsValid())   // ← NOVO
    {
        AllowedCancelTags.AddTag(FDFGameplayTags::Ability_Attack_Melee_Aerial);
    }
}
```

### 16.5 `UDFAbility_AerialAttack` — base genérica

**Novo arquivo:** `Source/DungeonForged/Public/GAS/Abilities/UDFAbility_AerialAttack.h`

```cpp
#pragma once
#include "GAS/UDFGameplayAbility.h"
#include "Combat/DFAerialAttackTypes.h"
#include "UDFAbility_AerialAttack.generated.h"

UENUM(BlueprintType)
enum class EDFAerialAttackKind : uint8
{
    Light,
    Heavy,
    Plunge
};

UCLASS(Abstract)
class DUNGEONFORGED_API UDFAbility_AerialAttack : public UDFGameplayAbility
{
    GENERATED_BODY()
public:
    UDFAbility_AerialAttack();

    UPROPERTY(EditDefaultsOnly, Category = "DF|Combat|Aerial")
    EDFAerialAttackKind Kind = EDFAerialAttackKind::Light;

    UPROPERTY(EditDefaultsOnly, Category = "DF|Combat|Aerial")
    TObjectPtr<UAnimMontage> AerialMontage;

    /** Plunge: launches character straight down with this Z speed. */
    UPROPERTY(EditDefaultsOnly, Category = "DF|Combat|Aerial|Plunge", meta = (EditCondition = "Kind==EDFAerialAttackKind::Plunge"))
    float PlungeDownVelocity = -1800.f;

    /** Plunge: AOE radius on ground impact. */
    UPROPERTY(EditDefaultsOnly, Category = "DF|Combat|Aerial|Plunge", meta = (EditCondition = "Kind==EDFAerialAttackKind::Plunge"))
    float PlungeImpactRadius = 350.f;

    /** Plunge: stay-in-place hover before slam (anticipation). */
    UPROPERTY(EditDefaultsOnly, Category = "DF|Combat|Aerial|Plunge", meta = (EditCondition = "Kind==EDFAerialAttackKind::Plunge"))
    float PlungeHoverDuration = 0.20f;

protected:
    virtual void PostInitProperties() override;

    virtual bool CanActivateAbility(...) const override;

    virtual void ActivateAbility(...) override;

    UFUNCTION()
    void OnAerialMontageCompleted();

    UFUNCTION()
    void OnPlungeHoverEnd();
};
```

**Cpp — `PostInitProperties`:**

```cpp
void UDFAbility_AerialAttack::PostInitProperties()
{
    Super::PostInitProperties();
    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        AbilityTags.AddTag(FDFGameplayTags::Ability_Attack_Melee_Aerial);

        // Cada subclasse adiciona a tag mais específica no seu próprio CDO.
        switch (Kind)
        {
        case EDFAerialAttackKind::Light:
            AbilityTags.AddTag(FDFGameplayTags::Ability_Attack_Melee_Aerial_Light);
            break;
        case EDFAerialAttackKind::Heavy:
            AbilityTags.AddTag(FDFGameplayTags::Ability_Attack_Melee_Aerial_Heavy);
            break;
        case EDFAerialAttackKind::Plunge:
            AbilityTags.AddTag(FDFGameplayTags::Ability_Attack_Melee_Aerial_Plunge);
            break;
        }

        ActivationOwnedTags.AddTag(FDFGameplayTags::State_Attacking);
        ActivationOwnedTags.AddTag(FDFGameplayTags::State_Attacking_Aerial);

        BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
        BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Stunned);
        BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dodging);
        // NOTA: NÃO bloqueamos por State.Attacking — aerial light pode chainar
        // light→light→light no ar. O cancel é controlado pelo cancel window.
    }
}
```

**Cpp — `CanActivateAbility`** (deve estar **no ar**):

```cpp
bool UDFAbility_AerialAttack::CanActivateAbility(...) const
{
    if (!Super::CanActivateAbility(...)) return false;
    if (!ActorInfo) return false;

    const ACharacter* const Char = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
    if (!Char) return false;

    // Tem que estar no ar OU ser plunge sem distinção (plunge pode ativar no chão e gerar pequeno hop).
    if (Kind != EDFAerialAttackKind::Plunge)
    {
        const UCharacterMovementComponent* const CMC = Char->GetCharacterMovement();
        if (!CMC || !CMC->IsFalling()) return false;
    }
    return true;
}
```

**Cpp — `ActivateAbility`** (extrair pontos chave):

```cpp
void UDFAbility_AerialAttack::ActivateAbility(...)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo, nullptr))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    ACharacter* const Char = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
    if (!Char) { EndAbility(...); return; }

    // Confirma continuação aérea para o combo component (cancela deferred reset).
    if (ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(Char))
    {
        if (UDFComboComponent* const Combo = PC->Combo) { Combo->ConfirmAerialContinuation(); }
    }

    if (Kind == EDFAerialAttackKind::Plunge)
    {
        // Hover anticipation: freeze vertical velocity briefly antes do slam.
        if (UCharacterMovementComponent* const CMC = Char->GetCharacterMovement())
        {
            CMC->Velocity.Z = 0.f;
            CMC->GravityScale = 0.f;
        }
        if (UAbilityTask_WaitDelay* const Hover =
                UAbilityTask_WaitDelay::WaitDelay(this, PlungeHoverDuration))
        {
            Hover->OnFinish.AddDynamic(this, &UDFAbility_AerialAttack::OnPlungeHoverEnd);
            Hover->ReadyForActivation();
        }
    }

    if (AerialMontage)
    {
        if (UAbilityTask_PlayMontageAndWait* const Task =
                UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
                    this, NAME_None, AerialMontage, 1.f, NAME_None, true, 1.f, 0.f, true))
        {
            Task->OnCompleted.AddDynamic(this, &UDFAbility_AerialAttack::OnAerialMontageCompleted);
            Task->OnInterrupted.AddDynamic(this, &UDFAbility_AerialAttack::OnAerialMontageCompleted);
            Task->ReadyForActivation();
        }
    }
}

void UDFAbility_AerialAttack::OnPlungeHoverEnd()
{
    ACharacter* const Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Char) return;
    if (UCharacterMovementComponent* const CMC = Char->GetCharacterMovement())
    {
        CMC->GravityScale = 2.5f;          // restore + boost para slam pesado
        CMC->Velocity = FVector(0,0, PlungeDownVelocity);
    }
}
```

### 16.6 `UDFAbility_Launcher` — ground attack que pop-up

**Novo arquivo:** `Source/DungeonForged/Public/GAS/Abilities/UDFAbility_Launcher.h`

Subclasse da ability de heavy melee existente, mas adiciona impulse vertical no notify de impact + arma o combo aerial mode:

```cpp
UCLASS()
class DUNGEONFORGED_API UDFAbility_Launcher : public UDFGameplayAbility
{
    GENERATED_BODY()
public:
    UDFAbility_Launcher();

    /** Vertical velocity applied to BOTH player and hit target. */
    UPROPERTY(EditDefaultsOnly, Category = "DF|Combat|Launcher")
    float LaunchZVelocity = 850.f;

    /** Horizontal kick (cm/s) applied to target (knocks them forward + up). */
    UPROPERTY(EditDefaultsOnly, Category = "DF|Combat|Launcher")
    float LaunchForwardVelocity = 400.f;

    UPROPERTY(EditDefaultsOnly, Category = "DF|Combat|Launcher")
    TObjectPtr<UAnimMontage> LauncherMontage;

protected:
    virtual void PostInitProperties() override;
    virtual void ActivateAbility(...) override;

    /** Hook chamado por um AnimNotify "LauncherImpact" no montage frame de hit. */
    UFUNCTION(BlueprintCallable, Category = "DF|Combat|Launcher")
    void ExecuteLaunchImpulse(AActor* HitTarget);
};
```

**Cpp:**

```cpp
void UDFAbility_Launcher::PostInitProperties()
{
    Super::PostInitProperties();
    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        AbilityTags.AddTag(FDFGameplayTags::Ability_Attack_Melee_Launcher);
        ActivationOwnedTags.AddTag(FDFGameplayTags::State_Launching);
        ActivationOwnedTags.AddTag(FDFGameplayTags::State_Attacking);
        // Launcher é "heavy" — bloqueia movimento durante o swing
        ActivationBlockedTags.AddTag(FDFGameplayTags::State_Stunned);
    }
}

void UDFAbility_Launcher::ExecuteLaunchImpulse(AActor* const HitTarget)
{
    ACharacter* const Self = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Self) return;

    // Player jumps com o alvo (acompanha o juggle).
    Self->LaunchCharacter(FVector(0, 0, LaunchZVelocity), false, true);

    // Inimigo pega lift + small forward push (knockback).
    if (ACharacter* const TargetChar = Cast<ACharacter>(HitTarget))
    {
        const FVector Fwd = Self->GetActorForwardVector();
        TargetChar->LaunchCharacter(
            FVector(Fwd.X * LaunchForwardVelocity, Fwd.Y * LaunchForwardVelocity, LaunchZVelocity),
            true, true);
    }

    // Arma o combo aerial mode no Self.
    if (ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(Self))
    {
        if (UDFComboComponent* const Combo = PC->Combo)
        {
            Combo->bHasAerialContinuation = true;
            // bIsAerialComboActive ficará true quando o aerial attack ativar
        }
    }
}
```

### 16.7 Input bindings

Hoje você tem `IA_Attack` (LMB) e `IA_SecondaryAttack` (RMB). O mesmo input pode produzir versão aérea ou terrestre — a decisão é em runtime via tag:

```cpp
// ADFPlayerCharacter::HandlePrimaryAttackPressed
void ADFPlayerCharacter::HandlePrimaryAttackPressed()
{
    if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponent())
    {
        const bool bAirborne = ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Jumping)
                            || ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Falling);
        const FGameplayTag TagToTry = bAirborne
            ? FDFGameplayTags::Ability_Attack_Melee_Aerial_Light
            : FDFGameplayTags::Ability_Attack_Melee_Light; // ou whatever ground tag você usa
        TryActivateByGameplayTag(TagToTry);
    }
}
```

E para o heavy:
```cpp
void ADFPlayerCharacter::HandleSecondaryAttackPressed()
{
    const bool bAirborne = (ASC has State_Jumping or State_Falling);
    const FGameplayTag TagToTry = bAirborne
        ? FDFGameplayTags::Ability_Attack_Melee_Aerial_Heavy
        : FDFGameplayTags::Ability_Attack_Melee_Heavy;
    TryActivateByGameplayTag(TagToTry);
}
```

**Plunge dedicada** (input combinado — Heavy + Down ou tecla específica):
```cpp
// Pode usar IA_Plunge separada, ou Heavy com modifier Down stick (gamepad)
void ADFPlayerCharacter::HandlePlungePressed()
{
    if (IsInAir()) // helper que checa MOVE_Falling
    {
        TryActivateByGameplayTag(FDFGameplayTags::Ability_Attack_Melee_Aerial_Plunge);
    }
}
```

### 16.8 OnLanded hook do ACharacter

**Arquivo:** [`ADFPlayerCharacter.cpp`](../../Source/DungeonForged/Private/Characters/ADFPlayerCharacter.cpp)

Override `Landed` para notificar o combo component:

```cpp
void ADFPlayerCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);
    if (UDFComboComponent* const C = Combo) { C->OnLanded(); }
}
```

### 16.9 Combo Data — referência cruzada armed/aerial

No `DT_Combos` (data table de combos), adicionar coluna para mapear o aerial equivalente:

```cpp
// Em FDFComboMontageRow (provavelmente DFDataTableStructs.h):
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Aerial")
TObjectPtr<UAnimMontage> AerialEquivalent;  // mesma posição do combo, mas no ar

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Aerial")
bool bIsLauncherStep = false;  // último step que ativa launcher
```

Quando `bIsAerialComboActive = true`, o `UDFComboComponent::PickNextMontage` consulta `AerialEquivalent` em vez de o montage default.

### 16.10 AnimNotify para LauncherImpact

**Novo arquivo:** `Source/DungeonForged/Public/Animation/AN/AnimNotify_LauncherImpact.h`

```cpp
UCLASS()
class UAnimNotify_LauncherImpact : public UAnimNotify
{
    GENERATED_BODY()
public:
    virtual void Notify(USkeletalMeshComponent* MeshComp, ...) override
    {
        Super::Notify(...);
        AActor* const Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
        if (!Owner) return;

        // Encontra o launcher ability ativo e dispara seu ExecuteLaunchImpulse
        if (UAbilitySystemComponent* const ASC = GetASC(Owner))
        {
            // Encontra atual ability instance via ActiveAbilities por tag
            TArray<UGameplayAbility*> Found;
            ASC->GetActivatableAbilities(); // pseudocode
            // ... loop até achar UDFAbility_Launcher e chamar ExecuteLaunchImpulse(HitTarget)
        }
    }
};
```

> Alternativa mais limpa: usar **UAbilityTask_WaitGameplayEvent** dentro da `UDFAbility_Launcher::ActivateAbility` e disparar via `UAbilitySystemBlueprintLibrary::SendGameplayEvent`. O notify só envia o event tag `Event.Combat.LauncherImpact` e a ability já está ouvindo.

### 16.11 Debug `df.AerialCombatDebug`

Adicionar comando que mostra:
```
[Aerial] InAir=1 ComboStep=2 AerialActive=1 LaunchPending=0 Continuation=1
[Aerial] LastInput=LMB MappedTo=Ability.Attack.Melee.Aerial.Light cancelWindow=open
```

### 16.12 Exemplo de combo completo

**Cena: "Light Light Heavy Launcher → Air Light Light Light → Plunge"**

```
Frame 0:    Player no chão, input LMB
            → UDFAbility_Light ativa → Combo step 0 → montage Light_01
Frame 30:   Light_01 abre ANS_DFAbilityCancelWindow
            → input LMB recebido → cancel + Light_02 (combo step 1)
Frame 70:   Light_02 cancel window → input RMB → Heavy ativa (combo step 2)
            Heavy é a "Launcher" desse weapon (DT_Combos.bIsLauncherStep=true)
            → UDFAbility_Launcher ativa
Frame 95:   Launcher montage AnimNotify_LauncherImpact dispara
            → ExecuteLaunchImpulse: player.LaunchCharacter(Z=850),
              enemy.LaunchCharacter(Fwd*400, Z=850)
            → Combo.bHasAerialContinuation = true
Frame 96:   Player sai do chão → OnMovementModeChanged Walking→Falling
            → State.Jumping tag adicionada
            → State.Launching ainda ativa (montage termina no ar)
            → Combo: NÃO faz reset porque bHasAerialContinuation=true
Frame 130:  Launcher montage termina, player ainda no ar
            → State.Attacking removida, State.Launching removida
            → Combo agora aguarda input aerial
Frame 145:  Input LMB no ar
            → HandlePrimaryAttackPressed vê State_Falling → TryActivate Ability.Aerial.Light
            → UDFAbility_AerialAttack(Kind=Light) ativa
            → ConfirmAerialContinuation → bIsAerialComboActive=true,
              State.Aerial.ComboActive adicionada
            → Aerial light montage roda (combo step 3, picked from AerialEquivalent)
Frame 175:  Aerial light cancel window → LMB → Aerial light 2 (step 4)
Frame 200:  Aerial light 2 cancel window → LMB → Aerial light 3 (step 5)
Frame 230:  Aerial light 3 cancel window → RMB+Down → Plunge ativa
            → Hover por 200ms → Velocity.Z = -1800
Frame 260:  Player toca o chão com plunge montage rodando
            → OnLanded chamado
            → Combo.OnLanded vê bIsAerialComboActive=true e bIsPlayingMontage=true
              → preserva o combo (não reseta) — plunge termina + AOE damage
Frame 280:  Plunge montage finish → Combo grace de 0.25s para nova entrada
            → Se LMB nesse intervalo: continua combo terrestre (step 6+ se DT permitir)
            → Se nada: ResetCombo
```

### 16.13 Checklist de validação combate aéreo

- [ ] LMB no chão → ground light; LMB no ar → aerial light (mesma tecla, mapa automaticamente)
- [ ] Ground combo (L L H) com último H = Launcher → pop-up player + inimigo
- [ ] Em juggle: aerial light chains 3 vezes (LMB LMB LMB no ar)
- [ ] Jump-cancel: durante cancel window de qualquer attack, Space pula → combo preservado
- [ ] Cancel window de ground attack fechada → Space pula → reset combo (correto)
- [ ] Plunge: RMB+Down no ar → hover 200ms → slam vertical
- [ ] Plunge AOE no impacto: inimigos dentro de 350cm recebem dano + knockup leve
- [ ] Land-cancel: aerial montage tocando quando aterrissa → não corta abruptamente
- [ ] Combo grace 0.35s após takeoff: input aerial nesse intervalo preserva combo counter
- [ ] Air dodge: 1 charge por pulo; reseta no land
- [ ] `State.Attacking.Aerial` ativa só durante montage aérea
- [ ] `State.Aerial.ComboActive` ativa entre primeira aerial attack e land
- [ ] `df.JumpDebug 2` + `df.AerialCombatDebug 1` mostra cadeia toda no log

### 16.14 Arquivos adicionais (combate aéreo)

| Arquivo | Status | O que tem |
|---|---|---|
| [`DFGameplayTags.h/.cpp`](../../Source/DungeonForged/Public/GAS/DFGameplayTags.h) | 🔧 Add | 8 tags novas (aerial states + ability variants) |
| `UDFAbility_AerialAttack.h/.cpp` (novo) | ✅ Criar | Base genérica para Light/Heavy/Plunge variants |
| `UDFAbility_Launcher.h/.cpp` (novo) | ✅ Criar | Ground attack que pop-up player + target |
| `AnimNotify_LauncherImpact.h/.cpp` (novo) | ✅ Criar | Notify que dispara o `ExecuteLaunchImpulse` |
| [`UDFComboComponent.h/.cpp`](../../Source/DungeonForged/Public/Combat/UDFComboComponent.h) | 🔧 Modificar | `bIsAerialComboActive`, `bHasAerialContinuation`, `OnLanded`, `RequestDeferredReset`, `ConfirmAerialContinuation`, `CancelCurrentMontage` |
| [`ANS_DFAbilityCancelWindow.cpp`](../../Source/DungeonForged/Private/Combat/AN/ANS_DFAbilityCancelWindow.cpp) | 🔧 Modificar | `Ability.Movement.Jump` + `Ability.Attack.Melee.Aerial` no `AllowedCancelTags` |
| [`ADFPlayerCharacter.h/.cpp`](../../Source/DungeonForged/Public/Characters/ADFPlayerCharacter.h) | 🔧 Modificar | Override `Landed`, attack handlers que rotam por airborne tag |
| `DT_Combos` row struct | 🔧 Modificar | `AerialEquivalent` (montage), `bIsLauncherStep` (bool) |
| Assets `GA_Aerial_Light/Heavy/Plunge/Launcher` | ✅ Criar | Blueprint subclasses configuradas com montages e tags |
| Montages aéreas | ✅ Criar | Light/Heavy/Plunge montages com anim notify states no cancel window |

---

---

## Arquivos referenciados

- [`DFAnimSetTypes.h`](../../Source/DungeonForged/Public/Animation/DFAnimSetTypes.h)
- [`UDFAnimInstance.h`](../../Source/DungeonForged/Public/Animation/UDFAnimInstance.h)
- [`UDFAnimInstance.cpp`](../../Source/DungeonForged/Private/Animation/UDFAnimInstance.cpp)
- [`UDFCharacterMovementComponent.h`](../../Source/DungeonForged/Public/Characters/UDFCharacterMovementComponent.h)
- [`UDFCharacterMovementComponent.cpp`](../../Source/DungeonForged/Private/Characters/UDFCharacterMovementComponent.cpp)
- [`DFGameplayTags.h`](../../Source/DungeonForged/Public/GAS/DFGameplayTags.h)
- [`UDFCombatTuningData.h`](../../Source/DungeonForged/Public/Data/UDFCombatTuningData.h)
- [`ADFPlayerCharacter.h`](../../Source/DungeonForged/Public/Characters/ADFPlayerCharacter.h)
- [`ADFRunPlayerController.h`](../../Source/DungeonForged/Public/GameModes/Run/ADFRunPlayerController.h)
- [`UDFCheatManager.cpp`](../../Source/DungeonForged/Private/Debug/UDFCheatManager.cpp)
- [`UDFLocomotionTypes.h`](../../Source/DungeonForged/Public/Animation/UDFLocomotionTypes.h) — `EDFMovementDirection`
- [`docs/improvements/01_GameFeel.md`](01_GameFeel.md) — camera kick on heavy land
- [`docs/improvements/15_DodgeAbility_4Way.md`](15_DodgeAbility_4Way.md) — air-dodge gating
- [`docs/improvements/16_LockOnSystem.md`](16_LockOnSystem.md) — camera suppression durante jump
