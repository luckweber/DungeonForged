# 08 — Accessibility

> **Objetivo:** que o jogo seja **jogável e prazeroso** para players com:
> - **Motion sickness** (camera shake, FOV bumps)
> - **Visual impairment** (cores, contraste, subtitles)
> - **Motor difficulty** (hold vs toggle, key remap, dificuldade ajustável)
> - **Cognitive load** (UI clutter, decisões rápidas)

DungeonForged já tem `FDFAccessibilitySettings` + `UDFAccessibilitySubsystem` + `UDFLocalizationSubsystem` — boa fundação. Este doc é sobre **acrescentar opções concretas**.

---

## Sumário rápido

| Categoria | Atual | Alvo |
|---|---|---|
| **Sliders de intensidade** | 0 | Camera shake %, Hit stop %, Screen FX % |
| **Toggles visuais** | poucos | Damage numbers on/off, blood on/off, gore on/off |
| **Colorblind modes** | nenhum | 3 modes (Protanopia, Deuteranopia, Tritanopia) |
| **Subtitles** | desconhecido | Confirmar dialog subs + SFX subs |
| **Hold vs Toggle** | enum existe | Per-ability toggle UI |
| **Difficulty modes** | só Heat | Story / Normal / Hard, independent of Heat |
| **Key remap** | existe | Confirmar 100% das ações remapeáveis |
| **Camera options** | sensitivity | + invert Y/X, look smoothing slider |
| **Text size** | desconhecido | 75% / 100% / 125% / 150% scaling |
| **Auto-loot** | desconhecido | Toggle: auto-pickup gold/items |

---

## 1. Camera Shake Intensity — `[CODE/UI]`

### 1.1 Slider

**Onde:** [`Source/DungeonForged/Public/Settings/DFAccessibilitySettings.h`](../../Source/DungeonForged/Public/Settings/DFAccessibilitySettings.h) (presumido)

```cpp
USTRUCT(BlueprintType)
struct FDFAccessibilitySettings
{
    UPROPERTY(EditAnywhere, meta=(ClampMin=0.0, ClampMax=1.5, UIMin=0.0, UIMax=1.5))
    float CameraShakeIntensity = 1.0f;   // 0 = off, 1 = default, 1.5 = exaggerated

    UPROPERTY(EditAnywhere, meta=(ClampMin=0.0, ClampMax=1.5))
    float HitStopIntensity = 1.0f;       // multiplies dur of hit stop

    UPROPERTY(EditAnywhere, meta=(ClampMin=0.0, ClampMax=1.5))
    float ScreenEffectsIntensity = 1.0f; // chromatic / flash / vignette

    UPROPERTY(EditAnywhere)
    bool bShowDamageNumbers = true;

    UPROPERTY(EditAnywhere)
    bool bShowBloodDecals = true;

    UPROPERTY(EditAnywhere)
    bool bMotionBlur = false;

    UPROPERTY(EditAnywhere)
    bool bFOVBumpsOnSprint = true;

    UPROPERTY(EditAnywhere)
    EDFColorBlindMode ColorBlindMode = EDFColorBlindMode::None;

    UPROPERTY(EditAnywhere, meta=(ClampMin=0.75, ClampMax=1.5))
    float UIScaleMultiplier = 1.0f;
};
```

### 1.2 Apply pattern

Em `UDFCameraShakeFunctionLibrary`, **toda chamada multiplica por `Settings.CameraShakeIntensity`**:

```cpp
static void PlayShake(UObject* WorldCtx, TSubclassOf<UCameraShakeBase> Class, float Scale = 1.f)
{
    if (!WorldCtx || !Class) return;
    APlayerController* PC = UGameplayStatics::GetPlayerController(WorldCtx, 0);
    if (!PC) return;

    const float UserScale = UDFAccessibilitySubsystem::Get(WorldCtx)->Settings.CameraShakeIntensity;
    const float FinalScale = Scale * UserScale;
    if (FinalScale <= KINDA_SMALL_NUMBER) return;   // off

    PC->ClientStartCameraShake(Class, FinalScale);
}
```

Mesmo padrão para hit stop:

```cpp
void UDFHitStopSubsystem::TriggerHitStop(float Duration, float Dilation, AActor* Excl)
{
    const float UserScale = UDFAccessibilitySubsystem::Get(this)->Settings.HitStopIntensity;
    const float FinalDuration = Duration * UserScale;
    if (FinalDuration <= 0.005f) return;   // off
    ApplyHitStop(Dilation, Excl);  // dilation não escala (only duration)
    // schedule restore at FinalDuration
}
```

### 1.3 Screen effects scaling

```cpp
void UDFScreenEffectsComponent::ApplyEffect(float Intensity, ...)
{
    const float UserScale = UDFAccessibilitySubsystem::Get(this)->Settings.ScreenEffectsIntensity;
    const float Final = Intensity * UserScale;
    // ...
}
```

### 1.4 UI Slider widget

Em `UDFSettingsAccessibilityPanel`:

```
Camera Shake Intensity:    [────●────────]  100%
                           ↑ live preview: 1 shake play em loop quando arrasta

Hit Stop Intensity:        [─────●───────]  100%
Screen Effects Intensity:  [─────●───────]  100%
```

`OnSliderValueChanged` aplica imediato (live preview shake) e salva no `UDFAccessibilitySubsystem::Save`.

---

## 2. Damage Numbers Toggle — `[CODE]`

```cpp
// UDFCombatTextSubsystem::SpawnFloatingText
void SpawnFloatingText(...)
{
    if (Type == ECombatTextType::Damage || Type == ECombatTextType::Crit)
    {
        if (!UDFAccessibilitySubsystem::Get(this)->Settings.bShowDamageNumbers) return;
    }
    // ...
}
```

Status/heal text **continua mesmo com damage off** (heal é útil ver).

---

## 3. Blood / Gore Toggle — `[CODE/ASSET]`

### 3.1 Blood decals

```cpp
void UDFMeleeTraceComponent::ApplyBloodDecal(const FHitResult& Hit)
{
    if (!UDFAccessibilitySubsystem::Get(this)->Settings.bShowBloodDecals) return;
    // ... spawn decal ...
}
```

### 3.2 Gore Niagara

Niagara de hit impact com sangue tem variant "clean" (sparks/dust ao invés de sangue):

```cpp
UNiagaraSystem* PickHitImpact(EHitImpactType Type)
{
    const bool bGore = UDFAccessibilitySubsystem::Get(this)->Settings.bShowBloodDecals;
    return bGore ? GoreVariants[Type] : CleanVariants[Type];
}
```

Asset duplicates necessários — adiar até ter feedback de players.

---

## 4. Colorblind Mode — `[CODE/ASSET]`

### 4.1 Modes

```cpp
UENUM(BlueprintType)
enum class EDFColorBlindMode : uint8
{
    None,
    Protanopia,    // red-blind
    Deuteranopia,  // green-blind (most common)
    Tritanopia,    // blue-blind (rare)
};
```

### 4.2 Where colors matter (audit)

| Onde | Cor atual | Problema |
|---|---|---|
| Health bar | Verde | Protanopia: verde vira amarelo-claro, baixo contraste com fundo |
| Mana bar | Azul | OK em todos modes |
| Stamina | Amarelo | OK |
| Damage text | Branco | OK |
| Crit text | Dourado | OK em todos |
| Heal text | Verde | Mesmo problema do HP bar |
| Status burn | Laranja | OK |
| Status freeze | Ciano | Tritanopia: vira cinza |
| Rarity Common | Cinza | OK |
| Rarity Uncommon | Verde | Protanopia/Deuteranopia: cinza-claro |
| Rarity Rare | Azul | OK |
| Rarity Epic | Roxo | OK |
| Rarity Legendary | Dourado | OK |
| Enemy outline elite | Amarelo/Laranja | OK |
| Damage direction | Vermelho | Protanopia: cinza |

### 4.3 Mitigation strategies

1. **Shape + Color** — ícones de rarity também variam de shape (Common = square, Uncommon = pentagon, etc.).
2. **Palette swap** por mode:

```cpp
FLinearColor UDFAccessibilitySubsystem::ResolveColor(EDFSemantic Semantic) const
{
    const EDFColorBlindMode Mode = Settings.ColorBlindMode;
    switch (Mode)
    {
        case EDFColorBlindMode::Deuteranopia:
            switch (Semantic)
            {
                case EDFSemantic::Health: return FLinearColor(1.f, 0.8f, 0.f);   // amber instead of green
                case EDFSemantic::Heal:   return FLinearColor(0.f, 0.8f, 1.f);   // cyan
                case EDFSemantic::Uncommon: return FLinearColor(0.4f, 0.8f, 1.f);
                // ...
            }
        case EDFColorBlindMode::Protanopia:
            // ...
        case EDFColorBlindMode::Tritanopia:
            // ...
        default:
            return GetDefaultColor(Semantic);
    }
}
```

3. **Postprocess filter** opcional (libera os assets de mudar) — adiar; abordagem 2 dá controle preciso.

### 4.4 UI bindings

`UDFAttributeBarWidget` lê cor via `UDFAccessibilitySubsystem::ResolveColor(EDFSemantic::Health)` em vez de hardcode.

---

## 5. Subtitles — `[ASSET/CODE]`

### 5.1 Subtitle settings

```cpp
UPROPERTY(EditAnywhere)
bool bSubtitlesEnabled = true;

UPROPERTY(EditAnywhere)
bool bShowSFXSubtitles = false;   // [audio cue: door closes]

UPROPERTY(EditAnywhere, meta=(ClampMin=0.75, ClampMax=1.5))
float SubtitleScale = 1.0f;

UPROPERTY(EditAnywhere)
float SubtitleBackgroundOpacity = 0.6f;
```

### 5.2 Trigger

Cada dialog soundwave registra subtitle via `USoundConcurrency::SubtitleText`. UE5 já tem `bMature` e `Subtitles` array no `USoundWave`.

Para SFX subtitles, em momentos importantes (boss roar, low health heartbeat, trap arming):

```cpp
if (UDFSubtitleSubsystem* S = World->GetSubsystem<UDFSubtitleSubsystem>())
{
    S->QueueSubtitle(TEXT("[boss roars]"), /*duration=*/ 3.f, /*tag=*/ EDFSubtitleTag::SFX);
}
```

`UDFSubtitleSubsystem` mantém queue, exibe em UMG widget no canto inferior.

---

## 6. Hold vs Toggle — `[CODE]`

Atual: existe `EAbilityActivationPolicy` mas não em UI.

### 6.1 Per-ability toggle

```cpp
USTRUCT(BlueprintType)
struct FDFAbilityControlPreference
{
    UPROPERTY(EditAnywhere) FGameplayTag AbilityTag;
    UPROPERTY(EditAnywhere) EAbilityActivationPolicy Mode = EAbilityActivationPolicy::Hold;
};

// FDFAccessibilitySettings
UPROPERTY(EditAnywhere)
TArray<FDFAbilityControlPreference> AbilityControlPreferences;
```

### 6.2 UI

Em settings:
```
Sprint:        ( ) Hold      (•) Toggle
IronSkin:      (•) Hold      ( ) Toggle
ManaShield:    ( ) Hold      (•) Toggle
Lock-on:       (•) Hold      ( ) Toggle
```

### 6.3 Apply

No `GA_Sprint::ActivateAbility`:

```cpp
const EAbilityActivationPolicy Mode = ResolveActivationMode(SprintAbilityTag);
if (Mode == EAbilityActivationPolicy::Toggle)
{
    if (bIsActive) { Deactivate(); } else { Activate(); }
}
else
{
    Activate();   // hold-driven (release ends)
}
```

---

## 7. Difficulty Modes (independent of Heat) — `[CODE]`

### 7.1 Modes

```cpp
UENUM(BlueprintType)
enum class EDFDifficulty : uint8
{
    Story,      // enemy dmg -40%, +50% loot; for narrative-first
    Normal,     // baseline
    Hard,       // enemy dmg +30%, -10% loot; tighter dodge windows
    Nightmare,  // +60% dmg, no auto-pickup, perma-death events
};
```

### 7.2 Effects

```cpp
void UDFRunManager::ApplyDifficultyToRun()
{
    switch (CurrentDifficulty)
    {
        case EDFDifficulty::Story:
            EnemyDamageMultiplier = 0.6f;
            LootMultiplier = 1.5f;
            DodgeIFrameBonus = 0.1f;
            break;
        case EDFDifficulty::Normal:
            // default values, no change
            break;
        case EDFDifficulty::Hard:
            EnemyDamageMultiplier = 1.3f;
            LootMultiplier = 0.9f;
            DodgeIFrameBonus = -0.05f;
            break;
        case EDFDifficulty::Nightmare:
            EnemyDamageMultiplier = 1.6f;
            LootMultiplier = 0.8f;
            DodgeIFrameBonus = -0.1f;
            // disable auto-pickup
            // permadeath = sem revive items
            break;
    }
}
```

### 7.3 Achievement gating

Story mode **não conta para achievements** (mas conta para MetaXP, com 0.5× multiplier). Hard/Nightmare = 1.2× / 1.5× MetaXP.

### 7.4 UI

No main menu pre-run:
```
Difficulty:  [ Story ]  [ Normal* ]  [ Hard ]  [ Nightmare ]
                                ↑
              "For experienced players. Standard challenge."
```

`*` = default. Tooltip por dificuldade.

---

## 8. Key Remap — confirmar `[CODE]`

`SavedKeyBindings` existe via Enhanced Input.

### 8.1 Audit

Garantir que **toda ação está remapeável**:
- Movement (WASD)
- Sprint (Shift)
- Dodge (Space or CTRL)
- Attack (LMB)
- Heavy attack (LMB held / Shift+LMB)
- Block (?)
- Lock-on (RMB)
- Ability slots Q/E/R/F
- Use item (G)
- Interact (E or F)
- Open inventory (I)
- Open map (M)
- Pause (ESC)

Cada um em `IA_*` (Input Action) registrado em `IMC_Default`.

### 8.2 UI

```
Sprint:         [ Shift     ]    [Rebind]
Dodge:          [ Space     ]    [Rebind]
Heavy Attack:   [ LMB held  ]    [Rebind]
...
```

`Rebind` button = waits for next input, validates not conflicting, save.

### 8.3 Gamepad

Confirmar bindings padrão para Xbox/PS controllers. UE5 nativamente suporta via Enhanced Input.

---

## 9. Look Smoothing & Sensitivity — `[CODE]`

### 9.1 Settings

```cpp
UPROPERTY(EditAnywhere) float MouseSensitivityX = 1.0f;
UPROPERTY(EditAnywhere) float MouseSensitivityY = 1.0f;
UPROPERTY(EditAnywhere) bool bInvertY = false;
UPROPERTY(EditAnywhere) bool bInvertX = false;
UPROPERTY(EditAnywhere) float LookSmoothing = 0.0f;   // 0 = raw, 1 = max smooth
UPROPERTY(EditAnywhere) float ControllerSensitivity = 1.0f;
UPROPERTY(EditAnywhere) float ControllerDeadzone = 0.15f;
```

### 9.2 Look smoothing implementation

```cpp
void ADFPlayerCharacter::Look(const FInputActionValue& Value)
{
    FVector2D Raw = Value.Get<FVector2D>();
    Raw.X *= Settings.MouseSensitivityX * (Settings.bInvertX ? -1.f : 1.f);
    Raw.Y *= Settings.MouseSensitivityY * (Settings.bInvertY ? -1.f : 1.f);

    if (Settings.LookSmoothing > 0.f)
    {
        const float Speed = FMath::Lerp(40.f, 5.f, Settings.LookSmoothing);
        SmoothLook = FMath::Vector2DInterpTo(SmoothLook, Raw, GetWorld()->GetDeltaSeconds(), Speed);
        Raw = SmoothLook;
    }

    AddControllerYawInput(Raw.X);
    AddControllerPitchInput(Raw.Y);
}
```

---

## 10. UI Text Scaling — `[CODE]`

### 10.1 Implementation

```cpp
// UDFInGameHUDWidget::NativeConstruct
const float Scale = UDFAccessibilitySubsystem::Get(this)->Settings.UIScaleMultiplier;
SetRenderScale(FVector2D(Scale, Scale));
```

Ou via `UDPIScale` no project settings (mais robusto):
- Settings → Engine → User Interface → DPI Scaling Curve
- Adicionar usuário-facing multiplier que mods o curve.

### 10.2 Per-widget scaling

Alguns widgets devem **não escalar** (boss bar, minimap) para não cobrir gameplay area. Adicionar `bIgnoreUIScale` no widget base.

---

## 11. Auto-loot — `[CODE]`

### 11.1 Setting

```cpp
UPROPERTY(EditAnywhere) bool bAutoPickupGold = true;
UPROPERTY(EditAnywhere) bool bAutoPickupHealth = true;
UPROPERTY(EditAnywhere) bool bAutoPickupConsumables = false;  // some players want choice
UPROPERTY(EditAnywhere) bool bAutoPickupGear = false;
```

### 11.2 Magnet radius

Quando auto-pickup ativo, items são "magnetizados" para o player em 300cm:

```cpp
void ADFLootDrop::Tick(float DeltaTime)
{
    if (!bAutoPickup) return;
    APawn* P = LocalPlayer();
    if (!P) return;
    const float Dist = FVector::Dist(GetActorLocation(), P->GetActorLocation());
    if (Dist < MagnetRadius)
    {
        FVector ToPlayer = (P->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        AddActorWorldOffset(ToPlayer * MagnetSpeed * DeltaTime, true);
        if (Dist < PickupRadius) PickUp(P);
    }
}
```

### 11.3 Pickup feedback

Cada pickup → SFX + combat text "+25 gold" + small Niagara sparkle.

---

## 12. Other accessibility ideas

### 12.1 High contrast mode

Toggle: substitui materials por **outline-only** look (pop-out de inimigos), aumenta contraste em UI elements.

### 12.2 Reduce motion

Toggle: desativa todos os FOV bumps, camera shakes, screen effects, motion blur, automatic camera transitions. Para player com motion sickness severo.

### 12.3 Slow combat mode

Toggle: globalTimeDilation = 0.7 durante combate. **Não conta para achievements/leaderboard**. Para player com motor delay.

### 12.4 Aim assist

Quando lock-on, **soft auto-target** acrescenta yaw +/- 10° para alvo mais próximo se input estiver close. Já existe lock-on; aim assist adicional para inputs livres.

### 12.5 Tutorial revisit

Replay tutorial scenes a qualquer momento via main menu → "Tutorials". Útil para retornar depois de hiato.

---

## 13. Checklist de "pronto"

- [ ] `FDFAccessibilitySettings` expandido com 15+ campos.
- [ ] Sliders: Camera shake, Hit stop, Screen effects (live preview).
- [ ] Toggles: damage numbers, blood decals, motion blur, FOV bumps.
- [ ] Colorblind 3 modes (Deuteranopia / Protanopia / Tritanopia) com palette swap.
- [ ] Subtitles toggle + SFX subtitles option + size slider.
- [ ] Hold/Toggle per-ability UI (Sprint, IronSkin, ManaShield, Lock-on).
- [ ] Difficulty modes 4 levels com effects mapeados.
- [ ] Key remap 100% das ações + gamepad bindings.
- [ ] Mouse/Controller sensitivity X/Y + invert + smoothing.
- [ ] UI text scaling 75-150%.
- [ ] Auto-pickup toggles (4 categorias) + magnet radius.
- [ ] Settings persistem em SaveGame e aplicam imediato (sem restart).

---

## Apêndice — princípios de acessibilidade

1. **Default sensato.** Defaults assumem nenhuma necessidade especial. Sliders **adicionam** opção, não substituem.
2. **Pré-visualização.** Mudar slider deve mostrar efeito imediato.
3. **Save independent.** Settings de acessibilidade salvam por slot OU global. Permitir global = usabilidade.
4. **Sem penalty.** Story mode dá menos MetaXP, mas não bloqueia conteúdo. Toggles visuais (blood off, etc.) não afetam achievements.
5. **Documentar.** Para cada toggle, hover tooltip explica o que faz e a quem ajuda.

DungeonForged já tem `FDFAccessibilitySettings` + subsystem; a maior parte é **acrescentar campos e bindings**, não infraestrutura.
