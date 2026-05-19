# 09 — Ability Hotbar & Input

> **Objetivo:** transformar a barra de habilidades em uma ferramenta **legível em 1 olhada**, **rebindável**, com cooldowns ricos, drag-and-drop fluido e tooltips informativos — estilo WoW/Diablo, sem clutter.

> Referência técnica completa: [`doc/Ability_Hotbar_GAS_Input.md`](../../doc/Ability_Hotbar_GAS_Input.md).
> Tipos: [`Source/DungeonForged/Public/UI/UDFAbilityHotbarWidget.h`](../../Source/DungeonForged/Public/UI/UDFAbilityHotbarWidget.h),
> [`Source/DungeonForged/Public/UI/UDFAbilitySlotWidget.h`](../../Source/DungeonForged/Public/UI/UDFAbilitySlotWidget.h),
> [`Source/DungeonForged/Public/Input/DFInputConfig.h`](../../Source/DungeonForged/Public/Input/DFInputConfig.h).

---

## Sumário rápido

| Eixo | Atual | Alvo | Esforço |
|---|---|---|---|
| Slots replicados | 4 (`CurrentAbilitySlots` init) | 12 (já suportado em C++, expandir default) | 5min |
| Slot widgets na UI | até 12 (`AbilitySlot1..12`) | 12 + grupos LMB/RMB visíveis | 1h |
| Input labels | `InputLabels` em `UPROPERTY` — vazio no WBP | auto-popular do `UDFInputRemappingSubsystem` | 2h |
| Cooldown visual | `M_CoolDown_Inst` (sweep) + texto | + flash quando 1s restante, + click-ready pulse | 1h |
| Tooltip | nenhum | hover delay 0.4s, descrição + custo + CD | 3h |
| Drag-and-drop | swap básico funcional | + ghost icon, snap haptic, sound | 2h |
| Ataque básico LMB | sempre fixo | mostrar slot dedicado e *highlighted* na barra | 30min |
| RMB ability | tag-driven (`RMBAbilityTryTags`) | slot RMB visível e bindável | 1h |
| Recharge flash | sem | flash branco curto quando CD termina | 30min |
| Out-of-resource | sem feedback | ícone vermelho + shake quando mana/stamina insuficiente | 1h |
| GCD (global cooldown) | sem | overlay sutil em todos os slots durante GCD 0.5s | 2h |
| Replication | `CurrentAbilitySlots` replicado | confirmar que `OnRep` dispara `RefreshHotbar` em todos os clients | 30min |

---

## 1. Slot count padrão — `[CONFIG]` <a id="slot-count"></a>

**Onde:** `ADFPlayerCharacter::CurrentAbilitySlots` é inicializado com **4 entradas** (ver `doc/Ability_Hotbar_GAS_Input.md` §1). O HUD já tem `AbilitySlot1..12` (BindWidgetOptional) em [`UDFAbilityHotbarWidget.h:45-79`](../../Source/DungeonForged/Public/UI/UDFAbilityHotbarWidget.h#L45).

### 1.1 Padronizar 12 slots

```cpp
// ADFPlayerCharacter.h (procurar onde CurrentAbilitySlots é declarado/initialized)
UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_AbilityBarSlots, Category="DF|Abilities")
TArray<FName> CurrentAbilitySlots;   // size = 12, NAME_None = empty

void ADFPlayerCharacter::ResetAbilityBarToDefault()
{
    CurrentAbilitySlots.Init(NAME_None, 12);   // [CONFIG] 12 = MaxBarSlots
}
```

E em `UDFRunManager::GrantAbilitiesForCurrentRun`, garantir que abilities sortadas para a run são distribuídas **a partir do slot 0**, mantendo livres os restantes para drag-and-drop manual.

### 1.2 Constante global

```cpp
// Source/DungeonForged/Public/UI/DFAbilityBarTypes.h
namespace DFAbilityBar
{
    static constexpr int32 MaxBarSlots = 12;
    static constexpr int32 LMBSlotIndex = INDEX_NONE;   // ataque básico não ocupa slot da barra
    static constexpr int32 RMBSlotIndex = INDEX_NONE;   // tag-driven
}
```

Todos os pontos que iteram slots usam essa constante (evita "magic 12").

---

## 2. Auto-popular input labels — `[CODE]` <a id="input-labels"></a>

**Onde:** [`UDFAbilityHotbarWidget.h:93`](../../Source/DungeonForged/Public/UI/UDFAbilityHotbarWidget.h#L93)

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|UI|Hotbar")
TArray<FText> InputLabels;   // hoje precisa ser populado no WBP
```

### 2.1 Problema

O artista/UI dev tem que preencher manualmente os textos `"1"`, `"2"`, ... `"="` no Blueprint. Quando o player rebinds (via `UDFInputRemappingSubsystem`), os labels **não atualizam**.

### 2.2 Solução: pull do remapping subsystem

```cpp
void UDFAbilityHotbarWidget::RebuildInputLabels()
{
    InputLabels.Reset();
    InputLabels.Reserve(DFAbilityBar::MaxBarSlots);

    UDFInputRemappingSubsystem* RM = GetGameInstance()->GetSubsystem<UDFInputRemappingSubsystem>();
    if (!RM)
    {
        for (int32 i = 0; i < DFAbilityBar::MaxBarSlots; ++i)
            InputLabels.Add(FText::AsNumber(i + 1));
        return;
    }

    static const FName SlotMappingPrefix("AbilityBar.Slot");
    for (int32 i = 0; i < DFAbilityBar::MaxBarSlots; ++i)
    {
        const FName MappingName(*FString::Printf(TEXT("%s%d"), *SlotMappingPrefix.ToString(), i + 1));
        FKey Key = RM->GetCurrentKeyForMapping(MappingName);
        InputLabels.Add(Key.IsValid() ? Key.GetDisplayName() : FText::FromString(TEXT("—")));
    }
}

void UDFAbilityHotbarWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RebuildInputLabels();
    if (UDFInputRemappingSubsystem* RM = GetGameInstance()->GetSubsystem<UDFInputRemappingSubsystem>())
    {
        RM->OnBindingsChanged.AddDynamic(this, &ThisClass::HandleBindingsChanged);
    }
}

UFUNCTION()
void HandleBindingsChanged() { RebuildInputLabels(); RefreshHotbar(); }
```

Resultado: rebind no Options → barra mostra o novo key automaticamente.

### 2.3 Glyphs por gamepad

Quando o último input foi gamepad (detect via `UEnhancedInputUserSettings::GetLastInputType()`), trocar `"1"` por imagem do botão correspondente (Xbox `Y`, PS `Triangle`).

```cpp
UPROPERTY(EditAnywhere, Category="DF|UI|Hotbar")
TObjectPtr<UDataTable> InputGlyphTable = nullptr;  // FKey → UTexture2D
```

`UDFAbilitySlotWidget::InputLabelText` → trocar por `UImage InputGlyphImage` quando o resolver retorna textura.

---

## 3. Cooldown polish — `[CODE/ASSET]` <a id="cooldown-polish"></a>

**Onde:** [`UDFAbilitySlotWidget.cpp`](../../Source/DungeonForged/Private/UI/UDFAbilitySlotWidget.cpp) — `UpdateCooldownVisuals`.

### 3.1 Adicionar 3 estados visuais

| Estado | Trigger | Visual |
|---|---|---|
| **Ready** | `Pct == 0` | ícone full color, leve glow se mouse hover |
| **On cooldown** | `0 < Pct ≤ 1` | radial sweep (já existe), número em segundos com 1 decimal se < 10s |
| **Almost ready** | `Pct ≤ 0.15` (≤ 15% restante) | overlay pulse + cor levemente amarela |
| **Just refreshed** | transition `Pct > 0 → 0` | flash branco 0.15s + sound `SFX_AbilityReady` |
| **Out of resource** | tem CD ready mas mana/stamina insuficiente | ícone tint vermelho, número de mana cintila |
| **GCD active** | Global cooldown ativo (0.5s pós-cast) | overlay sutil cinza translúcido em **todos** os slots |

```cpp
void UDFAbilitySlotWidget::UpdateCooldownVisuals()
{
    float const Pct = ComputeCooldownPercent();

    // Just-refreshed flash
    if (LastCooldownPct > 0.05f && Pct <= 0.f)
    {
        PlayReadyFlash();
        if (ReadySFX)
            UGameplayStatics::PlaySound2D(this, ReadySFX);
    }
    LastCooldownPct = Pct;

    if (CooldownOverlayMID)
    {
        CooldownOverlayMID->SetScalarParameterValue(CooldownMaterialParameter, Pct);
        CooldownOverlayMID->SetScalarParameterValue(CooldownAuxScalarParameter, Pct);
        // pulse amarelo quando Pct ≤ 0.15
        CooldownOverlayMID->SetScalarParameterValue(TEXT("AlmostReadyMix"), FMath::Max(0.f, 0.15f - Pct) / 0.15f);
    }

    if (CooldownText)
    {
        float const Remaining = ComputeCooldownRemainingSeconds();
        if (Remaining <= 0.f)
            CooldownText->SetText(FText::GetEmpty());
        else if (Remaining < 10.f)
            CooldownText->SetText(FText::AsNumber(FMath::RoundToInt(Remaining * 10.f) / 10.f));
        else
            CooldownText->SetText(FText::AsNumber(FMath::CeilToInt(Remaining)));
    }
}
```

### 3.2 Material `M_CoolDown_Inst` — adicionar params

| Param | Tipo | Função |
|---|---|---|
| `CooldownPercent` | scalar | já existe — sweep 0→1 |
| `AlmostReadyMix` | scalar | 0=sem mix; 1=tint amarelo (`#FFD24A`) |
| `ReadyFlash` | scalar | 0=normal; 1=branco; anim curva 0.15s pós-ready |
| `OutOfResourceMix` | scalar | 0=normal; 1=tint vermelho `#FF5050` |
| `GCDMix` | scalar | 0=clear; 1=cinza translúcido 30% opacidade |

### 3.3 Recharge flash anim

```cpp
void UDFAbilitySlotWidget::PlayReadyFlash()
{
    if (!CooldownOverlayMID) return;
    World->GetTimerManager().SetTimer(ReadyFlashTimer, [this]() {
        // anim 0 → 1 → 0 em 0.15s via Tick ou UMG anim
        ReadyFlashElapsed = 0.f;
        bReadyFlashActive = true;
    }, 0.001f, false);
}
```

Tick interpola `ReadyFlash` por 0.15s com curva sinusoidal.

---

## 4. Tooltip de habilidade — `[CODE/UI]` <a id="tooltip"></a>

### 4.1 Conteúdo do tooltip

```
┌────────────────────────────────────────┐
│  [Icon]  Fireball                       │
│          Active Ability — Fire          │
├────────────────────────────────────────┤
│  Hurls a ball of fire that explodes     │
│  on impact for 80 fire damage.          │
│                                          │
│  Cooldown: 6s                            │
│  Mana cost: 25                           │
│  Range: 1500cm                           │
├────────────────────────────────────────┤
│  Shift-click to drag-pin                 │
└────────────────────────────────────────┘
```

### 4.2 Implementação

```cpp
// FDFAbilityTableRow — campos novos
UPROPERTY(EditAnywhere, Category="UI")
FText DisplayDescription;
UPROPERTY(EditAnywhere, Category="UI")
FText AbilityKindLabel;       // "Active — Fire" / "Passive — Buff"
UPROPERTY(EditAnywhere, Category="UI|Stats")
float DisplayManaCost = 0.f;
UPROPERTY(EditAnywhere, Category="UI|Stats")
float DisplayCooldownSeconds = 0.f;   // visual only; CDO resolves real CD
UPROPERTY(EditAnywhere, Category="UI|Stats")
float DisplayRangeCm = 0.f;
```

```cpp
// UDFAbilitySlotWidget::NativeOnMouseEnter
World->GetTimerManager().SetTimer(TooltipHoverTimer, this, &ThisClass::ShowTooltip, 0.4f, false);

void UDFAbilitySlotWidget::ShowTooltip()
{
    if (!TooltipWidgetClass || AbilityTag.IsValid() == false) return;
    UDFAbilityTooltipWidget* Tip = CreateWidget<UDFAbilityTooltipWidget>(GetOwningPlayer(), TooltipWidgetClass);
    Tip->Populate(ResolveAbilityRow());
    Tip->AddToViewport(/*ZOrder=*/100);
    Tip->FollowMouse(true);
    ActiveTooltip = Tip;
}
```

Hover delay 0.4s evita popups acidentais ao varrer a barra.

### 4.3 Pinning

`Shift+click` no slot pin tooltip num canto da tela (UMG persistente até `Shift+click` de novo). Útil para builds.

---

## 5. Drag-and-drop polish — `[CODE/ASSET]` <a id="drag-drop"></a>

**Onde:** [`UDFAbilitySlotWidget.cpp`](../../Source/DungeonForged/Private/UI/UDFAbilitySlotWidget.cpp) — `NativeOnDragDetected` / `NativeOnDrop`.

### 5.1 Estado atual

`UDFAbilityBarDragDropOperation` swap funciona. Faltam visuais.

### 5.2 Melhorias

```cpp
void UDFAbilitySlotWidget::NativeOnDragDetected(...)
{
    Super::NativeOnDragDetected(...);

    // 1. Ghost icon (50% opacidade segue o cursor)
    auto* Op = NewObject<UDFAbilityBarDragDropOperation>(this);
    Op->SourceSlotIndex = BarSlotIndex;
    if (UImage* Ghost = NewObject<UImage>(this))
    {
        Ghost->SetBrushFromTexture(AbilityIconTexture);
        Ghost->SetRenderOpacity(0.5f);
        Op->DefaultDragVisual = Ghost;
    }
    Op->Pivot = EDragPivot::CenterCenter;

    // 2. Source slot dim
    if (AbilityIcon) AbilityIcon->SetRenderOpacity(0.3f);

    // 3. SFX drag-start
    if (DragStartSFX) UGameplayStatics::PlaySound2D(this, DragStartSFX);

    OutOperation = Op;
}

bool UDFAbilitySlotWidget::NativeOnDrop(...)
{
    if (auto* Op = Cast<UDFAbilityBarDragDropOperation>(InOperation))
    {
        // 4. Snap haptic + sound
        if (DropSnapSFX) UGameplayStatics::PlaySound2D(this, DropSnapSFX);
        if (auto* PC = GetOwningPlayer()) PC->PlayHapticEffect(SnapHaptic, EControllerHand::Left);

        // 5. Anim "swap" — slot piscando 0.2s amarelo
        PlaySwapPulseAnim();

        OwningHotbar->RequestSwapSlots(Op->SourceSlotIndex, BarSlotIndex);
        return true;
    }
    return false;
}

void UDFAbilitySlotWidget::NativeOnDragCancelled(...)
{
    if (AbilityIcon) AbilityIcon->SetRenderOpacity(1.f);
    Super::NativeOnDragCancelled(...);
}
```

### 5.3 Anti-griefing

Em **co-op futuro**, swap só funciona do **owning player** (validar `IsLocallyControlled()` antes de `RequestSwapSlots`). RPC server roda em `ADFPlayerCharacter`, valida ranges (`0..11`), e replica.

---

## 6. Slots LMB e RMB — visibilidade `[UI]` <a id="lmb-rmb"></a>

### 6.1 Estado atual

LMB ataque básico **sempre** funciona (não ocupa slot). RMB activa ability por **tag** (`RMBAbilityTryTags` em `ADFPlayerCharacter`) — invisível no HUD.

### 6.2 Sugestão de layout

```
┌──────┐  ┌─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┐  ┌──────┐
│ LMB  │  │1│2│3│4│5│6│7│8│9│0│-│=│  │ RMB  │
│Attack│  └─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┘  │ Spell│
└──────┘                                └──────┘
```

Dois slots laterais **fixos** (não draggables, não swap):
- **LMB**: mostra ícone do ataque básico atual (depende de arma equipada). Hover tooltip mostra "Basic Attack — 30 dmg".
- **RMB**: mostra ícone da primeira ability cujo `AbilityTag` matche `RMBAbilityTryTags`. Hover tooltip mostra spell + custo.

```cpp
// UDFAbilityHotbarWidget.h — novos slots
UPROPERTY(meta = (BindWidgetOptional))
TObjectPtr<UDFAbilitySlotWidget> LMBSlot = nullptr;
UPROPERTY(meta = (BindWidgetOptional))
TObjectPtr<UDFAbilitySlotWidget> RMBSlot = nullptr;

void RefreshLMBSlot();   // pega ataque básico da arma equipada
void RefreshRMBSlot();   // resolve primeira matching tag de RMBAbilityTryTags
```

### 6.3 Visual differentiator

LMB e RMB têm **borda dourada** sutil para distinguir dos 12 slots numerados (rebindáveis vs fixos).

---

## 7. Out-of-resource feedback — `[CODE/ASSET]` <a id="out-of-resource"></a>

### 7.1 Problema

Player clica ability sem mana → nada acontece. Frustração silenciosa.

### 7.2 Solução

```cpp
// UDFGameplayAbility::CanActivateAbility - quando falha por custo:
void UDFGameplayAbility::NotifyClientCostFailed_Implementation(FGameplayTag CostTag)
{
    if (UDFInGameHUDWidget* HUD = ...->GetHUDWidget())
    {
        HUD->Hotbar->FlashSlotInsufficientResource(GetAssetTags());
    }
}

void UDFAbilityHotbarWidget::FlashSlotInsufficientResource(FGameplayTagContainer Tags)
{
    for (UDFAbilitySlotWidget* S : Slots)
    {
        if (S && Tags.HasTagExact(S->AbilityTag))
        {
            S->PlayInsufficientResourceFlash();   // tint vermelho 0.4s + shake 6px
            break;
        }
    }
    // bonus: pulse rápido na barra de mana/stamina afetada
    if (Tags.HasTag(FDFGameplayTags::Cost_Mana)) PulseManaOrb();
    if (Tags.HasTag(FDFGameplayTags::Cost_Stamina)) PulseStaminaBar();
}
```

SFX leve `SFX_AbilityFail` (ascending → descending whoosh curto).

---

## 8. Global Cooldown overlay — `[CODE]` <a id="gcd"></a>

### 8.1 Quando

Após `CommitAbility` de qualquer slot, **todos** os slots ficam levemente cinza por **0.5s** (`GlobalCooldown`). Evita spam-cast e cria ritmo (WoW).

### 8.2 Implementação

```cpp
// UDFAbilitySystemComponent.h
UPROPERTY(EditAnywhere, Category="Cooldowns")
float GlobalCooldownSeconds = 0.5f;
UPROPERTY(EditAnywhere, Category="Cooldowns")
TSubclassOf<UGameplayEffect> GlobalCooldownGEClass;   // GE 0.5s com tag State.GlobalCooldown

bool UDFAbilitySystemComponent::TryActivateAbility(...)
{
    if (HasMatchingGameplayTag(FDFGameplayTags::State_GlobalCooldown))
        return false;
    bool const bOk = Super::TryActivateAbility(...);
    if (bOk && GlobalCooldownGEClass)
    {
        ApplyGameplayEffectToSelf(GlobalCooldownGEClass.GetDefaultObject(), 1.f, MakeEffectContext());
    }
    return bOk;
}
```

```cpp
// UDFAbilityHotbarWidget::NativeTick
bool const bGCD = LocalASC && LocalASC->HasMatchingGameplayTag(FDFGameplayTags::State_GlobalCooldown);
for (UDFAbilitySlotWidget* S : Slots) S->SetGCDActive(bGCD);
```

### 8.3 Exceções

Algumas abilities (ex: `Ability_Dodge`, `Ability_Interact`) têm tag `Ability.IgnoreGCD` no CDO → não aplicam GCD nem são bloqueadas por ele. Dodge **sempre** responsivo.

---

## 9. Persistir layout customizado — `[CODE]` <a id="persist-layout"></a>

### 9.1 Problema

Player drag-and-drop reorganiza barra → entra na próxima run → layout volta ao default.

### 9.2 Solução

```cpp
// DFSaveGame.h
UPROPERTY()
TMap<FName, TArray<FName>> AbilityBarLayoutPerClass;   // ClassRowName → 12 ability rows

// Ao iniciar run:
void UDFRunManager::GrantAbilitiesForCurrentRun()
{
    // ... grant abilities ...
    if (UDFSaveSlotManagerSubsystem* SM = GI->GetSubsystem<UDFSaveSlotManagerSubsystem>())
    {
        if (TArray<FName>* Saved = SM->GetSaveGame()->AbilityBarLayoutPerClass.Find(SelectedClassRow))
        {
            // Filtrar para só abilities granted nesta run (descartar entradas obsoletas)
            for (int32 i = 0; i < FMath::Min(Saved->Num(), DFAbilityBar::MaxBarSlots); ++i)
            {
                if (IsAbilityGrantedThisRun((*Saved)[i]))
                    PlayerChar->CurrentAbilitySlots[i] = (*Saved)[i];
            }
        }
    }
}

// Após swap:
void ADFPlayerCharacter::Server_SwapAbilityBarSlots_Implementation(int32 A, int32 B)
{
    // ... swap ...
    if (UDFSaveSlotManagerSubsystem* SM = GI->GetSubsystem<UDFSaveSlotManagerSubsystem>())
    {
        SM->GetSaveGame()->AbilityBarLayoutPerClass.Add(SelectedClassRow, CurrentAbilitySlots);
        SM->RequestSaveAsync();   // debounced 1s
    }
}
```

---

## 10. Debug & telemetria — `[CODE]` <a id="debug"></a>

### 10.1 Comandos existentes

`df.dumpabilities` em [`UDFCheatManager.cpp`](../../Source/DungeonForged/Private/Debug/UDFCheatManager.cpp) — dump dos specs do ASC.

### 10.2 Adicionar

```
df.bar.show            → log dos 12 slots (RowName, AbilityTag, CD remaining)
df.bar.fill            → preenche slots vazios com abilities da DT
df.bar.clear           → reset to NAME_None × 12
df.bar.simulatecd 3 5  → aplica CD 5s no slot 3 (testar overlay)
df.bar.gcd 0.5         → trigger GCD manual (testar visual)
```

### 10.3 Telemetria

```cpp
UE_LOG(LogDFTuning, Verbose, TEXT("Ability activated slot=%d tag=%s cd=%.2f gcd=%s"),
       SlotIndex, *AbilityTag.ToString(), CDSeconds, bGCDActive ? TEXT("Y") : TEXT("N"));
```

Grava run completa e revisa pacing das abilities.

---

## 11. Checklist de "pronto"

- [ ] `CurrentAbilitySlots` default size = 12.
- [ ] Constante `DFAbilityBar::MaxBarSlots` substitui literais.
- [ ] `RebuildInputLabels()` pull de `UDFInputRemappingSubsystem`, atualiza on rebind.
- [ ] Gamepad glyphs trocam labels quando último input = controller.
- [ ] Cooldown overlay tem 5 estados (Ready / On CD / Almost Ready / Just Refreshed / Out of Resource).
- [ ] GCD overlay aplica em todos os slots por 0.5s pós-cast.
- [ ] Tooltip aparece após 0.4s de hover com nome + desc + custo + CD + range.
- [ ] Drag-drop tem ghost icon, source dim, snap SFX, pulse anim.
- [ ] LMB slot fixo mostra ataque básico; RMB slot fixo mostra ability matched por tag.
- [ ] Out-of-resource flash tint vermelho + shake + pulse no orb.
- [ ] Layout salva em `DFSaveGame` por classe; restaura ao iniciar run.
- [ ] Cheat commands `df.bar.*` implementados.
- [ ] `LogDFTuning` ativo em ativações/CDs.

---

## Apêndice — fluxo end-to-end

```
1. Run start
   → UDFRunManager::GrantAbilitiesForCurrentRun
   → ASC.GiveAbility(Spec, InputID)
   → ADFPlayerCharacter.CurrentAbilitySlots[i] = RowName

2. CurrentAbilitySlots replicates
   → OnRep_AbilityBarSlots
   → UDFAbilityHotbarWidget.HandleAbilityBarSlotsChanged
   → RefreshHotbar (lê DT, popula 12 slots)
   → RebuildInputLabels (pull do remapping subsystem)

3. Player press "1"
   → EnhancedInput IA_AbilityBarSlot1
   → ASC.AbilityLocalInputPressed(InputID=1)
   → TryActivateAbility (checa GCD, custos, etc)
   → CommitAbility → aplica GE_Cooldown + GE_GlobalCooldown

4. Slot widget recebe OnActiveGameplayEffectAdded
   → Inicia ReadOnce + Timer 0.1s → UpdateCooldownVisuals
   → Material MID sweep CooldownPercent 1 → 0
   → Quando Pct ≤ 0 → PlayReadyFlash + SFX_AbilityReady

5. Player drag slot 1 → slot 5
   → NativeOnDragDetected (ghost icon 50%)
   → NativeOnDrop em slot 5 (snap SFX + pulse)
   → RequestSwapSlots(1, 5) → server RPC
   → Server swap CurrentAbilitySlots[1] <-> [5]
   → OnRep dispara em todos os clients → RefreshHotbar
   → Save async no DFSaveGame

6. Player rebind "1" → "Q" no Options
   → UDFInputRemappingSubsystem.OnBindingsChanged.Broadcast
   → UDFAbilityHotbarWidget.HandleBindingsChanged
   → RebuildInputLabels → InputLabels[0] = "Q"
   → RefreshHotbar (slot 0 mostra "Q")
```

---

> **Leitura cruzada:** [`02_Juice.md`](02_Juice.md) (hit stop on ability hit), [`07_UI_UX.md`](07_UI_UX.md) (HUD adaptativo — barra fade in/out), [`10_SettingsAndOptions.md`](10_SettingsAndOptions.md) (rebind UI).
