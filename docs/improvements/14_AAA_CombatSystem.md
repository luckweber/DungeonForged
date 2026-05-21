# 14 — Combate AAA: Guia Completo de Upgrade (C++ + Editor)

> **Objetivo:** elevar o sistema de combate de **AA+** para **AAA** completo (DMC/Bayonetta/God of War tier). Cobre tudo: blends de qualidade frame-perfect, style rating, variantes de combo, juggle/launcher, cancel hierarchy, anim curves, robustez online.
>
> **Pré-requisitos:** O documento [10_AAA_AimWarpCombat.md](10_AAA_AimWarpCombat.md) deve estar concluído. Este documento estende e complementa.
>
> **Estado atual da base (AA+):** ver tabela de gaps em §1.
>
> **Estilo deste guia:** cada seção tem ① Por que importa  ② Código C++  ③ Setup no Editor  ④ Validação.

---

## Sumário

- [1. Mapa de Gaps AA+ → AAA](#1-mapa-de-gaps-aa--aaa)
- [2. P0 — Cross-fade com curve não-linear (HermiteCubic)](#2-p0--cross-fade-com-curve-não-linear-hermitecubic)
- [3. P0 — Rate-scale local no impacto (impact framing)](#3-p0--rate-scale-local-no-impacto-impact-framing)
- [4. P1 — Style Rating System (S/A/B/C/D)](#4-p1--style-rating-system-sabcd)
- [5. P1 — Variantes múltiplas por step (variety)](#5-p1--variantes-múltiplas-por-step-variety)
- [6. P2 — Juggle / Launcher System](#6-p2--juggle--launcher-system)
- [7. P2 — Cancel Hierarchy (prioridade direcional)](#7-p2--cancel-hierarchy-prioridade-direcional)
- [8. P3 — Anim Curve em vez de AnimNotify](#8-p3--anim-curve-em-vez-de-animnotify)
- [9. P3 — Robustez online (rollback, dessync)](#9-p3--robustez-online-rollback-dessync)
- [10. Ordem recomendada de execução](#10-ordem-recomendada-de-execução)
- [11. Console commands & validação](#11-console-commands--validação)

---

## 1. Mapa de Gaps AA+ → AAA

Estado atual após o fix de cross-fade aplicado em [`UDFComboComponent`](../../Source/DungeonForged/Public/Combat/UDFComboComponent.h) e [`UDFAbility_Warrior_MeleeSwing`](../../Source/DungeonForged/Private/GAS/Abilities/Warrior/UDFAbility_Warrior_MeleeSwing.cpp):

| Pilar | Tier atual | Tier alvo | Prioridade | Esforço |
|---|---|---|---|---|
| Arquitetura GAS / replicação | AAA | AAA | — | — |
| Hit-stop por bands | AAA | AAA | — | — |
| Motion warping + aim | AAA | AAA | — | — |
| Blend curves (cross-fade) | AA+ | AAA | **P0** | 1h |
| Impact framing (rate scale local) | AA | AAA | **P0** | 2h |
| Variedade de moves (montages/step) | A | AAA | **P1** | 4–8h por classe |
| Style rating (S/A/B/C) | — | AAA | **P1** | 1 dia |
| Juggle / launcher / aerial | — | AAA | **P2** | 2 dias |
| Cancel hierarchy | A | AAA | **P2** | 4h |
| Anim curve (vs AnimNotify) | AA | AAA | **P3** | 4h |
| Server reconciliation | AA | AAA | **P3** | 1 dia |

> **Regra de ouro:** faça P0 → P1 → P2 → P3 em ordem. Cada nível depende da estabilidade do anterior. Não pule.

---

## 2. P0 — Cross-fade com curve não-linear (HermiteCubic)

### ① Por que importa
`FAlphaBlendArgs(time)` faz blend linear — perceptível como "fade chapado". AAA usa **HermiteCubic** (curva S) que acelera e desacelera suavemente. Comparativo: Bayonetta 3 usa Hermite + Custom curve. DMC5 usa Cubic. Sem isso, o cross-fade que aplicamos parece "interpolação de plástico".

### ② Código C++

**Arquivo:** [`Source/DungeonForged/Public/Combat/DFAnimCombatLibrary.h`](../../Source/DungeonForged/Public/Combat/DFAnimCombatLibrary.h)

Adicione um parâmetro `BlendOption`:

```cpp
// Source/DungeonForged/Public/Combat/DFAnimCombatLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "AlphaBlend.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DFAnimCombatLibrary.generated.h"

class UAnimInstance;
class UAnimMontage;

UCLASS()
class DUNGEONFORGED_API UDFAnimCombatLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Plays a montage with a custom blend-in time AND curve option. */
    UFUNCTION(BlueprintCallable, Category = "DF|Combat|Animation", meta = (DisplayName = "Play Montage With Blend In (Curve)"))
    static float PlayMontageWithBlendIn(
        UAnimInstance* AnimInstance,
        UAnimMontage* Montage,
        float PlayRate = 1.f,
        float BlendInTime = 0.12f,
        bool bStopAllMontages = false,
        EAlphaBlendOption BlendOption = EAlphaBlendOption::HermiteCubic);
};
```

**Arquivo:** [`Source/DungeonForged/Private/Combat/DFAnimCombatLibrary.cpp`](../../Source/DungeonForged/Private/Combat/DFAnimCombatLibrary.cpp)

```cpp
// Source/DungeonForged/Private/Combat/DFAnimCombatLibrary.cpp
#include "Combat/DFAnimCombatLibrary.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "AlphaBlend.h"

float UDFAnimCombatLibrary::PlayMontageWithBlendIn(
    UAnimInstance* AnimInstance,
    UAnimMontage* Montage,
    const float PlayRate,
    const float BlendInTime,
    const bool bStopAllMontages,
    const EAlphaBlendOption BlendOption)
{
    if (!AnimInstance || !Montage)
    {
        return 0.f;
    }
    const float ClampedBlendIn = FMath::Max(0.f, BlendInTime);

    FAlphaBlendArgs BlendIn;
    BlendIn.BlendTime = ClampedBlendIn;
    BlendIn.BlendOption = BlendOption;

    return AnimInstance->Montage_PlayWithBlendIn(
        Montage,
        BlendIn,
        PlayRate,
        EMontagePlayReturnType::MontageLength,
        /*InTimeToStartMontageAt*/ 0.f,
        bStopAllMontages);
}
```

**Arquivo:** [`Source/DungeonForged/Public/Combat/UDFComboComponent.h`](../../Source/DungeonForged/Public/Combat/UDFComboComponent.h)

Exponha a curva globalmente (com override por step):

```cpp
// Adicione em UDFComboComponent.h, perto das outras chain properties
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Combo|Animation")
TEnumAsByte<EAlphaBlendOption> ComboChainBlendOption = EAlphaBlendOption::HermiteCubic;
```

E em [`FDFComboStep`](../../Source/DungeonForged/Public/Data/DFDataTableStructs.h):

```cpp
// Adicione em FDFComboStep, junto com ChainBlendInTime
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
TEnumAsByte<EAlphaBlendOption> ChainBlendOption = EAlphaBlendOption::Type(255); // 255 = invalid sentinel "use default"
```

Helper de resolução (similar ao `ResolveChainBlendInForStep`):

```cpp
// UDFComboComponent.h
UFUNCTION(BlueprintPure, Category = "Combat|Combo")
EAlphaBlendOption ResolveChainBlendOptionForStep(int32 Step) const;
```

```cpp
// UDFComboComponent.cpp
EAlphaBlendOption UDFComboComponent::ResolveChainBlendOptionForStep(const int32 Step) const
{
    if (ComboSteps.IsValidIndex(Step))
    {
        const EAlphaBlendOption StepOption = static_cast<EAlphaBlendOption>(ComboSteps[Step].ChainBlendOption.GetValue());
        if (StepOption != static_cast<EAlphaBlendOption>(255))
        {
            return StepOption;
        }
    }
    return ComboChainBlendOption;
}
```

E na ativação do MeleeSwing ([`UDFAbility_Warrior_MeleeSwing.cpp:172`](../../Source/DungeonForged/Private/GAS/Abilities/Warrior/UDFAbility_Warrior_MeleeSwing.cpp)):

```cpp
const float ChainBlendIn = Combo ? Combo->ResolveChainBlendInForStep(ComboStep) : 0.12f;
const EAlphaBlendOption BlendOpt = Combo ? Combo->ResolveChainBlendOptionForStep(ComboStep)
                                          : EAlphaBlendOption::HermiteCubic;

// ...
const float Len = UDFAnimCombatLibrary::PlayMontageWithBlendIn(
    AnimInst, MontToPlay, 1.f, ChainBlendIn, /*bStopAll*/ false, BlendOpt);
```

### ③ Setup no Editor

1. Abra o Blueprint do `ADFPlayerCharacter` (`BP_PlayerCharacter`).
2. Selecione o `Combo` component no painel Components.
3. Expanda **DF | Combo | Animation**.
4. Configure:
   - `Combo Chain Montage Blend In Time` = **0.12** (default)
   - `Combo Chain Montage Stop Blend Out Time` = **0.10**
   - `Combo Chain Blend Option` = **HermiteCubic**
5. **Override por arma:** No Data Table de itens (`DT_Items`), em cada `FDFComboStep`:
   - Para **golpe de abertura** (rápido, snappy): `ChainBlendInTime` = 0.06 + `ChainBlendOption` = Linear
   - Para **golpes intermediários** (fluído): `ChainBlendInTime` = 0.12 + `ChainBlendOption` = HermiteCubic
   - Para **finisher** (peso, antecipação): `ChainBlendInTime` = 0.18 + `ChainBlendOption` = Cubic
6. Salve e teste com `df.DebugCombat 1` — o overlay vai mostrar o `runtimeBlendIn` aplicado.

### ④ Validação

- **Olhômetro:** Encadeie 3 ataques. O frame de transição deve parecer "respirar" em vez de cortar.
- **Console:** `df.DebugCombat 1` mostra `last chain runtimeBlendIn=0.120`.
- **Profiler:** No Anim Insights (Window → Developer Tools → Animation Insights), grave 5s de combo. Procure por gaps de peso na curva do slot — devem ser smooth, sem degraus.

---

## 3. P0 — Rate-scale local no impacto (impact framing)

### ① Por que importa
Você já tem hit-stop **global** ([`UDFHitStopSubsystem`](../../Source/DungeonForged/Public/FX/UDFHitStopSubsystem.h)) com bands tipadas. AAA top-tier faz **rate-scale no montage do atacante por 1-2 frames** independentemente do hit-stop global — isso dá a sensação de "peso do golpe entrando na carne" sem congelar a câmera/mundo. DMC5 chama isso de "impact freeze". Bayonetta combina hit-stop + rate-scale + screen shake.

### ② Código C++

**Conceito:** quando o MeleeTrace confirma um hit, fazer um `Montage_SetPlayRate(0.05f, ...)` por ~2 frames no montage atual do atacante, depois restaurar para 1.0.

**Arquivo novo:** `Source/DungeonForged/Public/FX/UDFImpactFramingComponent.h`

```cpp
// Source/DungeonForged/Public/FX/UDFImpactFramingComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UDFImpactFramingComponent.generated.h"

class UAnimMontage;

/**
 * Per-actor montage rate-scale on confirmed hits. Independent from global hit-stop —
 * gives the attacker a 1–2 frame "weight pause" without freezing the world.
 * Composes safely with UDFHitStopSubsystem (both can fire on the same hit).
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFImpactFramingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDFImpactFramingComponent();

    /** Duration (wall-clock seconds) of the rate scale window. ~0.03–0.06 typical. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|ImpactFraming", meta = (ClampMin = "0.0", ClampMax = "0.20"))
    float LightFreezeDuration = 0.04f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|ImpactFraming", meta = (ClampMin = "0.0", ClampMax = "0.20"))
    float HeavyFreezeDuration = 0.07f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|ImpactFraming", meta = (ClampMin = "0.0", ClampMax = "0.20"))
    float CriticalFreezeDuration = 0.10f;

    /** Play rate during the freeze (0.05 = near-stop, 0.25 = slow-mo). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|ImpactFraming", meta = (ClampMin = "0.01", ClampMax = "0.5"))
    float FreezeRate = 0.05f;

    UFUNCTION(BlueprintCallable, Category = "DF|ImpactFraming")
    void TriggerLight()    { TriggerCustom(LightFreezeDuration); }

    UFUNCTION(BlueprintCallable, Category = "DF|ImpactFraming")
    void TriggerHeavy()    { TriggerCustom(HeavyFreezeDuration); }

    UFUNCTION(BlueprintCallable, Category = "DF|ImpactFraming")
    void TriggerCritical() { TriggerCustom(CriticalFreezeDuration); }

    /** Custom duration. Restores prior rate when window expires. */
    UFUNCTION(BlueprintCallable, Category = "DF|ImpactFraming")
    void TriggerCustom(float Duration);

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void RestoreRate();
    class UAnimInstance* GetAnimInstance() const;

    FTimerHandle RestoreTimer;
    TWeakObjectPtr<UAnimMontage> FrozenMontage;
    float PriorRate = 1.f;
    bool bRateActive = false;
};
```

**Arquivo novo:** `Source/DungeonForged/Private/FX/UDFImpactFramingComponent.cpp`

```cpp
// Source/DungeonForged/Private/FX/UDFImpactFramingComponent.cpp
#include "FX/UDFImpactFramingComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"

UDFImpactFramingComponent::UDFImpactFramingComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

UAnimInstance* UDFImpactFramingComponent::GetAnimInstance() const
{
    if (const ACharacter* const C = Cast<ACharacter>(GetOwner()))
    {
        if (USkeletalMeshComponent* const M = C->GetMesh())
        {
            return M->GetAnimInstance();
        }
    }
    return nullptr;
}

void UDFImpactFramingComponent::TriggerCustom(const float Duration)
{
    if (Duration <= 0.f || bRateActive)
    {
        return;
    }
    UAnimInstance* const Anim = GetAnimInstance();
    if (!Anim)
    {
        return;
    }
    UAnimMontage* const Active = Anim->GetCurrentActiveMontage();
    if (!Active)
    {
        return;
    }
    PriorRate = Anim->Montage_GetPlayRate(Active);
    if (PriorRate <= 0.f)
    {
        PriorRate = 1.f;
    }
    Anim->Montage_SetPlayRate(Active, FreezeRate);
    FrozenMontage = Active;
    bRateActive = true;

    if (UWorld* const W = GetWorld())
    {
        W->GetTimerManager().SetTimer(
            RestoreTimer,
            FTimerDelegate::CreateUObject(this, &UDFImpactFramingComponent::RestoreRate),
            Duration,
            false);
    }
}

void UDFImpactFramingComponent::RestoreRate()
{
    if (!bRateActive)
    {
        return;
    }
    bRateActive = false;
    UAnimInstance* const Anim = GetAnimInstance();
    UAnimMontage* const Montage = FrozenMontage.Get();
    if (Anim && Montage && Anim->Montage_IsPlaying(Montage))
    {
        Anim->Montage_SetPlayRate(Montage, PriorRate);
    }
    FrozenMontage.Reset();
}

void UDFImpactFramingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* const W = GetWorld())
    {
        W->GetTimerManager().ClearTimer(RestoreTimer);
    }
    RestoreRate();
    Super::EndPlay(EndPlayReason);
}
```

**Integração no MeleeTrace:** em [`UDFMeleeTraceComponent::ApplyDamageToTarget`](../../Source/DungeonForged/Private/Combat/UDFMeleeTraceComponent.cpp), adicione no início (após validação):

```cpp
// Dentro de ApplyDamageToTarget, depois de validar Target
if (AActor* const Owner = GetOwner())
{
    if (UDFImpactFramingComponent* const Framing =
            Owner->FindComponentByClass<UDFImpactFramingComponent>())
    {
        if (bHeavySwingActive)
        {
            Framing->TriggerHeavy();
        }
        else
        {
            Framing->TriggerLight();
        }
    }
}
```

E no `ADFPlayerCharacter::ADFPlayerCharacter()` (e qualquer inimigo que faça melee):

```cpp
ImpactFraming = CreateDefaultSubobject<UDFImpactFramingComponent>(TEXT("ImpactFraming"));
```

### ③ Setup no Editor

1. Rebuild C++ depois de criar os arquivos.
2. Em `BP_PlayerCharacter`, confirme que apareceu o componente `ImpactFraming`.
3. Selecione ele → painel Details → categoria **DF | ImpactFraming**:
   - `Light Freeze Duration` = **0.04** (2 frames a 60fps)
   - `Heavy Freeze Duration` = **0.07** (4 frames)
   - `Critical Freeze Duration` = **0.10** (6 frames)
   - `Freeze Rate` = **0.05**
4. Adicione o mesmo componente em `BP_EnemyBase` (todos os inimigos melee herdam o feel).
5. Teste batendo no inimigo. Você deve sentir **peso** no atacante, mas o mundo continua rodando normalmente (diferente do hit-stop global).

### ④ Validação

- **Olhômetro:** O movimento do swing "trava" 2-4 frames no momento do contato. O resto da cena segue.
- **Console:** Adicione um log dentro de `TriggerCustom` (`UE_LOG(LogDungeonForged, Verbose, TEXT("ImpactFreeze: %.3fs at rate %.2f"), Duration, FreezeRate);`) e rode `Log LogDungeonForged Verbose`.
- **Frame trace:** Anim Insights → grave o swing inteiro. Você verá um plateau na timeline durante a janela de freeze.

---

## 4. P1 — Style Rating System (S/A/B/C/D)

### ① Por que importa
DMC, Bayonetta, e Hi-Fi Rush gamificam o combate com um **rating em tempo real** baseado em:
- **Variedade** (não repetir o mesmo move)
- **Não-tomar-dano**
- **Combos longos**
- **Parry / dodge sucessos**

Isso vicia o jogador a explorar o moveset todo em vez de spammar o mesmo combo. Você já tem `UDFComboPointsComponent` (currency style) mas falta o **rating**.

### ② Código C++

**Arquivo novo:** `Source/DungeonForged/Public/Combat/UDFStyleRatingComponent.h`

```cpp
// Source/DungeonForged/Public/Combat/UDFStyleRatingComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "UDFStyleRatingComponent.generated.h"

UENUM(BlueprintType)
enum class EDFStyleRank : uint8
{
    D, C, B, A, S, SS, SSS
};

USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFStyleEvent
{
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag MoveTag;    // e.g. Combat.Move.LightCombo.Step0

    UPROPERTY()
    float TimeSeconds = 0.f;

    UPROPERTY()
    float ScoreDelta = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStyleRankChanged, EDFStyleRank, NewRank, float, Score);

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFStyleRatingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDFStyleRatingComponent();

    /** Score thresholds for each rank (sorted ascending). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Style")
    TArray<float> RankThresholds = { 0.f, 50.f, 120.f, 240.f, 400.f, 600.f, 900.f };

    /** Score decays by this much per second when no hits land. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Style", meta = (ClampMin = "0.0"))
    float DecayPerSecond = 30.f;

    /** Repeating the same move within RepeatPenaltyWindow gives only this fraction of points. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Style", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RepeatPenaltyMultiplier = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Style", meta = (ClampMin = "0.5", ClampMax = "10.0"))
    float RepeatPenaltyWindow = 2.5f;

    /** Drop one full rank on damage received. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DF|Style")
    bool bDropOnDamage = true;

    UPROPERTY(BlueprintAssignable, Category = "DF|Style")
    FOnStyleRankChanged OnStyleRankChanged;

    UFUNCTION(BlueprintPure, Category = "DF|Style")
    EDFStyleRank GetCurrentRank() const { return CurrentRank; }

    UFUNCTION(BlueprintPure, Category = "DF|Style")
    float GetCurrentScore() const { return Score; }

    /** Add an event. Variety + base value drive the actual delta. */
    UFUNCTION(BlueprintCallable, Category = "DF|Style")
    void RecordMove(FGameplayTag MoveTag, float BaseValue);

    UFUNCTION(BlueprintCallable, Category = "DF|Style")
    void RecordParry()       { RecordRaw(40.f); }

    UFUNCTION(BlueprintCallable, Category = "DF|Style")
    void RecordDodgeFlawless() { RecordRaw(25.f); }

    /** Damage received: drops rank if bDropOnDamage. */
    UFUNCTION(BlueprintCallable, Category = "DF|Style")
    void NotifyDamageReceived(float Amount);

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    void RecordRaw(float Delta);
    void RecalculateRank();

    float Score = 0.f;
    EDFStyleRank CurrentRank = EDFStyleRank::D;
    TArray<FDFStyleEvent> RecentEvents;
};
```

A implementação (`.cpp`) é direta — tick decai, `RecordMove` aplica multiplicador de repetição comparando com `RecentEvents`, `RecalculateRank` faz binary search nos thresholds.

**Integração:** no `OnDirectMontageEnded` ou no hit confirm do MeleeTrace, faça:

```cpp
if (UDFStyleRatingComponent* const Style = Owner->FindComponentByClass<UDFStyleRatingComponent>())
{
    FGameplayTag MoveTag = FGameplayTag::RequestGameplayTag(FName(
        *FString::Printf(TEXT("Combat.Move.LightCombo.Step%d"), Combo->CurrentComboStep)));
    Style->RecordMove(MoveTag, 15.f);
}
```

E no `UGE_Damage_Physical` (ou no `UDFAttributeSet::PostGameplayEffectExecute`):

```cpp
if (UDFStyleRatingComponent* const Style = Owner->FindComponentByClass<UDFStyleRatingComponent>())
{
    Style->NotifyDamageReceived(DamageDelta);
}
```

### ③ Setup no Editor

1. Adicione `UDFStyleRatingComponent` ao `BP_PlayerCharacter` (não em inimigos).
2. Configure thresholds — sugestão balanceada:
   - D = 0, C = 50, B = 120, A = 240, S = 400, SS = 600, SSS = 900
3. `Decay Per Second` = **30** (jogador agressivo se mantém em A+)
4. `Repeat Penalty Multiplier` = **0.25** (4 hits do mesmo move = quase nada)
5. `Repeat Penalty Window` = **2.5s**
6. `Drop On Damage` = **true** (perde 1 rank ao ser atingido — punição leve, não destrutiva)
7. **Crie a UI:**
   - Crie `WBP_StyleRatingWidget` em `Content/UI/Combat/`
   - Adicione um `TextBlock` chamado `RankText`
   - No graph, bind ao evento `OnStyleRankChanged` do componente
   - Anime o text com Material/Animation por rank (SSS vibra, S brilha dourado, etc.)
8. Adicione o widget ao HUD em `ADFHUDBase`.
9. **Crie as tags**: em `Config/DefaultGameplayTags.ini` ou no `DFGameplayTagsManager`:
   ```ini
   +GameplayTagList=(Tag="Combat.Move.LightCombo.Step0", DevComment="Combo step 0 - light")
   +GameplayTagList=(Tag="Combat.Move.LightCombo.Step1", DevComment="...")
   +GameplayTagList=(Tag="Combat.Move.HeavyAttack",      DevComment="Heavy attack")
   +GameplayTagList=(Tag="Combat.Move.Dodge.Flawless",   DevComment="Perfect dodge")
   +GameplayTagList=(Tag="Combat.Move.Parry",            DevComment="Parry success")
   ```

### ④ Validação

- **Spam-test:** Aperte só light attack 20x — o rank deve subir rápido até C ou B e travar (penalty de repetição).
- **Variety-test:** Misture L/L/H/L/H/charged — o rank deve estourar pra S+ em ~10s.
- **Damage-test:** Tome um hit em SSS — deve cair pra SS.
- **Idle-test:** Pare por 5s em A — deve cair pra B.

---

## 5. P1 — Variantes múltiplas por step (variety)

### ① Por que importa
Hoje cada `FDFComboStep` tem **1 light + 1 heavy**. Com 3 steps você tem 6 montages totais (3 light + 3 heavy). DMC5/Bayonetta tem 30+ no moveset básico. A diferença não é só "mais conteúdo" — é que o **jogo escolhe variante baseado em direção, alvo, contexto**, evitando repetição visual.

### ② Código C++

Estenda `FDFComboStep` em [`DFDataTableStructs.h`](../../Source/DungeonForged/Public/Data/DFDataTableStructs.h):

```cpp
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFComboVariant
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
    TObjectPtr<UAnimMontage> Montage = nullptr;

    /** 0..1, higher = more often picked. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Weight = 1.f;

    /** Only pick this variant when these tags are present on the attacker (e.g. State.Crouching). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
    FGameplayTagContainer RequiredAttackerTags;

    /** Only pick this variant when these tags are present on the resolved target. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
    FGameplayTagContainer RequiredTargetTags;

    /** Don't pick if attacker has any of these tags. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
    FGameplayTagContainer BlockedAttackerTags;
};

USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFComboStep
{
    GENERATED_BODY()

    /** @deprecated kept for migration; if empty, LightVariants is used. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
    TObjectPtr<UAnimMontage> LightMontage = nullptr;

    /** New: weighted variants with tag filters. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
    TArray<FDFComboVariant> LightVariants;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
    TObjectPtr<UAnimMontage> HeavyBranchMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
    TArray<FDFComboVariant> HeavyBranchVariants;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo", meta = (ClampMin = "-1.0"))
    float ChainBlendInTime = -1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo", meta = (ClampMin = "-1.0"))
    float ChainBlendOutTime = -1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
    TEnumAsByte<EAlphaBlendOption> ChainBlendOption = EAlphaBlendOption::Type(255);
};
```

Função de resolução em [`UDFComboComponent`](../../Source/DungeonForged/Public/Combat/UDFComboComponent.h):

```cpp
// UDFComboComponent.h
UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
UAnimMontage* PickComboVariant(const TArray<FDFComboVariant>& Variants) const;
```

```cpp
// UDFComboComponent.cpp
UAnimMontage* UDFComboComponent::PickComboVariant(const TArray<FDFComboVariant>& Variants) const
{
    if (Variants.Num() == 0)
    {
        return nullptr;
    }
    const AActor* const Owner = GetOwner();
    const ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(Owner);
    const UAbilitySystemComponent* const ASC = PC ? PC->GetAbilitySystemComponent() : nullptr;

    FGameplayTagContainer AttackerTags;
    if (ASC)
    {
        ASC->GetOwnedGameplayTags(AttackerTags);
    }

    AActor* TargetActor = nullptr;
    if (PC && PC->MeleeAim)
    {
        TargetActor = PC->MeleeAim->ResolveCurrentTarget();
    }
    FGameplayTagContainer TargetTags;
    if (const IAbilitySystemInterface* const TargetASI = Cast<IAbilitySystemInterface>(TargetActor))
    {
        if (UAbilitySystemComponent* const TargetASC = TargetASI->GetAbilitySystemComponent())
        {
            TargetASC->GetOwnedGameplayTags(TargetTags);
        }
    }

    TArray<int32> EligibleIdx;
    float WeightSum = 0.f;
    for (int32 i = 0; i < Variants.Num(); ++i)
    {
        const FDFComboVariant& V = Variants[i];
        if (!V.Montage) continue;
        if (!V.RequiredAttackerTags.IsEmpty() && !AttackerTags.HasAll(V.RequiredAttackerTags)) continue;
        if (!V.BlockedAttackerTags.IsEmpty()  &&  AttackerTags.HasAny(V.BlockedAttackerTags)) continue;
        if (!V.RequiredTargetTags.IsEmpty()   && !TargetTags.HasAll(V.RequiredTargetTags))   continue;
        EligibleIdx.Add(i);
        WeightSum += V.Weight;
    }
    if (EligibleIdx.Num() == 0 || WeightSum <= 0.f)
    {
        return Variants[0].Montage; // safe fallback
    }
    float Roll = FMath::FRand() * WeightSum;
    for (const int32 Idx : EligibleIdx)
    {
        Roll -= Variants[Idx].Weight;
        if (Roll <= 0.f) return Variants[Idx].Montage;
    }
    return Variants.Last().Montage;
}
```

Use em `ResolveDirectionalComboMontage`:

```cpp
// substitua o branch "ComboSteps.IsValidIndex(Step)" para preferir variantes:
if (ComboSteps.IsValidIndex(Step))
{
    const FDFComboStep& StepData = ComboSteps[Step];
    if (bComboHeavyFinisherPending && StepData.HeavyBranchVariants.Num() > 0)
    {
        if (UAnimMontage* Pick = PickComboVariant(StepData.HeavyBranchVariants))
            return Pick;
    }
    if (bComboHeavyFinisherPending && StepData.HeavyBranchMontage)
    {
        return StepData.HeavyBranchMontage;
    }
    if (StepData.LightVariants.Num() > 0)
    {
        if (UAnimMontage* Pick = PickComboVariant(StepData.LightVariants))
            return Pick;
    }
    if (StepData.LightMontage)
    {
        return StepData.LightMontage;
    }
}
// ... resto inalterado
```

### ③ Setup no Editor

1. Abra `DT_Items` → linha da arma (ex: `Sword_Iron`).
2. Em `Weapon Melee Combo Steps`, para cada step:
   - **Step 0 (abertura):** 2 variantes — `M_Combo_01_Horizontal` (peso 1.0) e `M_Combo_01_Diagonal` (peso 0.5). Ambas sem tag filter.
   - **Step 1 (mid):** 3 variantes — `M_Combo_02_Spin` (1.0), `M_Combo_02_Crouch` (1.5, filtra `State.Crouching`), `M_Combo_02_OnTarget_Staggered` (2.0, filtra target `State.Staggered`).
   - **Step 2 (finisher):** 1 variante padrão + 1 cinemática filtrada por `Combat.Target.LowHealth` (você precisa setar essa tag quando alvo < 25%).
3. Salve, reabra para garantir que persistiu.
4. Tag dependencies: rode em `Config/DefaultGameplayTags.ini`:
   ```ini
   +GameplayTagList=(Tag="State.Crouching",         DevComment="Player is crouching")
   +GameplayTagList=(Tag="State.Staggered",         DevComment="Target is staggered")
   +GameplayTagList=(Tag="Combat.Target.LowHealth", DevComment="Target HP < 25%")
   ```

### ④ Validação

- Bate 10x no mesmo inimigo. Os Step 0 e Step 1 devem variar visualmente.
- Agache (input de crouch) e ataque — deve usar `M_Combo_02_Crouch`.
- Stagger o inimigo e ataque — deve usar a variante com filter de stagger.

---

## 6. P2 — Juggle / Launcher System

### ① Por que importa
Aerial combos são **identidade visual do gênero character action**. DMC tem "Stinger → High Time → Aerial Rave". Bayonetta tem "Witch Twist" juggle. Mesmo God of War 2018 (mais grounded) tem launchers no Hyperion Grip. Sem isso, o combate fica preso no chão.

### ② Código C++

**Estrutura no `FDFComboStep`:**

```cpp
// FDFComboStep — adicione
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Launch")
bool bIsLauncher = false;

/** Launch velocity applied to target on hit (local-space relative to attacker). */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Launch", meta = (EditCondition = "bIsLauncher"))
FVector LaunchVelocity = FVector(0.f, 0.f, 700.f);

/** Self-launch (attacker follows up to air): set Z > 0 for "Jump Cancel" launchers. */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Launch", meta = (EditCondition = "bIsLauncher"))
FVector SelfLaunchVelocity = FVector::ZeroVector;

/** Reduce target gravity by this fraction for N seconds. 0 = normal gravity. */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Launch", meta = (EditCondition = "bIsLauncher", ClampMin = "0.0", ClampMax = "1.0"))
float TargetGravityScale = 0.15f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Launch", meta = (EditCondition = "bIsLauncher", ClampMin = "0.1"))
float HangtimeSeconds = 1.5f;
```

**Componente novo (no atacante):** `UDFLauncherComponent` — ouve `OnHitConfirmed` do MeleeTrace, lê o `FDFComboStep` ativo, aplica launch.

```cpp
// Source/DungeonForged/Public/Combat/UDFLauncherComponent.h
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DUNGEONFORGED_API UDFLauncherComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    /** Apply launch to target (server-authoritative). */
    UFUNCTION(BlueprintCallable, Category = "DF|Launcher")
    void ApplyLaunch(AActor* Target, const FVector& LaunchVel, float TargetGravity, float Hangtime);

    /** Apply self-launch to attacker (jump-cancel). */
    UFUNCTION(BlueprintCallable, Category = "DF|Launcher")
    void ApplySelfLaunch(const FVector& SelfVel);

    /** Restore gravity after hangtime expires. */
    void RestoreTargetGravity(TWeakObjectPtr<ACharacter> TargetChar, float PriorGravity);
};
```

```cpp
// Source/DungeonForged/Private/Combat/UDFLauncherComponent.cpp
void UDFLauncherComponent::ApplyLaunch(AActor* const Target, const FVector& LaunchVel,
    const float TargetGravity, const float Hangtime)
{
    ACharacter* const TargetChar = Cast<ACharacter>(Target);
    if (!TargetChar) return;
    UCharacterMovementComponent* const CMC = TargetChar->GetCharacterMovement();
    if (!CMC) return;

    const AActor* const Owner = GetOwner();
    const FVector WorldVel = Owner
        ? Owner->GetActorTransform().TransformVectorNoScale(LaunchVel)
        : LaunchVel;
    TargetChar->LaunchCharacter(WorldVel, /*bXY*/ true, /*bZ*/ true);

    if (TargetGravity > 0.f && TargetGravity < 1.f)
    {
        const float Prior = CMC->GravityScale;
        CMC->GravityScale = TargetGravity;
        FTimerHandle TH;
        GetWorld()->GetTimerManager().SetTimer(TH,
            FTimerDelegate::CreateUObject(this, &UDFLauncherComponent::RestoreTargetGravity,
                TWeakObjectPtr<ACharacter>(TargetChar), Prior),
            Hangtime, false);
    }
}

void UDFLauncherComponent::RestoreTargetGravity(TWeakObjectPtr<ACharacter> TargetChar, const float PriorGravity)
{
    if (TargetChar.IsValid())
    {
        if (UCharacterMovementComponent* const CMC = TargetChar->GetCharacterMovement())
        {
            CMC->GravityScale = PriorGravity;
        }
    }
}

void UDFLauncherComponent::ApplySelfLaunch(const FVector& SelfVel)
{
    ACharacter* const Char = Cast<ACharacter>(GetOwner());
    if (!Char) return;
    const FVector WorldVel = Char->GetActorTransform().TransformVectorNoScale(SelfVel);
    Char->LaunchCharacter(WorldVel, true, true);
}
```

**Wire no MeleeTrace:** após `ApplyDamageToTarget`, se o step ativo tem `bIsLauncher`:

```cpp
// Dentro de ProcessHitResults após aplicar dano
if (UDFComboComponent* const Combo = Owner->FindComponentByClass<UDFComboComponent>())
{
    const int32 Step = Combo->CurrentComboStep;
    if (Combo->ComboSteps.IsValidIndex(Step) && Combo->ComboSteps[Step].bIsLauncher)
    {
        const FDFComboStep& S = Combo->ComboSteps[Step];
        if (UDFLauncherComponent* const Launcher = Owner->FindComponentByClass<UDFLauncherComponent>())
        {
            Launcher->ApplyLaunch(HitActor, S.LaunchVelocity, S.TargetGravityScale, S.HangtimeSeconds);
            if (!S.SelfLaunchVelocity.IsNearlyZero())
            {
                Launcher->ApplySelfLaunch(S.SelfLaunchVelocity);
            }
        }
    }
}
```

**Combos aéreos:** crie um novo array `AerialComboSteps` em [`UDFComboComponent`](../../Source/DungeonForged/Public/Combat/UDFComboComponent.h):

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo|Aerial")
TArray<FDFComboStep> AerialComboSteps;

UFUNCTION(BlueprintPure, Category = "Combat|Combo|Aerial")
bool IsOwnerAirborne() const;
```

Em `ResolveDirectionalComboMontage`, prefira `AerialComboSteps[Step]` quando `IsOwnerAirborne()` retornar true.

### ③ Setup no Editor

1. Adicione `UDFLauncherComponent` em `BP_PlayerCharacter`.
2. Crie 3 montages para combo aéreo: `M_Aerial_01`, `M_Aerial_02`, `M_Aerial_03_Slam`.
3. No `DT_Items`, na arma, expanda **Aerial Combo Steps**:
   - Step 0: M_Aerial_01, sem launcher
   - Step 1: M_Aerial_02, sem launcher
   - Step 2 (slam): M_Aerial_03_Slam + `bIsLauncher=true` + `LaunchVelocity (0, 0, -1500)` (slam pra baixo) + `SelfLaunchVelocity (0, 0, -800)` (jogador desce junto)
4. **Launcher no grounded:** marque o **Step 2 do combo terrestre** com `bIsLauncher=true` + `LaunchVelocity (0,0,700)` + `SelfLaunchVelocity (0,0,650)` + `TargetGravityScale=0.15` + `HangtimeSeconds=1.5`.
5. Input: o jogo deve reconhecer "estou no ar → use `AerialComboSteps`". Isso já vem de `IsOwnerAirborne()`.

### ④ Validação

- **Grounded launcher:** L→L→H (heavy finisher do step 2). Inimigo voa pra cima, jogador acompanha.
- **Aerial combo:** continue apertando attack no ar — deve trocar pra `AerialComboSteps`.
- **Slam:** Heavy aéreo no step 2 — inimigo bate no chão.

---

## 7. P2 — Cancel Hierarchy (prioridade direcional)

### ① Por que importa
Hoje qualquer ability com tag no `AllowedAbilityCancelTags` cancela durante a janela. AAA usa **hierarquia**:
- Light → Heavy: OK (DMC clássico)
- Heavy → Light: bloqueado durante recovery (peso do golpe)
- Anything → Dodge: SEMPRE OK (i-frames sagrados)
- Whiff (não acertou) vs Hit-confirm: prioridades diferentes

### ② Código C++

Estrutura nova em [`UDFCombatTuningData`](../../Source/DungeonForged/Public/Data/UDFCombatTuningData.h):

```cpp
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFCancelRule
{
    GENERATED_BODY()

    /** When the ACTIVE ability has any of these tags... */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cancel")
    FGameplayTagContainer FromAbilityTags;

    /** ...it can be canceled by abilities with these tags. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cancel")
    FGameplayTagContainer AllowedTargetTags;

    /** Only if the swing already confirmed a hit (true), or only on whiff (false), or both (default both). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cancel")
    bool bRequireHitConfirmed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cancel")
    bool bAllowOnWhiff = true;
};

// adicione em UDFCombatTuningData
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cancel")
TArray<FDFCancelRule> CancelRules;
```

Reescreva `IsAbilityCancellable` em [`UDFComboComponent.cpp`](../../Source/DungeonForged/Private/Combat/UDFComboComponent.cpp):

```cpp
bool UDFComboComponent::IsAbilityCancellable(const FGameplayTagContainer& AbilityTags) const
{
    if (!bAbilityCancelWindowActive || AbilityTags.IsEmpty())
    {
        return false;
    }
    // Legacy path (manual override via SetAbilityCancelWindow)
    if (!AllowedAbilityCancelTags.IsEmpty() && AbilityTags.HasAny(AllowedAbilityCancelTags))
    {
        return true;
    }
    // Rule-table path
    const UDFCombatTuningData* const Tuning = UDFAssetManager::Get().GetCombatTuningData();
    if (!Tuning) return false;

    const ADFPlayerCharacter* const PC = Cast<ADFPlayerCharacter>(GetOwner());
    const UAbilitySystemComponent* const ASC = PC ? PC->GetAbilitySystemComponent() : nullptr;
    if (!ASC) return false;

    FGameplayTagContainer ActiveAbilityTags;
    for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
    {
        if (Spec.IsActive() && Spec.Ability)
        {
            ActiveAbilityTags.AppendTags(Spec.Ability->AbilityTags);
        }
    }
    const bool bHitConfirmed = bSwingHitConfirmedThisActivation; // see below

    for (const FDFCancelRule& Rule : Tuning->CancelRules)
    {
        if (Rule.FromAbilityTags.IsEmpty() || !ActiveAbilityTags.HasAny(Rule.FromAbilityTags)) continue;
        if (!Rule.AllowedTargetTags.IsEmpty() && !AbilityTags.HasAny(Rule.AllowedTargetTags)) continue;
        if (Rule.bRequireHitConfirmed && !bHitConfirmed) continue;
        if (!Rule.bAllowOnWhiff && !bHitConfirmed) continue;
        return true;
    }
    return false;
}
```

Adicione o tracker de hit-confirmed:

```cpp
// UDFComboComponent.h — protected
bool bSwingHitConfirmedThisActivation = false;

// UDFComboComponent.cpp — em NotifyOwnerHitConfirmed, no início
bSwingHitConfirmedThisActivation = true;

// em NotifyAbilitySwingMontageStarted, no início
bSwingHitConfirmedThisActivation = false;
```

### ③ Setup no Editor

1. Abra o Data Asset `DA_CombatTuning` (criar se não existir, em `Content/Data/`).
2. Expanda **Cancel** → **Cancel Rules**.
3. Adicione regras (exemplos):

| From | Allowed Target | Hit Confirmed | On Whiff | Significado |
|---|---|---|---|---|
| `Ability.Attack.Melee.Light` | `Ability.Attack.Melee.Heavy` | false | true | Light → Heavy sempre |
| `Ability.Attack.Melee.Heavy` | `Ability.Attack.Melee.Light` | **true** | false | Heavy → Light **só se acertou** |
| `Ability.Attack.Melee` | `Ability.Movement.Dodge` | false | true | Dodge cancela qualquer coisa |
| `Ability.Attack.Melee` | `Ability.Parry` | false | true | Parry cancela qualquer coisa |

### ④ Validação

- L→H: deve sempre cancelar.
- H→L em whiff (atacou ar): deve **bloquear** (sente o peso).
- H→L em hit (acertou inimigo): deve cancelar.
- Qualquer→Dodge: sempre cancela.

---

## 8. P3 — Anim Curve em vez de AnimNotify

### ① Por que importa
Hoje o combo window depende do animator colocar `AN_ComboWindowOpen` no momento certo da timeline. Se re-importar a animação ou ajustar timing, o notify pode ficar fora de posição. AAA prefere **Anim Curves** ("ComboWindow" como float 0→1 ao longo da animação) — sobrevive a re-import e permite blend entre frames (ex: 0.5 = "metade aberto").

### ② Código C++

Adicione em [`UDFComboComponent`](../../Source/DungeonForged/Public/Combat/UDFComboComponent.h):

```cpp
/** Curve name read from active montage each tick. > Threshold = window open. */
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Curve")
FName ComboWindowCurveName = TEXT("ComboWindow");

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Curve", meta = (ClampMin = "0.0", ClampMax = "1.0"))
float ComboWindowCurveThreshold = 0.5f;

/** When true, falls back to AN_ComboWindowOpen notify; when false, only curve. */
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo|Curve")
bool bUseCurveInsteadOfNotify = false;
```

No `TickComponent`:

```cpp
void UDFComboComponent::TickComponent(const float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    TryAdvanceComboBranchFromHold();
    EvaluateComboCurveWindow();
    // ... resto
}

void UDFComboComponent::EvaluateComboCurveWindow()
{
    if (!bUseCurveInsteadOfNotify || !bPlayingComboMontage) return;
    UAnimInstance* const Anim = GetAnimInstance();
    if (!Anim) return;
    const float Val = Anim->GetCurveValue(ComboWindowCurveName);
    const bool bShouldOpen = Val >= ComboWindowCurveThreshold;
    if (bShouldOpen && !bComboWindowActive)
    {
        AdvanceCombo(TEXT("CurveOpen"), Anim->GetCurrentActiveMontage());
    }
    else if (!bShouldOpen && bComboWindowActive && Val < ComboWindowCurveThreshold * 0.5f)
    {
        // Hysteresis: only close when significantly below threshold
        bComboWindowActive = false;
    }
}
```

### ③ Setup no Editor

1. Em cada AnimMontage de combo (`M_Combo_01_Light`, etc.):
   - Abra a montage.
   - Window → Anim Curves.
   - Click **+ Add Curve** → nome: `ComboWindow` → tipo: **Float**.
   - Na timeline da curve, adicione 4 keys:
     - Frame inicial: 0.0
     - Frame onde abre a janela (ex: 60% da anim): 0.0
     - Frame da abertura (ex: 65%): 1.0
     - Frame de fechamento (ex: 90%): 1.0
     - Frame final: 0.0
2. **NÃO remova** os AnimNotify ainda — mantenha como fallback.
3. No `BP_PlayerCharacter` → `Combo` component:
   - `Use Curve Instead Of Notify` = **true**
4. Se algum montage não tiver a curve, ele cai no notify automaticamente.

### ④ Validação

- Ataque normalmente — combo window abre exatamente no ponto que você definiu na curve.
- Re-importe o montage com timing diferente — curve segue o tempo proporcional sem precisar reposicionar nada.
- Console `df.DebugCombat 1`: o overlay mostra "CurveOpen" como source da abertura.

---

## 9. P3 — Robustez online (rollback, dessync)

### ① Por que importa
Em PvP ou coop com >150ms ping, o `LocalPredicted` da GA pode dessincar: cliente acha que está no step 3, server insiste no step 2. AAA online (For Honor, Mordhau) faz **rollback explícito** — server envia estado autorizado, cliente desfaz e refaz.

### ② Código C++

Adicione um `RepNotify` no `LockedComboActivationStep`:

```cpp
// UDFComboComponent.h
UPROPERTY(ReplicatedUsing = OnRep_LockedComboStep, BlueprintReadOnly, Category = "Combat|Combo")
int32 LockedComboActivationStep = -1;

UFUNCTION()
void OnRep_LockedComboStep(int32 PreviousValue);
```

```cpp
// UDFComboComponent.cpp
void UDFComboComponent::OnRep_LockedComboStep(const int32 PreviousValue)
{
    // Server says step N, but local prediction is at M. If they differ significantly, reconcile.
    if (LockedComboActivationStep < 0) return;
    if (LockedComboActivationStep == PreviousValue) return;

    const int32 LocalStep = CurrentComboStep;
    const int32 ServerStep = LockedComboActivationStep;
    if (FMath::Abs(LocalStep - ServerStep) >= 1)
    {
        // Rollback: stop local prediction, snap to server, replay from there.
        UE_LOG(LogDungeonForged, Warning,
            TEXT("[Combo|Rollback] local=%d server=%d → reconciling"), LocalStep, ServerStep);
        if (UAnimInstance* Anim = GetAnimInstance())
        {
            for (auto& M : ComboMontages)
            {
                if (M && Anim->Montage_IsPlaying(M))
                {
                    Anim->Montage_Stop(0.05f, M);
                }
            }
        }
        CurrentComboStep = ServerStep;
        PlayCurrentComboMontage();
    }
}

void UDFComboComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UDFComboComponent, bComboChainAdvancePending, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UDFComboComponent, LockedComboActivationStep, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UDFComboComponent, bComboHeavyFinisherPending, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UDFComboComponent, CurrentComboStep, COND_OwnerOnly); // NEW
}
```

E replicar `CurrentComboStep` também (era local até agora).

### ③ Setup no Editor

1. Em **Editor Preferences** → **Play** → habilite **Use Network Emulation Profile**.
2. Crie um perfil: **NetworkEmulationProfile** chamado "HighLatency":
   - Min Latency: 100ms
   - Max Latency: 200ms
   - Packet Loss: 2%
3. PIE com **2 players** (Window → Multiplayer Preview).
4. Aplique o profile.
5. Tente combo no client — server pode reconciliar.

### ④ Validação

- Em PIE local: combos funcionam idênticos.
- Em PIE com network emulation HighLatency: o jogador pode ver micro-snap (rollback) mas o estado final está correto.
- Log `[Combo|Rollback]` aparece no Output Log do client quando rollback acontece.

---

## 10. Ordem recomendada de execução

```
Sprint 1 (P0 — feel fundamental)         ~1 dia
├─ §2 Cross-fade HermiteCubic
└─ §3 Impact framing (rate scale local)

Sprint 2 (P1 — gamification + variety)   ~3 dias
├─ §4 Style Rating System
└─ §5 Variantes múltiplas por step

Sprint 3 (P2 — depth do moveset)         ~3 dias
├─ §6 Juggle / Launcher
└─ §7 Cancel Hierarchy

Sprint 4 (P3 — robustez)                 ~2 dias
├─ §8 Anim Curve
└─ §9 Rollback online
```

> **Total: ~9 dias de desenvolvimento focado.** Compatível com 1 dev fullstack. Cada sprint pode ser entregue isoladamente.

---

## 11. Console commands & validação

### Commands úteis

```
df.DebugCombat 1               # overlay de combo (step, window, blend)
df.DebugCombat 0               # off
df.meleedebug verbose          # log detalhado de trace
df.meleedebug dump             # one-shot dump (sockets, sweep)
ShowDebug Animation            # anim instance state
Anim.Insights                  # gravar timeline de anim (Window menu)
```

### Checklist de validação AAA

- [ ] Cross-fade visível e suave (HermiteCubic) — §2
- [ ] Impact framing perceptível mas mundo não congela — §3
- [ ] Style rating sobe com variedade, cai com repetição — §4
- [ ] Pelo menos 2 variantes por step usadas em runtime — §5
- [ ] Launcher do step 2 manda inimigo no ar, segue aerial combo — §6
- [ ] H→L em whiff bloqueado, em hit-confirmed permitido — §7
- [ ] Combo abre via curve mesmo após re-import de montage — §8
- [ ] Rollback log aparece em 200ms latency PIE — §9

### Game feel target

- **Time-to-hit:** < 100ms do press até frame de impacto no swing 1
- **Combo continuation:** < 150ms entre fim do impacto e próximo swing começar
- **Blend visible duration:** 100–180ms (HermiteCubic é mais "encorpado" que linear na mesma duração)
- **Hit-stop + impact framing combo total:** 60–140ms dependendo da band

---

## Arquivos referenciados

- [UDFComboComponent.h](../../Source/DungeonForged/Public/Combat/UDFComboComponent.h)
- [UDFComboComponent.cpp](../../Source/DungeonForged/Private/Combat/UDFComboComponent.cpp)
- [UDFAbility_Warrior_MeleeSwing.cpp](../../Source/DungeonForged/Private/GAS/Abilities/Warrior/UDFAbility_Warrior_MeleeSwing.cpp)
- [DFAnimCombatLibrary.h](../../Source/DungeonForged/Public/Combat/DFAnimCombatLibrary.h)
- [DFDataTableStructs.h](../../Source/DungeonForged/Public/Data/DFDataTableStructs.h)
- [UDFMeleeTraceComponent.h](../../Source/DungeonForged/Public/Combat/UDFMeleeTraceComponent.h)
- [UDFMeleeAimComponent.h](../../Source/DungeonForged/Public/Combat/UDFMeleeAimComponent.h)
- [UDFHitStopSubsystem.h](../../Source/DungeonForged/Public/FX/UDFHitStopSubsystem.h)
- [UDFCombatTuningData.h](../../Source/DungeonForged/Public/Data/UDFCombatTuningData.h)
- [UDFComboPointsComponent.h](../../Source/DungeonForged/Public/Combat/UDFComboPointsComponent.h)
- [10_AAA_AimWarpCombat.md](10_AAA_AimWarpCombat.md) — pré-requisito

---

> **Nota final:** este documento é vivo. Atualize os checkboxes da §11 conforme cada sprint fechar. Mantenha as referências de arquivo sincronizadas se renomear componentes.
