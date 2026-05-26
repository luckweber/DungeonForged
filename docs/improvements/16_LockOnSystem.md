# 16 — Lock-On System: Guia Completo de Implementação (C++ + Editor)

> **Versão:** 2026-05-22
> **Objetivo:** guia de implementação completo para finalizar, integrar e polir o sistema de lock-on já existente no projeto.
>
> **Estado de implementação:** Os componentes C++ centrais (`UDFLockOnComponent`, `UDFCameraComponent`, `UDFMeleeAimComponent`, `UDFLockOnWidget`) **já existem e estão funcionais**. O que falta é a cola entre eles: input bindings, tags GAS, integração com dodge e movimento em strafe, parâmetros de tuning no DataAsset e debug commands.

---

## Sumário

- [1. Diagnóstico — o que existe vs o que falta](#1-diagnóstico--o-que-existe-vs-o-que-falta)
- [2. Arquitetura do sistema](#2-arquitetura-do-sistema)
- [3. C++ — GAS Tags no LockOnComponent](#3-c--gas-tags-no-lockoncomponent)
- [4. C++ — Input handlers no ADFPlayerCharacter](#4-c--input-handlers-no-adfplayercharacter)
- [5. C++ — Bindings no ADFRunPlayerController](#5-c--bindings-no-adfrunplayercontroller)
- [6. C++ — Rotação de movimento em strafe (CMC)](#6-c--rotação-de-movimento-em-strafe-cmc)
- [7. C++ — Dodge integrado ao lock-on](#7-c--dodge-integrado-ao-lock-on)
- [8. C++ — Tuning params no UDFCombatTuningData](#8-c--tuning-params-no-udfcombattuningdata)
- [9. C++ — Debug commands e visual](#9-c--debug-commands-e-visual)
- [10. Setup Editor passo-a-passo](#10-setup-editor-passo-a-passo)
- [11. Integração Hotbar / HUD](#11-integração-hotbar--hud)
- [12. Checklist de validação](#12-checklist-de-validação)
- [13. Tabela de arquivos](#13-tabela-de-arquivos)

---

## 1. Diagnóstico — o que existe vs o que falta

### O que já existe ✅

| Componente | Arquivo | O que faz |
|---|---|---|
| `UDFLockOnComponent` | [`Camera/UDFLockOnComponent.h`](../../Source/DungeonForged/Public/Camera/UDFLockOnComponent.h) | Sphere overlap + cone + LOS; `TryLockOn()`, `CycleLockOnTarget(float)`, `ReleaseLockOn()`; auto-break no Tick quando target morre ou sai de range |
| `UDFCameraComponent` | [`Camera/UDFCameraComponent.h`](../../Source/DungeonForged/Public/Camera/UDFCameraComponent.h) | State machine `Default / Combat / LockOn`; `EnableLockOn(Target)`, `DisableLockOn()`, `UpdateLockOnRotation(dt)` com slerp; arm length interpolado por estado |
| `UDFMeleeAimComponent` | [`Combat/UDFMeleeAimComponent.h`](../../Source/DungeonForged/Public/Combat/UDFMeleeAimComponent.h) | Priority chain: LockOn → Manual → AI Blackboard → Soft cone; `AcquireAndCommitTarget()`, `SnapYawTowardTarget()` |
| `UDFLockOnWidget` | [`UI/UDFLockOnWidget.h`](../../Source/DungeonForged/Public/UI/UDFLockOnWidget.h) | Widget com `UpdateScreenPosition(PC, WorldPos)` — segue o alvo na tela |
| `ADFPlayerCharacter::LockOnComponent` | [`Characters/ADFPlayerCharacter.h:79`](../../Source/DungeonForged/Public/Characters/ADFPlayerCharacter.h) | Campo `TObjectPtr<UDFLockOnComponent> LockOnComponent` já criado |
| `State_Targeting` tag | [`GAS/DFGameplayTags.h:150`](../../Source/DungeonForged/Public/GAS/DFGameplayTags.h) | Definida como `"State.Targeting"` |
| `bIsLockedOn`, `bShouldStrafe` | [`Animation/UDFAnimInstance.h:111,143`](../../Source/DungeonForged/Public/Animation/UDFAnimInstance.h) | Lidas do ASC (`HasMatchingGameplayTag(State_Targeting)`) e ativam strafe blend no AnimGraph |

### O que falta ❌

| Gap | Onde corrigir |
|---|---|
| `State_Targeting` nunca é **setada** — `TryLockOn()`/`ReleaseLockOn()` não tocam a tag | [`UDFLockOnComponent.cpp`](../../Source/DungeonForged/Private/Camera/UDFLockOnComponent.cpp) §3 |
| Sem `IA_LockOn` / `IA_CycleLockOn` — lock-on não tem binding de input | Editor + `ADFRunPlayerController` §5 |
| Sem `HandleLockOnToggle()` / `HandleCycleLockOn()` em `ADFPlayerCharacter` | [`ADFPlayerCharacter.h/.cpp`](../../Source/DungeonForged/Public/Characters/ADFPlayerCharacter.h) §4 |
| `bUseControllerDesiredRotation` não muda em lock-on — character não strafa | `UDFCharacterMovementComponent` §6 |
| Dodge em lock-on usa câmera para resolver direção, não alvo | [`DFAbility_Dodge.cpp`](../../Source/DungeonForged/Private/GAS/Abilities/DFAbility_Dodge.cpp) §7 |
| Sem parâmetros de tuning para lock-on no DataAsset | [`UDFCombatTuningData.h`](../../Source/DungeonForged/Public/Data/UDFCombatTuningData.h) §8 |
| Sem `df.LockOnDebug` — sem visibilidade de candidatos, range, LOS | `UDFCheatManager.cpp` §9 |
| Auto-break por tempo parcialmente ausente (range check ok, mas sem "grace delay") | `UDFLockOnComponent.cpp` §3.3 |

---

## 2. Arquitetura do sistema

```
Jogador aperta Tab / Middle Mouse
         │
         ▼
ADFPlayerCharacter::HandleLockOnToggle()
         │
         ├─► LockOnComponent->TryLockOn()
         │       │
         │       ├─ OverlapSphere(LockOnRange)
         │       ├─ Filtra: IsActorValidEnemyType + AngleFromForward + LOS + HP > 0
         │       ├─ Ordena por distância → pega nearest
         │       ├─ CameraComponent->EnableLockOn(Target)
         │       ├─ EnsureLockOnWidget() → AddToViewport
         │       └─ ASC->AddLooseGameplayTag(State.Targeting)  ← FALTA
         │
         └─► (se já locked) LockOnComponent->ReleaseLockOn()
                 ├─ CameraComponent->DisableLockOn()
                 ├─ Widget->SetVisibility(Collapsed)
                 └─ ASC->RemoveLooseGameplayTag(State.Targeting)  ← FALTA

Tick (UDFLockOnComponent):
  se bIsLockedOn:
    se !IsTargetValid(CurrentTarget): ReleaseLockOn()
    senão: UpdateIndicator(dt)

Tick (UDFCameraComponent):
  se State==LockOn && LockOnTarget válido:
    UpdateLockOnRotation(dt)  ← slerp câmera para o alvo

NativeUpdateAnimation (UUDFAnimInstance):
  bIsLockedOn = ASC->HasMatchingGameplayTag(State.Targeting)  ← já lê a tag
  bShouldStrafe = !bIsDead && (bIsLockedOn || bIsInCombat)
  → Drive strafe blend no AnimGraph
```

---

## 3. C++ — GAS Tags no LockOnComponent

### 3.1 Problema: `State_Targeting` nunca é setada

**Arquivo:** [`Source/DungeonForged/Private/Camera/UDFLockOnComponent.cpp`](../../Source/DungeonForged/Private/Camera/UDFLockOnComponent.cpp)

`TryLockOn()` ativa o `LockOnWidget` e chama `Camera->EnableLockOn(Pick)`, mas nunca seta a tag `State.Targeting`. Resultado: `UUDFAnimInstance::bIsLockedOn` fica sempre `false`, e o strafe blend nunca ativa.

### 3.2 Patch — `TryLockOn()`

Adicionar include e a adição da tag após confirmar o lock:

```cpp
// UDFLockOnComponent.cpp — adicionar includes no topo
#include "AbilitySystemBlueprintLibrary.h"
#include "GAS/DFGameplayTags.h"

// Dentro de TryLockOn(), logo após "bIsLockedOn = true;":
if (FDFGameplayTags::State_Targeting.IsValid())
{
    if (UAbilitySystemComponent* const ASC =
            UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
    {
        ASC->AddLooseGameplayTag(FDFGameplayTags::State_Targeting);
    }
}
```

### 3.3 Patch — `ReleaseLockOn()`

Remover a tag ao soltar:

```cpp
// Dentro de ReleaseLockOn(), logo antes de "CurrentTarget = nullptr;":
if (FDFGameplayTags::State_Targeting.IsValid())
{
    if (UAbilitySystemComponent* const ASC =
            UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
    {
        ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_Targeting, 0);
    }
}
```

### 3.4 Auto-break com grace delay

Atualmente o `Tick` solta o lock imediatamente quando `IsTargetValid()` falha. Para evitar solturas involuntárias durante hit-stun (alvo teleporta brevemente por network), adicionar um grace delay:

**Header** (`UDFLockOnComponent.h`) — adicionar na seção `protected`:

```cpp
/** Grace period before releasing lock-on when target leaves range/LOS (seconds). */
UPROPERTY(EditDefaultsOnly, Category = "DF|LockOn", meta = (ClampMin = "0.0"))
float AutoBreakGraceDelay = 0.4f;

float TimeTargetInvalid = 0.f; // accumulated time target has been invalid
```

**TickComponent patch:**

```cpp
void UDFLockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!bIsLockedOn) return;

    if (!IsTargetValid(CurrentTarget.Get()))
    {
        TimeTargetInvalid += DeltaTime;
        if (TimeTargetInvalid >= AutoBreakGraceDelay)
        {
            ReleaseLockOn();
        }
        return;
    }
    TimeTargetInvalid = 0.f; // reset grace when target is valid again
    UpdateIndicator(DeltaTime);
}
```

**`ReleaseLockOn()`** — adicionar reset do acumulador no início:
```cpp
TimeTargetInvalid = 0.f;
```

---

## 4. C++ — Input handlers no ADFPlayerCharacter

### 4.1 Declaração — Header

**Arquivo:** [`Source/DungeonForged/Public/Characters/ADFPlayerCharacter.h`](../../Source/DungeonForged/Public/Characters/ADFPlayerCharacter.h)

Adicionar na seção de `protected` input handlers (junto com `HandleDodgePressed`, etc.):

```cpp
// ── Lock-On ──────────────────────────────────────────────────────────
/** Tab / Middle Mouse: toggle lock-on (try if off, release if on). */
void HandleLockOnToggle();

/** Q / Left stick flick: cycle to previous target. */
void HandleCycleLockOnLeft();

/** E / Right stick flick: cycle to next target. */
void HandleCycleLockOnRight();
```

### 4.2 Implementação — .cpp

**Arquivo:** [`Source/DungeonForged/Private/Characters/ADFPlayerCharacter.cpp`](../../Source/DungeonForged/Private/Characters/ADFPlayerCharacter.cpp)

```cpp
void ADFPlayerCharacter::HandleLockOnToggle()
{
    if (!LockOnComponent) return;

    if (LockOnComponent->IsLockedOn())
    {
        LockOnComponent->ReleaseLockOn();
    }
    else
    {
        const bool bLocked = LockOnComponent->TryLockOn();
#if !UE_BUILD_SHIPPING
        if (!bLocked)
        {
            UE_LOG(LogDungeonForged, Log, TEXT("[LockOn] TryLockOn FAIL — no valid target in range"));
        }
#endif
    }
}

void ADFPlayerCharacter::HandleCycleLockOnLeft()
{
    if (LockOnComponent && LockOnComponent->IsLockedOn())
    {
        LockOnComponent->CycleLockOnTarget(-1.f);
    }
}

void ADFPlayerCharacter::HandleCycleLockOnRight()
{
    if (LockOnComponent && LockOnComponent->IsLockedOn())
    {
        LockOnComponent->CycleLockOnTarget(1.f);
    }
}
```

---

## 5. C++ — Bindings no ADFRunPlayerController

### 5.1 Declarar as Input Actions

**Arquivo:** [`Source/DungeonForged/Public/GameModes/Run/ADFRunPlayerController.h`](../../Source/DungeonForged/Public/GameModes/Run/ADFRunPlayerController.h)

Adicionar junto com os outros `UInputAction*`:

```cpp
/** Tab / Middle Mouse: toggle lock-on. */
UPROPERTY(EditDefaultsOnly, Category = "DF|Input|LockOn")
TObjectPtr<UInputAction> IA_LockOn;

/** Q / left stick flick: cycle to previous target. */
UPROPERTY(EditDefaultsOnly, Category = "DF|Input|LockOn")
TObjectPtr<UInputAction> IA_CycleLockOnLeft;

/** E / right stick flick: cycle to next target. */
UPROPERTY(EditDefaultsOnly, Category = "DF|Input|LockOn")
TObjectPtr<UInputAction> IA_CycleLockOnRight;
```

### 5.2 Bindar no SetupInputComponent

**Arquivo:** [`Source/DungeonForged/Private/GameModes/Run/ADFRunPlayerController.cpp`](../../Source/DungeonForged/Private/GameModes/Run/ADFRunPlayerController.cpp)

Na função `SetupInputComponent` (onde os outros bindings estão), adicionar:

```cpp
// Lock-On
if (IA_LockOn && PC)
{
    EnhancedInput->BindAction(IA_LockOn, ETriggerEvent::Started,
        PC, &ADFPlayerCharacter::HandleLockOnToggle);
}
if (IA_CycleLockOnLeft && PC)
{
    EnhancedInput->BindAction(IA_CycleLockOnLeft, ETriggerEvent::Started,
        PC, &ADFPlayerCharacter::HandleCycleLockOnLeft);
}
if (IA_CycleLockOnRight && PC)
{
    EnhancedInput->BindAction(IA_CycleLockOnRight, ETriggerEvent::Started,
        PC, &ADFPlayerCharacter::HandleCycleLockOnRight);
}
```

---

## 6. C++ — Rotação de movimento em strafe (CMC)

### 6.1 Problema

Quando em lock-on, o personagem deve **strafe** (mover lateralmente sem girar) em vez de girar para a direção do movimento. Isso é controlado pelo `bOrientRotationToMovement` do `UCharacterMovementComponent`.

- `bOrientRotationToMovement = true` → personagem rotaciona para o vetor de movimento (padrão fora de combate)
- `bOrientRotationToMovement = false` + `bUseControllerDesiredRotation = true` → personagem rotaciona para a câmera (strafe / lock-on)

### 6.2 Abordagem — callback no LockOnComponent

O lugar mais limpo é notificar o `UDFCharacterMovementComponent` quando o lock-on muda.

**Opção A (recomendada): callback delegate em `UDFLockOnComponent`:**

Em [`UDFLockOnComponent.h`](../../Source/DungeonForged/Public/Camera/UDFLockOnComponent.h):

```cpp
/** Fired when lock-on is acquired or released. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLockOnChanged, bool /* bIsLocked */);
FOnLockOnChanged OnLockOnChanged;
```

Em `TryLockOn()`, depois de `bIsLockedOn = true`:
```cpp
OnLockOnChanged.Broadcast(true);
```

Em `ReleaseLockOn()`, depois de `bIsLockedOn = false`:
```cpp
OnLockOnChanged.Broadcast(false);
```

**Em `ADFPlayerCharacter::BeginPlay()` (ou `PossessedBy`):**

```cpp
if (LockOnComponent)
{
    LockOnComponent->OnLockOnChanged.AddWeakLambda(this,
        [this](const bool bIsLocked)
        {
            if (UDFCharacterMovementComponent* const CMC =
                    Cast<UDFCharacterMovementComponent>(GetCharacterMovement()))
            {
                CMC->SetStrafeMode(bIsLocked);
            }
        });
}
```

**Em [`UDFCharacterMovementComponent.h`](../../Source/DungeonForged/Public/Characters/UDFCharacterMovementComponent.h) — adicionar:**

```cpp
/** Switch between strafe (lock-on / combat) and orient-to-movement (exploration) rotation modes. */
UFUNCTION(BlueprintCallable, Category = "DF|Movement|Strafe")
void SetStrafeMode(bool bStrafe);

UPROPERTY(BlueprintReadOnly, Category = "DF|Movement|Strafe")
bool bIsStrafing = false;
```

**Em `UDFCharacterMovementComponent.cpp`:**

```cpp
void UDFCharacterMovementComponent::SetStrafeMode(const bool bStrafe)
{
    bIsStrafing = bStrafe;
    bOrientRotationToMovement   = !bStrafe;
    bUseControllerDesiredRotation = bStrafe;
    // Tighten friction in strafe for snappier stops.
    BrakingFrictionFactor = bStrafe ? 2.0f : 1.0f;
}
```

> **Nota:** Se o personagem já usa `bUseControllerDesiredRotation` fora de combate (e.g. câmera relativa), ajuste só `bOrientRotationToMovement`. Verifique o CDO do `BP_JCHero_Character`.

---

## 7. C++ — Dodge integrado ao lock-on

### 7.1 Problema

Quando em lock-on e o jogador aperta `Forward + Dodge`, o dodge deve ser **em relação ao alvo de lock-on**, não em relação à câmera livre. Sem isso, o dodge pode empurrar o jogador para longe do inimigo mesmo que o input diga "para frente".

O comportamento desejado (padrão Souls):
- **Lock-on OFF**: dodge na direção da câmera / input global
- **Lock-on ON**: dodge em relação ao **alvo** (Forward = em direção ao alvo, Backward = away)

### 7.2 Patch — `UDFAbility_Dodge::ResolveDodgeDirection()`

**Arquivo:** [`Source/DungeonForged/Private/GAS/Abilities/DFAbility_Dodge.cpp`](../../Source/DungeonForged/Private/GAS/Abilities/DFAbility_Dodge.cpp)

A resolução atual usa o local-space do actor (actor forward = câmera-aligned quando `bUseControllerDesiredRotation = true`). Em lock-on o actor já estará face-to-target (via `bUseControllerDesiredRotation`), então a direção **já é correta sem mudança**. O único ajuste necessário é:

**Garantir que em lock-on `bRotateToDodgeDirection = false`** — não reorientar o actor antes da montage, porque ele já está orientado para o alvo. Se rotacionar, vai "destravar" a câmera durante o dodge.

```cpp
// UDFAbility_Dodge::ActivateAbility — substituir o bloco de rotação:
const bool bIsLockedOn = [this]() -> bool {
    const FGameplayAbilityActorInfo* const I = GetCurrentActorInfo();
    if (!I || !I->AbilitySystemComponent.IsValid()) return false;
    return I->AbilitySystemComponent->HasMatchingGameplayTag(FDFGameplayTags::State_Targeting);
}();

if (bRotateToDodgeDirection && !bIsLockedOn && !DodgeDirWorld.IsNearlyZero())
{
    FRotator FaceRot = DodgeDirWorld.GetSafeNormal().Rotation();
    FaceRot.Pitch = 0.f;
    FaceRot.Roll  = 0.f;
    Char->SetActorRotation(FaceRot);
}
```

### 7.3 Dodge lateral em lock-on — clareza de direção

Quando locked-on e o jogador pressiona `A` (strafe left) + dodge, o esperado é um **roll lateral para a esquerda em relação ao alvo**. Como o personagem já strafa (actor yaw = face-to-target), o input local de `A` já corresponde a "left relativo ao alvo". Então não é necessário nenhum ajuste extra — o octant resolver já vai mapear `Y < 0` para `Left` / `ForwardLeft`.

### 7.4 Exposição no tuning (opcional)

Em `UDFCombatTuningData`, adicionar (ver §8):
```cpp
/** When locked-on, suppress bRotateToDodgeDirection to keep facing the target. */
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge")
bool bDodgeKeepFacingTargetOnLockOn = true;
```

---

## 8. C++ — Tuning params no UDFCombatTuningData

**Arquivo:** [`Source/DungeonForged/Public/Data/UDFCombatTuningData.h`](../../Source/DungeonForged/Public/Data/UDFCombatTuningData.h)

Adicionar categoria `"LockOn"` com os parâmetros que hoje são magic-numbers no `UDFLockOnComponent`:

```cpp
// ── Lock-On ──────────────────────────────────────────────────────────
/** Sphere radius for lock-on candidate search (cm). Mirrors UDFLockOnComponent::LockOnRange. */
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LockOn", meta = (ClampMin = "0.0"))
float LockOnRange = 1500.f;

/** Full cone angle in front of player for initial lock (degrees). */
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LockOn", meta = (ClampMin = "0.0", ClampMax = "180.0"))
float LockOnConeAngle = 60.f;

/** Time in seconds a target can be out of LOS before lock auto-breaks. */
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LockOn", meta = (ClampMin = "0.0"))
float LockOnAutoBreakGraceDelay = 0.4f;

/** Camera rotation interp speed when tracking locked target (higher = tighter follow). */
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LockOn", meta = (ClampMin = "1.0"))
float LockOnCameraInterpSpeed = 12.f;

/** % of LockOnRange below which the lock-on indicator pulses (warning). */
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LockOn", meta = (ClampMin = "0.0", ClampMax = "1.0"))
float LockOnWarningRangePercent = 0.15f;

/** Suppress actor rotation on dodge when locked-on (keeps facing target during roll). */
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LockOn")
bool bDodgeKeepFacingTargetOnLockOn = true;
```

> **Uso:** `UDFLockOnComponent` pode ler esses valores em `BeginPlay()` via `UDFAssetManager::GetCombatTuningDataSafe()` e sobrescrever os defaults locais. Isso permite afinar sem recompilar.

**Snippet para `UDFLockOnComponent::BeginPlay()`:**

```cpp
if (const UDFCombatTuningData* const Tuning = UDFAssetManager::GetCombatTuningDataSafe())
{
    LockOnRange         = Tuning->LockOnRange;
    LockOnAngle         = Tuning->LockOnConeAngle;
    AutoBreakGraceDelay = Tuning->LockOnAutoBreakGraceDelay;
}
```

---

## 9. C++ — Debug commands e visual

### 9.1 CVar `df.DebugLockOn`

**Novo arquivo:** `Source/DungeonForged/Private/Combat/DFLockOnDebug.cpp`  
**(ou adicionar no bloco `#if !UE_BUILD_SHIPPING` do `UDFCheatManager.cpp`)**

```cpp
// Dentro do namespace anônimo de UDFCheatManager.cpp

#if !UE_BUILD_SHIPPING

static TAutoConsoleVariable<int32> CVarDF_DebugLockOn(
    TEXT("df.DebugLockOn"),
    0,
    TEXT("DungeonForged lock-on debug.\n")
    TEXT(" 0: Off\n")
    TEXT(" 1: Log [LockOn|...] no Output (ativação, candidatos, break)\n")
    TEXT(" 2: Log + Sphere overlay no mundo (range, cone, candidatos)"),
    ECVF_Cheat);

static void Cmd_df_lockondebug(TArray<FString> const& Args)
{
    IConsoleVariable* const Cv = IConsoleManager::Get().FindConsoleVariable(TEXT("df.DebugLockOn"));
    if (!Cv)
    {
        DF_LOG(Warning, "df.LockOnDebug: df.DebugLockOn CVar não encontrado");
        return;
    }

    if (Args.Num() > 0)
    {
        const FString A = Args[0].ToLower();
        if (A == TEXT("dump"))
        {
            Cv->Set(1, ECVF_SetByConsole);
            UWorld* const W = GetCheatWorld();
            if (ADFPlayerCharacter* const P = GetLocalDFPawn(W))
            {
                if (UDFLockOnComponent* const LOC = P->LockOnComponent)
                {
                    const bool bLocked = LOC->IsLockedOn();
                    const AActor* const T = LOC->GetCurrentTarget();
                    DF_LOG(Log, "df.LockOnDebug dump: locked=%d target=%s",
                        bLocked ? 1 : 0, *GetNameSafe(T));
                }
            }
            return;
        }
        if (A == TEXT("0") || A == TEXT("off"))       { Cv->Set(0, ECVF_SetByConsole); }
        else if (A == TEXT("1") || A == TEXT("log"))  { Cv->Set(1, ECVF_SetByConsole); }
        else if (A == TEXT("2") || A == TEXT("draw") || A == TEXT("on"))
                                                      { Cv->Set(2, ECVF_SetByConsole); }
        else
        {
            DF_LOG(Warning, "df.LockOnDebug: use [0|1|2|dump|log|draw|on|off]");
            return;
        }
    }
    else
    {
        const int32 Next = Cv->GetInt() >= 2 ? 0 : (Cv->GetInt() + 1);
        Cv->Set(Next, ECVF_SetByConsole);
    }

    DF_LOG(Log, "df.LockOnDebug: df.DebugLockOn=%d (0=off 1=log 2=log+draw)", Cv->GetInt());
    DF_LOG(Log, "  dump → estado atual do LockOnComponent");
    DF_LOG(Log, "  2 → sphere de range + cone + candidatos na tela");
}

static FAutoConsoleCommand GCmdLockOnDebug(
    TEXT("df.LockOnDebug"),
    TEXT("Lock-on debug: toggle df.DebugLockOn (0/1/2). Args: dump | 0 | 1 | 2 | log | draw | on | off."),
    FConsoleCommandWithArgsDelegate::CreateStatic(&Cmd_df_lockondebug));

#endif
```

### 9.2 Visual debug no `UDFLockOnComponent::TickComponent`

Adicionar ao final do tick (depois de `UpdateIndicator`):

```cpp
#if !UE_BUILD_SHIPPING
if (IConsoleVariable* const Cv = IConsoleManager::Get().FindConsoleVariable(TEXT("df.DebugLockOn")))
{
    if (Cv->GetInt() >= 2 && GetWorld() && GetOwner())
    {
        const FVector Origin = GetOwner()->GetActorLocation();
        // Range sphere
        DrawDebugSphere(GetWorld(), Origin, LockOnRange, 24, bIsLockedOn ? FColor::Green : FColor::Yellow, false, -1.f, 0, 1.f);
        // Forward vector (cone center)
        DrawDebugDirectionalArrow(GetWorld(), Origin,
            Origin + GetOwner()->GetActorForwardVector() * LockOnRange * FMath::Cos(FMath::DegreesToRadians(LockOnAngle * 0.5f)),
            20.f, FColor::Orange, false, -1.f, 0, 2.f);
        // Current target line
        if (bIsLockedOn && CurrentTarget.IsValid())
        {
            DrawDebugLine(GetWorld(), Origin, CurrentTarget->GetActorLocation(), FColor::Red, false, -1.f, 0, 3.f);
        }
    }
}
#endif
```

### 9.3 Log ao adquirir lock

Em `TryLockOn()` após setar `bIsLockedOn = true`:

```cpp
#if !UE_BUILD_SHIPPING
UE_LOG(LogDungeonForged, Log, TEXT("[LockOn] Acquired target=%s dist=%.0f"),
    *GetNameSafe(Pick),
    FVector::Dist(GetOwner()->GetActorLocation(), Pick->GetActorLocation()));
#endif
```

Em `ReleaseLockOn()`:

```cpp
#if !UE_BUILD_SHIPPING
UE_LOG(LogDungeonForged, Log, TEXT("[LockOn] Released (was target=%s)"), *GetNameSafe(CurrentTarget.Get()));
#endif
```

### 9.4 Comandos de console disponíveis após implementação

| Comando | Efeito |
|---|---|
| `df.LockOnDebug` | Toggle entre 0 / 1 / 2 |
| `df.LockOnDebug 0` / `off` | Desliga debug |
| `df.LockOnDebug 1` / `log` | Só log (Output Log, filtro `[LockOn]`) |
| `df.LockOnDebug 2` / `draw` / `on` | Log + sphere + cone + linha ao alvo |
| `df.LockOnDebug dump` | Imprime estado atual (locked?, target, distância) |
| `showdebug AbilitySystem` | Confirmar `State.Targeting` ativa/inativa |
| `df.DodgeDebug 2` | Ver direção do dodge relativa ao alvo |

---

## 10. Setup Editor passo-a-passo

### Passo 1: Compilar C++

Após aplicar os patches das seções §3–§9, compilar via **Live Coding** ou rebuild completo.

### Passo 2: Criar Input Actions

No **Content Browser → Content/DungeonForged/Input/Actions/**:

1. **`IA_LockOn`**
   - Blueprint Class: `Input Action`
   - Value Type: `Digital (bool)`
   - Triggers: `Pressed`

2. **`IA_CycleLockOnLeft`**
   - Value Type: `Digital (bool)`
   - Triggers: `Pressed`

3. **`IA_CycleLockOnRight`**
   - Value Type: `Digital (bool)`
   - Triggers: `Pressed`

### Passo 3: Configurar IMC_DFDefault

Abrir `Content/DungeonForged/Input/IMC_DFDefault.uasset`:

| Input Action | Tecla (PC) | Gamepad |
|---|---|---|
| `IA_LockOn` | `Tab` ou `Middle Mouse Button` | `Right Thumbstick Press` |
| `IA_CycleLockOnLeft` | `Q` | `Stick flick Left (negative X)` |
| `IA_CycleLockOnRight` | `E` | `Stick flick Right (positive X)` |

> **Nota:** Para stick flick no gamepad, use um trigger `Pressed` com Threshold 0.5 para evitar disparo acidental durante movimento.

### Passo 4: Configurar BP_DFRunPlayerController

Abrir o Blueprint do player controller. No painel **Class Defaults**, assignar:
- `IA_LockOn` → o asset criado no Passo 2
- `IA_CycleLockOnLeft` → idem
- `IA_CycleLockOnRight` → idem

### Passo 5: Configurar GA_Knight_Dodge (já existe)

Abrir `GA_Knight_Dodge`. Verificar `bRotateToDodgeDirection`:
- Se `UDFCombatTuningData::bDodgeKeepFacingTargetOnLockOn = true`, o patch do §7.2 já suprime a rotação em lock-on automaticamente.
- Não é necessário mudar o asset.

### Passo 6: Criar WBP_LockOnIndicator

1. **Content Browser → UI** → Blueprint Widget → pai `UDFLockOnWidget`
2. Nome: `WBP_LockOnIndicator`
3. No Designer: adicionar um `Image` e renomear para `IndicatorImage`
   - Textura: anel fino (círculo). Pode ser uma textura `T_LockOn_Ring` feita no material com bordas alfa arredondadas, ou importar PNG
   - Tamanho: 64×64 px
4. **Cor padrão:** branco-cinza (0.8, 0.8, 0.8, 1.0)

**Animação de pulsação (opcional, para alvo no limite do range):**
- No AnimGraph do widget, criar uma UMG Animation `Anim_Pulse`
  - Track: `IndicatorImage → Color and Opacity → Lerp entre branco e amarelo em 0.4s loop`
- Em C++ (ou BP), chamar `PlayAnimation(Anim_Pulse)` quando target está nos últimos `LockOnWarningRangePercent` do range

### Passo 7: Configurar LockOnComponent no Blueprint do personagem

Em `BP_JCHero_Character` (ou onde quer que `UDFLockOnComponent` esteja):

| Propriedade | Valor | Onde |
|---|---|---|
| `Lock On Range` | **1500** (ou ler do DataAsset) | Class Defaults → DF | LockOn |
| `Lock On Angle` | **60** | Class Defaults → DF | LockOn |
| `Lock Target Class` | `ADFEnemyBase` | Class Defaults → DF | LockOn |
| `Lock On Widget Class` | `WBP_LockOnIndicator` | Class Defaults → DF | LockOn | UI |
| `Auto Break Grace Delay` | **0.4** | Class Defaults |

### Passo 8: Configurar UDFCombatTuningData

Abrir o DataAsset de tuning (geralmente em `Content/DungeonForged/Data/DA_CombatTuning.uasset`):

| Param | Valor |
|---|---|
| Lock On Range | 1500 |
| Lock On Cone Angle | 60 |
| Lock On Auto Break Grace Delay | 0.4 |
| Lock On Camera Interp Speed | 12.0 |
| Lock On Warning Range Percent | 0.15 |
| Dodge Keep Facing Target On Lock On | ✓ |

### Passo 9: Verificar AnimBP

Em `ABP_JCHero`, confirmar que:
- A variável `bShouldStrafe` (de `UUDFAnimInstance`) está sendo usada para blend entre locomotion normal e strafe blendspace
- A variável `bIsLockedOn` está sendo exposta (já existe no h como `BlueprintReadOnly`)

Se o AnimBP não tiver strafe blendspace:
1. Criar `BS_Strafe` (Blend Space 2D, eixo X = Speed, eixo Y = Direction -180/180)
2. Em `StateMachine → Locomotion`, adicionar condição: se `bShouldStrafe` → usar `BS_Strafe`; senão → usar `BS_Walk/Run`

### Passo 10: Testar

PIE, abrir console (`~`):
```
df.LockOnDebug 2
```

Checklist de teste:
1. Inimigo na frente → Tab → lock ativa, câmera gira → ✓
2. Sem inimigo → Tab → log "no valid target" → ✓
3. Locked → Q / E → cicla entre inimigos → ✓
4. Inimigo morre → lock solta automaticamente (com 0.4s grace) → ✓
5. `showdebug AbilitySystem` → tag `State.Targeting` aparece enquanto locked → ✓
6. Em lock-on, mover + dodge → personagem rola lateral/forward/backward relativo ao alvo → ✓
7. Sair do range → lock solta após 0.4s → ✓

---

## 11. Integração Hotbar / HUD

O lock-on **não deve aparecer como slot de hotbar** (é mecânica de movimento/câmera, não ability). A integração com o HUD é:

### 11.1 Cooldown visual — não aplicável

Lock-on não tem cooldown — é toggle. Sem necessidade de cooldown display.

### 11.2 HUD Combat Mode fade

O sistema de HUD adaptativo (doc `07_UI_UX.md`) usa `State_InCombat` para fade in/out. Lock-on usa `State_Targeting`. Se você quiser que o HUD também apareça durante lock-on (sem combate ativo):

Em `WBP_HUD` (Blueprint), na lógica de fade:
```
(HasTag(State.InCombat) OR HasTag(State.Targeting)) → Fade In HUD
```

### 11.3 Stamina Bar

A `UDFAbilityHotbarWidget` já tem `StaminaBar`. Nenhuma mudança necessária — dodge com lock-on já drena stamina normalmente.

### 11.4 Indicador de alvo ativo no HUD (opcional)

Um pequeno ícone na parte inferior-central da tela mostrando o HP do alvo locked-on:
- Blueprint widget `WBP_LockOnTargetInfo` com `ProgressBar` para HP e `TextBlock` para nome
- Em `Tick` no widget: lê `PlayerCharacter->LockOnComponent->GetCurrentTarget()` → get HP attribute
- Aparece com `FadeIn` quando lock ativa, some com `FadeOut` quando solta

---

## 12. Checklist de validação

### Funcionalidade básica
- [ ] Tab / Middle Mouse ativa lock-on no inimigo mais próximo dentro de 1500cm
- [ ] Tag `State.Targeting` está **ativa** no ASC enquanto locked (verificar com `showdebug AbilitySystem`)
- [ ] `bIsLockedOn` e `bShouldStrafe` no AnimInstance são `true` quando locked
- [ ] Câmera segue o alvo suavemente (sem snap), interp speed ≈ 12
- [ ] Tab novamente solta o lock
- [ ] Inimigo morre → lock solta automaticamente (com 0.4s grace)
- [ ] Inimigo sai do range → lock solta após 0.4s

### Ciclo de alvo
- [ ] Q cicla para o alvo anterior (entre inimigos válidos em view)
- [ ] E cicla para o próximo alvo
- [ ] Ciclo funciona mesmo com apenas 1 inimigo (sem crash, sem loop infinito)

### Movimento em strafe
- [ ] Em lock-on, personagem enfrenta o alvo enquanto move lateralmente
- [ ] `bOrientRotationToMovement = false` quando locked
- [ ] Ao soltar lock, `bOrientRotationToMovement = true` volta

### Dodge integrado
- [ ] Forward + Dodge em lock-on → rola em direção ao alvo
- [ ] Backward + Dodge em lock-on → rola para longe do alvo
- [ ] Lateral + Dodge em lock-on → rola para o lado
- [ ] Actor **não rotaciona** para a direção do dodge quando em lock-on (fica face-to-target)

### Debug
- [ ] `df.LockOnDebug 2` mostra sphere de range + cone + linha ao alvo
- [ ] `df.LockOnDebug dump` imprime estado no Output Log
- [ ] `df.LockOnDebug off` desliga sem crash

### AAA polish (opcional pós-MVP)
- [ ] Widget `WBP_LockOnIndicator` aparece sobre o alvo
- [ ] Widget pulsa em amarelo quando alvo está no último 15% do range
- [ ] `WBP_LockOnTargetInfo` exibe HP do alvo no HUD

---

## 13. Tabela de arquivos

| Arquivo | Status | O que muda |
|---|---|---|
| [`UDFLockOnComponent.h`](../../Source/DungeonForged/Public/Camera/UDFLockOnComponent.h) | 🔧 Modificar | `OnLockOnChanged` delegate, `AutoBreakGraceDelay`, `TimeTargetInvalid` |
| [`UDFLockOnComponent.cpp`](../../Source/DungeonForged/Private/Camera/UDFLockOnComponent.cpp) | 🔧 Modificar | `TryLockOn()`/`ReleaseLockOn()` set/remove `State.Targeting`; Tick com grace delay; log + debug draw |
| [`ADFPlayerCharacter.h`](../../Source/DungeonForged/Public/Characters/ADFPlayerCharacter.h) | 🔧 Modificar | `HandleLockOnToggle()`, `HandleCycleLockOnLeft()`, `HandleCycleLockOnRight()` |
| [`ADFPlayerCharacter.cpp`](../../Source/DungeonForged/Private/Characters/ADFPlayerCharacter.cpp) | 🔧 Modificar | Implementação dos handlers; bind `OnLockOnChanged` → `SetStrafeMode` |
| [`ADFRunPlayerController.h`](../../Source/DungeonForged/Public/GameModes/Run/ADFRunPlayerController.h) | 🔧 Modificar | `IA_LockOn`, `IA_CycleLockOnLeft`, `IA_CycleLockOnRight` fields |
| [`ADFRunPlayerController.cpp`](../../Source/DungeonForged/Private/GameModes/Run/ADFRunPlayerController.cpp) | 🔧 Modificar | Bindings Enhanced Input → handlers |
| [`UDFCharacterMovementComponent.h`](../../Source/DungeonForged/Public/Characters/UDFCharacterMovementComponent.h) | 🔧 Modificar | `SetStrafeMode(bool)`, `bIsStrafing` |
| [`UDFCharacterMovementComponent.cpp`](../../Source/DungeonForged/Private/Characters/UDFCharacterMovementComponent.cpp) | 🔧 Modificar | `SetStrafeMode()` implementation |
| [`DFAbility_Dodge.cpp`](../../Source/DungeonForged/Private/GAS/Abilities/DFAbility_Dodge.cpp) | 🔧 Modificar | Suprimir `bRotateToDodgeDirection` quando `State.Targeting` ativa |
| [`UDFCombatTuningData.h`](../../Source/DungeonForged/Public/Data/UDFCombatTuningData.h) | 🔧 Modificar | Adicionar categoria `"LockOn"` com 6 params |
| [`UDFCheatManager.cpp`](../../Source/DungeonForged/Private/Debug/UDFCheatManager.cpp) | 🔧 Modificar | `df.LockOnDebug` command + `Cmd_df_lockondebug` |
| `Content/.../IA_LockOn.uasset` | ✅ Criar | Input Action Digital Pressed |
| `Content/.../IA_CycleLockOnLeft.uasset` | ✅ Criar | Input Action Digital Pressed |
| `Content/.../IA_CycleLockOnRight.uasset` | ✅ Criar | Input Action Digital Pressed |
| `Content/.../IMC_DFDefault.uasset` | 🔧 Modificar | Bind 3 novas IAs |
| `Content/.../WBP_LockOnIndicator.uasset` | ✅ Criar | Widget com `IndicatorImage`, pai `UDFLockOnWidget` |
| `Content/.../DA_CombatTuning.uasset` | 🔧 Modificar | Setar valores da categoria LockOn |

---

## Próximos passos (pós-MVP)

1. **Pulsação no widget** — `Anim_Pulse` quando alvo a <15% do range
2. **Target Health Bar** — `WBP_LockOnTargetInfo` no HUD com HP do alvo
3. **Lock-on durante dodge** — suprimir ciclo de alvo enquanto dodge ativo (evitar troca acidental)
4. **Soft-lock em combate** — `bIsInCombat && !bIsLockedOn` → câmera inclina ligeiramente em direção ao inimigo mais próximo sem travar
5. **Lock-on com ranged abilities** — abilities de distância têm um `Target Actor` que é passado automaticamente do `LockOnComponent->GetCurrentTarget()`

---

## Arquivos referenciados

- [`UDFLockOnComponent.h`](../../Source/DungeonForged/Public/Camera/UDFLockOnComponent.h)
- [`UDFLockOnComponent.cpp`](../../Source/DungeonForged/Private/Camera/UDFLockOnComponent.cpp)
- [`UDFCameraComponent.h`](../../Source/DungeonForged/Public/Camera/UDFCameraComponent.h)
- [`UDFMeleeAimComponent.h`](../../Source/DungeonForged/Public/Combat/UDFMeleeAimComponent.h)
- [`UDFLockOnWidget.h`](../../Source/DungeonForged/Public/UI/UDFLockOnWidget.h)
- [`ADFPlayerCharacter.h`](../../Source/DungeonForged/Public/Characters/ADFPlayerCharacter.h)
- [`ADFRunPlayerController.h`](../../Source/DungeonForged/Public/GameModes/Run/ADFRunPlayerController.h)
- [`UDFCharacterMovementComponent.h`](../../Source/DungeonForged/Public/Characters/UDFCharacterMovementComponent.h)
- [`DFAbility_Dodge.h`](../../Source/DungeonForged/Public/GAS/Abilities/DFAbility_Dodge.h)
- [`UDFCombatTuningData.h`](../../Source/DungeonForged/Public/Data/UDFCombatTuningData.h)
- [`DFGameplayTags.h`](../../Source/DungeonForged/Public/GAS/DFGameplayTags.h)
- [`docs/improvements/01_GameFeel.md`](01_GameFeel.md) — §7 Strafe/Lock-On Feel
- [`docs/improvements/15_DodgeAbility_4Way.md`](15_DodgeAbility_4Way.md) — Dodge 8-way
- [`docs/improvements/07_UI_UX.md`](07_UI_UX.md) — HUD adaptativo
