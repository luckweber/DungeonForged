# 15 — Dodge 8-Way Armado/Desarmado: Guia Completo (C++ + Editor)

> **Objetivo:** estender o `UDFAbility_Dodge` existente para suportar **8 direções** (octant snapping — Forward, ForwardRight, Right, BackwardRight, Backward, BackwardLeft, Left, ForwardLeft) com variantes armada e desarmada, integradas a hotbar, estamina, GAS, combo cancel windows, AnimInstance e damage flow.
>
> **Estado de implementação C++:** ✅ **CONCLUÍDO** — O C++ foi além do plano original (4-way) e entregou **8-way completo**. O que resta é o setup de assets no Editor (montages, GA_Knight_Dodge, IMC) descrito na §12.
>
> **Estado atual da base:** [`UDFAbility_Dodge.h`](../../Source/DungeonForged/Public/GAS/Abilities/DFAbility_Dodge.h) já existe — toca **uma** montage. Este guia transforma em sistema 8-way completo.
>
> **Pré-requisitos:**
> - GAS configurado (já está — `UDFAbilitySystemComponent`, `UDFAttributeSet`, tags em [`FDFGameplayTags`](../../Source/DungeonForged/Public/GAS/DFGameplayTags.h))
> - `UDFCharacterMovementComponent` com `PerformDodge`, `GetDodgeDirection`, `DodgeIFrame` (já está — [`UDFCharacterMovementComponent.h:47-91`](../../Source/DungeonForged/Public/Characters/UDFCharacterMovementComponent.h:47))
> - Hotbar configurado ([`UDFAbilityHotbarWidget`](../../Source/DungeonForged/Public/UI/UDFAbilityHotbarWidget.h))
>
> **Estilo do guia:** cada seção tem ① Estado atual  ② Mudança proposta (C++)  ③ Setup no Editor  ④ Validação.

---

## Sumário

- [1. Diagnóstico do sistema atual](#1-diagnóstico-do-sistema-atual)
- [2. Arquitetura proposta](#2-arquitetura-proposta)
- [3. C++ — Extensão da `UDFAbility_Dodge`](#3-c--extensão-da-udfability_dodge)
- [4. C++ — Estrutura `FDFDodgeAnimSet`](#4-c--estrutura-fdfdodgeanimset)
- [5. C++ — Resolução armado vs desarmado](#5-c--resolução-armado-vs-desarmado)
- [6. Integração GAS — Tags, Cost, Cooldown](#6-integração-gas--tags-cost-cooldown)
- [7. Integração Combo — Cancel windows](#7-integração-combo--cancel-windows)
- [8. Integração Estamina](#8-integração-estamina)
- [9. Integração Hotbar](#9-integração-hotbar)
- [10. Integração AnimInstance (BP)](#10-integração-animinstance-bp)
- [11. Integração Damage / I-Frames](#11-integração-damage--i-frames)
- [12. Setup Editor passo-a-passo](#12-setup-editor-passo-a-passo)
- [13. Console commands & validação](#13-console-commands--validação)
- [14. Tabela de pacotes que você toca](#14-tabela-de-pacotes-que-você-toca)

---

## 1. Diagnóstico do sistema atual

### O que já existe ✓

[`UDFAbility_Dodge.h`](../../Source/DungeonForged/Public/GAS/Abilities/DFAbility_Dodge.h):
```cpp
class DUNGEONFORGED_API UDFAbility_Dodge : public UDFGameplayAbility
{
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Dodge")
    TObjectPtr<UAnimMontage> DodgeMontage;  // ← ÚNICO montage
    // ...
};
```

[`UDFAbility_Dodge.cpp`](../../Source/DungeonForged/Private/GAS/Abilities/DFAbility_Dodge.cpp) — toca uma única montage e dispara `CMC->PerformDodge(Dir)`.

[`UDFCharacterMovementComponent.h:47-91`](../../Source/DungeonForged/Public/Characters/UDFCharacterMovementComponent.h:47):
- `DodgeCooldown = 0.7s`
- `DodgeDistance = 600cm`
- `DodgeDuration = 0.35s`
- `IFrameDuration = 0.35s`
- `GetDodgeDirection()` retorna `GetLastInputVector()` ou `-actor forward` (backward fallback)
- `PerformDodge(Dir)` adiciona `State.Dodging` + `State.Invulnerable` via `AddLooseGameplayTag`

Tags GAS já registradas em [`DFGameplayTags.cpp`](../../Source/DungeonForged/Private/GAS/DFGameplayTags.cpp):
- `Ability.Movement.Dodge` (linha 305)
- `State.Dodging` (linha 390)
- `State.Invulnerable` (linha 385)

### O que foi implementado ✅

1. **`EDFDodgeDirection` 8-way** (octant snapping via `DFSnapLocalInputToDodgeDirection`) em [`DFDodgeTypes.h`](../../Source/DungeonForged/Public/Combat/DFDodgeTypes.h)
2. **`FDFDodgeAnimSet`** com 8 slots (Forward/ForwardRight/Right/BackwardRight/Backward/BackwardLeft/Left/ForwardLeft) + resolve chain por diagonal → cardinal → Backward
3. **Variante armada vs desarmada** — `IsOwnerArmed()` detecta `Equipment->IsSlotEmpty(Weapon)`
4. **Cancel-into-dodge** durante combo — `ANS_DFAbilityCancelWindow` adiciona `Ability.Movement.Dodge` ao `AllowedCancelTags` por default
5. **Custo de estamina** — `GetEffectiveDodgeStaminaCost()` consulta `UDFCombatTuningData::DodgeStaminaCost` (default 20)
6. **Root motion inteligente** — `DFMontageHasRootMotion()` detecta RM na montage; sem RM usa `FRootMotionSource_MoveToForce` programático
7. **`bRotateToDodgeDirection`** — alinha o actor na direção do dodge antes de rodar a montage
8. **`DFDodgeDebug`** — CVar `df.DebugDodge` (0/1/2) + cheat `df.DodgeDebug dump`
9. **`UDFAnimInstance`** expõe `bIsDodging` + `LastDodgeDirection` para AnimGraph

### O que falta (setup de asset no Editor) ❌

1. **Montages de dodge** — criar/importar 8 montages (4 armed + 4 unarmed) ou no mínimo 1 Backward genérico
2. **`GA_Knight_Dodge`** (já criado como asset) — configurar `UnarmedAnimSet` + `ArmedAnimSet` no Class Defaults
3. **`IMC_DFDefault`** — confirmar bind `IA_Dodge → SpaceBar` (asset já criado)
4. **Grant da ability** — adicionar `GA_Knight_Dodge` ao character ou à `DT_Class`

---

## 2. Arquitetura proposta

```
                          ┌──────────────────────────────────┐
Player presses Dodge  ─→  │ Input Action IA_Dodge            │
                          └────────┬─────────────────────────┘
                                   │
                                   ▼
                          ┌──────────────────────────────────┐
                          │ ASC->TryActivateAbilityByTag      │
                          │   Tag: Ability.Movement.Dodge     │
                          └────────┬─────────────────────────┘
                                   │
                                   ▼
                          ┌──────────────────────────────────┐
                          │ UDFAbility_Dodge::ActivateAbility │
                          │                                   │
                          │ 1. ResolveDirection()  →  enum    │
                          │    {Forward, Back, Left, Right}   │
                          │                                   │
                          │ 2. ResolveAnimSet()               │
                          │    if armed:    UseArmedSet       │
                          │    else:        UseUnarmedSet     │
                          │                                   │
                          │ 3. PickMontage(Set, Direction)    │
                          │                                   │
                          │ 4. CMC->PerformDodge(WorldDir)    │
                          │    └─ adds State.Dodging          │
                          │    └─ adds State.Invulnerable     │
                          │    └─ root motion / impulse       │
                          │                                   │
                          │ 5. PlayMontageAndWait(picked)     │
                          │                                   │
                          │ 6. WaitDelay(DodgeDuration)       │
                          │    └─ EndAbility on finish        │
                          └───────────────────────────────────┘
```

### Decisões de design

| Decisão | Escolha | Por quê |
|---|---|---|
| 1 ability ou 4? | **1 ability, montage resolvido dentro** | Reusa cost/cooldown/cancel rules. 4 abilities multiplicaria boilerplate sem ganho. |
| Resolução de direção | **Local-space do owner, octant snapping (atan2 → 45° sectors)** | `DFSnapLocalInputToDodgeDirection` replica o esquema do `UDFComboComponent`; 8-way cobre diagonais sem custo extra de design. |
| Armed/Unarmed split | **Por equip slot (Weapon)** | Já existe `Equipment->IsSlotEmpty(EEquipmentSlot::Weapon)`. |
| Fallback direção sem input | **Backward** | Padrão Souls — knight recua quando confuso. Mais defensivo. |
| 4-way ou 8-way? | **8-way (octant) com fallback chain** | Diagonais: ForwardLeft → [Forward, Left, Backward]. Custo: 8 slots no struct; ganho: roll diagonal fluido. |
| Root motion | **Detecta RM na montage; sem RM usa MoveToForce programático** | `DFMontageHasRootMotion()` evita double-displacement quando anim RM já move o char. |

---

## 3. C++ — Extensão da `UDFAbility_Dodge`

### Header — enum direção + anim sets (implementado em `DFDodgeTypes.h`)

> **✅ Já implementado** — ver [`Source/DungeonForged/Public/Combat/DFDodgeTypes.h`](../../Source/DungeonForged/Public/Combat/DFDodgeTypes.h).
> O enum foi movido para `DFDodgeTypes.h` (separado de `DFAbility_Dodge.h`) para reutilização por CMC e AnimInstance.

**Arquivo:** [`Source/DungeonForged/Public/Combat/DFDodgeTypes.h`](../../Source/DungeonForged/Public/Combat/DFDodgeTypes.h)

```cpp
/** Octant order matches atan2(Right, Forward) snapped to 45° sectors. */
UENUM(BlueprintType)
enum class EDFDodgeDirection : uint8
{
    Forward, ForwardRight, Right, BackwardRight,
    Backward, BackwardLeft, Left, ForwardLeft
};

/** Per-stance dodge montages (8-way). Null entries use fallback chain. */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFDodgeAnimSet
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Dodge|AnimSet")
    TObjectPtr<UAnimMontage> Forward = nullptr;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Dodge|AnimSet")
    TObjectPtr<UAnimMontage> ForwardRight = nullptr;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Dodge|AnimSet")
    TObjectPtr<UAnimMontage> Right = nullptr;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Dodge|AnimSet")
    TObjectPtr<UAnimMontage> BackwardRight = nullptr;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Dodge|AnimSet")
    TObjectPtr<UAnimMontage> Backward = nullptr;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Dodge|AnimSet")
    TObjectPtr<UAnimMontage> BackwardLeft = nullptr;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Dodge|AnimSet")
    TObjectPtr<UAnimMontage> Left = nullptr;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DF|Dodge|AnimSet")
    TObjectPtr<UAnimMontage> ForwardLeft = nullptr;

    UAnimMontage* Resolve(EDFDodgeDirection Dir) const;  // switch sobre os 8 valores
    bool IsValid() const;  // any slot non-null
};

UCLASS()
class DUNGEONFORGED_API UDFAbility_Dodge : public UDFGameplayAbility
{
    GENERATED_BODY()

public:
    UDFAbility_Dodge();

    /** Used when no weapon is equipped (unarmed roll set). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Dodge|Unarmed")
    FDFDodgeAnimSet UnarmedAnimSet;

    /** Used when a weapon is equipped (armed roll set — typically wider stance, weapon-aware poses). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Dodge|Armed")
    FDFDodgeAnimSet ArmedAnimSet;

    /** @deprecated Legacy single montage. Used as final fallback when both sets fail. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Dodge|Legacy")
    TObjectPtr<UAnimMontage> DodgeMontage;

    /**
     * Input velocity threshold (cm/s, local-space) to snap to a cardinal direction.
     * Below this, falls back to Backward (defensive default).
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|DF|Dodge", meta = (ClampMin = "0.0"))
    float DirectionalInputThreshold = 80.f;

    /** Resolves the dodge direction from local input. Exposed for debug / Blueprint hooks. */
    UFUNCTION(BlueprintCallable, Category = "Ability|DF|Dodge")
    EDFDodgeDirection ResolveDodgeDirection() const;

    /** Picks the montage matching current stance (armed/unarmed) and direction, with fallback chain. */
    UFUNCTION(BlueprintCallable, Category = "Ability|DF|Dodge")
    UAnimMontage* ResolveDodgeMontage(EDFDodgeDirection Direction) const;

protected:
    virtual void PostInitProperties() override;

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

    UFUNCTION()
    void OnDodgeDurationElapsed();

    UFUNCTION()
    void OnDodgeMontageCompleted();

    UFUNCTION()
    void OnDodgeMontageCancelled();

    /** True when the owner has a weapon equipped (Equipment->IsSlotEmpty(Weapon) == false). */
    bool IsOwnerArmed() const;
};
```

### Implementation — `.cpp`

**Arquivo:** [`Source/DungeonForged/Private/GAS/Abilities/DFAbility_Dodge.cpp`](../../Source/DungeonForged/Private/GAS/Abilities/DFAbility_Dodge.cpp)

```cpp
// Source/DungeonForged/Private/GAS/Abilities/DFAbility_Dodge.cpp
#include "GAS/Abilities/DFAbility_Dodge.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Characters/ADFPlayerCharacter.h"
#include "Characters/UDFCharacterMovementComponent.h"
#include "Equipment/DFEquipmentTypes.h"
#include "Equipment/UDFEquipmentComponent.h"
#include "FX/UDFScreenEffectsComponent.h"
#include "GAS/DFGameplayTags.h"
#include "GameFramework/Character.h"

UDFAbility_Dodge::UDFAbility_Dodge()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    AbilityCost_Mana = 0.f;
    AbilityCost_Stamina = 20.f;   // ← era 0; agora cobra estamina
    AbilityMontage = nullptr;
}

void UDFAbility_Dodge::PostInitProperties()
{
    Super::PostInitProperties();
    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        AbilityTags.AddTag(FDFGameplayTags::Ability_Movement_Dodge);
        BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dead);
        BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Stunned);
        BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Dodging);
        BlockAbilitiesWithTag.AddTag(FDFGameplayTags::State_Exhausted);
        // Dodge cancels in-flight melee swings (universal escape).
        CancelAbilitiesWithTag.AddTag(FDFGameplayTags::Ability_Attack_Melee);
    }
}

bool UDFAbility_Dodge::IsOwnerArmed() const
{
    const FGameplayAbilityActorInfo* const Info = GetCurrentActorInfo();
    if (!Info) return false;
    const ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(Info->AvatarActor.Get());
    if (!PC) return false;
    const UDFEquipmentComponent* const Eq = PC->Equipment;
    if (!Eq) return false;
    return !Eq->IsSlotEmpty(EEquipmentSlot::Weapon);
}

// ✅ Implementação real em DFAbility_Dodge.cpp — usa helpers de DFDodgeTypes.h
EDFDodgeDirection UDFAbility_Dodge::ResolveDodgeDirection() const
{
    // DFResolveLocalMovementIntent: LastInputVector fallback para velocity
    const FVector LocalInput = DFResolveLocalMovementIntent(Char, CMC, DirectionalInputThreshold);
    if (LocalInput.SizeSquared2D() < ThresholdSq)
    {
        return EDFDodgeDirection::Backward; // defensive default — sem input
    }
    // atan2(Y, X) → round para octante mais próximo (45° por sector)
    return DFSnapLocalInputToDodgeDirection(LocalInput.GetSafeNormal2D());
}

UAnimMontage* UDFAbility_Dodge::ResolveDodgeMontage(const EDFDodgeDirection Direction) const
{
    const bool bArmed = IsOwnerArmed();
    const FDFDodgeAnimSet& Primary   = bArmed ? ArmedAnimSet   : UnarmedAnimSet;
    const FDFDodgeAnimSet& Secondary = bArmed ? UnarmedAnimSet : ArmedAnimSet;

    // DFGetDodgeDirectionResolveOrder: diagonal → cardinais → Backward
    TArray<EDFDodgeDirection> TryOrder;
    DFGetDodgeDirectionResolveOrder(Direction, TryOrder);

    for (const EDFDodgeDirection Dir : TryOrder)
        if (UAnimMontage* M = Primary.Resolve(Dir)) return M;
    for (const EDFDodgeDirection Dir : TryOrder)
        if (UAnimMontage* M = Secondary.Resolve(Dir)) return M;
    return DodgeMontage.Get(); // legacy fallback
}

UAnimMontage* UDFAbility_Dodge::ResolveDodgeMontage(EDFDodgeDirection Direction) const
{
    const bool bArmed = IsOwnerArmed();
    const FDFDodgeAnimSet& Primary = bArmed ? ArmedAnimSet : UnarmedAnimSet;
    const FDFDodgeAnimSet& Secondary = bArmed ? UnarmedAnimSet : ArmedAnimSet;

    if (UAnimMontage* const M = Primary.Resolve(Direction)) return M;
    // Primary stance fallback: same direction → Backward (defensive)
    if (UAnimMontage* const M = Primary.Resolve(EDFDodgeDirection::Backward)) return M;
    // Cross-stance fallback (e.g., unarmed set has Forward but armed doesn't)
    if (UAnimMontage* const M = Secondary.Resolve(Direction)) return M;
    // Last resort: legacy single montage
    return DodgeMontage.Get();
}

void UDFAbility_Dodge::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    (void)TriggerEventData;
    if (!ActorInfo)
    {
        EndAbility(Handle, nullptr, ActivationInfo, true, true);
        return;
    }
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo, nullptr))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    ACharacter* const Char = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
    UDFCharacterMovementComponent* const CMC = Char ? Cast<UDFCharacterMovementComponent>(Char->GetCharacterMovement()) : nullptr;
    if (!CMC)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // Resolve direction BEFORE consuming input vector (PerformDodge reads it too as fallback).
    const EDFDodgeDirection Direction = ResolveDodgeDirection();
    UAnimMontage* const PickedMontage = ResolveDodgeMontage(Direction);

    // Apply server-authoritative cost (stamina) — UDFGameplayAbility helper drains the attribute.
    if (UAbilitySystemComponent* const ASC = GetAbilitySystemComponentFromActorInfo())
    {
        if (ASC->GetOwner() && ASC->GetOwner()->HasAuthority())
        {
            ApplyResourceCostsToOwner(ASC);
        }
    }

    // Trigger movement impulse + State.Dodging + State.Invulnerable.
    const FVector DodgeDirWorld = CMC->GetDodgeDirection();
    CMC->PerformDodge(DodgeDirWorld);

    const float D = FMath::Max(0.01f, CMC->DodgeDuration);

    if (ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(Char))
    {
        if (PC->IsLocallyControlled() && PC->ScreenEffects)
        {
            PC->ScreenEffects->ApplyDodgeJuice(D);
        }
    }

    if (PickedMontage)
    {
        if (UAbilityTask_PlayMontageAndWait* const MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
                this, NAME_None, PickedMontage, 1.f, NAME_None, true, 1.f, 0.f, true))
        {
            MontageTask->OnCompleted.AddDynamic(this, &UDFAbility_Dodge::OnDodgeMontageCompleted);
            MontageTask->OnInterrupted.AddDynamic(this, &UDFAbility_Dodge::OnDodgeMontageCancelled);
            MontageTask->OnCancelled.AddDynamic(this, &UDFAbility_Dodge::OnDodgeMontageCancelled);
            MontageTask->ReadyForActivation();
        }
    }

    if (UAbilityTask_WaitDelay* const Wait = UAbilityTask_WaitDelay::WaitDelay(this, D))
    {
        Wait->OnFinish.AddDynamic(this, &UDFAbility_Dodge::OnDodgeDurationElapsed);
        Wait->ReadyForActivation();
    }
    else
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    }
}

void UDFAbility_Dodge::OnDodgeDurationElapsed()
{
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UDFAbility_Dodge::OnDodgeMontageCompleted() { /* time authority is WaitDelay */ }
void UDFAbility_Dodge::OnDodgeMontageCancelled() { /* same */ }
```

---

## 4. C++ — Estrutura `FDFDodgeAnimSet`

Já incluída no header acima. Reuso possível:

- **Outras classes (Mage/Rogue):** crie subclasses ou apenas use o mesmo struct com sets diferentes
- **Per-weapon:** poderia mover a struct para `FDFItemTableRow` e ter dodge específico por arma (ex: arma pesada usa anim mais lenta). Fora do escopo deste guia mas trivial estender.

---

## 5. C++ — Resolução armado vs desarmado

A função `IsOwnerArmed()` usa o `UDFEquipmentComponent` que já existe:

```cpp
bool UDFAbility_Dodge::IsOwnerArmed() const
{
    // ...
    return !Eq->IsSlotEmpty(EEquipmentSlot::Weapon);
}
```

**Edge cases cobertos pela fallback chain do `ResolveDodgeMontage`:**

| Cenário | Resultado |
|---|---|
| Armado, Forward set, jogador pressiona W | `ArmedAnimSet.Forward` ✓ |
| Armado, jogador pressiona W mas Armed.Forward é null | `ArmedAnimSet.Backward` (defensive fallback) |
| Armado, ambos Forward e Backward são null | `UnarmedAnimSet.Forward` (cross-stance) |
| Tudo null | `DodgeMontage` legacy (último recurso) |
| Tudo null incluindo DodgeMontage | nullptr → não toca anim mas CMC->PerformDodge ainda roda |

---

## 6. Integração GAS — Tags, Cost, Cooldown

### Tags

Já existentes em [`FDFGameplayTags`](../../Source/DungeonForged/Public/GAS/DFGameplayTags.h):
- `Ability.Movement.Dodge` — identificador da habilidade
- `State.Dodging` — set pelo CMC durante dodge
- `State.Invulnerable` — set pelo CMC durante i-frames
- `State.Exhausted` — blocking tag

**Tags adicionais opcionais** (criar em [`DFGameplayTags.cpp`](../../Source/DungeonForged/Private/GAS/DFGameplayTags.cpp)):
```cpp
DF_TAG(Event_Dodge_Started)("Event.Dodge.Started", "Fired when dodge ability activates");
DF_TAG(Event_Dodge_PerfectFrame)("Event.Dodge.PerfectFrame", "Fired on flawless dodge (no damage taken in window)");
```

Use cases:
- `Event.Dodge.Started`: outras habilidades podem ouvir (ex: passiva que dá speed buff em dodge)
- `Event.Dodge.PerfectFrame`: alimenta o style rating system mencionado no doc 14

### Cost — Stamina

No construtor:
```cpp
AbilityCost_Stamina = 20.f;
```

O `UDFGameplayAbility::ApplyResourceCostsToOwner()` já dreina o atributo. Se você quiser via GameplayEffect ao invés de manual drain, configure `CostGameplayEffectClass = GE_Cost_Dodge` no CDO.

### Cooldown

Para usar a GE de cooldown (opcional, hoje usa só `CMC->DodgeCooldown` interno):

1. Crie `GE_Cooldown_Dodge` (Blueprint asset baseado em `UGameplayEffect`):
   - Duration Policy: Has Duration
   - Duration: 0.7s (igual ao `CMC->DodgeCooldown`)
   - Grant Tag: `Cooldown.Dodge`
2. No CDO da ability: `CooldownGameplayEffectClass = GE_Cooldown_Dodge`
3. Em `PostInitProperties`:
   ```cpp
   ActivationBlockedTags.AddTag(FName("Cooldown.Dodge"));
   ```

**Recomendado manter cooldown no CMC** por enquanto — mais simples, integra bem com prediction.

---

## 7. Integração Combo — Cancel windows

A `UDFAbility_Dodge` precisa **cancelar combos em andamento** e **ser cancelable pelo combo cancel window**.

### Cancelar combo em andamento (já ok)

`CancelAbilitiesWithTag.AddTag(Ability_Attack_Melee)` já está no `PostInitProperties`. Quando dodge ativa, qualquer melee swing rodando é cancelado.

### Permitir dodge cancel durante combo (novo)

Em [`UDFComboComponent::IsAbilityCancellable`](../../Source/DungeonForged/Public/Combat/UDFComboComponent.h:281), o dodge precisa estar nos allowed tags durante o cancel window.

**Setup no `ANS_DFAbilityCancelWindow`** (anim notify state já existe):
1. Abre cada melee montage (`Combo_Attack_02_01`, etc.)
2. No `ANS_DFAbilityCancelWindow`, no campo `Allowed Cancel Tags`, adiciona:
   - `Ability.Movement.Dodge` ✓
   - `Ability.Parry` (se aplicável)

Isso permite que o jogador esquive durante a janela do combo sem precisar esperar o swing terminar.

### Combo state limpo no dodge

Quando dodge ativa, o combo deve ser resetado. No `ActivateAbility` da Dodge, adicione (opcional):
```cpp
if (ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(Char))
{
    if (UDFComboComponent* const Combo = PC->Combo)
    {
        Combo->ResetCombo();
    }
}
```

Sem isso, se o jogador esquiva no meio do combo step 2, ele pode retomar do step 3 quando voltar. AAA games (Souls) sempre resetam o combo no dodge — quebra de fluxo é intencional.

---

## 8. Integração Estamina

### Custo via attribute set

`UDFGameplayAbility::ApplyResourceCostsToOwner` dreina `Stamina` direto. Para `AbilityCost_Stamina = 20.f`:

```cpp
// UDFGameplayAbility.cpp (já implementado pelo seu sistema)
void UDFGameplayAbility::ApplyResourceCostsToOwner(UAbilitySystemComponent* ASC)
{
    if (AbilityCost_Stamina > 0.f)
    {
        const UDFAttributeSet* Attrs = ASC->GetSet<UDFAttributeSet>();
        if (Attrs)
        {
            const float NewStamina = FMath::Max(0.f, Attrs->GetStamina() - AbilityCost_Stamina);
            const_cast<UDFAttributeSet*>(Attrs)->SetStamina(NewStamina);
        }
    }
    // ... outros costs
}
```

### CanActivateAbility — gating por estamina

Override `CanActivateAbility` para bloquear se estamina < custo:

```cpp
// DFAbility_Dodge.cpp — adicionar
bool UDFAbility_Dodge::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }
    if (AbilityCost_Stamina > 0.f && ActorInfo)
    {
        if (const UAbilitySystemComponent* const ASC = ActorInfo->AbilitySystemComponent.Get())
        {
            if (const UDFAttributeSet* const Attrs = ASC->GetSet<UDFAttributeSet>())
            {
                if (Attrs->GetStamina() < AbilityCost_Stamina)
                {
                    return false;
                }
            }
        }
    }
    return true;
}
```

Adiciona o protótipo no header também:
```cpp
virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
```

### Exhausted state

Já existe — `BlockAbilitiesWithTag.AddTag(State_Exhausted)` no `PostInitProperties` impede dodge quando o `UDFStaminaExhaustionComponent` aplicou a tag. Não precisa fazer mais nada.

---

## 9. Integração Hotbar

O Dodge **NÃO deve aparecer no hotbar** como ability slot — é movimento básico, não habilidade gastável. Mas o hotbar deve **reagir ao estado de cooldown**.

### Cooldown widget na UI

No `WBP_AbilityHotbar`, **não adicione slot pra dodge**. Mas se quiser mostrar cooldown visual (ex: ícone no canto da tela quando dodge tá disponível):

1. Cria widget `WBP_DodgeIndicator` com:
   - `Image_DodgeIcon` (ícone do dodge)
   - `Image_Cooldown` (overlay escuro com percentual)
2. No BP graph, Tick: lê `CMC->TimeLastDodge` e `CMC->DodgeCooldown`, calcula percentual
3. Adiciona em `WBP_HUD` em posição discreta (canto inferior direito)

### Stamina bar reflete custo

O `UDFAbilityHotbarWidget` já tem `StaminaBar`. Não precisa modificar — quando dodge dreina 20 stamina via `ApplyResourceCostsToOwner`, o `OnRep_Stamina` no `UDFAttributeSet` notifica e o bar atualiza.

### Input binding (Enhanced Input)

Crie um Input Action:
1. **Content Browser → Input → Input Action** → nome `IA_Dodge`
2. Value Type: `Digital (bool)`
3. Triggers: **Pressed**

No Input Mapping Context (`IMC_Player` ou similar):
- Adicione `IA_Dodge`
- Bind a `SpaceBar` (PC) e `Gamepad Right Shoulder` (controller)

No `BP_JCHero_Character` (ou `ADFPlayerCharacter`):
1. **Event Graph** → `EnhancedInputAction IA_Dodge` (Triggered)
2. Conecte em uma função `TryDodge`:

```
Event Triggered ──▶ [Get Ability System Component]
                       │
                       ▼
                   [Try Activate Abilities By Tag]
                   - Gameplay Ability Tags: Ability.Movement.Dodge
                   - Allow Remote Activation: ✓
```

Sem custom code C++, só BP graph wiring.

---

## 10. Integração AnimInstance (BP)

### Property exposta para AnimGraph

Em `UDFAnimInstance.h` (provavelmente já tem algo similar):
```cpp
UPROPERTY(BlueprintReadOnly, Category = "DF|Anim|State")
bool bIsDodging = false;

UPROPERTY(BlueprintReadOnly, Category = "DF|Anim|State")
EDFDodgeDirection LastDodgeDirection = EDFDodgeDirection::Backward;
```

No `NativeUpdateAnimation`:
```cpp
if (const ACharacter* const Char = Cast<ACharacter>(GetOwningActor()))
{
    if (const UDFCharacterMovementComponent* const CMC = Cast<UDFCharacterMovementComponent>(Char->GetCharacterMovement()))
    {
        bIsDodging = CMC->bIsDodging;
    }
}
```

### AnimBP usage

No `ABP_JCHero` (Anim Blueprint do personagem):

**State Machine "Locomotion":**
- Não precisa novo state — a montage do dodge usa **Slot 'DefaultSlot'** e sobrepõe a locomotion naturalmente.
- O slot blend é controlado pela própria montage (BlendIn/BlendOut do asset).

**Optional polish — Lean during dodge:**
- Cria float `DodgeLean` que vai de -1 (left) a +1 (right) durante o dodge
- Usa em um Additive layer no AnimGraph pra inclinar o torso na direção do roll
- Fora do escopo deste doc mas é o tipo de polish que separa AA de AAA

---

## 11. Integração Damage / I-Frames

### State.Invulnerable bloqueia dano

Já implementado pelo `UDFCharacterMovementComponent::PerformDodge`:
```cpp
ASC->AddLooseGameplayTag(FDFGameplayTags::State_Dodging);
ASC->AddLooseGameplayTag(FDFGameplayTags::State_Invulnerable);
```

Em `UGE_Damage_Physical` / `UGE_Damage_Magic` (ou no `PostGameplayEffectExecute` do `UDFAttributeSet`), checa antes de aplicar:

```cpp
// Pseudo-code, ajusta para a estrutura real do seu damage execution:
if (TargetASC->HasMatchingGameplayTag(FDFGameplayTags::State_Invulnerable))
{
    return; // damage ignored
}
```

Confirma se isso já existe no seu `UDFAttributeSet::PreAttributeChange` ou no `UDFTrueDamageExecution`. Se não, adiciona.

### Perfect dodge bonus

Quando o jogador esquiva **exatamente no momento do impacto** (dodge ativa <100ms antes do hit), você pode trigger um efeito:
- Slow motion 0.3s
- Damage buff temporário
- Style rating boost

Implementação (opcional, futura):
1. No `UDFMeleeTraceComponent` (ou onde inimigo aplica dano), antes de aplicar damage spec, se `Target->HasMatchingGameplayTag(State_Invulnerable)` E `WorldTime - DodgeStartTime < 0.1s`:
   - Send Gameplay Event `Event.Dodge.PerfectFrame`
   - Apply `GE_Buff_PerfectDodge` (slow-mo + damage up)

Bayonetta usa exatamente esse padrão (Witch Time). Não precisa fazer agora, só deixei a porta aberta.

---

## 12. Setup Editor passo-a-passo

### Passo 1: Rebuild C++

Live Coding ou full rebuild via Visual Studio. Necessário pra `EDFDodgeDirection`, `FDFDodgeAnimSet`, novos métodos aparecerem no editor.

### Passo 2: Criar GA_Dodge (Blueprint asset)

1. Content Browser → `Content/GAS/Abilities/`
2. **Add → Blueprint Class** → parent: `UDFAbility_Dodge`
3. Nome: `GA_Dodge`
4. Abre o asset, vai em **Class Defaults**

### Passo 3: Configurar anim sets

No `GA_Knight_Dodge`, categoria **Ability | DF | Dodge | Unarmed**:
- `Unarmed Anim Set → Forward`: `M_Roll_Unarmed_Forward`
- `Unarmed Anim Set → Backward`: `M_Roll_Unarmed_Backward`
- `Unarmed Anim Set → Left`: `M_Roll_Unarmed_Left`
- `Unarmed Anim Set → Right`: `M_Roll_Unarmed_Right`
- Slots diagonais (ForwardRight, BackwardRight, etc.): **deixar nulos** para usar o fallback chain automático

Categoria **Ability | DF | Dodge | Armed**:
- `Armed Anim Set → Forward/Backward/Left/Right`: similar ao unarmed
- **Mínimo viável:** preencher apenas `Backward` unarmed → todos os outros herdam via fallback

(Substitua os nomes pelos seus assets reais. Se tiver somente 1 montage, assign em todos os slots da set primária — o fallback chain garante que nunca fica sem anim.)

### Passo 4: Grant a ability ao personagem

No `ADFPlayerCharacter` (ou na classe Warrior em `DT_Classes`), adiciona `GA_Dodge` ao array de granted abilities.

Procura por código tipo:
```cpp
StartupAbilities.Add(GA_Dodge);
```
Ou no DT, na coluna `GrantedAbilities`, adiciona uma row com `GA_Dodge`.

### Passo 5: Bind input

Como descrito em §9. `IA_Dodge` → `TryActivateAbilityByTag(Ability.Movement.Dodge)`.

### Passo 6: Ajustar montages — Anim Notify

Em cada uma das 8 montages (4 armed + 4 unarmed):
1. Abre o asset
2. Adiciona `AN_DodgeStart` no frame 0 (opcional, pra spawn de VFX de poeira)
3. Adiciona `AN_DodgeEnd` no último frame (opcional, pra cleanup de VFX)
4. **NÃO** adicione `AN_ComboWindowOpen` nem `ANS_DFCancelWindow` — dodge não chaina em combo

### Passo 7: Configurar BlendIn/Out das montages

Cada montage:
- `Blend In → Blend Time: 0.05` (entrada rápida pra responsividade)
- `Blend Out → Blend Time: 0.15` (saída suave de volta pra locomotion)
- `Enable Auto Blend Out: ✓`

### Passo 8: Tunar parâmetros no CMC

Em `BP_JCHero_Character` (ou onde quer que `UDFCharacterMovementComponent` esteja exposto):

| Param | Valor sugerido | Por quê |
|---|---|---|
| `Dodge Cooldown` | **0.7s** | Já é default. Souls usa ~0.5s mas seu combat é mais rápido |
| `Dodge Distance` | **500-600cm** | Curto pra Knight (era 600 default — pode reduzir se sentir longe demais) |
| `Dodge Duration` | **0.35s** | Já é default. Bate com tamanho médio das anims de roll |
| `IFrame Duration` | **0.25-0.30s** | Menor que DodgeDuration — pune timing ruim |

### Passo 9: Adicionar custo de estamina

No CDO de `GA_Dodge`:
- `Ability Cost Stamina`: **20** (era 0)

Stamina max típico = 100. 5 dodges esgotam totalmente. Player gasta 1 dodge ≈ 4 segundos de regen.

### Passo 10: Testar

PIE com `df.DebugCombat 1` ativo. Testa:
- Press dodge sem input → backward ✓
- W + dodge → forward ✓
- A + dodge → left ✓
- D + dodge → right ✓
- S + dodge → backward ✓
- Equipa espada → testa novamente → deve usar `ArmedAnimSet`
- Desequipa → deve usar `UnarmedAnimSet`
- Dodge no meio de um combo step 2 → combo deve cancelar
- Spam dodge → 2º press dentro do cooldown deve ignorar
- Drena stamina → dodge bloqueado (State_Exhausted)

---

## 13. Console commands & validação

### Commands

```
showdebug AbilitySystem       # mostra tags ativas: State.Dodging, State.Invulnerable
showdebug Animation           # state machine atual + montage tocando
df.DebugCombat 1              # overlay com info de combo (mostra step interrupt)
```

### Checklist de validação AAA

- [ ] 8-way: cada direção cardinal e diagonal toca a montage correta
- [ ] Diagonal sem montage específica faz fallback correto para cardinals (e.g. ForwardLeft → Forward)
- [ ] Armado vs desarmado resolve corretamente
- [ ] Fallback funciona: se Armed.Forward é null, usa Armed.Backward
- [ ] State.Invulnerable bloqueia damage durante i-frames (boss attack passa sem dano)
- [ ] Combo é resetado ao esquivar (não retoma do step antigo)
- [ ] Stamina drena 20 por dodge
- [ ] Cooldown impede spam (0.7s entre dodges)
- [ ] Exhausted bloqueia dodge
- [ ] Dodge cancela melee swing em andamento
- [ ] Hotbar stamina bar atualiza ao dodge
- [ ] Animation blend não dá snap visual (BlendIn/Out tunado)
- [ ] Replicação ok em PIE 2-player (LocalPredicted)

### Frame-target ideal

- **Time-to-iframe**: <60ms do press até a tag `State.Invulnerable` ativa
- **Total dodge animation**: 0.7-0.9s (não mais que 1s — senão sente pesado demais)
- **Recovery após dodge**: <0.2s antes de poder atacar/dodge novamente

---

## 14. Tabela de pacotes que você toca

| Arquivo | Status | O que tem / foi feito |
|---|---|---|
| [`DFDodgeTypes.h`](../../Source/DungeonForged/Public/Combat/DFDodgeTypes.h) | ✅ Novo | `EDFDodgeDirection` 8-way, `FDFDodgeAnimSet`, helpers `DFSnapLocalInputToDodgeDirection`, `DFResolveLocalMovementIntent`, `DFGetDodgeDirectionResolveOrder` |
| [`DFDodgeTypes.cpp`](../../Source/DungeonForged/Private/Combat/DFDodgeTypes.cpp) | ✅ Novo | `DFMontageHasRootMotion()` — inspeciona slots e sequences |
| [`DFDodgeDebug.h`](../../Source/DungeonForged/Public/Combat/DFDodgeDebug.h) | ✅ Novo | API de debug: `IsLogEnabled`, `IsDrawEnabled`, `Logf`, `DrawDodgeArrow`, `DumpLocalDodgeState` |
| [`DFDodgeDebug.cpp`](../../Source/DungeonForged/Private/Combat/DFDodgeDebug.cpp) | ✅ Novo | CVar `df.DebugDodge` (0/1/2), console command `df.DodgeDebug` |
| [`DFAbility_Dodge.h`](../../Source/DungeonForged/Public/GAS/Abilities/DFAbility_Dodge.h) | ✅ Atualizado | `UnarmedAnimSet`, `ArmedAnimSet`, `bPreferAnimRootMotion`, `bRotateToDodgeDirection`, métodos `Resolve*` |
| [`DFAbility_Dodge.cpp`](../../Source/DungeonForged/Private/GAS/Abilities/DFAbility_Dodge.cpp) | ✅ Atualizado | 8-way direction resolve, root motion detection, combo reset, screen effects |
| [`UDFCharacterMovementComponent.h`](../../Source/DungeonForged/Public/Characters/UDFCharacterMovementComponent.h) | ✅ Atualizado | `LastDodgeDirection`, `IFrameDuration`, `GetDodgeCooldownRemaining()` |
| [`UDFCharacterMovementComponent.cpp`](../../Source/DungeonForged/Private/Characters/UDFCharacterMovementComponent.cpp) | ✅ Atualizado | `PerformDodge` com `FRootMotionSource_MoveToForce`, `EndIFrameState`, `EndDodgingState` |
| [`UDFAnimInstance.h`](../../Source/DungeonForged/Public/Animation/UDFAnimInstance.h) | ✅ Atualizado | `bIsDodging`, `LastDodgeDirection` — expostos para AnimGraph |
| [`UDFCombatTuningData.h`](../../Source/DungeonForged/Public/Data/UDFCombatTuningData.h) | ✅ Atualizado | `DodgeIFrameDuration`, `DodgeCooldown`, `DodgeStaminaCost` |
| [`ANS_DFAbilityCancelWindow.cpp`](../../Source/DungeonForged/Private/Combat/AN/ANS_DFAbilityCancelWindow.cpp) | ✅ Atualizado | `Ability.Movement.Dodge` no `AllowedCancelTags` por default |
| `GA_Knight_Dodge.uasset` | ✅ Criado | Blueprint de `UDFAbility_Dodge` — falta configurar anim sets no editor |
| `IA_Dodge.uasset` | ✅ Criado | Input Action Digital(bool) Pressed |
| `IMC_DFDefault.uasset` | ✅ Modificado | Bind `IA_Dodge` adicionado |
| `DT_Abilities.uasset` / `DT_Class.uasset` | ❓ Verificar | Grant da `GA_Knight_Dodge` ao character startup |
| 8× `M_Roll_*_*.uasset` | ❌ Pendente | Criar montages de roll unarmed/armed por direção |
| `WBP_DodgeIndicator.uasset` (opcional) | ❌ Pendente | Cooldown visual no HUD |

---

## Próximos passos depois desse setup

1. **Criar montages de roll** — mínimo viável: 1 backward unarmed; ideal: 4 cardinals × 2 stances = 8 montages
2. **Configurar `GA_Knight_Dodge`** — abrir o asset no editor e atribuir montages nos slots `UnarmedAnimSet` / `ArmedAnimSet`
3. **Confirmar grant** — verificar que `GA_Knight_Dodge` está no startup do personagem (`DT_Class` ou `ADFPlayerCharacter::StartupAbilities`)
4. **Perfect dodge / Witch Time** — event-based detection de "dodged at hit frame" → `Event.Dodge.PerfectFrame`
5. **Dodge attack** — segurar attack durante dodge → ataque com momentum (estilo Sekiro)
6. **Sound/VFX** — `AN_DodgeStart` notify: spawn de poeira + whoosh sound
7. **Camera shake leve** no dodge (pequeno kick pra dar peso)

---

> **Notas finais:**
> - O `DodgeMontage` legacy está mantido no header como último fallback. Só deprecar com `UE_DEPRECATED` quando todas as classes/armas tiverem sets configurados.
> - O sistema é **modular** — Rogue Dash usa a mesma `FDFDodgeAnimSet`, só troca os assets para montages de dash em vez de roll.
> - O fallback chain `DFGetDodgeDirectionResolveOrder` garante que diagonais sem montage específica nunca quebram — sempre encontram uma cardinal válida.

---

## Arquivos referenciados

- [UDFAbility_Dodge.h](../../Source/DungeonForged/Public/GAS/Abilities/DFAbility_Dodge.h)
- [UDFAbility_Dodge.cpp](../../Source/DungeonForged/Private/GAS/Abilities/DFAbility_Dodge.cpp)
- [UDFCharacterMovementComponent.h](../../Source/DungeonForged/Public/Characters/UDFCharacterMovementComponent.h)
- [DFGameplayTags.h](../../Source/DungeonForged/Public/GAS/DFGameplayTags.h)
- [DFGameplayTags.cpp](../../Source/DungeonForged/Private/GAS/DFGameplayTags.cpp)
- [UDFAttributeSet.h](../../Source/DungeonForged/Public/GAS/UDFAttributeSet.h)
- [UDFComboComponent.h](../../Source/DungeonForged/Public/Combat/UDFComboComponent.h)
- [UDFAbilityHotbarWidget.h](../../Source/DungeonForged/Public/UI/UDFAbilityHotbarWidget.h)
- [ADFPlayerCharacter.h](../../Source/DungeonForged/Public/Characters/ADFPlayerCharacter.h)
- [10_AAA_AimWarpCombat.md](10_AAA_AimWarpCombat.md) — sistemas de combate adjacentes
- [14_AAA_CombatSystem.md](14_AAA_CombatSystem.md) — roadmap AAA combat
