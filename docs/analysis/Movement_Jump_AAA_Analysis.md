# Análise Profunda — Movimentação & Pulo (Roadmap AAA)

> **Data:** 2026-05-25
> **Escopo:** `UDFCharacterMovementComponent`, `ACharacter::Jump()` chain, `UUDFAnimInstance` jump state, `UDFLauncherComponent`, `UDFComboComponent` (aerial), `ADFPlayerCharacter::Jump`.
> **Objetivo do projeto:** ação 3rd-person estilo hack-and-slash, **com air combo + air dash + pulo + double jump**. Referências AAA: Devil May Cry 5, Bayonetta 3, Astro Bot, Marvel's Spider-Man 2, God of War Ragnarök, Sekiro.
> **Nível atual:** sólido para "explorer / souls-like", **insuficiente** para AAA-action-aéreo. Existem fundações boas (network prediction, GAS tags, landing recovery, fall gravity), mas faltam mecânicas-chave (double jump, air dash, jump cancel out-of-attack, juggle stability, coyote time, input buffer, wall interactions).

---

## Sumário

- [1. Diagnóstico — o que já está pronto](#1-diagnóstico--o-que-já-está-pronto)
- [2. Gap Analysis — o que falta para "AAA"](#2-gap-analysis--o-que-falta-para-aaa)
- [3. Benchmark — números de jogos AAA](#3-benchmark--números-de-jogos-aaa)
- [4. Roadmap proposto (5 fases)](#4-roadmap-proposto-5-fases)
- [5. Fase 1 — Polish de fundação (game-feel grounded)](#5-fase-1--polish-de-fundação-game-feel-grounded)
- [6. Fase 2 — Double jump & air movement](#6-fase-2--double-jump--air-movement)
- [7. Fase 3 — Air dash (com i-frames opcionais)](#7-fase-3--air-dash-com-i-frames-opcionais)
- [8. Fase 4 — Air combo & juggle](#8-fase-4--air-combo--juggle)
- [9. Fase 5 — Camera, animação, FX, áudio (AAA polish)](#9-fase-5--camera-animação-fx-áudio-aaa-polish)
- [10. Riscos técnicos & contramedidas](#10-riscos-técnicos--contramedidas)
- [11. Network prediction — pontos críticos](#11-network-prediction--pontos-críticos)
- [12. Checklist de Quality Bar AAA](#12-checklist-de-quality-bar-aaa)
- [13. Tabela de arquivos a tocar](#13-tabela-de-arquivos-a-tocar)
- [14. Próximos passos sugeridos](#14-próximos-passos-sugeridos)
- [15. Análise dos logs runtime (`df.JumpDebug`)](#15-análise-dos-logs-runtime-dfjumpdebug)
- [16. Análise dos logs runtime — Locomoção (`df.LocomotionDebug 4`)](#16-análise-dos-logs-runtime--locomoção-dflocomotiondebug-4)

---

## 1. Diagnóstico — o que já está pronto

### 1.1 Movement Component ([UDFCharacterMovementComponent.h](../../Source/DungeonForged/Public/Characters/UDFCharacterMovementComponent.h))

| Sistema | Status | Comentário |
|---|---|---|
| `RunSpeed` (540) / `SprintSpeed` (750) / `CrouchSpeed` (200) | ✅ | Boa separação; numérica saudável |
| `RotationRate` 720°/s | ✅ | Snappy o suficiente para hack-and-slash |
| Strafe mode (lock-on) | ✅ | `SetStrafeMode` troca `bOrientRotationToMovement` ↔ `bUseControllerDesiredRotation` |
| Dodge com root motion + i-frames | ✅ | `FRootMotionSource_MoveToForce`, `DodgeCooldown` 0.7s, `IFrameDuration` 0.35s |
| Network prediction de sprint | ✅ | `FSavedMove_DF::bWantsSprint` via `FLAG_Custom_0` |
| `DFJumpZVelocity` 550, `DFAirControl` 0.35, `DFGravityScale` 1.7 | ✅ | Tunados via `UDFCombatTuningData` |
| `DFFallGravityMultiplier` 1.25 (post-apex) | ✅ | **Critical AAA pattern** — pulo "snappy" |
| `DFJumpCooldown` 0.20s (anti-spam) | ✅ | Bem dimensionado |
| `DFJumpStaminaCost` 10 | ✅ | Gate em `DoJump` |
| `LandingHorizontalVelocityRetain` 0.4 | ✅ | **Critical AAA pattern** — "stick the landing" anti-slide |
| `LandingBrakingDeceleration` 4096 | ✅ | Dobro do default UE; landing snap |
| `bAirDodgeUsedThisJump` (bool) | ⚠️ Stub | Variável existe mas **não há ability de air dodge usando-a** |
| `SyncJumpLooseTagsWhileGrounded` | ✅ | Limpa State.Jumping/Falling stale |
| AddJumpLooseTagOnce (evita stacks N×) | ✅ | Padrão correto p/ GAS loose tags |

### 1.2 Player Character ([ADFPlayerCharacter.cpp:605-649](../../Source/DungeonForged/Private/Characters/ADFPlayerCharacter.cpp))

```cpp
void ADFPlayerCharacter::Jump()
{
    // Hard blockers: Dead, Stunned, Dodging, Exhausted, Landing
    // Soft blocker: Attacking (cancelable se AbilityCancelWindow aberta → cancela montage e pula)
    // → Super::Jump()
}
```

**O bom:** gate via tags GAS, cancel window respeitada, log via `DFJumpDebug`.
**O ruim:** **não tem double jump** (`JumpMaxCount = 1` implícito), **não tem coyote time**, **não tem input buffer pre-grounded** (você só pode pular se já tocou o chão).

### 1.3 Anim Instance ([UDFAnimInstance.h](../../Source/DungeonForged/Public/Animation/UDFAnimInstance.h))

Estado de pulo já está **excepcionalmente bem modelado** para single-jump:
- `bIsJumping`, `bIsFalling`, `bIsLanding` separados
- `LastJumpDirection` capturada no takeoff (anti-popping)
- `AirTime`, `VerticalVelocity`, `PredictedLandingDistance` (line trace down)
- Transições nomeadas (`bTransition_LocomotionToJumpStart`, etc.) com tuning de blend in/out individual
- `bHasPassedJumpApex` latched (evita flicker no Vz≈0)
- `JumpLoopPhaseTime` para gates de transição

**Falta:** estado para **double jump** (segundo arco), **air dash** (estado independente), **wall slide/grab**, **fall recovery** (long fall = animação diferente).

### 1.4 Combo Component ([UDFComboComponent.h:80-81](../../Source/DungeonForged/Public/Combat/UDFComboComponent.h))

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Aerial")
TArray<FDFComboStep> AerialComboSteps;  // ← Stub existe, mas o sistema usa apenas se IsOwnerAirborne()
```

`HasAerialContinuation()` retorna true se o array tem itens. **A fundação está lá** — falta o gameplay (montages aéreas, anti-gravity hangtime, launcher hook automático).

### 1.5 Launcher ([UDFLauncherComponent.cpp](../../Source/DungeonForged/Private/Combat/UDFLauncherComponent.cpp))

```cpp
void ApplyLaunch(AActor* Target, FVector LaunchVel, float TargetGravity, float Hangtime);
void ApplySelfLaunch(FVector SelfVel);
```

**Pronto** para launcher attacks (DMC-style "stinger → high time"). Restaura gravidade do target após `Hangtime`. **O que falta:** integração com combo aéreo (auto-jump-cancel após launcher), launcher → tracking jump (já citado no doc 17), hit-stop sincronizado.

### 1.6 GAS Tags (já registradas)

```
State.Jumping       ✅
State.Falling       ✅
State.Landing       ✅
State.Dodging       ✅
State.Attacking     ✅
State.Combat.AbilityCancelWindow.Open  ✅
```

**Faltam (sugestões):**
```
State.DoubleJumping
State.AirDashing
State.WallSliding   (futuro)
State.Juggled       (no inimigo, controla hangtime extra)
State.AerialCombo   (no player, libera combos aéreos)
```

---

## 2. Gap Analysis — o que falta para "AAA"

### Tier S (bloqueia AAA, **obrigatório**)

| # | Gap | Impacto | Onde resolver |
|---|---|---|---|
| **G1** | Sem **double jump** | Skill ceiling baixo, sem expressividade aérea | `UDFCharacterMovementComponent::DoJump` + `JumpMaxCount=2` + AnimSet |
| **G2** | Sem **air dash** | Não há "i-frame aéreo" nem mobility skill | Nova GA `UDFAbility_AirDash` + estado no CMC |
| **G3** | Sem **jump cancel out of ground attack** (jump-cancel é só o stub do AbilityCancelWindow) | Combos terrestres não fluem para o ar | `UDFComboComponent` + `ANS_DFAbilityCancelWindow` allow Ability.Jump |
| **G4** | Sem **launcher → auto aerial follow** | Player precisa pular manualmente após launcher → janela perdida | `Launcher` dispara `TrackingJump` (citado no doc 17 §17) |
| **G5** | Sem **coyote time** (grace period após sair de plataforma) | Pulos parecem "robar input"; AAA padrão é 80-150ms | `UDFCharacterMovementComponent::TickComponent` track time since last `MOVE_Walking` |
| **G6** | Sem **jump input buffer** (pré-aterrissagem) | Player que aperta jump 50ms antes de tocar o chão perde o input | `ADFPlayerCharacter::Jump` enfileira se `State.Falling` & GroundDist < threshold |
| **G7** | **Hard tag `State.Landing` bloqueia jump** ([ADFPlayerCharacter.cpp:624](../../Source/DungeonForged/Private/Characters/ADFPlayerCharacter.cpp)) | Quebra "chain jumps" (Bayonetta) | Soft gate: aceita input em `State.Landing` mas só consome no fim do recovery |

### Tier A (alta prioridade, AAA polish)

| # | Gap | Impacto | Onde resolver |
|---|---|---|---|
| **G8** | Sem **variable jump height** (apex cut ao soltar botão) | Pulo tem só uma altura | `ADFPlayerCharacter::StopJumping` aplica `Velocity.Z *= 0.4` se ainda Vz>0 |
| **G9** | Sem **wall jump / wall slide** | Mobility aérea limitada | Sphere trace no front+up durante `MOVE_Falling` |
| **G10** | Sem **fall damage / long-fall recovery anim** | Long jumps acabam abruptos | `AirTime > LongFallThreshold` (~1.2s) → land montage diferente |
| **G11** | Sem **air momentum preservation** ao pular durante sprint | Pulos parados e sprintando têm mesma distância | Boost de `Velocity.X/Y` no `DoJump` se `bIsSprinting` |
| **G12** | **`AerialComboSteps` vazio na prática** | Stub existe mas sem montages aéreas | Criar montages `AS_AerialCombo_{1,2,3}` |
| **G13** | Sem **anti-gravity hangtime** durante air combo hit | Inimigo cai durante combo aéreo, perde-se "Style" | `UDFAbility` aérea reduz `GravityScale` no owner enquanto montage roda |
| **G14** | **Aerial combo NÃO usa `MotionWarping`** para fechar gap até o inimigo | Player ataca no vazio | `UANS_DFMeleeWarp` precisa working in air |

### Tier B (AAA "feels"; faz a diferença)

| # | Gap | Impacto | Onde resolver |
|---|---|---|---|
| **G15** | Não há **camera kick** no jump (subtle FOV pulse) | Falta de "weight" | `UDFCameraComponent` listen `OnDFMovementModeChanged` Walking→Falling |
| **G16** | Sem **rumble** no jump / land / air dash | Sem feedback tátil | Force feedback effect no `ACharacter::OnLanded` |
| **G17** | Footstep no jump start não soa "esforço" | Áudio fraco | Anim notify `Whoosh` no `Jump_Start_*` |
| **G18** | Apex flash (Bayonetta) / particle no apex | Sem polish visual | `AnimNotify_JumpApex` (já planejado no doc 17 §8) |
| **G19** | Landing dust não escala com `AirTime` | Long-fall = poof igual a short-jump | `Niagara` user param `Intensity` = clamp(AirTime/1.0) |
| **G20** | Sem **slowmo no juggle** (DMC-style "S+ Stylish") | Aerial combo parece liso | Hit-stop no air opponent + `WorldSettings::SetTimeDilation 0.85` por 80ms |

### Tier C (futuro/opcional)

| # | Gap | Quando preocupar |
|---|---|---|
| G21 | Wall run | Se virar action-platformer (Spider-Man) |
| G22 | Grapple hook | Mecânica nova fora do escopo atual |
| G23 | Ledge grab/climb | Se houver verticality nos dungeons |
| G24 | Stomp / dive attack | Easy win se quiser DMC-style |
| G25 | Air taunt (style only) | DMC/Bayonetta flavor |

---

## 3. Benchmark — números de jogos AAA

Valores aproximados (medidos/relatados pela comunidade). **Use para sanity-check do seu tuning.**

| Mecânica | Seu jogo | DMC5 | Bayonetta 3 | Astro Bot | Spider-Man 2 | Recomendado p/ DF |
|---|---|---|---|---|---|---|
| Jump Z velocity (cm/s) | 550 | ~700 | ~750 | ~600 | ~650 | **600-680** (subir um pouco) |
| Air control (0-1) | 0.35 | 0.85 | 0.9 | 0.8 | 0.7 | **0.65** (action-game range) |
| Gravity scale (rising) | 1.7 | 1.6 | 1.5 | 1.0 | 1.4 | manter **1.7** (DungeonForged é mais "weighty") |
| Fall gravity mult (post-apex) | 1.25 | 1.4 | 1.5 | 1.8 | 1.3 | **1.4-1.5** (subir levemente) |
| Coyote time | 0ms ❌ | ~100ms | ~120ms | ~150ms | ~100ms | **100ms** |
| Jump buffer (pre-land) | 0ms ❌ | ~150ms | ~180ms | ~200ms | ~120ms | **150ms** |
| Variable jump | ❌ | ✓ | ✓ | ✓ | ✓ | implementar |
| Double jump count | 1 ❌ | 2 (skill) | 3 (witch time loop) | 2 | 1+web swing | **2** |
| Air dash count per arc | ❌ | 1 (skill upgrade: 2) | unlimited (with cost) | 0 | 0 | **1** com cost (estamina/cooldown) |
| Air dash distance | — | 350cm | 400cm | — | — | **400cm** |
| Air dash i-frames | — | 0 (mobility only) | 0.2s (witch time gate) | — | — | **opcional**: 0.15s se gastar dodge charge |
| Landing recovery | 200ms ✓ | ~150ms (no whiff) | ~120ms | ~250ms | ~180ms | manter, mas **0ms se entrar em combat-land** |
| Aerial combo extended hangtime | — | ✓ (anti-grav) | ✓ | — | ✓ | **GravityScale 0.4 durante swing** |

**Conclusão do benchmark:**
- Seu **`AirControl 0.35` é baixo demais para action-aéreo** — DMC/Bayonetta usam 0.7-0.9. Subir para `0.6-0.7` muda completamente a sensação no ar.
- O resto está **muito bem calibrado** para o gênero (souls-like-flavored action).
- As 3 maiores wins de feel: **coyote time + jump buffer + variable jump height**. São ~50 linhas de código combinadas.

---

## 4. Roadmap proposto (5 fases)

Cada fase é **independentemente shippable** — pode parar em qualquer uma e o sistema fica coerente.

```
┌──────────────────────────────────────────────────────────────────────┐
│ FASE 1 — Polish de fundação (1-2 dias)                               │
│   • Coyote time           • Jump input buffer                        │
│   • Variable jump height  • Soft Landing gate                        │
│   • Sprint→jump momentum  • Long-fall anim threshold                 │
└────────────────────────────┬─────────────────────────────────────────┘
                             │ check: jump feels "AAA-grounded"
                             ▼
┌──────────────────────────────────────────────────────────────────────┐
│ FASE 2 — Double Jump (2-3 dias)                                      │
│   • JumpMaxCount=2        • Tag State.DoubleJumping                  │
│   • Anim set DoubleJump   • Stamina cost separado                    │
│   • AnimNotify_DoubleJumpApex                                        │
└────────────────────────────┬─────────────────────────────────────────┘
                             │ check: double jump fluido, sem bug visual
                             ▼
┌──────────────────────────────────────────────────────────────────────┐
│ FASE 3 — Air Dash (3-4 dias)                                         │
│   • UDFAbility_AirDash    • RootMotionSource_MoveToForce             │
│   • 1 dash per arc        • Cancel air dash → restart hangtime       │
│   • Anim 8-way (8 montages curtas)                                   │
└────────────────────────────┬─────────────────────────────────────────┘
                             │ check: mobility aérea suficiente
                             ▼
┌──────────────────────────────────────────────────────────────────────┐
│ FASE 4 — Air Combo (4-6 dias)                                        │
│   • AerialComboSteps montages reais                                  │
│   • GravityScale 0.4 durante swing (anti-gravity)                    │
│   • Launcher → auto-pursuit (doc 17 §17 já existe)                   │
│   • Aerial finisher (smash down → ground hit-stop)                   │
│   • MotionWarping aéreo                                              │
└────────────────────────────┬─────────────────────────────────────────┘
                             │ check: 3-hit combo aéreo + finisher
                             ▼
┌──────────────────────────────────────────────────────────────────────┐
│ FASE 5 — Polish AAA (2-3 dias)                                       │
│   • Camera kick + FOV     • Rumble jump/dash/land                    │
│   • Apex VFX/SFX          • Landing dust by AirTime                  │
│   • Style rating hooks    • Slowmo on aerial finisher                │
└──────────────────────────────────────────────────────────────────────┘
```

**Total estimado:** 12-18 dias de dev focado (1 dev).

---

## 5. Fase 1 — Polish de fundação (game-feel grounded)

### 5.1 Coyote Time (~80-100ms grace após sair de plataforma)

**Arquivo:** [`UDFCharacterMovementComponent.h`](../../Source/DungeonForged/Public/Characters/UDFCharacterMovementComponent.h) + `.cpp`

```cpp
// Header
UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump", meta = (ClampMin = "0.0", ClampMax = "0.5"))
float CoyoteTime = 0.10f;

UFUNCTION(BlueprintPure, Category = "DF|Movement|Jump")
bool IsWithinCoyoteWindow() const;

protected:
    float TimeLastLeftGround = -1.f;  // Quando saiu de Walking, não-por-jump
```

```cpp
// .cpp — OnMovementModeChanged Walking → Falling
if (PreviousMovementMode == MOVE_Walking && NewMode == MOVE_Falling)
{
    if (Velocity.Z <= 1.f)  // saiu pela borda (não pulou)
    {
        TimeLastLeftGround = GetWorld()->GetTimeSeconds();
    }
    // ... resto do código existente
}

bool UDFCharacterMovementComponent::IsWithinCoyoteWindow() const
{
    if (TimeLastLeftGround < 0.f || !GetWorld()) return false;
    return (GetWorld()->GetTimeSeconds() - TimeLastLeftGround) <= CoyoteTime;
}

// DoJump: aceita pulo se grounded OR within coyote window
bool UDFCharacterMovementComponent::DoJump(bool bReplayingMoves)
{
    const bool bCanJumpAirborne = MovementMode == MOVE_Falling && IsWithinCoyoteWindow();
    if (MovementMode != MOVE_Walking && !bCanJumpAirborne)
    {
        // Default UE check (CanAttemptJump) ainda bloqueia airborne — manualmente forçamos:
        if (!bCanJumpAirborne) return false;
    }
    // ... resto
}
```

**Validação:** correr até a borda, soltar input por 80ms enquanto cai, apertar jump → pulo conta normal.

### 5.2 Jump Input Buffer (~120-150ms pré-aterrissagem)

**Arquivo:** [`ADFPlayerCharacter.h`](../../Source/DungeonForged/Public/Characters/ADFPlayerCharacter.h) + `.cpp`

```cpp
// Header
UPROPERTY(EditDefaultsOnly, Category = "DF|Movement|Jump", meta = (ClampMin = "0.0", ClampMax = "0.4"))
float JumpInputBufferDuration = 0.15f;

private:
    float JumpInputBufferedUntil = -1.f;
```

```cpp
// ADFPlayerCharacter::Jump()
void ADFPlayerCharacter::Jump()
{
    // Se está no ar OU em landing recovery, bufferiza
    if (UDFCharacterMovementComponent* CMC = Cast<UDFCharacterMovementComponent>(GetCharacterMovement()))
    {
        const bool bLanding = GetAbilitySystemComponent() &&
            GetAbilitySystemComponent()->HasMatchingGameplayTag(FDFGameplayTags::State_Landing);
        const bool bFallingNearGround = CMC->IsFalling() &&
            /* GroundDistance < 250 from AnimInstance */ true;

        if (bLanding || bFallingNearGround)
        {
            JumpInputBufferedUntil = GetWorld()->GetTimeSeconds() + JumpInputBufferDuration;
            DFJumpDebug::Log(TEXT("Jump BUFFERED"));
            return;
        }
    }
    // ... resto do código atual
}

// Listen Walking transition (criar handler do delegate OnDFMovementModeChanged)
void ADFPlayerCharacter::OnMovementModeChanged_DF(EMovementMode New, EMovementMode Prev, uint8 PrevCustom)
{
    if (New == MOVE_Walking && JumpInputBufferedUntil > GetWorld()->GetTimeSeconds())
    {
        JumpInputBufferedUntil = -1.f;
        Jump();  // re-entrega o input bufferizado
    }
}
```

**Validação:** apertar jump enquanto está em air (perto do chão) → toca o chão → pula sem segunda interação.

### 5.3 Variable Jump Height (apex cut)

**Arquivo:** [`UDFCharacterMovementComponent.cpp`](../../Source/DungeonForged/Private/Characters/UDFCharacterMovementComponent.cpp)

Já existe `ACharacter::StopJumping()` (released input). Hook nele:

```cpp
// Player character
void ADFPlayerCharacter::StopJumping()
{
    Super::StopJumping();
    if (UDFCharacterMovementComponent* CMC = Cast<UDFCharacterMovementComponent>(GetCharacterMovement()))
    {
        if (CMC->IsFalling() && CMC->Velocity.Z > 0.f)
        {
            CMC->Velocity.Z *= 0.4f;  // corta o apex
            DFJumpDebug::Logf(TEXT("Apex cut, Vz=%.0f"), CMC->Velocity.Z);
        }
    }
}
```

Tunável: expor `ApexCutScale` em `UDFCombatTuningData`.

### 5.4 Soft Landing Gate (permite chain-jump)

**Arquivo:** [`ADFPlayerCharacter.cpp:624`](../../Source/DungeonForged/Private/Characters/ADFPlayerCharacter.cpp) — remover `State_Landing` do `HardBlockers` e fazer buffer:

```cpp
// Remova State_Landing do HardBlockers (linha ~624)
// Em vez disso, antes do Super::Jump(), aceita mas redirige para o buffer:
if (ASC->HasMatchingGameplayTag(FDFGameplayTags::State_Landing))
{
    JumpInputBufferedUntil = GetWorld()->GetTimeSeconds() + JumpInputBufferDuration;
    return;
}
```

E no fim do landing recovery (timer no CMC), checar o buffer.

### 5.5 Sprint → Jump Momentum Preservation

**Arquivo:** [`UDFCharacterMovementComponent.cpp`](../../Source/DungeonForged/Private/Characters/UDFCharacterMovementComponent.cpp) — `DoJump`:

```cpp
bool UDFCharacterMovementComponent::DoJump(bool bReplayingMoves)
{
    // ... gates existentes ...
    const bool bOk = Super::DoJump(bReplayingMoves);
    if (bOk)
    {
        // Sprint-jump boost: 25% extra horizontal
        if (bIsSprinting)
        {
            Velocity.X *= 1.25f;
            Velocity.Y *= 1.25f;
        }
        TimeLastJump = GetWorld()->GetTimeSeconds();
        bAirDodgeUsedThisJump = false;
    }
    return bOk;
}
```

### 5.6 Long-Fall Anim Threshold

**Arquivo:** [`UDFAnimInstance.cpp`](../../Source/DungeonForged/Private/Animation/UDFAnimInstance.cpp) (em `NativeUpdateAnimation` após `bWasInAir && !bNowInAir`):

```cpp
UPROPERTY(BlueprintReadOnly, Transient, Category = "DF|Locomotion|Jump")
bool bIsLongFallLanding = false;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Locomotion|Jump")
float LongFallAirTimeThreshold = 1.2f;

// On landing frame:
bIsLongFallLanding = (AirTime > LongFallAirTimeThreshold);
```

No AnimBP: state `JumpLand` checa `bIsLongFallLanding` para usar anim mais longa/heavy.

---

## 6. Fase 2 — Double jump & air movement

### 6.1 CMC: aceitar segundo pulo

UE5 `ACharacter` já tem `JumpMaxCount`. Basta:

```cpp
// ADFPlayerCharacter (BeginPlay ou no class default)
JumpMaxCount = 2;
JumpMaxHoldTime = 0.f;  // não queremos hold-jump duration
```

E em `DoJump`, distinguir:

```cpp
bool UDFCharacterMovementComponent::DoJump(bool bReplayingMoves)
{
    const bool bIsDoubleJump = (MovementMode == MOVE_Falling) && (CharacterOwner->JumpCurrentCount >= 1);

    if (bIsDoubleJump)
    {
        // Reset velocity vertical (assim parece "novo impulso")
        Velocity.Z = 0.f;

        // Custo extra de stamina
        if (DFDoubleJumpStaminaCost > 0.f)
        {
            // gate via ASC
        }

        // Tag separada
        if (UAbilitySystemComponent* ASC = ...)
        {
            ASC->AddLooseGameplayTag(FDFGameplayTags::State_DoubleJumping);
        }

        // Boost ligeiro do Z (menor que primeiro jump)
        const float SavedJZ = JumpZVelocity;
        JumpZVelocity *= 0.85f;
        const bool bOk = Super::DoJump(bReplayingMoves);
        JumpZVelocity = SavedJZ;
        return bOk;
    }

    return Super::DoJump(bReplayingMoves);
}
```

### 6.2 Tag nova

Adicionar em `DFGameplayTags.h`:

```cpp
static FGameplayTag State_DoubleJumping;
```

E em `.cpp`: `UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_DoubleJumping, "State.DoubleJumping", "...")`

### 6.3 AnimSet

Expandir `FUDJumpAnimSet` ([DFAnimSetTypes.h](../../Source/DungeonForged/Public/Animation/DFAnimSetTypes.h)) com:

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Double")
TObjectPtr<UAnimSequenceBase> DoubleJump_Start;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Double")
TObjectPtr<UAnimSequenceBase> DoubleJump_Loop;  // opcional, pode reusar Loop
```

AnimBP: state machine ganha branch "DoubleJump" entre Loop e Land:
```
Loop → (input Jump & JumpCount=1) → DoubleJump_Start → DoubleJump_Loop → Land
```

### 6.4 Reset do contador

Já existe em `OnMovementModeChanged` Falling→Walking. Basta confirmar que `CharacterOwner->JumpCurrentCount` é reset (UE5 default faz isso).

**Adicionar:** reset também ao tocar uma wall (futuro: wall jump refresh).

### 6.5 Anim notify específico

`UDFAnimNotify_DoubleJumpApex` (cópia do JumpApex) — burst de FX maior no apex do segundo pulo (DMC pattern: feather/ring de partículas).

---

## 7. Fase 3 — Air dash (com i-frames opcionais)

### 7.1 Design

| Atributo | Valor |
|---|---|
| Mode | GAS Ability (`UDFAbility_AirDash`) — predita |
| Trigger | Input button (dedicar: `IA_AirDash` ou re-bind `IA_Dodge` quando airborne) |
| Disponibilidade | 1 use per air arc (igual `bAirDodgeUsedThisJump` que **já existe** na CMC) |
| Distância | 400 cm em 0.25s |
| Direção | 8-way snap (input ou camera forward se sem input) |
| Custo | 15 stamina |
| Cooldown | 0.4s (sobre o uso, não no air-arc) |
| i-frames | 0 default; **opt-in** 0.15s se for "Air Dodge" (gasta dodge charge) |
| Gravity | Override 0 durante dash; restore depois |

### 7.2 Implementação

**Novo header:** `Source/DungeonForged/Public/GAS/Abilities/UDFAbility_AirDash.h`

```cpp
UCLASS()
class DUNGEONFORGED_API UDFAbility_AirDash : public UGameplayAbility
{
    GENERATED_BODY()
public:
    UDFAbility_AirDash();
    virtual bool CanActivateAbility(...) const override;
    virtual void ActivateAbility(...) override;

    UPROPERTY(EditDefaultsOnly, Category="DF|AirDash")
    float DashDistance = 400.f;

    UPROPERTY(EditDefaultsOnly, Category="DF|AirDash")
    float DashDuration = 0.25f;

    UPROPERTY(EditDefaultsOnly, Category="DF|AirDash")
    TArray<TObjectPtr<UAnimMontage>> DashMontages_8Way;  // F, FR, R, BR, B, BL, L, FL

    UPROPERTY(EditDefaultsOnly, Category="DF|AirDash")
    bool bGrantIFrames = false;

    UPROPERTY(EditDefaultsOnly, Category="DF|AirDash")
    float IFrameDuration = 0.15f;
};
```

**`CanActivateAbility`** checks:
- `MovementMode == MOVE_Falling`
- `!CMC->bAirDodgeUsedThisJump` (já existe!)
- Sem `State.Dodging`, `State.Stunned`, `State.Exhausted`
- Stamina >= 15

**`ActivateAbility`:**
1. `CMC->bAirDodgeUsedThisJump = true`
2. Add `State.AirDashing` loose tag
3. Save `CMC->GravityScale`; set to 0
4. Apply `FRootMotionSource_MoveToForce` (mesmo pattern do `PerformDodge`)
5. Play directional montage
6. After `DashDuration`: restore gravity, remove tag, set `Velocity` to small forward residual (mantém momentum)
7. If `bGrantIFrames`: add `State.Invulnerable` for `IFrameDuration`

### 7.3 Cancel chain

Air dash deve permitir **cancel out of aerial attack** e **into aerial attack**. Adicionar regra em `UDFCombatTuningData::CancelRules`:

```
FromAbilityTags: Ability.Combo.Aerial.*
AllowedTargetTags: Ability.AirDash
```

### 7.4 Reset

`OnMovementModeChanged` Falling→Walking → `bAirDodgeUsedThisJump = false` (já existe).

### 7.5 Air dash → ground combo continuity

Ao tocar o chão dentro de 0.5s do air dash, **skipar landing recovery** (mantém momentum + permite ataque imediato):

```cpp
if (PreviousMovementMode == MOVE_Falling && NewMode == MOVE_Walking)
{
    const bool bRecentAirDash = (GetWorld()->GetTimeSeconds() - TimeLastAirDash) < 0.5f;
    if (bRecentAirDash)
    {
        DFLandingRecoveryWindow = 0.f;  // skip
    }
}
```

---

## 8. Fase 4 — Air combo & juggle

### 8.1 Aerial Combo Steps reais

**Já existe a infra** ([UDFComboComponent.cpp:338, 1598](../../Source/DungeonForged/Private/Combat/UDFComboComponent.cpp)): se `IsOwnerAirborne()` e `AerialComboSteps` não-vazio, usa aquele array.

**O que faltam são as 3 montages aéreas em `DA_PlayerWeapon`:**
- `AS_Aerial_1` (slash horizontal, mantém altura)
- `AS_Aerial_2` (uppercut sutil, eleva player ~50cm)
- `AS_Aerial_3` (smash down, launcher reverso — empurra inimigo para baixo)

Cada montage tem:
- `AN_TraceStart` / `AN_TraceEnd` (já existe)
- `AN_ComboWindowOpen` (já existe)
- `ANS_DFAbilityCancelWindow` (já existe, allow `Ability.Jump`, `Ability.AirDash`)

### 8.2 Anti-gravity hangtime

**Padrão DMC:** durante swing aéreo, `GravityScale` cai para ~0.3 → player "flutua" entre hits.

Implementação: novo `UAnimNotifyState_AerialHangtime` no início de cada `AS_Aerial_*`:

```cpp
UCLASS()
class DUNGEONFORGED_API UANS_DFAerialHangtime : public UAnimNotifyState
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere)
    float HangtimeGravityScale = 0.35f;

    virtual void NotifyBegin(USkeletalMeshComponent*, UAnimSequenceBase*, float, ...) override;
    virtual void NotifyEnd(USkeletalMeshComponent*, UAnimSequenceBase*, ...) override;

private:
    float SavedGravity = 1.7f;
};
```

`NotifyBegin`: save current GravityScale; set to HangtimeGravityScale.
`NotifyEnd`: restore.

### 8.3 Launcher → auto-pursuit

**Já planejado** no doc 17 §17 (commit `e8a8ffc`). Resumo: após launcher hit, dispatch automático de `Ability.TrackingJump` que faz o player perseguir o target em arco. Verificar status do PR.

### 8.4 Aerial finisher (smash down)

Step 3 do combo aéreo (`AS_Aerial_3`):
- Aplica `LaunchCharacter(FVector(0,0,-1200), false, true)` no player (DMC "rainstorm" reverso)
- Hit-stop maior (`Hangtime` 0.12s no `UDFImpactFramingComponent`)
- Ao tocar o chão → ground shake + camera kick + `AS_GroundSlam_Impact`
- Reset combo, deixa player em `State.Landing` com recovery 0.3s (mais que normal — peso)

### 8.5 MotionWarping aéreo

`UANS_DFMeleeWarp` deve aceitar warping enquanto airborne. Verificar [combat skill setup](../../Source/DungeonForged/Public/Combat/AN/) — o warp target lookup já busca lock-on, mas precisa garantir que ele aceita target acima/abaixo do player (não só XY plane).

### 8.6 Inimigo: juggle state

Para AAA juggle, inimigo precisa:
- Tag `State.Juggled` enquanto airborne
- `GravityScale` reduzido enquanto juggle (já feito via `Launcher::ApplyLaunch(TargetGravity, Hangtime)`)
- Anim de "flutuando atordoado" em vez de hit-react normal
- Hit count contador → após N hits cai mesmo com gravity baixa (anti infinite combo)

```cpp
// UDFEnemyBase
UPROPERTY()
int32 AerialHitsInCurrentJuggle = 0;

UPROPERTY(EditDefaultsOnly, Category="DF|Combat")
int32 MaxAerialHitsBeforeDrop = 5;

void OnTakeAerialHit() {
    if (++AerialHitsInCurrentJuggle >= MaxAerialHitsBeforeDrop) {
        GetCharacterMovement()->GravityScale = 1.f;  // força queda
    }
}
```

---

## 9. Fase 5 — Camera, animação, FX, áudio (AAA polish)

### 9.1 Camera

**Arquivo:** [`UDFCameraComponent.cpp`](../../Source/DungeonForged/Private/Camera/UDFCameraComponent.cpp)

```cpp
// Listen ao OnDFMovementModeChanged
void HandleMovementModeChanged(EMovementMode New, EMovementMode Prev, uint8) {
    if (Prev == MOVE_Walking && New == MOVE_Falling) {
        // Jump: FOV +3 por 0.15s
        StartFOVPulse(+3.f, 0.15f);
        // Camera lag temporariamente menor (acompanha mais rápido)
        GetSpringArm()->CameraLagSpeed = 25.f;
    }
    else if (Prev == MOVE_Falling && New == MOVE_Walking) {
        // Land: subtle camera drop
        StartCameraOffset(FVector(0, 0, -8), 0.18f);
        // Restaurar lag
        GetSpringArm()->CameraLagSpeed = 10.f;
    }
}
```

Air dash: shake horizontal + FOV +5 por 0.2s.

### 9.2 Rumble

`UForceFeedbackEffect`:
- `FFB_JumpLight` (jump padrão, 0.1s, intensity 0.3)
- `FFB_DoubleJump` (0.15s, intensity 0.5)
- `FFB_AirDash` (0.2s, intensity 0.7)
- `FFB_Land_Soft` (AirTime < 0.5s, 0.08s, 0.2)
- `FFB_Land_Hard` (AirTime > 1.2s, 0.25s, 0.9)

Hook em `OnLanded`, `DoJump`, `UDFAbility_AirDash`.

### 9.3 VFX

| Evento | VFX | Onde |
|---|---|---|
| Jump start | `NS_JumpDust_Small` (5cm circle, foot socket) | `AN_JumpTakeoff` no `Jump_Start_*` |
| Double jump | `NS_DoubleJumpRing` (anel feather expandindo) | `AN_DoubleJumpRing` |
| Apex | `NS_ApexHighlight` (subtle vertical light burst) | `AN_JumpApex` (já planejado §8) |
| Air dash | `NS_AirDashTrail` (trail por DashDuration) | begin/end notify state |
| Landing (escalado) | `NS_LandDust` user param Intensity | `AN_Land` no `Jump_End_*` |
| Aerial finisher impact | `NS_GroundSlam` + crack decal | end of `AS_Aerial_3` |

### 9.4 Áudio

**Arquivo:** [`DFAudio` (componente)](../../Source/DungeonForged/Public/Audio/UDFAudioComponent.h)

| Evento | Som |
|---|---|
| Jump | `SFX_Jump_Whoosh` (curto, 0.2s) |
| Double jump | `SFX_DoubleJump_Whoosh` (mais agudo, com pitch +200c) |
| Apex | (silêncio — efeito por contraste) |
| Air dash | `SFX_AirDash` (whoosh + zip) |
| Landing | `SFX_Land_Soft` / `SFX_Land_Hard` (escalado) |
| Aerial swing | `SFX_AerialSwing_*` (3 variantes randomized) |
| Smash down impact | `SFX_GroundSlam_Impact` (baixa + bass shake) |

### 9.5 Slowmo / Hit-stop

`UDFImpactFramingComponent` já existe. Hooks adicionais:
- Aerial finisher: `SetTimeDilation 0.5` por 0.12s (mais longo que ground)
- Style "S" rank durante combo aéreo: bullet-time momentâneo no último hit

### 9.6 Style Rating

`UDFStyleRatingComponent` (já existe). Bumps específicos:
- +5 score por hit aéreo (vs +2 ground)
- +20 por air dash entre hits ("Aerial Rave")
- +50 por combo aéreo de 3+ hits sem tocar chão
- -100 por whiff aéreo (ataca o vazio no ar)

---

## 10. Riscos técnicos & contramedidas

| Risco | Probabilidade | Impacto | Mitigação |
|---|---|---|---|
| Network desync com double jump prediction | Alta | Alto | `FSavedMove_DF::bWantsDoubleJump` no compressed flag; usar `JumpCurrentCount` (já replicado) |
| Air dash com root motion + servidor: client sees teleport, server sees walking | Média | Alto | `FRootMotionSource_MoveToForce` é prediction-safe (já usado em Dodge); copy pattern exato |
| Anti-gravity hangtime persiste se montage interrompida | Alta | Médio | `NotifyEnd` sempre restaura (UE garante chamada mesmo em interrupt) |
| Aerial combo trace falha — montage no ar mas inimigo fora do range | Alta | Médio | MotionWarping aéreo + raycast wider (1.3× ground trace radius) |
| Performance: line trace `PredictedLandingDistance` toda frame | Baixa | Médio | Só rodar enquanto `bIsFalling && Vz < 0` (já está assim no doc) |
| Coyote time + jump buffer combinados = double jump fantasma | Média | Médio | Buffer e coyote são **mutuamente exclusivos**: se coyote ativa, buffer não pode dar trigger no mesmo arco |
| Long-fall anim plays mas player foi launched (não pulou) | Média | Baixo | Distinguir `bWasLaunched` (flag set ao receber `LaunchCharacter`) |
| Air dash em scenery (poste, beira) → player atravessa | Alta | Alto | `bRestrictSpeedToExpected = true` no RootMotionSource (já faz isso no Dodge); collision sweep no path |
| State.AirDashing pode persistir se ability for cancelada por respawn | Baixa | Médio | `ApplyDefaultPassiveGameplayEffects` clear all loose tags ao respawn |
| Aerial combo + lock-on: camera fica olhando para cima eternamente | Média | Médio | `UDFLockOnComponent` clamp pitch máximo (-30°/+30°) durante aerial; ou unlock auto |

---

## 11. Network prediction — pontos críticos

### 11.1 Padrão existente (sprint) — boa referência

[`FSavedMove_DF`](../../Source/DungeonForged/Public/Characters/UDFCharacterMovementComponent.h:211) usa `FLAG_Custom_0` para `bWantsSprint`. Padrão correto.

### 11.2 Para double jump

UE5 já replica `JumpCurrentCount` automaticamente. **Nada extra precisa** ser feito no `FSavedMove_DF` para o double jump — `Super::DoJump` já incrementa o count.

**Atenção:** se você modificar `Velocity.Z = 0.f` antes do `Super::DoJump` no double jump, isso **não** é replicado por padrão. Solução: fazer dentro de `Super::DoJump` override **antes** de `Super::Super::DoJump`, garantindo que `FSavedMove::ReplaySamples` capture a velocidade modificada. Ou usar `LaunchCharacter` (já é replicado).

### 11.3 Para air dash

**Não use** `FRootMotionSource_MoveToForce` em listen-server sem cuidado: a `MoveTo` precisa de StartLocation/EndLocation autoritários. Como já se faz no Dodge — `PerformDodge` chamado pelo client, RM source aplicado por ambos (`bLocalPredicted` na GA). **Copiar pattern.**

### 11.4 Coyote time

**Server-side only** ou cliente também? Recomendação: **client-side** (parte do input grace), com server validation tolerante (server aceita `DoJump` se "recém saiu de walking" — já é o caso na engine).

### 11.5 Jump buffer

**Client-side only.** Server nunca vê o buffer — só vê o `Jump()` final quando o buffer expira. Zero overhead de rede.

### 11.6 Variable jump height

`Velocity.Z *= 0.4` é uma mudança de velocidade — **precisa** ir via prediction. Patch: adicionar `FLAG_Custom_1` no `FSavedMove_DF` para `bWantsApexCut`.

### 11.7 Anti-gravity hangtime durante combo

`GravityScale` **é replicado** automaticamente (UE built-in). Mas mudanças via `NotifyBegin/End` precisam acontecer em **ambos os lados**. Como notifies rodam no AnimBP que roda em todo lado (server + clients), isso já funciona. **Mas atenção:** se o player tem `bUseClientSideAnimation = true` (default), o server pode não rodar o notify; nesse caso usar Multicast.

---

## 12. Checklist de Quality Bar AAA

Use como **definition of done** ao final de cada fase.

### Game-feel (sentir)
- [ ] Jump é responsivo (input → takeoff < 16ms = 1 frame @60fps)
- [ ] Pular ao correr cobre ≥ 1.2× a distância de pular parado
- [ ] Soltar o botão de jump enquanto sobe corta a altura (variable jump)
- [ ] Coyote time funciona: andar para fora de plataforma + jump em 100ms = pulo válido
- [ ] Input buffer: apertar jump 150ms antes do touchdown = pulo imediato ao pisar
- [ ] Double jump tem timing forgiving (pode acionar a qualquer hora no arco)
- [ ] Air dash chega em 400cm e tem snap perceptível (não "interp suave")
- [ ] Landing recovery não trava combat se acabou de fazer air dash

### Audio-visual
- [ ] Cada uma das 11 anim slots (5 start + 1 loop + 5 land) é tocada (df.JumpDebug)
- [ ] Apex VFX dispara consistentemente no top do arco
- [ ] Landing dust escala com AirTime
- [ ] Camera tem subtle kick no jump (FOV +3) e drop no land (offset -8)
- [ ] Rumble disponível em 5 variantes (jump/dj/dash/land-soft/land-hard)
- [ ] Áudio distingue jump vs double jump (pitch diferente)
- [ ] Whiff aéreo tem swing-miss SFX (não silêncio)

### Combate aéreo
- [ ] Combo terrestre → jump cancel → combo aéreo flui sem visualmente "resetar"
- [ ] Launcher attack → player automaticamente persegue (tracking jump)
- [ ] Inimigo juggled tem GravityScale 0.4 (visível: flutua entre hits)
- [ ] Aerial finisher (smash) cria ground crack + camera shake
- [ ] Style rating sobe mais por combat aéreo que terrestre

### Network
- [ ] Listen-server: outros players veem jumps remotos suaves
- [ ] Dedicated: ping 80ms client predict + reconcile sem visual snap
- [ ] Air dash com root motion não dessincroniza em ping 150ms
- [ ] Spam jump não cria duplicate tags no servidor (count == 1 sempre)

### Debug
- [ ] `df.JumpDebug 3` mostra estado completo (count, AirTime, Vz, tags)
- [ ] `df.JumpDebug.AirDash` mostra dash count, distance, dir
- [ ] On-screen overlay opcional para combo step (G/A) + style rank

### Performance
- [ ] PredictedLandingDistance line trace só roda enquanto Vz < 0
- [ ] AnimNotifyState_AerialHangtime sempre restaura GravityScale (mesmo em interrupt)
- [ ] Nenhum tick novo no CMC (reusar TickComponent existente)

---

## 13. Tabela de arquivos a tocar

### Modificações (existentes)

| Arquivo | O quê |
|---|---|
| [`Source/DungeonForged/Public/Characters/UDFCharacterMovementComponent.h`](../../Source/DungeonForged/Public/Characters/UDFCharacterMovementComponent.h) | Coyote, IsWithinCoyoteWindow, TimeLastLeftGround |
| [`Source/DungeonForged/Private/Characters/UDFCharacterMovementComponent.cpp`](../../Source/DungeonForged/Private/Characters/UDFCharacterMovementComponent.cpp) | DoJump (double jump branch), sprint→jump momentum, OnMovementMode coyote tracking |
| [`Source/DungeonForged/Public/Characters/ADFPlayerCharacter.h`](../../Source/DungeonForged/Public/Characters/ADFPlayerCharacter.h) | JumpInputBufferedUntil, StopJumping, OnMovementModeChanged_DF |
| [`Source/DungeonForged/Private/Characters/ADFPlayerCharacter.cpp`](../../Source/DungeonForged/Private/Characters/ADFPlayerCharacter.cpp) | Jump (buffer), StopJumping (apex cut), Listen movement delegate, set JumpMaxCount=2 |
| [`Source/DungeonForged/Public/Animation/UDFAnimInstance.h`](../../Source/DungeonForged/Public/Animation/UDFAnimInstance.h) | bIsDoubleJumping, bIsAirDashing, bIsLongFallLanding |
| [`Source/DungeonForged/Private/Animation/UDFAnimInstance.cpp`](../../Source/DungeonForged/Private/Animation/UDFAnimInstance.cpp) | Track JumpCurrentCount, AirDash tag, long fall threshold |
| [`Source/DungeonForged/Public/Animation/DFAnimSetTypes.h`](../../Source/DungeonForged/Public/Animation/DFAnimSetTypes.h) | FUDJumpAnimSet add DoubleJump_Start/Loop + AirDash_8Way |
| [`Source/DungeonForged/Public/GAS/DFGameplayTags.h`](../../Source/DungeonForged/Public/GAS/DFGameplayTags.h) | State.DoubleJumping, State.AirDashing, State.AerialCombo, State.Juggled |
| [`Source/DungeonForged/Public/Data/UDFCombatTuningData.h`](../../Source/DungeonForged/Public/Data/UDFCombatTuningData.h) | CoyoteTime, JumpInputBufferDuration, ApexCutScale, DoubleJumpStaminaCost, AirDash* params, AerialHangtimeGravityScale |
| [`Source/DungeonForged/Private/Combat/UDFLauncherComponent.cpp`](../../Source/DungeonForged/Private/Combat/UDFLauncherComponent.cpp) | Dispatch tracking jump hook após ApplyLaunch |
| [`Source/DungeonForged/Public/Combat/UDFComboComponent.h`](../../Source/DungeonForged/Public/Combat/UDFComboComponent.h) | (já tem AerialComboSteps; configurar via DA) |
| [`Source/DungeonForged/Private/Camera/UDFCameraComponent.cpp`](../../Source/DungeonForged/Private/Camera/UDFCameraComponent.cpp) | FOV pulse / camera offset listen |

### Novos arquivos

| Arquivo | Propósito |
|---|---|
| `Source/DungeonForged/Public/GAS/Abilities/UDFAbility_AirDash.h/.cpp` | GA de air dash |
| `Source/DungeonForged/Public/Animation/UANS_DFAerialHangtime.h/.cpp` | NotifyState anti-gravity durante swing aéreo |
| `Source/DungeonForged/Public/Animation/UDFAnimNotify_DoubleJumpApex.h/.cpp` | Anim notify para FX/SFX do segundo apex |
| `Source/DungeonForged/Public/Combat/UDFAerialJuggleComponent.h/.cpp` (opcional) | No inimigo — gerencia hit count e force-fall após N hits |

### Conteúdo / assets a criar

| Asset | Onde |
|---|---|
| `IA_AirDash` (Input Action) | `Content/DungeonForged/Input/Actions/` |
| `IMC_DFDefault` modificado | adicionar binding `IA_AirDash` |
| `AS_AirDash_F/B/L/R/FL/FR/BL/BR` (8 montages) | `Content/Assets/Animations/JCHero/` |
| `AS_Aerial_1/2/3` montages | `Content/Assets/Animations/JCHero/Combat/` |
| `NS_DoubleJumpRing` (Niagara) | `Content/DungeonForged/VFX/` |
| `NS_AirDashTrail` (Niagara) | `Content/DungeonForged/VFX/` |
| `SFX_AirDash` / `SFX_DoubleJump` (Sound Wave) | `Content/DungeonForged/SFX/` |
| `FFB_AirDash` / `FFB_DoubleJump` | `Content/DungeonForged/Input/Rumble/` |
| `DA_CombatTuning` updated values | (já existe — edit no editor) |

---

## 14. Próximos passos sugeridos

### Ordem de execução (sequência mínima viável → AAA)

1. **HOJE — Fase 1 (1-2 dias):** coyote time + jump buffer + variable jump + sprint→jump momentum + long-fall threshold.
   *Por quê primeiro:* máximo de "feel" por linha de código, ZERO arte nova, melhora o pulo single imediatamente.

2. **Próxima semana — Fase 2 (2-3 dias):** double jump (CMC + tags + AnimSet + 2 montages).
   *Por quê depois da F1:* double jump revela bugs de "single jump não-AAA" se Fase 1 não tiver sido feita.

3. **Semana seguinte — Fase 3 (3-4 dias):** air dash GA.
   *Por quê depois do double jump:* compartilha tags airborne, e o air dash precisa do double jump para sentir "fluido" (DMC pattern).

4. **Semana 4 — Fase 4 (4-6 dias):** air combo real + launcher pursuit + anti-grav hangtime.
   *Por quê:* é a maior wave de trabalho — só compensa com fundação dos 3 anteriores.

5. **Semana 5 — Fase 5 (2-3 dias):** camera, áudio, VFX, rumble, style rating bumps.
   *Por quê último:* polish só funciona se o gameplay base é bom. Senão é "lipstick on a pig".

### Quick wins se quiser ver resultado **hoje**

- **15 min:** subir `JumpAirControl` em `DA_CombatTuning` de 0.35 → 0.65. Já muda dramaticamente a sensação.
- **10 min:** subir `JumpFallGravityMultiplier` de 1.25 → 1.45. Pulo "snappy" estilo Mario.
- **30 min:** implementar variable jump height (§5.3 acima). 5 linhas, transforma o pulo.

### Como medir sucesso por fase

- **Fase 1:** abrir o jogo, gravar 30s correndo e pulando, comparar com vídeo de DMC5/Bayonetta. Deve "sentir comparável".
- **Fase 2:** double jump deve permitir alcançar plataforma a 700cm vertical (1 jump = 400cm, 2 jumps = 700cm).
- **Fase 3:** air dash 400cm consistentemente, sem "trespasse" de paredes finas.
- **Fase 4:** 3-hit aerial combo deve manter o inimigo no ar por ≥ 1.8s (cima do hangtime mínimo do Bayonetta).
- **Fase 5:** "blind test" — peça a alguém fora do projeto para jogar 5min. Se eles falarem "isso parece um jogo AAA", você chegou lá.

---

## 15. Status de implementação (C++ — 2026-05-25)

| Fase | Item | Status | Notas |
|------|------|--------|-------|
| **1** | Coyote time | ✅ | `CoyoteTime`, `IsWithinCoyoteWindow()`, `RequestJump()` |
| **1** | Jump input buffer | ✅ | `JumpInputBufferDuration`, soft `State.Landing` |
| **1** | Variable jump (apex cut) | ✅ | `StopJumping()` + `JumpApexCutScale` |
| **1** | Soft landing gate | ✅ | Landing → buffer, não hard block |
| **1** | Sprint → jump momentum | ✅ | `SprintJumpHorizontalBoost` em `DoJump` |
| **1** | Long-fall threshold | ✅ | `bIsLongFallLanding` no AnimInstance |
| **2** | Double jump | ✅ | `JumpMaxCount=2`, `DFDoubleJumpZScale`, tags |
| **2** | AnimSet double jump | ✅ | `DoubleJump_Start/Loop` + `GetJumpDouble*` |
| **2** | AnimBP double state | ⏳ Editor | Wire `bIsDoubleJumping` no SM |
| **3** | Air dash GA | ✅ | `UDFAbility_AirDash` — grant na DT |
| **3** | Dodge só no chão | ✅ | Air usa AirDash |
| **3** | Landing skip pós dash | ✅ | `AirDashLandingRecoverySkipWindow` |
| **4** | Launcher auto-jump | ✅ | Sem `SelfLaunchVelocity` → `Jump()` |
| **4** | Aerial hangtime notify | ✅ | `AnimNotifyState_AerialHangtime` |
| **4** | Aerial combo montages | ⏳ Content | Infra `AerialComboSteps` já existe |
| **5** | Camera / rumble / VFX | ⏳ | Fora do scope deste commit |

**Editor / content pendente:**
1. `DA_CombatTuning` — rever novos campos (coyote, buffer, air dash, double jump).
2. `DT_Abilities` — adicionar row `UDFAbility_AirDash` + input bind.
3. `ABP_JSHeroCharacter` — estado Double Jump + `bIsLongFallLanding` no Land.
4. JumpSet — assign `DoubleJump_Start` (pode reusar Start ou clip dedicado).
5. Montages aéreas — colocar `ANS_DFAbilityCancelWindow` + `AnimNotifyState_AerialHangtime`.

---

## Referências cruzadas no projeto

- [`docs/improvements/17_JumpSystem.md`](../improvements/17_JumpSystem.md) — sistema direcional de pulo (§16 Combate Aéreo, §17 Launcher→TrackingJump, §18 Troubleshooting)
- [`docs/improvements/15_DodgeAbility_4Way.md`](../improvements/15_DodgeAbility_4Way.md) — pattern de root motion + i-frames a copiar no air dash
- [`docs/improvements/16_LockOnSystem.md`](../improvements/16_LockOnSystem.md) — strafe + camera durante combate aéreo
- [`docs/improvements/14_AAA_CombatSystem.md`](../improvements/14_AAA_CombatSystem.md) — combo grounded (extender pattern para aerial)
- [`docs/improvements/10_AAA_AimWarpCombat.md`](../improvements/10_AAA_AimWarpCombat.md) — motion warping (aplicar em aerial swings)
- [`docs/analysis/Combat_Advanced_Report.md`](Combat_Advanced_Report.md) — análise prévia do combate (esse doc complementa)

---

## TL;DR

**Você tem 70% das fundações.** O CMC, AnimInstance, GAS tags, network prediction e Launcher estão prontos. Faltam **7 itens críticos** para AAA:

1. Coyote time (~80-100ms)
2. Jump input buffer (~150ms)
3. Variable jump height (apex cut)
4. Double jump (`JumpMaxCount = 2` + anim)
5. Air dash (nova GA)
6. Air combo montages reais (infra já existe!)
7. Anti-gravity hangtime durante swing aéreo

**Investimento estimado:** 12-18 dias de dev. **Maior ROI:** Fase 1 (1-2 dias, 80% do "feel AAA grounded"). Comece por aí.

---

## 15. Análise dos logs runtime (`df.JumpDebug`)

> **Captura:** sessão de play real ~120s, com `df.JumpDebug 3+4 dump` (incluindo `[Jump|Deep]` snapshots).
> **O que mudou desde a análise inicial:** o usuário **já implementou parte da Fase 1** — vejo `Apex cut Vz=...` e `Jump BUFFERED until ...` nos logs, então **variable jump height e jump input buffer já estão funcionando**. Ainda restam coyote time, sprint→jump momentum e long-fall threshold.

### 15.1 O que está funcionando ✅

| Sistema | Evidência no log |
|---|---|
| **Variable jump (apex cut)** | `[Jump] Apex cut Vz=233` (Vz cai de 730 → 233 ao soltar botão) |
| **Jump input buffer** | `[Jump] Jump BUFFERED until 121.620s` → `[Jump] Jump BUFFER consumed` |
| **Captura de direção no takeoff** | `Takeoff dir=1 speed=540 StartLen=0.750s` (forward jump = `Jump_Combat_Start_F_0`) |
| **Fall gravity multiplier** | Vz vai de +730 → +234 (rising lento) → −556 (falling rápido) — boost pós-apex visível |
| **Pré-blend Loop→LandPrep** | `Loop->Prep=ON` aparece quando `PredLand=20 Alpha=0.92` (perto do chão) |
| **Land direcional** | `Jump_Combat_End_F_0_Seq` é usado quando `dir=1` (forward landing) |
| **Sync de loose tags grounded** | `Transition OFF: Land->Loco` limpa estados corretamente |

### 15.2 Bugs detectados nos logs 🐛

#### Bug #1 (CRÍTICO) — **State Machine do AnimBP fica "stuck" em Jump Loop**

Repetidamente vejo o SM travado em `Jump Loop` com `elapsed` crescendo absurdamente, **mesmo com o player já no chão**:

```log
[Jump] Transition OFF: Land->Loco
[Jump] DoJump OK JumpZ=750
[Jump|Deep]   SM[1] State=Jump Loop elapsed=2.078s weight=1.00   ← deveria estar em Locomotion!
```

E em outra captura, o SM permanece em "Jump Loop" por **3.5+ segundos**, atravessando múltiplos pulos:

```log
[Jump|Deep]   SM[1] State=Jump Loop elapsed=3.127s weight=1.00
[Jump|Deep]   SM[1] State=Jump Loop elapsed=3.370s weight=1.00
[Jump|Deep]   SM[1] State=Jump Loop elapsed=3.548s weight=1.00
[Jump|Deep]   SM[1] State=Jump Loop elapsed=3.627s weight=1.00
```

**Diagnóstico:**
- Os flags `bTransition_*` no C++ disparam corretamente (`READY → ON → OFF`).
- Mas o State Machine no `ABP_DungeonForged` **não está consumindo esses flags** — provavelmente as regras de transição no AnimGraph estão usando outras condições (ex: `bIsFalling`, `bIsLanding`) que entram em race com chain-jumps via buffer.
- O sintoma só aparece em **chain-jumps** (buffer consumed → takeoff antes do Land state completar a entrada).

**Fix sugerido (no AnimBP):**
- Abrir `ABP_JSHeroCharacter` ou equivalente.
- Em cada transição do SM `Locomotion → JumpStart → JumpLoop → Land → Locomotion`, trocar a condição para usar **exclusivamente** o flag correspondente do AnimInstance:
  - Locomotion → JumpStart: `bTransition_LocomotionToJumpStart`
  - JumpStart → JumpLoop: `bTransition_JumpStartToLoop`
  - JumpLoop → Land: `bTransition_JumpLoopToLand`
  - Land → Locomotion: `bTransition_LandToLocomotion`
  - **Adicionar:** Jump Loop → JumpStart (re-entry para chain-jump): condition `bTransition_LocomotionToJumpStart && bJumpArcActive` (já existe — só wire).
- Garantir `Blend Logic = Standard Blend` e `Transition Priority Order` correto (priorizar Land→Loco quando há buffer consumed).

#### Bug #2 (ALTO) — **`State.Landing` tag não é limpa no takeoff bufferizado**

Logo após `Jump BUFFER consumed` e novo `DoJump OK`, vejo:

```log
[Jump|Deep]   J=1 F=0 InAir=1 Apex=0 Arc=1 L=1 | Vz=722 Air=0.02s
```

`L=1` (`State.Landing` ativo) **enquanto o player está subindo com Vz=722**. O takeoff Walking→Falling deveria limpar `State.Landing`, mas em [`UDFCharacterMovementComponent.cpp:200-228`](../../Source/DungeonForged/Private/Characters/UDFCharacterMovementComponent.cpp) o branch só adiciona `State.Jumping/Falling` sem chamar `ClearJumpLandingLooseTag(ASC)`.

**Fix:**

```cpp
// UDFCharacterMovementComponent::OnMovementModeChanged
if (PreviousMovementMode == MOVE_Walking && NewMode == MOVE_Falling)
{
    bJumpFallingTagActive = false;
    if (ASC)
    {
        ClearJumpLandingLooseTag(ASC);  // ← ADICIONAR: limpa stale Landing
        if (Velocity.Z > 1.f)
        {
            AddJumpLooseTagOnce(ASC, FDFGameplayTags::State_Jumping);
        }
        // ...
    }
}
```

E também cancelar o `TimerHandle_EndLanding` para não disparar depois do takeoff e limpar a tag *Jumping* por engano.

#### Bug #3 (ALTO) — **`PredictedLandingDistance` stale durante subida**

```log
J=1 F=0 InAir=1 Apex=0 Arc=1 L=1 | Vz=722 Air=0.02s Gnd=0.00s PredLand=20 Alpha=0.92
```

Vz=722 (subindo!), mas `PredLand=20 Alpha=0.92` (sistema acha que vai aterrissar em 20cm). Isso vai disparar `Loop→LandPrep` no frame seguinte mesmo o player tendo acabado de iniciar um pulo novo.

**Causa:** o `else` que reseta `PredictedLandingDistance = LandPredictionTraceMax` **provavelmente não está sendo executado durante `bIsJumping` (Vz > 0)** — só durante grounded. O snapshot stale do frame anterior persiste.

**Fix em `UDFAnimInstance.cpp` (NativeUpdateAnimation):**

```cpp
// Predict landing distance only while falling AND in air
if (bIsFalling && bIsInAir && OwningCharacter)
{
    // line trace logic
}
else
{
    PredictedLandingDistance = LandPredictionTraceMax;  // sempre reseta quando não-falling
}
```

E adicionar uma guarda no takeoff frame:

```cpp
if (!bWasInAir && bNowInAir)
{
    // ... existing capture of direction, AirTime reset ...
    PredictedLandingDistance = LandPredictionTraceMax;  // ← reset no takeoff
}
```

#### Bug #4 (MÉDIO) — **`bJumpArcActive` (Arc=1) persiste por longo tempo no Locomotion**

```log
[Jump|Deep]   J=0 F=0 InAir=0 Apex=0 Arc=1 L=0 | Vz=0 Air=0.00s Gnd=0.36s
```

Player grounded há 0.36s, sem Landing, mas `Arc=1`. O `bJumpArcActive` está pendurado.

**Fix:** o latch `bJumpArcActive` deveria ser liberado quando `bTransition_LandToLocomotion` dispara (já existe no log) ou quando `GroundedTime > LandingRecoveryWindow + 0.05`.

Em `UDFAnimInstance.cpp`:

```cpp
// No final de NativeUpdateAnimation, depois das transitions
if (bTransition_LandToLocomotion || (GroundedTime > LandingRecoveryWindow + 0.05f))
{
    bJumpArcActive = false;
}
```

#### Bug #5 (BAIXO) — **`JumpStartMinPlayTime` mismatch com Forward anim**

```log
[Jump|Deep] Tune| JumpStartMinPlayTime (0.420s) differs from Start anim (0.750s) — using max at runtime (0.750s)
```

O sistema lida bem (usa max), mas o tuning configurado (`0.420s`) corresponde ao Jump_Start_0 (idle, 0.417s) e não considera que o Forward jump tem **0.750s**. Resultado: a transição Start→Loop fica esperando 750ms quando o player jumps forward, mas o pulo (`Air time`) só dura ~0.50s → o player **aterrissa antes do Loop sequer começar** (e o sintoma é que o "Jump Loop" anim nunca é visível em pulos curtos forward).

Vejo isso explícito no log:
```log
[Jump] Landed dir=1 loopPhase=0.084s recovery=0.350s   ← landed com loopPhase=0.084s = nunca chegou ao Loop visualmente
```

**Fix:**
- Tornar `JumpStartMinPlayTime` por direção (ou auto-detectar pelo `GetJumpStartAnim()->GetPlayLength()` no takeoff — você já cachea `CachedJumpStartPlayTime`).
- Garantir que o gate de Start→Loop usa `CachedJumpStartPlayTime` (real) em vez de `JumpStartMinPlayTime` (configurado).
- Alternativa: reduzir o `Jump_Combat_Start_F_0` para ~0.42s no editor (re-time da anim), igualando o idle. Para AAA, **ambas devem ter mesma duração** — facilita o tuning e elimina branches.

### 15.3 Findings de tuning observados (sanity)

| Métrica | Observado no log | Comentário |
|---|---|---|
| Jump Z velocity inicial | `Vz=730-734` | Subiu de 550 → 750 no `DA_CombatTuning`. ✅ |
| Apex cut residual | `Vz=177-289` (var.) | Tap rápido = ~180, hold longo = ~290. Range saudável. |
| Air time (jump padrão) | ~0.50s | OK para `JumpZ=750`, `Gravity=1.7`, `FallMult=1.25`. |
| Air time (long jump observado) | 0.95s | Vejo um pulo que durou 0.95s — provavelmente cima de plataforma. Long-fall threshold (1.2s) ainda não acionou. |
| Forward jump dist horizontal | `speed=540` no takeoff | Idêntico a sprint speed; sem boost de sprint→jump (G11 ainda pendente). |
| Land recovery window | 0.350s | Confirmado em todos os "recovery=0.350s". Bom range. |
| `Direction` no Land | `dir=0` idle, `dir=1` forward | Captura correta no takeoff, persistente. ✅ |

### 15.4 Prioridade revisada (após logs)

Com Apex Cut e Jump Buffer já implementados, a prioridade **muda**:

| # | Item | Prioridade | Razão |
|---|---|---|---|
| 1 | **Fix Bug #1 (AnimBP SM stuck)** | 🔴 P0 — agora | Bloqueia qualquer trabalho aéreo futuro |
| 2 | **Fix Bug #2 (State.Landing stale)** | 🔴 P0 — agora | Bug de tag pode bloquear abilities |
| 3 | **Fix Bug #3 (PredLand stale)** | 🟠 P1 | Visual: Land Prep dispara prematuro |
| 4 | **Fix Bug #4 (JumpArcActive latch)** | 🟡 P2 | Cosmético; debug overlay |
| 5 | **Fix Bug #5 (StartLen mismatch)** | 🟠 P1 | Forward jump nunca mostra Loop visualmente |
| 6 | Coyote time (Fase 1, §5.1) | 🟠 P1 | Continua sendo win de feel |
| 7 | Sprint→jump momentum (Fase 1, §5.5) | 🟡 P2 | Win simples mas menor que coyote |
| 8 | Long-fall anim threshold (Fase 1, §5.6) | 🟡 P2 | Polish; pulo de 0.95s já existe nos logs |
| 9 | Double jump (Fase 2) | 🟠 P1 | Maior win de skill-ceiling |
| 10 | Air dash (Fase 3) | 🟡 P2 | Após estabilidade do single+double |

### 15.5 Checklist de regressão após os fixes

Use `df.JumpDebug 3+4 dump` e verifique:

- [ ] **Pulo simples (idle)**: SM passa por `Locomotion → JumpStart → JumpLoop → Land → Locomotion` em ordem, sem ficar stuck.
- [ ] **Pulo forward (W pressionado)**: idem, mas com `Jump_Combat_Start_F_0` e `Jump_Combat_End_F_0`.
- [ ] **Chain-jump (jump → bufferizar segundo jump → tocar chão)**: SM transita `Land → Locomotion → JumpStart` (não fica em Jump Loop).
- [ ] **Apex cut**: solte o botão na subida → `Vz` cai abruptamente (já validado nos logs).
- [ ] **`State.Landing` no `L=` flag**: deve ser 0 sempre que `J=1` ou `F=1` (no ar).
- [ ] **`PredLand` no flag**: deve ser `1000` (max trace) quando `J=1` (subindo) ou grounded; só baixa quando `F=1` (falling).
- [ ] **`Arc` no flag**: cai para 0 quando `Land→Loco` completa OU `GroundedTime > 0.4s`.
- [ ] **Tune mismatch warning**: não deve mais aparecer (`JumpStartMinPlayTime differs from Start anim`).

### 15.6 Sumário runtime

**Boa notícia:** Apex cut + Jump Buffer funcionam. Você já tem **2 dos 7 gaps Tier-S resolvidos** (Variable Jump + Jump Buffer). Faltam 5 (coyote, double jump, air dash, aerial combo real, anti-grav hangtime).

**Má notícia:** Há 5 bugs ativos no estado atual — sendo 2 deles **críticos** (AnimBP SM stuck + State.Landing stale). Esses **precisam ser resolvidos antes** de adicionar double jump / air dash, senão o caos vai se multiplicar com novas mecânicas aéreas.

**Recomendação imediata:** dedicar 1 dia para os 5 bugs (todos são 5-30 linhas de fix cada). Depois prosseguir com Coyote Time (15min) e então Double Jump (Fase 2).

---

## 16. Análise dos logs runtime — Locomoção (`df.LocomotionDebug 4`)

> **Captura:** sessão de play (~90s), com `df.LocomotionDebug 4` ligado.
> **Sintoma reportado:** "tirou slow [exhaustion] mas Stop está travando de novo".
> **Diagnóstico:** **mesmo padrão do Bug #1 do Jump SM** — AnimBP State Machine não consome os flags de transição C++. A travada do Stop é causa-raiz idêntica.

### 16.1 Sequência observada: Run → Stop trava

Sequência simplificada do log (player decelerando após soltar input em velocidade Run=429):

```log
1) Spd=429 Gait=2 (Run)  | I>S=0 S>L=1 L>P=0 S>M=1 P>I=0     ← rodando OK
2) Spd=234 Gait=1 (Walk) | I>S=0 S>L=0 L>P=1 S>M=0 P>I=0     ← Loop>Stop dispara
3) Spd=106 Gait=1        | I>S=0 S>L=0 L>P=1 S>M=0 P>I=0
4) Spd=28  Gait=0 (Idle) | I>S=0 S>L=0 L>P=1 S>M=0 P>I=0
5) Spd=0   Gait=0        | I>S=0 S>L=0 L>P=1 S>M=0 P>I=1     ← Stop>Idle dispara
6) Spd=0   Gait=0        | I>S=0 S>L=0 L>P=1 S>M=0 P>I=1     ← FLAGS PERSISTEM
7) Spd=0   Gait=0        | I>S=0 S>L=0 L>P=1 S>M=0 P>I=1     ← (continua indef.)
```

E no HUD dump correspondente:

```log
== Locomotion [MAIN] ABP_JSHeroCharacter_C ==
Speed=0 Gait=Idle Dir=Forward Accel=0
Trans: Idle>Start=0 Start>Loop=0 Loop>Stop=1 Stop>Move=0 Stop>Idle=1 TIP=0
Anim Stop = Run_Stop_F_0_Seq
Stop=0/202(consumed 0) Time=0.77/1.50(end 0.77)   ← TRAVADO em Time=0.77
```

### 16.2 Smoking gun

Três pistas confirmam que é **AnimBP SM stuck na transição Stop → Idle**:

| Pista | Significado |
|---|---|
| `Time=0.77/1.50(end 0.77)` repetido em dumps consecutivos | A timeline do Stop **parou** exatamente no end-fraction. Não está avançando para a próxima keyframe. |
| `Stop=0/202(consumed 0)` ou `consumed 5/17` | A distância-matching consumiu **17 cm de 202 cm** previstos antes de a velocidade zerar. O Stop foi cortado prematuro pela queda de velocidade. |
| `Loop>Stop=1` **e** `Stop>Idle=1` simultaneamente, persistentes | C++ está reportando duas transições prontas ao mesmo tempo — o SM não consumiu nenhuma. |

Ao mesmo tempo, dados de tuning mostram inconsistências:

```log
WARN AuthoredLoopSpeed(429) != loop Distance avg(167) — tune Class Defaults
```

→ `Walk_Loop_F_0_Seq` declara velocidade de 429 cm/s mas a curva Distance dela só dá 167 cm/s. Diferença **2.5×**. Não é a causa do travamento mas afeta stride scale e dist-matching.

### 16.3 Root cause: idêntica ao Bug #1 do Jump SM

O padrão é o **mesmo bug** já documentado para o Jump (§15.2 Bug #1):

| Camada | Comportamento |
|---|---|
| C++ AnimInstance | Atualiza `bTransition_StopToIdle = true` corretamente |
| C++ time-tracker | Atualiza `Time=0.77/1.50` correctamente até atingir o end-fraction |
| **AnimBP State Machine** | **Não usa** `bTransition_StopToIdle` como regra de transição da State `Stop` → `Idle` |
| Resultado visível | Stop anim freezeada no último frame, personagem não volta ao idle anim, "trava" |

### 16.4 Fix: refatorar o AnimBP State Machine de locomoção

Em `ABP_JSHeroCharacter` (ou equivalente compartilhado):

1. Abrir o State Machine principal de locomoção (provavelmente `Locomotion SM` ou `Main States`).
2. Listar todas as State→State transitions:
   - `Idle → Start`
   - `Start → Loop`
   - `Loop → Stop`
   - `Stop → Idle`
   - `Stop → Move` (re-engage quando solto e volta a pressionar)
3. Em **cada uma** dessas transições, abrir a **transition rule** (graph dentro do nó).
4. Substituir a condição atual (provavelmente baseada em `Speed`, `Time Remaining`, ou similar) por **leitura direta do flag correspondente do AnimInstance**:
   - `Idle → Start`: `Get bTransition_IdleToStart`
   - `Start → Loop`: `Get bTransition_StartToLoop`
   - `Loop → Stop`: `Get bTransition_LoopToStop`
   - `Stop → Idle`: `Get bTransition_StopToIdle`
   - `Stop → Move`: `Get bTransition_StopToMove`
5. Em "Transition Settings" de cada transição:
   - **Duration**: 0.10–0.15s (crossfade curto)
   - **Blend Logic**: Standard Blend
   - **Priority Order**: Stop→Idle > Stop→Move > demais (resolve conflito quando os dois flags ficam true)
6. **Garantir que NÃO há "Automatic Rule based on Sequence Player in State"** marcado nas transições — esse modo ignora `bTransition_*` e usa só o tempo restante da animação. Provavelmente é isso que está causando o stuck (anim chega ao end-fraction mas a regra "Automatic" não dispara porque o tempo total da Run_Stop_F_0_Seq é 1.50s e o end-fraction é 0.77s — a engine só faz auto-blend nos últimos N% do tempo TOTAL).

### 16.5 Bug colateral: Anim Stop nunca consome distância completa

```log
Stop=0/202(consumed 0)      ← player parou antes de qualquer movimento de Stop
Stop=64/202(consumed 5)
Stop=194/202(consumed 7)
Stop=1/202(consumed 17)     ← 17 cm de 202 cm planejados
```

O player **desacelera mais rápido do que a animação Stop esperava**. A `Run_Stop_F_0_Seq` quer percorrer 202 cm enquanto o CMC só precisa de ~50-100 cm para chegar a 0. Resultado: a animação é cortada no meio e o tracker `Stop=consumed` fica abaixo do esperado, deixando estado ambíguo.

**Fix de tuning (não é bug crítico, mas afeta a "qualidade" do stop):**

Em `UDFCharacterMovementComponent` (Class Defaults):
- `BrakingDecelerationWalking`: hoje provavelmente está em 2048 ou 4096. Reduzir para **1200-1600** quando NÃO está em landing recovery, para deixar o stop visual completar.
- Alternativa: aumentar `BrakingFriction` para suavizar.

Ou re-export a `Run_Stop_F_0_Seq` com distância de 100 cm (não 202 cm), batendo com a realidade física.

### 16.6 Bug colateral: `MaxWS=429` mas `RunSpeed=540` configurado

```log
Spd=429 ... MaxWS=429 ... RunCfg=429 SprintCfg=750
```

Mas no [`UDFCharacterMovementComponent.h:36`](../../Source/DungeonForged/Public/Characters/UDFCharacterMovementComponent.h) o default é `RunSpeed = 540.f`. O log mostra 429.

Hipóteses (em ordem de probabilidade):
1. **`DA_CombatTuning` ou `BP_JCHero_Character` override** — alguém setou `RunSpeed=429` no BP/DA para casar com `Walk_Loop_F_0_Seq` `avgSpd~429`.
2. **Stamina exhaustion reduzindo MaxWalkSpeed** — improvável, log mostra `Sprint=0` e não há indicador de exhaust.
3. **MaxWS sendo recalculado em outro lugar** — verificar `RefreshMaxWalkSpeed`.

O valor de 429 não é problema **se intencional** (provável: igualar ao avgSpd do loop anim). Mas o `WARN AuthoredLoopSpeed(429) != loop Distance avg(167)` indica que **mesmo o 429 está errado para a anim Walk** — o player rodando a 429 mas com Walk_Loop avgSpd=167 vai exibir foot sliding 2.5×.

**Fix:**
- Decidir: `RunSpeed = 429` (casa com Run_Loop_F_0_Seq que tem `avgSpd~429`) e `WalkSpeed/Gait Walk threshold` em 167 cm/s para casar Walk_Loop.
- Ou re-export anims com velocidade real desejada.
- Atualizar `GaitThr Walk>=50 Run>=350` para refletir a curva de avgSpd correta.

### 16.7 Bug colateral: enemy ABP gera log poluído

```log
== Locomotion [MAIN] ABP_DeepSeaLizard_C ==
Speed=0 Gait=Idle ...
Anim Start = None
Anim Loop = None
Anim Stop = None
RunCfg=0 SprintCfg=0
```

O `UDFLocomotionDebug` está iterando sobre **todos os AnimInstances UDF-derived** no mundo. Para o inimigo (`ABP_DeepSeaLizard`) que não usa o sistema de Start/Loop/Stop direcional, isso polui o log com 70% de linhas vazias.

**Fix simples:** no helper de log, skipar AnimInstances cujo `ActiveAnimSet.MovementBlendSpace == nullptr` AND `Start == nullptr` AND `Loop == nullptr` (sinal de que não usa o sistema direcional). Ou expor um bool `bEnableLocomotionDebugLog` na AnimInstance e default false em inimigos.

### 16.8 Sumário do bug do Stop

| Item | Valor |
|---|---|
| **Causa-raiz** | AnimBP State Machine não usa flags `bTransition_*ToIdle` como regra de transição |
| **Sintoma** | Stop animation freezeada no end-fraction (`Time=0.77/1.50`); player visualmente travado |
| **Mesma causa que** | Bug #1 (Jump SM stuck) — §15.2 |
| **Fix** | Refatorar todas as transition rules do SM de locomoção para usar `Get bTransition_*` (§16.4) |
| **Tempo estimado** | 30-60 min (apenas editor; toca ~5 transition rules no AnimBP) |
| **Não-bloqueia, mas é cleanup** | Stop distance mismatch (§16.5), MaxWS vs Anim avgSpd (§16.6), log do enemy (§16.7) |

### 16.9 Recomendação consolidada

**Faça um único pass no AnimBP** que conserta TANTO o Jump SM stuck QUANTO o Locomotion Stop stuck — é o mesmo padrão. Para cada State Machine do AnimBP:

1. Listar transições.
2. Para cada uma: verificar se a transition rule **lê o flag `bTransition_*` correspondente** do AnimInstance.
3. Se não lê: substituir a lógica antiga pela leitura do flag.
4. Verificar "Automatic Rule based on Sequence Player in State" — **desligar** se ativo (deixa o C++ controlar tempo de transição via flag, não a engine).
5. Setar Priority Order onde houver conflito (Stop→Idle deve preceder Stop→Move).

Tempo total: ~1h para Locomotion SM + Jump SM. Após isso, ambos os bugs P0 (§15.4 #1 e §16) resolvem-se simultaneamente.
