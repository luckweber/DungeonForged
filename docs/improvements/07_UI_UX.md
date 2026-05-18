# 07 — UI / UX

> **Objetivo:** que a HUD **suma quando não precisa**, **destaque quando precisa**, e que cada tela transicional (boss intro, defeat, victory) tenha personalidade.

---

## Sumário rápido

| Eixo | Atual | Alvo |
|---|---|---|
| HUD adaptativo | sempre visível | fade in/out por combate |
| Damage direction | tag interna existe | indicador no screen edge |
| Boss intro | desconhecido | letterbox + name reveal + roar |
| Defeat polish | text + summary | death cause + best-floor highlight |
| Victory | summary widget | confetti + speed comparison |
| Ability bar | 4 slots | + cooldown spinner + recharge flash |
| Health bar | bar | + segmented damage taken indicator (last 3s) |
| Tooltip | desconhecido | hover delay 0.4s, click-to-pin |
| Loading screen | hint texts (`docs/`) | hint texts + class lore + boss preview |

---

## 1. HUD Adaptativo — `[CODE]` <a id="hud-adaptativo"></a>

### 1.1 Conceito

Souls/Bloodborne minimal HUD: barras fade out fora de combate, fade in dentro. Reduz visual clutter durante exploração, e quando combat starts player **sabe imediatamente** (HUD aparece = "estou em perigo").

### 1.2 Implementação

```cpp
// UDFInGameHUDWidget.h
UPROPERTY(EditAnywhere, Category="HUD")
float HUDFadeInDuration = 0.2f;
UPROPERTY(EditAnywhere, Category="HUD")
float HUDFadeOutDuration = 1.2f;
UPROPERTY(EditAnywhere, Category="HUD")
float CombatExitDelay = 4.f;   // seconds out of combat before fade out

void OnCombatStateChanged(bool bInCombat);
void TickHUDOpacity(float DeltaTime);

float CurrentHUDOpacity = 0.f;
float TargetHUDOpacity = 0.f;
float LastCombatTime = -100.f;
```

```cpp
void UDFInGameHUDWidget::OnCombatStateChanged(bool bInCombat)
{
    if (bInCombat)
    {
        LastCombatTime = World->GetTimeSeconds();
        TargetHUDOpacity = 1.f;
    }
    else
    {
        // start exit timer
        World->GetTimerManager().SetTimer(CombatExitHandle, [this]() {
            TargetHUDOpacity = 0.f;
        }, CombatExitDelay, false);
    }
}

void UDFInGameHUDWidget::TickHUDOpacity(float DeltaTime)
{
    const float Speed = (TargetHUDOpacity > CurrentHUDOpacity) ? (1.f / HUDFadeInDuration)
                                                                : (1.f / HUDFadeOutDuration);
    CurrentHUDOpacity = FMath::FInterpConstantTo(CurrentHUDOpacity, TargetHUDOpacity, DeltaTime, Speed);
    SetRenderOpacity(CurrentHUDOpacity);
}
```

### 1.3 Elementos sempre visíveis

Apenas estes ficam sempre visíveis (não fadeem):

- **Minimap** (orientação espacial sempre útil)
- **Floor indicator** ("Floor 4")
- **Gold** (player checa frequentemente)

O resto (HP/MP/Stamina, ability bar, status effects) fade out 4s after combat ends.

### 1.4 Tags para "in combat"

`State.InCombat` é setada por:
- Player hits inimigo
- Player toma dano
- Inimigo entrou em `AICombatState`
- Boss room entered

Removida:
- 4s sem qualquer dos triggers acima
- Floor cleared (`OnFloorCleared` delegate)
- Player morre

```cpp
void ADFPlayerCharacter::OnDamageDealt() { EnterCombat(); }
void ADFPlayerCharacter::OnDamageTaken() { EnterCombat(); }

void ADFPlayerCharacter::EnterCombat()
{
    ASC->AddLooseGameplayTag(FDFGameplayTags::State_InCombat, 1);
    LastCombatActivity = World->GetTimeSeconds();
    World->GetTimerManager().ClearTimer(CombatExitHandle);
    OnCombatStateChanged.Broadcast(true);
}

void ADFPlayerCharacter::ScheduleCombatExit()
{
    World->GetTimerManager().SetTimer(CombatExitHandle, [this]() {
        if (World->GetTimeSeconds() - LastCombatActivity >= 4.f)
        {
            ASC->RemoveLooseGameplayTag(FDFGameplayTags::State_InCombat, 1);
            OnCombatStateChanged.Broadcast(false);
        }
    }, 4.f, false);
}
```

---

## 2. Damage direction indicator — `[CODE/ASSET]` <a id="dano-direcional"></a>

### 2.1 Visual

Vermelho radial gradient no edge do screen, do lado da fonte do dano. Igual ao Call of Duty / Bloodborne sangue.

```
+---------------------+
|  RED FADE  ←        |
|                     |
|     [crosshair]     |
|                     |
|                     |
+---------------------+
```

### 2.2 Implementação

UMG widget `WBP_DamageDirection` overlay no HUD:

```cpp
// Border de 4 lados (Top/Right/Bottom/Left) com material radial
UPROPERTY(meta=(BindWidget)) UImage* TopIndicator;
UPROPERTY(meta=(BindWidget)) UImage* RightIndicator;
UPROPERTY(meta=(BindWidget)) UImage* BottomIndicator;
UPROPERTY(meta=(BindWidget)) UImage* LeftIndicator;
```

Cada `UImage` usa o mesmo material (gradient radial), pivot ajustado para o lado correspondente.

### 2.3 Pulse trigger

Já existe `HitDirection2D` em `OnHitReceived`. Converter para screen edge:

```cpp
void UDFInGameHUDWidget::OnDamageReceived(float Amount, const FVector& WorldDir)
{
    APawn* Pawn = GetOwningPlayerPawn();
    if (!Pawn) return;

    const FVector Fwd = Pawn->GetActorForwardVector().GetSafeNormal2D();
    const FVector Right = Pawn->GetActorRightVector().GetSafeNormal2D();
    const FVector ToHit = WorldDir.GetSafeNormal2D();

    const float Dot = FVector::DotProduct(Fwd, ToHit);   // -1 (back) .. +1 (front)
    const float Side = FVector::DotProduct(Right, ToHit); // -1 (left) .. +1 (right)

    UImage* Target = nullptr;
    if (FMath::Abs(Dot) > FMath::Abs(Side))
        Target = (Dot > 0) ? TopIndicator : BottomIndicator;
    else
        Target = (Side > 0) ? RightIndicator : LeftIndicator;

    const float Intensity = FMath::Clamp(Amount / MaxHealth, 0.05f, 1.f);
    PulseIndicator(Target, Intensity, /*duration=*/ 1.2f);
}
```

`PulseIndicator` é uma UMG animation que faz fade in 0.1s + fade out 1.1s.

### 2.4 Off-screen enemy indicator

Quando inimigo ataca de fora do view frustum, mostrar **arrow vermelho** no edge apontando para o ponto exato do enemy (não só o quadrante). Souls-style.

```cpp
const FVector2D ScreenPos = ProjectWorldToScreen(EnemyLocation);
const FVector2D Center = ViewportSize / 2.f;
if (!IsInViewportBounds(ScreenPos))
{
    const FVector2D Edge = ProjectToScreenEdge(ScreenPos, Center, ViewportSize);
    ShowArrowAt(Edge, AngleFromCenter(Edge, Center));
}
```

---

## 3. Boss Intro — `[CODE/UI]`

### 3.1 Sequence

Quando player triggers `ADFBossTriggerVolume`:

```
T=0.0s  : Pause player input (`DisableInput`)
        : Camera transition to BossCam (4s sequencer ou manual lerp)
        : SFX rumble + boss roar
        : Letterbox bars fade in (top + bottom black 200px each)
        : Music: BossIntro stinger plays
T=2.0s  : Boss name reveal — large text fade in center
        : Subtitle: "Phase 1 of 3" small text
T=3.5s  : Boss name fade out
        : Camera fades back to player
        : Letterbox stays for 0.5s
T=4.0s  : Letterbox fades out
        : Player input enabled
        : Music: BossPhase1 starts
        : Boss aggro activates
```

### 3.2 Implementation

```cpp
class UDFBossIntroSequence : public UObject
{
    UFUNCTION(BlueprintCallable, Category="DF|Boss")
    void PlaySequence(APlayerController* PC, ADFBossBase* Boss);

    UPROPERTY(EditAnywhere) TSubclassOf<UCameraShakeBase> RoarShake;
    UPROPERTY(EditAnywhere) USoundBase* IntroSting;
    UPROPERTY(EditAnywhere) USoundBase* BossRoar;
    UPROPERTY(EditAnywhere) TSubclassOf<UUserWidget> LetterboxWidgetClass;
    UPROPERTY(EditAnywhere) TSubclassOf<UUserWidget> BossNameWidgetClass;
};

void UDFBossIntroSequence::PlaySequence(APlayerController* PC, ADFBossBase* Boss)
{
    if (!PC || !Boss) return;
    APawn* P = PC->GetPawn();
    if (P) DisableInput(P, PC);

    UUserWidget* Letterbox = CreateWidget(PC, LetterboxWidgetClass);
    Letterbox->AddToViewport(100);
    Letterbox->PlayAnimationByName(TEXT("FadeIn"));

    UGameplayStatics::PlaySound2D(PC, IntroSting);
    UGameplayStatics::PlaySoundAtLocation(PC, BossRoar, Boss->GetActorLocation());

    if (RoarShake) PC->ClientStartCameraShake(RoarShake);

    // T=2s: name reveal
    World->GetTimerManager().SetTimer(NameRevealHandle, [PC, this, Boss, Letterbox]() {
        UDFBossNameWidget* Name = CreateWidget<UDFBossNameWidget>(PC, BossNameWidgetClass);
        Name->SetBossName(Boss->GetBossDisplayName());
        Name->AddToViewport(101);

        // T=3.5s: end
        World->GetTimerManager().SetTimer(EndHandle, [this, PC, Letterbox, Name]() {
            Name->PlayAnimationByName(TEXT("FadeOut"));
            Letterbox->PlayAnimationByName(TEXT("FadeOut"));
            World->GetTimerManager().SetTimer(ReleaseHandle, [PC]() {
                if (APawn* P = PC->GetPawn()) EnableInput(P, PC);
            }, 0.5f, false);
        }, 1.5f, false);
    }, 2.f, false);
}
```

### 3.3 Skip option

Hold `[ESC]` por 0.5s salta o intro inteiro (segunda+ vez no mesmo boss). Memorizar `bSeenIntro` em `UDFRunManager` (resetado a cada run).

---

## 4. Defeat Screen — `[UI]`

`UDFDefeatScreenWidget` já existe. Polishes:

### 4.1 Death cause prominente

Header com cause + visual da fonte do dano:

```
┌─────────────────────────────────────────┐
│           [icon do boss/enemy/trap]     │
│                                         │
│         DEFEATED BY                     │
│       Necromancer Lord                  │
│                                         │
│  ──────────────────────────────         │
│   Floor reached:     7    ← highlight if best
│   Kills:           42   ← highlight if best
│   Run time:      24:18                  │
│   Gold collected:  847                  │
│   Abilities found:  6                   │
│  ──────────────────────────────         │
│                                         │
│   MetaXP gained:    +73                 │
│                                         │
│       [Return to Nexus]                 │
└─────────────────────────────────────────┘
```

### 4.2 Best floor / kills highlight

Se `FloorReached > Save->BestFloorReached`:
```
   Floor reached:     7    ← gold underline + "NEW BEST!" badge
```

UMG animation: text scale 1.2 + golden flash + sting SFX.

### 4.3 Last-second highlight

Capturar últimos 5s da run via `URecordedCameraComponent` (sequencer) e mostrar replay reverso no header. **Luxo** — adiar.

### 4.4 Encouraging messages

Pool de 20 frases random:
- "Death is just another teacher."
- "The next descent awaits."
- "Even legends fall before they rise."
- "The void remembers."

Aleatório no defeat screen, fora do bloco de stats. Tom Hades/Soulsborne.

---

## 5. Victory Screen — `[UI]`

`UDFVictoryScreenWidget` já existe. Polishes:

### 5.1 Confetti / particles

Niagara overlay com:
- Gold particle rain (subtle, sparse)
- Boss soul absorption VFX no canto
- Light burst no player

### 5.2 Tempo comparison

```
   ┌─ THIS RUN ─┐    ┌─ BEST ─┐
   │   24:18    │    │ 21:54  │ ← +2:24
```

Highlight verde se beat o best; vermelho se pior.

### 5.3 Boss kill replay

3s slow-mo do final hit. Capturable via `USkeletalMeshComponent::FreezeSimulation` + camera focus.

### 5.4 Continue / Return options

```
   [Continue to next run]   (continua build se ainda quiser; rare modes)
   [Return to Nexus]        (default)
   [View Run Recap]          (full stats / timeline)
```

---

## 6. Ability Bar polish — `[UI/CODE]`

### 6.1 Cooldown radial fill

Cada slot `UDFAbilitySlotWidget` deve ter overlay com **radial mask** que enche conforme cooldown. Já tem material para cooldown overlay (do `Game_Analysis.md`).

### 6.2 Recharge flash

Quando cooldown completes, flash white por 0.3s + sting:

```cpp
// UDFAbilitySlotWidget
void OnCooldownCompleted()
{
    PlayAnimationByName(TEXT("RechargeFlash"));
    UGameplayStatics::PlaySound2D(this, RechargeSting);
}
```

### 6.3 Cost preview

Quando hover (ou tap key gentle), mostra tooltip: damage, cost, cooldown, descrição. Hover delay 0.4s para evitar acidente.

### 6.4 Not-enough-resource feedback

Player aperta Q mas não tem mana → ability não cast → flash vermelho no slot + sting "click" + barra de mana pulsa.

```cpp
// no GA_Ability::CheckCost failure:
if (UDFInGameHUDWidget* HUD = ...)
{
    HUD->FlashAbilitySlot(SlotIndex, FLinearColor::Red);
    HUD->PulseManaBar();
}
```

---

## 7. Health Bar segments — `[UI]`

### 7.1 Damage taken segment

Mostrar **último damage taken** como gradient vermelho que decai em 3s. Useful for player tracking "how much was that hit".

```
HealthBar:
[████████████░░░░░░] ← current HP (green)
                ▲▲▲▲ ← last damage segment (red, fading 3s)
```

Implementação:

```cpp
// UDFAttributeBarWidget
void OnHealthChanged(float Current, float Max)
{
    const float Delta = Last - Current;
    if (Delta > 0.f)
    {
        DamageTakenSegment->SetValue(Delta / Max);
        DamageTakenSegment->PlayAnimationByName(TEXT("FadeOut3s"));
    }
    Last = Current;
}
```

### 7.2 Low HP pulse

Já tem `LowHealthSetEnabledFromRatio` em `UDFScreenEffectsComponent`. Adicionar **HUD-level**:
- HP < 25%: borda pulsa vermelho a 1Hz
- HP < 10%: pulse a 2Hz + audio heartbeat

### 7.3 Heal feedback

Quando cura > 5%, **green segment** sobre o HP gained, fade 1.5s:

```
[████████████████░░] ← current
            ▲▲▲▲ ← last heal segment (green)
```

---

## 8. Tooltip System — `[UI/CODE]`

Padronizar tooltips para items, abilities, shrines, events.

### 8.1 Hover delay

```cpp
class UDFTooltipManager
{
    UPROPERTY(EditAnywhere) float HoverDelay = 0.4f;

    void RegisterHover(UWidget* Source, TFunction<void(UWidget*)> ShowTooltip);
};
```

Hover delay evita tooltip spam ao mover mouse.

### 8.2 Click-to-pin

`Right-click` em uma tooltip = pin (fica fixa em canto, jogador pode clicar fora sem fechar). Useful para comparar items.

### 8.3 Rich tooltip content

```
┌─ Frostbite ────────────────────┐
│  [icon]                        │
│  Rare ability                  │
│  ─────────────                 │
│  Deals 80-120 Frost damage     │
│  in a 600cm cone.              │
│  Applies Freeze for 1.5s.      │
│                                │
│  Mana cost:    30              │
│  Cooldown:     8s              │
│  Range:        600cm           │
│                                │
│  ─────────────                 │
│  "The chill of the void."      │
└────────────────────────────────┘
```

Components: header, body, stats, lore footer. Em `UDFTooltipWidget`.

---

## 9. Loading Screen — `[UI/CONTENT]`

### 9.1 Componentes

```
┌────────────────────────────────────────┐
│  [Floor / Boss / Class image — full BG]│
│                                        │
│  Loading: Floor 7 — Profundezas        │
│                                        │
│  ▓▓▓▓▓▓▓▓░░░░░░░░  64%                │
│                                        │
│  ────────────────────────              │
│  Tip: "Hold attack to charge a Heavy   │
│  swing — costs stamina but breaks      │
│  Shielders' guard."                    │
└────────────────────────────────────────┘
```

### 9.2 Hint pool

50+ hints categorizadas:
- Combat tips (10): "Dodge gives 0.35s of invulnerability."
- Class tips (5 per class = 15): "Rogue combo points decay in 8s."
- Enemy tips (10): "Casters die fastest. Interrupt by attacking during cast."
- Lore tips (10): "The Nexus was built by..."
- Run tips (5): "Elite rooms guarantee Epic ability drops."

Random pick. Localize em `pt-BR` e `en-US`.

### 9.3 Class lore card

Se loading vai para `L_Run*`, mostrar **class lore card** no canto: bio + signature ability + tip de play.

---

## 10. Menu / Pause `[UI]`

### 10.1 Pause durante run

Pause **não pausa o tempo em multiplayer**. Em single, pausa real.

Conteúdo:
- Continue
- Restart Floor (consume gold or item to retry, optional design)
- Run Recap (current stats so far)
- Settings (sub-menu)
- Abandon Run (warning prompt + confirm)
- Main Menu (warning prompt)

### 10.2 Settings sub-menu

Tabbed:
- **Graphics** (resolution, fullscreen, scale, VSync, FPS cap)
- **Audio** (master, music, SFX, ambient, voice, UI sliders)
- **Controls** (rebind keys, sensitivity, invert Y)
- **Gameplay** (subtitles, hint frequency, accessibility — link to doc 08)
- **Accessibility** (camera shake intensity, hit stop intensity, damage numbers, colorblind)

---

## 11. Run Recap — `[UI]`

`URecapWidget` ou `URunRecapSubsystem` consolidated:

### 11.1 Timeline view

```
00:00 ─ Run started (Mage class)
01:24 ─ Floor 1 cleared (4 kills)
03:12 ─ Picked: FrostBolt (Rare)
04:50 ─ Floor 2 cleared
06:38 ─ Event: Faustian Pact (took +20% dmg, -50 HP)
...
24:18 ─ Killed by Necromancer Lord (Phase 2)
```

Skim view permite ver "where did the run go wrong" depois.

### 11.2 Build snapshot

Final stats + abilities + items collected. Útil para "como eu fiz aquela build OP?"

---

## 12. Checklist de "pronto"

- [ ] HUD fade out fora de combat (4s delay), fade in dentro.
- [ ] Minimap, floor indicator, gold sempre visíveis.
- [ ] Damage direction indicator (4 edges + arrow off-screen).
- [ ] Boss intro sequence (letterbox + name reveal + roar + 4s pause).
- [ ] Defeat screen: death cause + best-floor highlight.
- [ ] Victory screen: confetti + best-time comparison.
- [ ] Ability slot: radial cooldown + recharge flash + cost preview.
- [ ] Health bar: damage taken segment, low HP pulse, heal segment.
- [ ] Tooltips: hover 0.4s delay + click-to-pin.
- [ ] Loading: hint pool de 50+ + class lore card.
- [ ] Pause menu com Abandon Run prompt.
- [ ] Settings com accessibility sliders.
- [ ] Run Recap com timeline e build snapshot.

---

## Apêndice — princípios de UX

1. **Show, then hide.** Toda info aparece quando necessária e some quando não.
2. **Color = semântica.** Vermelho = perigo/dano, verde = cura, azul = mana, dourado = recompensa.
3. **One animation per beat.** Não acumular efeitos competindo por atenção.
4. **Always reversible.** Toda decisão de menu (exceto Abandon) tem "cancel".
5. **Predictable timing.** Hover delay 0.4s sempre. Fade out 1.2s sempre. Player aprende.

DungeonForged tem cobertura completa. Polishes são marginais mas alto retorno.
